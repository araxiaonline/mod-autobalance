#include "ScriptMgr.h"
#include "AutoBalance.h"
#include "Player.h"
#include "Map.h"
#include "Log.h"
#include "LootMgr.h"



class AutoBalance_GlobalScript : public GlobalScript {
public:
    AutoBalance_GlobalScript() : GlobalScript("AutoBalance_GlobalScript") { }

    void OnAfterUpdateEncounterState(Map* map, EncounterCreditType type,  uint32 /*creditEntry*/, Unit* /*source*/, Difficulty /*difficulty_fixed*/, DungeonEncounterList const* /*encounters*/, uint32 /*dungeonCompleted*/, bool updated) override {
        //if (!dungeonCompleted)
        //    return;

        if (!sAutoBalancer->rewardEnabled || !updated)
            return;

        if (map->GetPlayersCountExceptGMs() < sAutoBalancer->MinPlayerReward)
            return;

        AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        // skip if it's not a pre-wotlk dungeon/raid and if it's not scaled
        if (!sAutoBalancer->LevelScaling || mapABInfo->mapLevel <= 70 || mapABInfo->lfgMinLevel <= 70
            // skip when not in dungeon or not kill credit
            || type != ENCOUNTER_CREDIT_KILL_CREATURE || !map->IsDungeon())
            return;

        Map::PlayerList const &playerList = map->GetPlayers();

        if (playerList.IsEmpty())
            return;

        uint32 reward = map->ToInstanceMap()->GetMaxPlayers() > 5 ? sAutoBalancer->rewardRaid : sAutoBalancer->rewardDungeon;
        if (!reward)
            return;

        //instanceStart=0, endTime;
        uint8 difficulty = map->GetDifficulty();

        for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        {
            if (!itr->GetSource() || itr->GetSource()->IsGameMaster() || itr->GetSource()->getLevel() < DEFAULT_MAX_LEVEL)
                continue;

            itr->GetSource()->AddItem(reward, 1 + difficulty); // difficulty boost
        }
    }

    /**
     * This is used to provide better rewards to players when they complete a dungeon.
     * The rewards are scaled up versions of the original drop.
     * @brief Called after a player has looted an item from a creature.
    */
    void OnBeforeDropAddItem(Player const* player, Loot& loot, bool canRate, uint16 lootMode, LootStoreItem* LootStoreItem, LootStore const& store) override {

        if(LootStoreItem->itemid == 0) {
            return;
        }

        ItemTemplate const* newItem = sObjectMgr->GetItemTemplate(LootStoreItem->itemid);
        if (!newItem) {
            LOG_INFO("server", "> OnBeforeDropAddItem: Item not found for itemid {}", LootStoreItem->itemid);
            return;
        }

        Map* map = player->GetMap();
        const Group* group = player->GetGroup();

        // 3 things things need to happen
        // 1. Is the instance scaled up to max level or beyond?
        // 2. Is the loot quality rare or higher?
        // 3. What is the difficulty of the instances?  2 - Mythic 3 - Legendary 4 - Ascendant

        // 1. Is the instance scaled up to max level or beyond?
        AutoBalanceMapInfo *mapABInfo = map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
        if (!mapABInfo || !mapABInfo->isLevelScalingEnabled || player->getLevel() < 80)
        {
            return;
        }

        // The items are deterministic based ont the id just need to add the correct id starting point
        uint32 idStart = 0;

        // 2. Is the loot quality rare or higher?
        if (newItem->Quality < 3) {
            return;
        }

        if(!group)
        {
            LOG_INFO("server", "> OnBeforeDropAddItem: Player {} is not in a group.", player->GetName());
            return;
        }

        uint8 grpDifficulty = sAutoBalancer->GetGroupDifficulty(group);

        // 3. What is the difficulty of the instances?  2 - Mythic 3 - Legendary 4 - Ascendant
        switch(grpDifficulty) {
            case 2:
                idStart = 20000000;
                break;
            case 3:
                idStart = 21000000;
                break;
            case 4:
                idStart = 22000000;
                break;
            default:
                break;
        }

        if (!group)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("autobalance: player {} is not in a group.", player->GetName());
            return;
        }

        if (sAutoBalancer->GetGroupDifficulty(group) < 2)
        {
            return;
        }

        // LOG_INFO("server", "> OnBeforeDropAddItem:              Current Loot Drop Item {}", LootStoreItem->itemid);
        int newItemId = LootStoreItem->itemid + idStart;

        ItemTemplate const* lookupItem = sObjectMgr->GetItemTemplate(newItemId);

        if(!lookupItem) {
            LOG_INFO("server", "> OnBeforeDropAddItem:              New Loot Item not found");
            return;
        } else {
            LOG_INFO(
                "server", "> OnBeforeDropAddItem:  New ITEM        ItemName {} Quality {} ItemLevel {}",
                lookupItem->Name1,
                lookupItem->Quality,
                lookupItem->ItemLevel
            );
            LootStoreItem->itemid = newItemId;

            // Revalidate the LootStoreItem to ensure consistency
            if (!LootStoreItem->IsValid(store, newItemId)) {
                LOG_ERROR("server", "> OnBeforeDropAddItem: LootStoreItem is not valid after updating itemid to {}", newItemId);
                return;
            }

            // Optionally log other properties for debugging
            LOG_INFO(
                "server", "> OnBeforeDropAddItem: Updated LootStoreItem properties - reference {}, mincount {}, maxcount {}",
                LootStoreItem->reference,
                LootStoreItem->mincount,
                LootStoreItem->maxcount
            );

        }

        LOG_INFO("server", "> OnBeforeDropAddItem: Final LootStoreItem->itemid {}", LootStoreItem->itemid);

    }
};