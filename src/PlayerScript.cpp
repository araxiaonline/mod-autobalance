#include "ScriptMgr.h"
#include "Player.h"
#include "Map.h"
#include "Creature.h"
#include "AutoBalance.h"
#include "MapMgr.h"
#include "Chat.h"
#include "Log.h"

class AutoBalance_PlayerScript : public PlayerScript
{
    public:
        AutoBalance_PlayerScript()
            : PlayerScript("AutoBalance_PlayerScript")
        {
        }

        void OnLogin(Player *Player) override
        {
            if (sAutoBalancer->EnableGlobal && sAutoBalancer->Announcement) {
                ChatHandler(Player->GetSession()).SendSysMessage("This server is running the |cff4CFF00AutoBalance |rmodule.");
            }
        }

        virtual void OnLevelChanged(Player* player, uint8 oldlevel) override
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_PlayerScript::OnLevelChanged(): {} has leveled from {} to {}",
                player->GetName(),
                oldlevel,
                player->getLevel()
            );
            
            if (!player || player->IsGameMaster())
                return;

            Map* map = player->GetMap();

            if (!map || !map->IsDungeon())
                return;

            // first update the map's player stats
            sAutoBalancer->UpdateMapPlayerStats(map);

            // schedule all creatures for an update
            sAutoBalancer->lastConfigTime = 
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
        }

        void OnGiveXP(Player* player, uint32& amount, Unit* victim, uint8 /*xpSource*/) override
        {
            Map* map = player->GetMap();

            // If this isn't a dungeon, make no changes
            if (!map->IsDungeon() || !victim)
                return;

            AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

            if (victim && sAutoBalancer->RewardScalingXP && mapABInfo->enabled)
            {
                Map* map = player->GetMap();

                AutoBalanceCreatureInfo *creatureABInfo=victim->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

                if (map->IsDungeon())
                {
                    if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_DYNAMIC)
                    {
                        LOG_DEBUG("module.AutoBalance",
                            "AutoBalance_PlayerScript::OnGiveXP(): Distributing XP from '{}' to '{}' in dynamic mode - {}->{}",
                            victim->GetName(),
                            player->GetName(),
                            amount,
                            uint32(amount * creatureABInfo->XPModifier)
                        );

                        amount = uint32(amount * creatureABInfo->XPModifier);
                    }
                    else if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_FIXED)
                    {
                        // Ensure that the players always get the same XP, even when entering the dungeon alone
                        auto maxPlayerCount = ((InstanceMap*)sMapMgr->FindMap(map->GetId(), map->GetInstanceId()))->GetMaxPlayers();
                        auto currentPlayerCount = map->GetPlayersCountExceptGMs();
                        
                        LOG_DEBUG("module.AutoBalance",
                            "AutoBalance_PlayerScript::OnGiveXP(): Distributing XP from '{}' to '{}' in fixed mode - {}->{}",
                            victim->GetName(),
                            player->GetName(),
                            amount,
                            uint32(amount * creatureABInfo->XPModifier * ((float)currentPlayerCount / maxPlayerCount))
                        );
                        
                        amount = uint32(amount * creatureABInfo->XPModifier * ((float)currentPlayerCount / maxPlayerCount));
                    }
                }
            }
        }


        // void OnBeforeDropAddItem
        void OnBeforeLootMoney(Player* player, Loot* loot) override
        {
            Map* map = player->GetMap();

            // If this isn't a dungeon, make no changes
            if (!map->IsDungeon())
                return;

            AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
            ObjectGuid sourceGuid = loot->sourceWorldObjectGUID;

            if (mapABInfo->enabled && sAutoBalancer->RewardScalingMoney)
            {
                // if the loot source is a creature, honor the modifiers for that creature
                if (sourceGuid.IsCreature())
                {
                    Creature* sourceCreature = ObjectAccessor::GetCreature(*player, sourceGuid);
                    AutoBalanceCreatureInfo *creatureABInfo=sourceCreature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

                    // Dynamic Mode
                    if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_DYNAMIC)
                    {
                        LOG_DEBUG("module.AutoBalance",
                            "AutoBalance_PlayerScript::OnBeforeLootMoney(): Distributing money from '{}' in dynamic mode - {}->{}",
                            sourceCreature->GetName(),
                            loot->gold,
                            uint32(loot->gold * creatureABInfo->MoneyModifier)
                        );
                        
                        loot->gold = uint32(loot->gold * creatureABInfo->MoneyModifier);
                    }
                    // Fixed Mode
                    else if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_FIXED)
                    {
                        // Ensure that the players always get the same money, even when entering the dungeon alone
                        auto maxPlayerCount = ((InstanceMap*)sMapMgr->FindMap(map->GetId(), map->GetInstanceId()))->GetMaxPlayers();
                        auto currentPlayerCount = map->GetPlayersCountExceptGMs();
                        
                        LOG_DEBUG("module.AutoBalance",
                            "AutoBalance_PlayerScript::OnBeforeLootMoney(): Distributing money from '{}' in fixed mode - {}->{}",
                            sourceCreature->GetName(),
                            loot->gold,
                            uint32(loot->gold * creatureABInfo->MoneyModifier * ((float)currentPlayerCount / maxPlayerCount))
                        );
                        
                        loot->gold = uint32(loot->gold * creatureABInfo->MoneyModifier * ((float)currentPlayerCount / maxPlayerCount));
                    }
                }
                // for all other loot sources, just distribute in Fixed mode as though the instance was full
                else
                {
                    auto maxPlayerCount = ((InstanceMap*)sMapMgr->FindMap(map->GetId(), map->GetInstanceId()))->GetMaxPlayers();
                    auto currentPlayerCount = map->GetPlayersCountExceptGMs();

                    LOG_DEBUG("module.AutoBalance",
                        "AutoBalance_PlayerScript::OnBeforeLootMoney(): Distributing money from a non-creature in fixed mode - {}->{}",
                        loot->gold,
                        uint32(loot->gold * ((float)currentPlayerCount / maxPlayerCount))
                    );

                    loot->gold = uint32(loot->gold * ((float)currentPlayerCount / maxPlayerCount));
                }
            }
        }
};

void AddPlayerScripts()
{
    new AutoBalance_PlayerScript();
}