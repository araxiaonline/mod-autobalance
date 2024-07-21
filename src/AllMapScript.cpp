#include "ScriptMgr.h"
#include "Log.h"
#include "Player.h"
#include "AutoBalancer.h"
#include "Chat.h"
#include "MapMgr.h"

class AutoBalance_AllMapScript : public AllMapScript
{
    public:
    AutoBalance_AllMapScript()
        : AllMapScript("AutoBalance_AllMapScript")
        {
        }

        void OnCreateMap(Map* map)
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllMapScript::OnCreateMap(): {}", map->GetMapName());

            if (!map->IsDungeon() && !map->IsRaid())
                return;

            // get the map's info
            AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

            // get the map's LFG stats
            LFGDungeonEntry const* dungeon = GetLFGDungeon(map->GetId(), map->GetDifficulty());
            if (dungeon) {
                mapABInfo->lfgMinLevel = dungeon->MinLevel;
                mapABInfo->lfgMaxLevel = dungeon->MaxLevel;
                mapABInfo->lfgTargetLevel = dungeon->TargetLevel;
            }

            // load the map's settings
            sAutoBalancer->LoadMapSettings(map);
        }

        void OnPlayerEnterAll(Map* map, Player* player)
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllMapScript::OnPlayerEnterAll(): {}", map->GetMapName());
            if (!map->IsDungeon() && !map->IsRaid())
                return;

            if (player->IsGameMaster())
                return;

            // get the map's info
            AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

            Group* group = player->GetGroup();
            if (group)
            {
                // if the player is in a group, set the player count to the group size
                mapABInfo->customDifficulty = sAutoBalancer->GetGroupDifficulty(group);
            }

            // recalculate the zone's level stats
            mapABInfo->highestCreatureLevel = 0;
            mapABInfo->lowestCreatureLevel = 0;
            mapABInfo->avgCreatureLevel = 0;
            mapABInfo->activeCreatureCount = 0;

            // see which existing creatures are active
            for (std::vector<Creature*>::iterator creatureIterator = mapABInfo->allMapCreatures.begin(); creatureIterator != mapABInfo->allMapCreatures.end(); ++creatureIterator)
            {
                sAutoBalancer->AddCreatureToMapData(*creatureIterator, false, nullptr, true);
            }

            // determine if the map should be enabled for scaling based on the current settings
            mapABInfo->enabled = sAutoBalancer->ShouldMapBeEnabled(map);

            // updates the player count, player levels for the map
            sAutoBalancer->UpdateMapPlayerStats(map);

            if (sAutoBalancer->PlayerChangeNotify && sAutoBalancer->EnableGlobal && mapABInfo->enabled)
            {
                if (map->GetEntry()->IsDungeon() && player)
                {
                    Map::PlayerList const &playerList = map->GetPlayers();
                    if (!playerList.IsEmpty())
                    {
                        for (Map::PlayerList::const_iterator playerIteration = playerList.begin(); playerIteration != playerList.end(); ++playerIteration)
                        {
                            if (Player* playerHandle = playerIteration->GetSource())
                            {
                                ChatHandler chatHandle = ChatHandler(playerHandle->GetSession());
                                auto instanceMap = ((InstanceMap*)sMapMgr->FindMap(map->GetId(), map->GetInstanceId()));

                                std::string instanceDifficulty; if (instanceMap->IsHeroic()) instanceDifficulty = "Heroic"; else instanceDifficulty = "Normal";

                                chatHandle.PSendSysMessage(
                                    "|cffFF0000 [AutoBalance]|r|cffFF8000 %s enters %s (%u-player %s). Player count set to %u (Player Difficulty Offset = %u) |r",
                                    player->GetName().c_str(),
                                    map->GetMapName(),
                                    instanceMap->GetMaxPlayers(),
                                    instanceDifficulty,
                                    mapABInfo->playerCount + sAutoBalancer->PlayerCountDifficultyOffset,
                                    sAutoBalancer->PlayerCountDifficultyOffset
                                );
                            }
                        }
                    }
                }
            }
        }

        void OnPlayerLeaveAll(Map* map, Player* player)
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllMapScript::OnPlayerLeaveAll(): {}", map->GetMapName());
            if (!sAutoBalancer->EnableGlobal)
                return;

            // get the map's info
            AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

            // recalculate the zone's level stats
            mapABInfo->highestCreatureLevel = 0;
            mapABInfo->lowestCreatureLevel = 0;
            mapABInfo->avgCreatureLevel = 0;
            mapABInfo->activeCreatureCount = 0;

            // see which existing creatures are active
            for (std::vector<Creature*>::iterator creatureIterator = mapABInfo->allMapCreatures.begin(); creatureIterator != mapABInfo->allMapCreatures.end(); ++creatureIterator)
            {
                sAutoBalancer->AddCreatureToMapData(*creatureIterator, false, player, true);
            }

            // determine if the map should be enabled for scaling based on the current settings
            mapABInfo->enabled = sAutoBalancer->ShouldMapBeEnabled(map);

            bool areAnyPlayersInCombat = false;

            // updates the player count and levels for the map
            if (map->GetEntry() && map->GetEntry()->IsDungeon())
            {
                // determine if any players in the map are in combat
                // if so, do not adjust the player count
                Map::PlayerList const& mapPlayerList = map->GetPlayers();
                for (Map::PlayerList::const_iterator itr = mapPlayerList.begin(); itr != mapPlayerList.end(); ++itr)
                {
                    if (Player* mapPlayer = itr->GetSource())
                    {
                        if (mapPlayer->IsInCombat() && mapPlayer->GetMap() == map)
                        {
                            areAnyPlayersInCombat = true;

                            // notify the player that they left the instance while combat was in progress
                            ChatHandler chatHandle = ChatHandler(player->GetSession());
                            chatHandle.PSendSysMessage("|cffFF0000 [AutoBalance]|r|cffFF8000 You left the instance while combat was in progress. The instance player count is still %u.", mapABInfo->playerCount);

                            break;
                        }
                    }
                }
                if (areAnyPlayersInCombat)
                {
                    for (Map::PlayerList::const_iterator itr = mapPlayerList.begin(); itr != mapPlayerList.end(); ++itr)
                    {
                        if (Player* mapPlayer = itr->GetSource())
                        {
                            // only for the players who are in the instance and did not leave
                            if (mapPlayer != player)
                            {
                                ChatHandler chatHandle = ChatHandler(mapPlayer->GetSession());
                                chatHandle.PSendSysMessage("|cffFF0000 [AutoBalance]|r|cffFF8000 %s left the instance while combat was in progress. The instance player count is still %u.", player->GetName().c_str(), mapABInfo->playerCount);
                            }
                        }
                    }
                }
                else
                {
                    mapABInfo->playerCount = map->GetPlayersCountExceptGMs() - 1;
                }
            }

            if (sAutoBalancer->PlayerChangeNotify && !player->IsGameMaster() && !areAnyPlayersInCombat && sAutoBalancer->EnableGlobal && mapABInfo->enabled)
            {
                if (map->GetEntry()->IsDungeon() && player)
                {
                    Map::PlayerList const &playerList = map->GetPlayers();
                    if (!playerList.IsEmpty())
                    {
                        for (Map::PlayerList::const_iterator playerIteration = playerList.begin(); playerIteration != playerList.end(); ++playerIteration)
                        {
                            Player* mapPlayer = playerIteration->GetSource();
                            if (mapPlayer && mapPlayer != player)
                            {
                                ChatHandler chatHandle = ChatHandler(mapPlayer->GetSession());
                                chatHandle.PSendSysMessage(
                                    "|cffFF0000 [AutoBalance]|r|cffFF8000 %s left the instance. Player count set to %u (Player Difficulty Offset = %u) |r",
                                    player->GetName().c_str(),
                                    mapABInfo->playerCount,
                                    sAutoBalancer->PlayerCountDifficultyOffset
                                );
                            }
                        }
                    }
                }
            }
        }
};