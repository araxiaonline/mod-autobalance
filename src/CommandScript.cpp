#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "ChatCommand.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

class AutoBalance_CommandScript : public CommandScript
{
public:
    AutoBalance_CommandScript() : CommandScript("AutoBalance_CommandScript") { }

    std::vector<ChatCommand> GetCommands() const
    {
        static std::vector<ChatCommand> ABCommandTable =
        {
            { "setoffset",        SEC_GAMEMASTER,                        true, &HandleABSetOffsetCommand,                 "Sets the global Player Difficulty Offset for instances. Example: (You + offset(1) = 2 player difficulty)." },
            { "getoffset",        SEC_PLAYER,                            true, &HandleABGetOffsetCommand,                 "Shows current global player offset value." },
            { "checkmap",         SEC_GAMEMASTER,                        true, &HandleABCheckMapCommand,                  "Run a check for current map/instance, it can help in case you're testing autobalance with GM." },
            { "mapstat",          SEC_PLAYER,                            true, &HandleABMapStatsCommand,                  "Shows current autobalance information for this map" },
            { "creaturestat",     SEC_PLAYER,                            true, &HandleABCreatureStatsCommand,             "Shows current autobalance information for selected creature." },
            { "mythic",           SEC_PLAYER,                            true, &HandleABMythicCommand,                    "Sets the group difficulty to Mythic" },
            { "legendary",        SEC_PLAYER,                            true, &HandleABLegendaryCommand,                 "Sets the group difficulty to Legendary" },
            { "ascendant",        SEC_PLAYER,                            true, &HandleABAscendantCommand,                 "Sets the group difficulty to Ascendant" },
            { "getdifficulty",    SEC_PLAYER,                            true, &HandleABGetDifficultyCommand,             "Shows the current group difficulty" },

        };

        static std::vector<ChatCommand> commandTable =
        {
            { "autobalance",     SEC_PLAYER,                             false, NULL,                      "", ABCommandTable },
            { "ab",              SEC_PLAYER,                             false, NULL,                      "", ABCommandTable },
        };
        return commandTable;
    }

    static bool HandleABSetOffsetCommand(ChatHandler* handler, const char* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(".autobalance setoffset #");
            handler->PSendSysMessage("Sets the Player Difficulty Offset for instances. Example: (You + offset(1) = 2 player difficulty).");
            return false;
        }
        char* offset = strtok((char*)args, " ");
        int32 offseti = -1;

        if (offset)
        {
            offseti = (uint32)atoi(offset);
            handler->PSendSysMessage("Changing Player Difficulty Offset to %i.", offseti);
            sAutoBalancer->PlayerCountDifficultyOffset = offseti;
            sAutoBalancer->lastConfigTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            return true;
        }
        else
            handler->PSendSysMessage("Error changing Player Difficulty Offset! Please try again.");
        return false;
    }

    static bool HandleABGetOffsetCommand(ChatHandler* handler, const char* /*args*/)
    {
        handler->PSendSysMessage("Current Player Difficulty Offset = %i", sAutoBalancer->PlayerCountDifficultyOffset);
        return true;
    }

    static bool HandleABCheckMapCommand(ChatHandler* handler, const char* args)
    {
        Player *pl = handler->getSelectedPlayer();

        if (!pl)
        {
            handler->SendSysMessage(LANG_SELECT_PLAYER_OR_PET);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AutoBalanceMapInfo *mapABInfo=pl->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        mapABInfo->playerCount = pl->GetMap()->GetPlayersCountExceptGMs();

        Map::PlayerList const &playerList = pl->GetMap()->GetPlayers();
        uint8 level = 0;
        if (!playerList.IsEmpty())
        {
            for (Map::PlayerList::const_iterator playerIteration = playerList.begin(); playerIteration != playerList.end(); ++playerIteration)
            {
                if (Player* playerHandle = playerIteration->GetSource())
                {
                    if (playerHandle->getLevel() > level)
                        mapABInfo->mapLevel = level = playerHandle->getLevel();
                }
            }
        }

        HandleABMapStatsCommand(handler, args);

        return true;
    }

    static bool HandleABMapStatsCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player *player;
        player = handler->getSelectedPlayer() ? handler->getSelectedPlayer() : handler->GetPlayer();

        AutoBalanceMapInfo *mapABInfo=player->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        if (player->GetMap()->IsDungeon() || player->GetMap()->IsRaid())
        {
            handler->PSendSysMessage("---");
            handler->PSendSysMessage("Map: %s (ID: %u)", player->GetMap()->GetMapName(), player->GetMapId());
            handler->PSendSysMessage("Players on map: %u (Lvl %u - %u)",
                                    mapABInfo->playerCount,
                                    mapABInfo->lowestPlayerLevel,
                                    mapABInfo->highestPlayerLevel
                                    );
            handler->PSendSysMessage("Map Level: %u%s", (uint8)(mapABInfo->avgCreatureLevel+0.5f),
                                                        mapABInfo->isLevelScalingEnabled ? std::string("->") + std::to_string(mapABInfo->highestPlayerLevel) + std::string(" (Level Scaling Enabled)") : std::string(" (Level Scaling Disabled)")
                                    );
            handler->PSendSysMessage("LFG Range: Lvl %u - %u (Target: Lvl %u)", mapABInfo->lfgMinLevel, mapABInfo->lfgMaxLevel, mapABInfo->lfgTargetLevel);
            handler->PSendSysMessage("Active Creatures in map: %u (Lvl %u - %u | Avg Lvl %.2f)",
                                    mapABInfo->activeCreatureCount,
                                    mapABInfo->lowestCreatureLevel,
                                    mapABInfo->highestCreatureLevel,
                                    mapABInfo->avgCreatureLevel
                                    );
            handler->PSendSysMessage("Total Creatures in map: %u",
                                    mapABInfo->allMapCreatures.size()
                                    );

            return true;
        }
        else
        {
            handler->PSendSysMessage("The target is not in a dungeon or battleground.");
            return true;
        }
    }

    static bool HandleABCreatureStatsCommand(ChatHandler* handler, const char* /*args*/)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AutoBalanceCreatureInfo *creatureABInfo=target->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");
        AutoBalanceMapInfo *mapABInfo=target->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        handler->PSendSysMessage("---");
        handler->PSendSysMessage("%s (%u%s%s), %s",
                                  target->GetName(),
                                  creatureABInfo->UnmodifiedLevel,
                                  mapABInfo->isLevelScalingEnabled ? std::string("->") + std::to_string(creatureABInfo->selectedLevel) : "",
                                  target->IsDungeonBoss() ? " | Boss" : "",
                                  creatureABInfo->isActive ? "Active for Map Stats" : "Ignored for Map Stats");
        handler->PSendSysMessage("Health multiplier: %.3f", creatureABInfo->HealthMultiplier);
        handler->PSendSysMessage("Mana multiplier: %.3f", creatureABInfo->ManaMultiplier);
        handler->PSendSysMessage("Armor multiplier: %.3f", creatureABInfo->ArmorMultiplier);
        handler->PSendSysMessage("Damage multiplier: %.3f", creatureABInfo->DamageMultiplier);
        handler->PSendSysMessage("CC Duration multiplier: %.3f", creatureABInfo->CCDurationMultiplier);
        handler->PSendSysMessage("XP multiplier: %.3f  Money multiplier: %.3f", creatureABInfo->XPModifier, creatureABInfo->MoneyModifier);

        return true;

    }

    static bool HandleABMythicCommand(ChatHandler* handler, const char* /*args*/)
    {
        sAutoBalancer->SetGroupDifficulty(handler->GetPlayer(), GROUP_DIFFICULTY_MYTHIC);
        handler->PSendSysMessage("autobalance: group difficulty set to Mythic. Rewards improvements x1. level(81-83) recommended.");
        return true;
    }

    static bool HandleABLegendaryCommand(ChatHandler* handler, const char* /*args*/)
    {
        sAutoBalancer->SetGroupDifficulty(handler->GetPlayer(), GROUP_DIFFICULTY_LEGENDARY);
        handler->PSendSysMessage("autobalance: group difficulty set to Mythic. Reward improvements x2 level(85) recommended.");
        return true;
    }

    static bool HandleABAscendantCommand(ChatHandler* handler, const char* /*args*/)
    {
        sAutoBalancer->SetGroupDifficulty(handler->GetPlayer(), GROUP_DIFFICULTY_ASCENDANT);
        handler->PSendSysMessage("autobalance: group difficulty set to Ascendant. Reward improvements x3 leve(85) required.");

        return true;
    }

    static bool HandleABGetDifficultyCommand(ChatHandler* handler, const char*) {
        Player* player = handler->GetPlayer();
        Group* group = player->GetGroup();
        if (!group)
        {
            handler->PSendSysMessage("autobalance: You are not in a group.");
            return true;
        }

        uint8 difficulty = 0;
        difficulty = sAutoBalancer->GetGroupDifficulty(group);

        char* difficultyStr = new char[20];
        switch (difficulty)
        {
            case GROUP_DIFFICULTY_NORMAL:
                strcpy(difficultyStr, "Normal");
                break;
            case GROUP_DIFFICULTY_HEROIC:
                strcpy(difficultyStr, "Heroic");
                break;
            case GROUP_DIFFICULTY_MYTHIC:
                strcpy(difficultyStr, "Mythic");
                break;
            case GROUP_DIFFICULTY_LEGENDARY:
                strcpy(difficultyStr, "Legendary");
                break;
            case GROUP_DIFFICULTY_ASCENDANT:
                strcpy(difficultyStr, "Ascendant");
                break;

            default:
                break;
        }

        handler->PSendSysMessage("autobalance: group difficulty is set to %u (%s)", difficulty, difficultyStr);
        return true;
    }
};