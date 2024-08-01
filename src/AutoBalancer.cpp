#include "AutoBalancer.h"
#include "Log.h"
#include "MapMgr.h"
#include "World.h"
#include "Player.h"
#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#include <random>

#define MAXIMUM_PLAYER_COUNT 200

// Define static members
std::map<int, int> AutoBalancer::forcedCreatureIds;
std::map<uint32, uint8> AutoBalancer::enabledDungeonIds;
std::map<uint32, AutoBalanceInflectionPointSettings> AutoBalancer::dungeonOverrides;
std::map<uint32, AutoBalanceInflectionPointSettings> AutoBalancer::bossOverrides;
std::map<uint32, AutoBalanceStatModifiers> AutoBalancer::statModifierOverrides;
std::map<uint32, AutoBalanceStatModifiers> AutoBalancer::statModifierBossOverrides;
std::map<uint32, AutoBalanceStatModifiers> AutoBalancer::statModifierCreatureOverrides;
std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> AutoBalancer::levelScalingDynamicLevelOverrides;
std::map<uint32, uint32> AutoBalancer::levelScalingDistanceCheckOverrides;
int8 AutoBalancer::PlayerCountDifficultyOffset;
float AutoBalancer::MythicMultiplier, AutoBalancer::LegendaryMultiplier, AutoBalancer::AscendantMultiplier;
bool AutoBalancer::LevelScaling;
int8 AutoBalancer::LevelScalingSkipHigherLevels, AutoBalancer::LevelScalingSkipLowerLevels;
int8 AutoBalancer::LevelScalingDynamicLevelCeilingDungeons, AutoBalancer::LevelScalingDynamicLevelFloorDungeons, AutoBalancer::LevelScalingDynamicLevelCeilingRaids, AutoBalancer::LevelScalingDynamicLevelFloorRaids;
int8 AutoBalancer::LevelScalingDynamicLevelCeilingHeroicDungeons, AutoBalancer::LevelScalingDynamicLevelFloorHeroicDungeons, AutoBalancer::LevelScalingDynamicLevelCeilingHeroicRaids, AutoBalancer::LevelScalingDynamicLevelFloorHeroicRaids;
ScalingMethod AutoBalancer::LevelScalingMethod;
uint32 AutoBalancer::rewardRaid;
uint32 AutoBalancer::rewardDungeon;
uint32 AutoBalancer::MinPlayerReward;
bool AutoBalancer::Announcement;
bool AutoBalancer::LevelScalingEndGameBoost, AutoBalancer::PlayerChangeNotify, AutoBalancer::rewardEnabled;
float AutoBalancer::MinHPModifier, AutoBalancer::MinManaModifier, AutoBalancer::MinDamageModifier, AutoBalancer::MinCCDurationModifier, AutoBalancer::MaxCCDurationModifier;
ScalingMethod AutoBalancer::RewardScalingMethod;
bool AutoBalancer::RewardScalingXP, AutoBalancer::RewardScalingMoney;
float AutoBalancer::RewardScalingXPModifier, AutoBalancer::RewardScalingMoneyModifier;
uint64_t AutoBalancer::lastConfigTime;
bool AutoBalancer::EnableGlobal;
bool AutoBalancer::Enable5M, AutoBalancer::Enable10M, AutoBalancer::Enable15M, AutoBalancer::Enable20M, AutoBalancer::Enable25M, AutoBalancer::Enable40M;
bool AutoBalancer::Enable5MHeroic, AutoBalancer::Enable10MHeroic, AutoBalancer::Enable25MHeroic;
bool AutoBalancer::EnableOtherNormal, AutoBalancer::EnableOtherHeroic;
float AutoBalancer::InflectionPoint, AutoBalancer::InflectionPointCurveFloor, AutoBalancer::InflectionPointCurveCeiling, AutoBalancer::InflectionPointBoss;
float AutoBalancer::InflectionPointHeroic, AutoBalancer::InflectionPointHeroicCurveFloor, AutoBalancer::InflectionPointHeroicCurveCeiling, AutoBalancer::InflectionPointHeroicBoss;
float AutoBalancer::InflectionPointRaid, AutoBalancer::InflectionPointRaidCurveFloor, AutoBalancer::InflectionPointRaidCurveCeiling, AutoBalancer::InflectionPointRaidBoss;
float AutoBalancer::InflectionPointRaidHeroic, AutoBalancer::InflectionPointRaidHeroicCurveFloor, AutoBalancer::InflectionPointRaidHeroicCurveCeiling, AutoBalancer::InflectionPointRaidHeroicBoss;
float AutoBalancer::InflectionPointRaid10M, AutoBalancer::InflectionPointRaid10MCurveFloor, AutoBalancer::InflectionPointRaid10MCurveCeiling, AutoBalancer::InflectionPointRaid10MBoss;
float AutoBalancer::InflectionPointRaid10MHeroic, AutoBalancer::InflectionPointRaid10MHeroicCurveFloor, AutoBalancer::InflectionPointRaid10MHeroicCurveCeiling, AutoBalancer::InflectionPointRaid10MHeroicBoss;
float AutoBalancer::InflectionPointRaid15M, AutoBalancer::InflectionPointRaid15MCurveFloor, AutoBalancer::InflectionPointRaid15MCurveCeiling, AutoBalancer::InflectionPointRaid15MBoss;
float AutoBalancer::InflectionPointRaid20M, AutoBalancer::InflectionPointRaid20MCurveFloor, AutoBalancer::InflectionPointRaid20MCurveCeiling, AutoBalancer::InflectionPointRaid20MBoss;
float AutoBalancer::InflectionPointRaid25M, AutoBalancer::InflectionPointRaid25MCurveFloor, AutoBalancer::InflectionPointRaid25MCurveCeiling, AutoBalancer::InflectionPointRaid25MBoss;
float AutoBalancer::InflectionPointRaid25MHeroic, AutoBalancer::InflectionPointRaid25MHeroicCurveFloor, AutoBalancer::InflectionPointRaid25MHeroicCurveCeiling, AutoBalancer::InflectionPointRaid25MHeroicBoss;
float AutoBalancer::InflectionPointRaid40M, AutoBalancer::InflectionPointRaid40MCurveFloor, AutoBalancer::InflectionPointRaid40MCurveCeiling, AutoBalancer::InflectionPointRaid40MBoss;
float AutoBalancer::StatModifier_Global, AutoBalancer::StatModifier_Health, AutoBalancer::StatModifier_Mana, AutoBalancer::StatModifier_Armor, AutoBalancer::StatModifier_Damage, AutoBalancer::StatModifier_SpellDamage, AutoBalancer::StatModifier_DoTDamage, AutoBalancer::StatModifier_CCDuration;
float AutoBalancer::StatModifierHeroic_Global, AutoBalancer::StatModifierHeroic_Health, AutoBalancer::StatModifierHeroic_Mana, AutoBalancer::StatModifierHeroic_Armor, AutoBalancer::StatModifierHeroic_Damage, AutoBalancer::StatModifierHeroic_CCDuration;
float AutoBalancer::StatModifierRaid_Global, AutoBalancer::StatModifierRaid_Health, AutoBalancer::StatModifierRaid_Mana, AutoBalancer::StatModifierRaid_Armor, AutoBalancer::StatModifierRaid_Damage, AutoBalancer::StatModifierRaid_CCDuration;
float AutoBalancer::StatModifierRaidHeroic_Global, AutoBalancer::StatModifierRaidHeroic_Health, AutoBalancer::StatModifierRaidHeroic_Mana, AutoBalancer::StatModifierRaidHeroic_Armor, AutoBalancer::StatModifierRaidHeroic_Damage, AutoBalancer::StatModifierRaidHeroic_CCDuration;
float AutoBalancer::StatModifierRaid10M_Global, AutoBalancer::StatModifierRaid10M_Health, AutoBalancer::StatModifierRaid10M_Mana, AutoBalancer::StatModifierRaid10M_Armor, AutoBalancer::StatModifierRaid10M_Damage, AutoBalancer::StatModifierRaid10M_CCDuration;
float AutoBalancer::StatModifierRaid10MHeroic_Global, AutoBalancer::StatModifierRaid10MHeroic_Health, AutoBalancer::StatModifierRaid10MHeroic_Mana, AutoBalancer::StatModifierRaid10MHeroic_Armor, AutoBalancer::StatModifierRaid10MHeroic_Damage, AutoBalancer::StatModifierRaid10MHeroic_CCDuration;
float AutoBalancer::StatModifierRaid15M_Global, AutoBalancer::StatModifierRaid15M_Health, AutoBalancer::StatModifierRaid15M_Mana, AutoBalancer::StatModifierRaid15M_Armor, AutoBalancer::StatModifierRaid15M_Damage, AutoBalancer::StatModifierRaid15M_CCDuration;
float AutoBalancer::StatModifierRaid20M_Global, AutoBalancer::StatModifierRaid20M_Health, AutoBalancer::StatModifierRaid20M_Mana, AutoBalancer::StatModifierRaid20M_Armor, AutoBalancer::StatModifierRaid20M_Damage, AutoBalancer::StatModifierRaid20M_CCDuration;
float AutoBalancer::StatModifierRaid25M_Global, AutoBalancer::StatModifierRaid25M_Health, AutoBalancer::StatModifierRaid25M_Mana, AutoBalancer::StatModifierRaid25M_Armor, AutoBalancer::StatModifierRaid25M_Damage, AutoBalancer::StatModifierRaid25M_CCDuration;
float AutoBalancer::StatModifierRaid25MHeroic_Global, AutoBalancer::StatModifierRaid25MHeroic_Health, AutoBalancer::StatModifierRaid25MHeroic_Mana, AutoBalancer::StatModifierRaid25MHeroic_Armor, AutoBalancer::StatModifierRaid25MHeroic_Damage, AutoBalancer::StatModifierRaid25MHeroic_CCDuration;
float AutoBalancer::StatModifierRaid40M_Global, AutoBalancer::StatModifierRaid40M_Health, AutoBalancer::StatModifierRaid40M_Mana, AutoBalancer::StatModifierRaid40M_Armor, AutoBalancer::StatModifierRaid40M_Damage, AutoBalancer::StatModifierRaid40M_CCDuration;
float AutoBalancer::StatModifier_Boss_Global, AutoBalancer::StatModifier_Boss_Health, AutoBalancer::StatModifier_Boss_Mana, AutoBalancer::StatModifier_Boss_Armor, AutoBalancer::StatModifier_Boss_Damage, AutoBalancer::StatModifier_Boss_CCDuration;
float AutoBalancer::StatModifierHeroic_Boss_Global, AutoBalancer::StatModifierHeroic_Boss_Health, AutoBalancer::StatModifierHeroic_Boss_Mana, AutoBalancer::StatModifierHeroic_Boss_Armor, AutoBalancer::StatModifierHeroic_Boss_Damage, AutoBalancer::StatModifierHeroic_Boss_CCDuration;
float AutoBalancer::StatModifierRaid_Boss_Global, AutoBalancer::StatModifierRaid_Boss_Health, AutoBalancer::StatModifierRaid_Boss_Mana, AutoBalancer::StatModifierRaid_Boss_Armor, AutoBalancer::StatModifierRaid_Boss_Damage, AutoBalancer::StatModifierRaid_Boss_CCDuration;
float AutoBalancer::StatModifierRaidHeroic_Boss_Global, AutoBalancer::StatModifierRaidHeroic_Boss_Health, AutoBalancer::StatModifierRaidHeroic_Boss_Mana, AutoBalancer::StatModifierRaidHeroic_Boss_Armor, AutoBalancer::StatModifierRaidHeroic_Boss_Damage, AutoBalancer::StatModifierRaidHeroic_Boss_CCDuration;
float AutoBalancer::StatModifierRaid10M_Boss_Global, AutoBalancer::StatModifierRaid10M_Boss_Health, AutoBalancer::StatModifierRaid10M_Boss_Mana, AutoBalancer::StatModifierRaid10M_Boss_Armor, AutoBalancer::StatModifierRaid10M_Boss_Damage, AutoBalancer::StatModifierRaid10M_Boss_CCDuration;
float AutoBalancer::StatModifierRaid10MHeroic_Boss_Global, AutoBalancer::StatModifierRaid10MHeroic_Boss_Health, AutoBalancer::StatModifierRaid10MHeroic_Boss_Mana, AutoBalancer::StatModifierRaid10MHeroic_Boss_Armor, AutoBalancer::StatModifierRaid10MHeroic_Boss_Damage, AutoBalancer::StatModifierRaid10MHeroic_Boss_CCDuration;
float AutoBalancer::StatModifierRaid15M_Boss_Global, AutoBalancer::StatModifierRaid15M_Boss_Health, AutoBalancer::StatModifierRaid15M_Boss_Mana, AutoBalancer::StatModifierRaid15M_Boss_Armor, AutoBalancer::StatModifierRaid15M_Boss_Damage, AutoBalancer::StatModifierRaid15M_Boss_CCDuration;
float AutoBalancer::StatModifierRaid20M_Boss_Global, AutoBalancer::StatModifierRaid20M_Boss_Health, AutoBalancer::StatModifierRaid20M_Boss_Mana, AutoBalancer::StatModifierRaid20M_Boss_Armor, AutoBalancer::StatModifierRaid20M_Boss_Damage, AutoBalancer::StatModifierRaid20M_Boss_CCDuration;
float AutoBalancer::StatModifierRaid25M_Boss_Global, AutoBalancer::StatModifierRaid25M_Boss_Health, AutoBalancer::StatModifierRaid25M_Boss_Mana, AutoBalancer::StatModifierRaid25M_Boss_Armor, AutoBalancer::StatModifierRaid25M_Boss_Damage, AutoBalancer::StatModifierRaid25M_Boss_CCDuration;
float AutoBalancer::StatModifierRaid25MHeroic_Boss_Global, AutoBalancer::StatModifierRaid25MHeroic_Boss_Health, AutoBalancer::StatModifierRaid25MHeroic_Boss_Mana, AutoBalancer::StatModifierRaid25MHeroic_Boss_Armor, AutoBalancer::StatModifierRaid25MHeroic_Boss_Damage, AutoBalancer::StatModifierRaid25MHeroic_Boss_CCDuration;
float AutoBalancer::StatModifierRaid40M_Boss_Global, AutoBalancer::StatModifierRaid40M_Boss_Health, AutoBalancer::StatModifierRaid40M_Boss_Mana, AutoBalancer::StatModifierRaid40M_Boss_Armor, AutoBalancer::StatModifierRaid40M_Boss_Damage, AutoBalancer::StatModifierRaid40M_Boss_CCDuration;

AutoBalancer::AutoBalancer()
{
    //ctor
    lastConfigTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    LOG_INFO("AutoBalance", "[AutoBalance] Autobalance enabled!");
}

AutoBalancer::~AutoBalancer()
{
    //dtor
}

void AutoBalancer::LoadEnabledDungeons(std::string dungeonIdString) // Used for reading the string from the configuration file for selecting dungeons to scale
{
    std::string delimitedValue;
    std::stringstream dungeonIdStream;

    dungeonIdStream.str(dungeonIdString);
    while (std::getline(dungeonIdStream, delimitedValue, ',')) // Process each dungeon ID in the string, delimited by the comma - "," and then space " "
    {
        std::string pairOne, pairTwo;
        std::stringstream dungeonPairStream(delimitedValue);
        dungeonPairStream>>pairOne>>pairTwo;
        auto dungeonMapId = atoi(pairOne.c_str());
        auto minPlayers = atoi(pairTwo.c_str());
        enabledDungeonIds[dungeonMapId] = minPlayers;
    }
}

std::map<uint32, AutoBalanceInflectionPointSettings> AutoBalancer::LoadInflectionPointOverrides(std::string dungeonIdString) // Used for reading the string from the configuration file for selecting dungeons to override
{
    std::string delimitedValue;
    std::stringstream dungeonIdStream;
    std::map<uint32, AutoBalanceInflectionPointSettings> overrideMap;

    dungeonIdStream.str(dungeonIdString);
    while (std::getline(dungeonIdStream, delimitedValue, ',')) // Process each dungeon ID in the string, delimited by the comma - "," and then space " "
    {
        std::string val1, val2, val3, val4;
        std::stringstream dungeonPairStream(delimitedValue);
        dungeonPairStream >> val1 >> val2 >> val3 >> val4;

        auto dungeonMapId = atoi(val1.c_str());

        // Replace any missing values with -1
        if (val2.empty()) { val2 = "-1"; }
        if (val3.empty()) { val3 = "-1"; }
        if (val4.empty()) { val4 = "-1"; }

        AutoBalanceInflectionPointSettings ipSettings = AutoBalanceInflectionPointSettings(
            atof(val2.c_str()),
            atof(val3.c_str()),
            atof(val4.c_str())
        );

        overrideMap[dungeonMapId] = ipSettings;
    }

    return overrideMap;
}

std::map<uint32, AutoBalanceStatModifiers> AutoBalancer::LoadStatModifierOverrides(std::string dungeonIdString) // Used for reading the string from the configuration file for per-dungeon stat modifiers
{
    std::string delimitedValue;
    std::stringstream dungeonIdStream;
    std::map<uint32, AutoBalanceStatModifiers> overrideMap;

    dungeonIdStream.str(dungeonIdString);
    while (std::getline(dungeonIdStream, delimitedValue, ',')) // Process each dungeon ID in the string, delimited by the comma - "," and then space " "
    {
        std::string val1, val2, val3, val4, val5, val6, val7;
        std::stringstream dungeonStream(delimitedValue);
        dungeonStream >> val1 >> val2 >> val3 >> val4 >> val5 >> val6 >> val7;

        auto dungeonMapId = atoi(val1.c_str());

        // Replace any missing values with -1
        if (val2.empty()) { val2 = "-1"; }
        if (val3.empty()) { val3 = "-1"; }
        if (val4.empty()) { val4 = "-1"; }
        if (val5.empty()) { val5 = "-1"; }
        if (val6.empty()) { val6 = "-1"; }
        if (val7.empty()) { val7 = "-1"; }

        AutoBalanceStatModifiers statSettings = AutoBalanceStatModifiers(
            atof(val2.c_str()),
            atof(val3.c_str()),
            atof(val4.c_str()),
            atof(val5.c_str()),
            atof(val6.c_str()),
            atof(val7.c_str())
        );

        overrideMap[dungeonMapId] = statSettings;
    }

    return overrideMap;
}

std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> AutoBalancer::LoadDynamicLevelOverrides(std::string dungeonIdString) // Used for reading the string from the configuration file for per-dungeon dynamic level overrides
{
    std::string delimitedValue;
    std::stringstream dungeonIdStream;
    std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> overrideMap;

    dungeonIdStream.str(dungeonIdString);
    while (std::getline(dungeonIdStream, delimitedValue, ',')) // Process each dungeon ID in the string, delimited by the comma - "," and then space " "
    {
        std::string val1, val2, val3, val4, val5;
        std::stringstream dungeonStream(delimitedValue);
        dungeonStream >> val1 >> val2 >> val3 >> val4 >> val5;

        auto dungeonMapId = atoi(val1.c_str());

        // Replace any missing values with -1
        if (val2.empty()) { val2 = "-1"; }
        if (val3.empty()) { val3 = "-1"; }
        if (val4.empty()) { val3 = "-1"; }
        if (val5.empty()) { val3 = "-1"; }

        AutoBalanceLevelScalingDynamicLevelSettings dynamicLevelSettings = AutoBalanceLevelScalingDynamicLevelSettings(
            atoi(val2.c_str()),
            atoi(val3.c_str()),
            atoi(val4.c_str()),
            atoi(val5.c_str())
        );

        overrideMap[dungeonMapId] = dynamicLevelSettings;
    }

    return overrideMap;
}

std::map<uint32, uint32> AutoBalancer::LoadDistanceCheckOverrides(std::string dungeonIdString)
{
    std::string delimitedValue;
    std::stringstream dungeonIdStream;
    std::map<uint32, uint32> overrideMap;

    dungeonIdStream.str(dungeonIdString);
    while (std::getline(dungeonIdStream, delimitedValue, ',')) // Process each dungeon ID in the string, delimited by the comma - "," and then space " "
    {
        std::string val1, val2;
        std::stringstream dungeonStream(delimitedValue);
        dungeonStream >> val1 >> val2;

        auto dungeonMapId = atoi(val1.c_str());
        overrideMap[dungeonMapId] = atoi(val2.c_str());
    }

    return overrideMap;
}


bool AutoBalancer::isEnabledDungeon(uint32 dungeonId)
{
    return (enabledDungeonIds.find(dungeonId) != enabledDungeonIds.end());
}

bool AutoBalancer::perDungeonScalingEnabled()
{
    return (!enabledDungeonIds.empty());
}

bool AutoBalancer::hasDungeonOverride(uint32 dungeonId)
{
    return (dungeonOverrides.find(dungeonId) != dungeonOverrides.end());
}

bool AutoBalancer::hasBossOverride(uint32 dungeonId)
{
    return (bossOverrides.find(dungeonId) != bossOverrides.end());
}

bool AutoBalancer::hasStatModifierOverride(uint32 dungeonId)
{
    return (statModifierOverrides.find(dungeonId) != statModifierOverrides.end());
}

bool AutoBalancer::hasStatModifierBossOverride(uint32 dungeonId)
{
    return (statModifierBossOverrides.find(dungeonId) != statModifierBossOverrides.end());
}

bool AutoBalancer::hasStatModifierCreatureOverride(uint32 creatureId)
{
    return (statModifierCreatureOverrides.find(creatureId) != statModifierCreatureOverrides.end());
}

bool AutoBalancer::hasDynamicLevelOverride(uint32 dungeonId)
{
    return (levelScalingDynamicLevelOverrides.find(dungeonId) != levelScalingDynamicLevelOverrides.end());
}

bool AutoBalancer::hasLevelScalingDistanceCheckOverride(uint32 dungeonId)
{
    return (levelScalingDistanceCheckOverrides.find(dungeonId) != levelScalingDistanceCheckOverrides.end());
}

bool AutoBalancer::ShouldMapBeEnabled(Map* map)
{
    if (!map->IsDungeon() && !map->IsRaid())
    {
        // we're not in a dungeon or a raid, we never scale
        return false;
    }

    // get the current instance map
    auto instanceMap = ((InstanceMap*)sMapMgr->FindMap(map->GetId(), map->GetInstanceId()));

    // if there wasn't one, then we're not in an instance
    if (!instanceMap)
    {
        return false;
    }

    // get the max player count for the instance
    auto maxPlayerCount = instanceMap->GetMaxPlayers();

    // if the player count is less than 1, then we're not in an instance
    if (maxPlayerCount < 1)
    {
        return false;
    }

    // use the configuration variables to determine if this instance type/size should have scaling enabled
    if (instanceMap->IsHeroic())
    {
        switch (maxPlayerCount)
        {
            case 5:
                return Enable5MHeroic;
            case 10:
                return Enable10MHeroic;
            case 25:
                return Enable25MHeroic;
            default:
                return EnableOtherHeroic;
        }
    }
    else
    {
        switch (maxPlayerCount)
        {
            case 5:
                return Enable5M;
            case 10:
                return Enable10M;
            case 15:
                return Enable15M;
            case 20:
                return Enable20M;
            case 25:
                return Enable25M;
            case 40:
                return Enable40M;
            default:
                return EnableOtherNormal;
        }
    }
}

void AutoBalancer::LoadMapSettings(Map* map)
{
    // Load (or create) the map's info
    AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

    // create an InstanceMap object
    InstanceMap* instanceMap = map->ToInstanceMap();

	//check for null pointer
	if (!map)
	{
		return;
	}

	if (!map->IsDungeon() && !map->IsRaid())
	{
		return;
	}

    // should the map be enabled at all?
    mapABInfo->enabled = ShouldMapBeEnabled(map);

    //
    // Dynamic Level Scaling Floor and Ceiling
    //

    // 5-player normal dungeons
    if (instanceMap->GetMaxPlayers() <= 5 && !instanceMap->IsHeroic())
    {
        mapABInfo->levelScalingDynamicCeiling = LevelScalingDynamicLevelCeilingDungeons;
        mapABInfo->levelScalingDynamicFloor = LevelScalingDynamicLevelFloorDungeons;

    }
    // 5-player heroic dungeons
    else if (instanceMap->GetMaxPlayers() <= 5 && instanceMap->IsHeroic())
    {
        mapABInfo->levelScalingDynamicCeiling = LevelScalingDynamicLevelCeilingHeroicDungeons;
        mapABInfo->levelScalingDynamicFloor = LevelScalingDynamicLevelFloorHeroicDungeons;
    }
    // Normal raids
    else if (instanceMap->GetMaxPlayers() > 5 && !instanceMap->IsHeroic())
    {
        mapABInfo->levelScalingDynamicCeiling = LevelScalingDynamicLevelCeilingRaids;
        mapABInfo->levelScalingDynamicFloor = LevelScalingDynamicLevelFloorRaids;
    }
    // Heroic raids
    else if (instanceMap->GetMaxPlayers() > 5 && instanceMap->IsHeroic())
    {
        mapABInfo->levelScalingDynamicCeiling = LevelScalingDynamicLevelCeilingHeroicRaids;
        mapABInfo->levelScalingDynamicFloor = LevelScalingDynamicLevelFloorHeroicRaids;
    }
    // something went wrong
    else
    {
        LOG_ERROR("module.AutoBalance",
            "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Unable to determine dynamic scaling floor and ceiling for instance {}.",
            instanceMap->GetMapName()
        );
        mapABInfo->levelScalingDynamicCeiling = 3;
        mapABInfo->levelScalingDynamicFloor = 5;
    }

    //
    // Level Scaling Skip Levels
    //


    // Load the global settings into the map
    mapABInfo->levelScalingSkipHigherLevels = LevelScalingSkipHigherLevels;
    mapABInfo->levelScalingSkipLowerLevels = LevelScalingSkipLowerLevels;

    //
    // Per-instance overrides, if applicable
    //
    if (hasDynamicLevelOverride(map->GetId()))
    {
        AutoBalanceLevelScalingDynamicLevelSettings* myDynamicLevelSettings =
            &levelScalingDynamicLevelOverrides[map->GetId()];

        // LevelScaling.SkipHigherLevels
        if (myDynamicLevelSettings->skipHigher != -1)
            mapABInfo->levelScalingSkipHigherLevels = myDynamicLevelSettings->skipHigher;

        // LevelScaling.SkipLowerLevels
        if (myDynamicLevelSettings->skipLower != -1)
            mapABInfo->levelScalingSkipLowerLevels = myDynamicLevelSettings->skipLower;

        // LevelScaling.DynamicLevelCeiling
        if (myDynamicLevelSettings->ceiling != -1)
            mapABInfo->levelScalingDynamicCeiling = myDynamicLevelSettings->ceiling;

        // LevelScaling.DynamicLevelFloor
        if (myDynamicLevelSettings->floor != -1)
            mapABInfo->levelScalingDynamicFloor = myDynamicLevelSettings->floor;
    }
}

void AutoBalancer::AddCreatureToMapData(Creature* creature, bool addToCreatureList = true, Player* playerToExcludeFromChecks = nullptr, bool forceRecalculation = false)
{
    // make sure we have a creature and that it's assigned to a map
    if (!creature || !creature->GetMap())
        return;

    // if this isn't a dungeon or a battleground, skip
    if (!creature->GetMap()->IsDungeon() && !creature->GetMap()->IsRaid())
        return;

    // get AutoBalance data
    InstanceMap* instanceMap =
        ((InstanceMap*)sMapMgr->FindMap(creature->GetMapId(), creature->GetInstanceId()));
    AutoBalanceMapInfo *mapABInfo =
        instanceMap->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
    AutoBalanceCreatureInfo *creatureABInfo =
        creature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

    // store the creature's original level if this is the first time seeing it
    if (creatureABInfo->UnmodifiedLevel == 0)
    {
        // handle summoned creatures
        if (creature->IsSummon())
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Creature {} ({}->\?\?) is a summon.", creature->GetName(), creature->GetLevel());
            if (creature->ToTempSummon() &&
                creature->ToTempSummon()->GetSummoner() &&
                creature->ToTempSummon()->GetSummoner()->ToCreature())
            {
                Creature* summoner = creature->ToTempSummon()->GetSummoner()->ToCreature();
                if (!summoner)
                {
                    creatureABInfo->UnmodifiedLevel = mapABInfo->avgCreatureLevel;
                    LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Summoned creature {} ({}) is not owned by a summoner.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
                }
                else
                {
                    Creature* summonerCreature = summoner->ToCreature();
                    AutoBalanceCreatureInfo *summonerABInfo=summonerCreature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

                    if (summonerABInfo->UnmodifiedLevel > 0)
                    {
                        creatureABInfo->UnmodifiedLevel = summonerABInfo->UnmodifiedLevel;
                        LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Summoned creature {} ({}) owned by {} ({}->{})", creature->GetName(), creatureABInfo->UnmodifiedLevel, summonerCreature->GetName(), summonerABInfo->UnmodifiedLevel, summonerCreature->GetLevel());
                    }
                    else
                    {
                        creatureABInfo->UnmodifiedLevel = summonerCreature->GetLevel();
                        LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Summoned creature {} ({}) owned by {} ({})", creature->GetName(), creatureABInfo->UnmodifiedLevel, summonerCreature->GetName(), summonerCreature->GetLevel());
                    }
                }
            }
            else
            {
                creatureABInfo->UnmodifiedLevel = mapABInfo->avgCreatureLevel;
                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Summoned creature {} ({}) does not have a summoner.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
            }

            // if this is a summon, we shouldn't track it in any list and it does not 
            // contribute to the average level
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): Summoned creature {} ({}) will not affect the map's stats.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
            return;

        }
        // creature isn't a summon, just store their unmodified level
        else
        {
            creatureABInfo->UnmodifiedLevel = creature->GetLevel();
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({})", creature->GetName(), creatureABInfo->UnmodifiedLevel);
        }
    }

    // if this is a creature controlled by the player, skip
    if (((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer()))
    {
        LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is controlled by the player - skip.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
        return;
    }

    // if this is a non-relevant creature, skip
    if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
    {
        LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is a critter, totem, or trigger - skip.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
        return;
    }

    // if the creature level is below 85% of the minimum LFG level, 
    // assume it's a flavor creature and shouldn't be tracked or modified
    if (creatureABInfo->UnmodifiedLevel < ((float)mapABInfo->lfgMinLevel * .85f))
    {
        LOG_DEBUG("module.AutoBalance",
            "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is below 85% of the LFG min level of {} and is NOT tracked.",
            creature->GetName(),
            creatureABInfo->UnmodifiedLevel,
            mapABInfo->lfgMinLevel
        );
        return;
    }

    // if the creature level is above 125% of the maximum LFG level, 
    // assume it's a flavor creature or holiday boss and shouldn't be tracked or modified
    if (creatureABInfo->UnmodifiedLevel > ((float)mapABInfo->lfgMaxLevel * 1.15f))
    {
        LOG_DEBUG("module.AutoBalance",
            "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is above 115% of the LFG max level of {} and is NOT tracked.",
            creature->GetName(),
            creatureABInfo->UnmodifiedLevel,
            mapABInfo->lfgMaxLevel
        );
        return;
    }

    // is this creature already in the map's creature list?
    bool isCreatureAlreadyInCreatureList = creatureABInfo->isInCreatureList;

    // add the creature to the map's creature list if configured to do so
    if (addToCreatureList && !isCreatureAlreadyInCreatureList)
    {
        mapABInfo->allMapCreatures.push_back(creature);
        creatureABInfo->isInCreatureList = true;
        LOG_DEBUG("module.AutoBalance",
            "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is creature #{} in the creature list.",
            creature->GetName(),
            creatureABInfo->UnmodifiedLevel,
            mapABInfo->allMapCreatures.size()
        );
    }

    // alter stats for the map if needed
    bool isIncludedInMapStats = true;

    // if this creature was already in the creature list, don't consider it for map stats (again)
    // exception for if forceRecalculation is true (used on player enter/exit to recalculate map stats)
    if (isCreatureAlreadyInCreatureList && !forceRecalculation)
    {
        isIncludedInMapStats = false;
    }

    Map::PlayerList const &playerList = creature->GetMap()->GetPlayers();
    if (!playerList.IsEmpty())
    {
        // only do these additional checks if we still think they need to be applied to the map stats
        if (isIncludedInMapStats)
        {
            // if the creature is vendor, trainer, or has gossip, don't use it to update map stats
            if  ((creature->IsVendor() ||
                    creature->HasNpcFlag(UNIT_NPC_FLAG_GOSSIP) ||
                    creature->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER) ||
                    creature->HasNpcFlag(UNIT_NPC_FLAG_TRAINER) ||
                    creature->HasNpcFlag(UNIT_NPC_FLAG_TRAINER_PROFESSION) ||
                    creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR) ||
                    creature->HasUnitFlag(UNIT_FLAG_IMMUNE_TO_PC) ||
                    creature->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE)) &&
                    (!creature->IsDungeonBoss())
                )
            {
                LOG_DEBUG("module.AutoBalance",
                    "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is a a vendor, trainer, or is otherwise not attackable - do not include in map stats.",
                    creature->GetName(),
                    creatureABInfo->UnmodifiedLevel
                );
                isIncludedInMapStats = false;
            }
            else
            {
                // if the creature is friendly to a player, don't use it to update map stats
                for (Map::PlayerList::const_iterator playerIteration = playerList.begin(); playerIteration != playerList.end(); ++playerIteration)
                {
                    Player* playerHandle = playerIteration->GetSource();

                    // if this player matches the player we're supposed to skip, skip
                    if (playerHandle == playerToExcludeFromChecks)
                    {
                        continue;
                    }

                    // if the creature is friendly and not a boss
                    if (creature->IsFriendlyTo(playerHandle) && !creature->IsDungeonBoss())
                    {
                        LOG_DEBUG("module.AutoBalance",
                            "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is friendly to {} - do not include in map stats.",
                            creature->GetName(),
                            creatureABInfo->UnmodifiedLevel,
                            playerHandle->GetName()
                        );
                        isIncludedInMapStats = false;
                        break;
                    }
                }

                // perform the distance check if an override is configured for this map
                if (hasLevelScalingDistanceCheckOverride(instanceMap->GetId()))
                {
                    uint32 distance = levelScalingDistanceCheckOverrides[instanceMap->GetId()];
                    bool isPlayerWithinDistance = false;

                    for (Map::PlayerList::const_iterator playerIteration = playerList.begin(); playerIteration != playerList.end(); ++playerIteration)
                    {
                        Player* playerHandle = playerIteration->GetSource();

                        // if this player matches the player we're supposed to skip, skip
                        if (playerHandle == playerToExcludeFromChecks)
                        {
                            continue;
                        }

                        if (playerHandle->IsWithinDist(creature, 500))
                        {
                            LOG_DEBUG("module.AutoBalance",
                                "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is in range ({} world units) of player {} and is considered active.",
                                creature->GetName(),
                                creatureABInfo->UnmodifiedLevel,
                                distance,
                                playerHandle->GetName()
                            );
                            isPlayerWithinDistance = true;
                            break;
                        }
                        else
                        {
                            LOG_DEBUG("module.AutoBalance",
                                "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is NOT in range ({} world units) of any player and is NOT considered active.",
                                creature->GetName(),
                                creature->GetLevel(),
                                distance
                            );
                        }
                    }

                    // if no players were within the distance, don't include this creature in the map stats
                    if (!isPlayerWithinDistance)
                        isIncludedInMapStats = false;
                }
            }
        }

        if (isIncludedInMapStats)
        {
            // mark this creature as being considered in the map stats
            creatureABInfo->isActive = true;

            // update the highest and lowest creature levels
            if (creatureABInfo->UnmodifiedLevel > mapABInfo->highestCreatureLevel || mapABInfo->highestCreatureLevel == 0)
                mapABInfo->highestCreatureLevel = creatureABInfo->UnmodifiedLevel;
            if (creatureABInfo->UnmodifiedLevel < mapABInfo->lowestCreatureLevel || mapABInfo->lowestCreatureLevel == 0)
                mapABInfo->lowestCreatureLevel = creatureABInfo->UnmodifiedLevel;

            // calculate the new average creature level
            float creatureCount = mapABInfo->activeCreatureCount;
            float newAvgCreatureLevel = (((float)mapABInfo->avgCreatureLevel * creatureCount) + (float)creatureABInfo->UnmodifiedLevel) / (creatureCount + 1.0f);
            mapABInfo->avgCreatureLevel = newAvgCreatureLevel;

            // increment the active creature counter
            mapABInfo->activeCreatureCount++;

            LOG_DEBUG("module.AutoBalance",
                "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is included in map stats, adjusting avgCreatureLevel to {}",
                creature->GetName(),
                creatureABInfo->UnmodifiedLevel,
                newAvgCreatureLevel
            );

            // reset the last config time so that the map data will get updated
            lastConfigTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            LOG_DEBUG("module.AutoBalance",
                "AutoBalance_AllCreature::AddCreatureToMapData(): lastConfigTime reset to {}",
                lastConfigTime
            );
        }
        else if (isCreatureAlreadyInCreatureList)
        {
            LOG_DEBUG("module.AutoBalance",
                "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is already included in map stats.",
                creature->GetName(),creatureABInfo->UnmodifiedLevel
            );
        }
        else
        {
            LOG_DEBUG("module.AutoBalance",
                "AutoBalance_AllCreature::AddCreatureToMapData(): {} ({}) is NOT included in map stats.",
                creature->GetName(),
                creatureABInfo->UnmodifiedLevel
            );
        }

        LOG_DEBUG("module.AutoBalance",
            "AutoBalance_AllCreature::AddCreatureToMapData(): There are {} active creatures.",
            mapABInfo->activeCreatureCount
        );
    }
}

void AutoBalancer::RemoveCreatureFromMapData(Creature* creature)
{
    // get map data
    AutoBalanceMapInfo *mapABInfo = 
        creature->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

    // if the creature is in the all creature list, remove it
    if (mapABInfo->allMapCreatures.size() > 0)
    {
        for (
                std::vector<Creature*>::iterator creatureIteration = mapABInfo->allMapCreatures.begin();
                creatureIteration != mapABInfo->allMapCreatures.end();
                ++creatureIteration
            )
        {
            if (*creatureIteration == creature)
            {
                LOG_DEBUG("module.AutoBalance",
                    "AutoBalance_AllCreature::RemoveCreatureFromMapData(): {} ({}) is in the creature list and will be removed. There are {} creatures left.",
                    creature->GetName(),
                    creature->GetLevel(),
                    mapABInfo->allMapCreatures.size() - 1
                );
                mapABInfo->allMapCreatures.erase(creatureIteration);

                // mark this creature as removed
                AutoBalanceCreatureInfo *creatureABInfo = 
                    creature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");
                creatureABInfo->isInCreatureList = false;
                break;
            }
        }
    }
}

void AutoBalancer::UpdateMapLevelIfNeeded(Map* map)
{
    // get map data
    AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

    // if map needs update
    if (mapABInfo->configTime != lastConfigTime)
    {
        LOG_DEBUG("module.AutoBalance",
            "UpdateMapLevelIfNeeded(): Map {} config is out of date ({} != {}) and will be updated.",
            map->GetMapName(),
            mapABInfo->configTime,
            lastConfigTime
        );

        // load the map's settings
        LoadMapSettings(map);

        // if LevelScaling is disabled OR if the average creature level is inside the skip range,
        // set the map level to the average creature level, rounded to the nearest integer
        if (!LevelScaling ||
            ((mapABInfo->avgCreatureLevel <= mapABInfo->highestPlayerLevel + mapABInfo->levelScalingSkipHigherLevels && mapABInfo->levelScalingSkipHigherLevels != 0) &&
              (mapABInfo->avgCreatureLevel >= mapABInfo->highestPlayerLevel - mapABInfo->levelScalingSkipLowerLevels && mapABInfo->levelScalingSkipLowerLevels != 0))
           )
        {
            mapABInfo->mapLevel = (uint8)(mapABInfo->avgCreatureLevel + 0.5f);
            mapABInfo->isLevelScalingEnabled = false;
        }
        // If the average creature level is lower than the highest player level,
        // set the map level to the average creature level, rounded to the nearest integer
        else if (mapABInfo->avgCreatureLevel <= mapABInfo->highestPlayerLevel)
        {
            mapABInfo->mapLevel = (uint8)(mapABInfo->avgCreatureLevel + 0.5f);
            mapABInfo->isLevelScalingEnabled = true;
        }
        // caps at the highest player level
        else
        {
            mapABInfo->mapLevel = mapABInfo->highestPlayerLevel;
            mapABInfo->isLevelScalingEnabled = true;
        }

        LOG_DEBUG("module.AutoBalance",
            "UpdateMapLevelIfNeeded(): Map {} level is now {}.",
            map->GetMapName(),
            mapABInfo->mapLevel
        );

        // mark the config updated
        mapABInfo->configTime = lastConfigTime;
    }
}

void AutoBalancer::UpdateMapPlayerStats(Map* map)
{
    // get the map's info
    AutoBalanceMapInfo *mapABInfo=map->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

    // get the map's player list
    Map::PlayerList const &playerList = map->GetPlayers();

    // if there are players on the map
    if (!playerList.IsEmpty())
    {
        uint8 highestPlayerLevel = 0;
        uint8 lowestPlayerLevel = 0;

        // iterate through the players and update the highest and lowest player levels
        for (
                Map::PlayerList::const_iterator playerIteration = playerList.begin();
                playerIteration != playerList.end();
                ++playerIteration
            )
        {
            Player* playerHandle = playerIteration->GetSource();
            if (playerHandle && !playerHandle->IsGameMaster())
            {
                if (playerHandle->getLevel() > highestPlayerLevel || highestPlayerLevel == 0)
                    highestPlayerLevel = playerHandle->getLevel();

                if (playerHandle->getLevel() < lowestPlayerLevel || lowestPlayerLevel == 0)
                    lowestPlayerLevel = playerHandle->getLevel();
            }
            mapABInfo->highestPlayerLevel = highestPlayerLevel;
            mapABInfo->lowestPlayerLevel = lowestPlayerLevel;
        }

        LOG_DEBUG("module.AutoBalance",
            "UpdateMapPlayerStats(): Map {} player level range: {} - {}.",
            map->GetMapName(),
            mapABInfo->lowestPlayerLevel,
            mapABInfo->highestPlayerLevel
        );
    }

    // update the player count
    mapABInfo->playerCount = map->GetPlayersCountExceptGMs();
}

// Used for reading the string from the configuration file to for those creatures 
// who need to be scaled for XX number of players.
void AutoBalancer::LoadForcedCreatureIdsFromString(std::string creatureIds, int forcedPlayerCount) 
{
    std::string delimitedValue;
    std::stringstream creatureIdsStream;

    creatureIdsStream.str(creatureIds);
    // Process each Creature ID in the string, delimited by the comma - ","
    while (std::getline(creatureIdsStream, delimitedValue, ','))
    {
        int creatureId = atoi(delimitedValue.c_str());
        if (creatureId >= 0)
        {
            forcedCreatureIds[creatureId] = forcedPlayerCount;
        }
    }
}

int AutoBalancer::GetForcedNumPlayers(int creatureId)
{
    // Don't want the forcedCreatureIds map to blowup to a massive empty array
    if (forcedCreatureIds.find(creatureId) == forcedCreatureIds.end())
    {
        return -1;
    }
    return forcedCreatureIds[creatureId];
}

void AutoBalancer::SetGroupDifficulty(Player* player, uint8 difficulty) {

    Group* group = player->GetGroup();
    LOG_INFO("module.AutoBalance",
        "Setting group difficulty to {} for group {}",
        difficulty,
        group->GetGUID().GetCounter()
    );
    CharacterDatabase.DirectExecute(
        "UPDATE group_difficulty SET difficulty = {} WHERE guid = {}",
        difficulty,
        group->GetGUID().GetCounter()
    );
}

uint8 AutoBalancer::GetGroupDifficulty(const Group* group) {
    if(!group)
        return 0;

    QueryResult result = CharacterDatabase.Query(
        "SELECT difficulty FROM group_difficulty WHERE guid = {}",
        group->GetGUID().GetCounter()
    );

    if (!result) {
        return 0;
    }

    Field* fields = result->Fetch();

    if(!fields)
    {
        LOG_INFO("module.AutoBalance",
            "No difficulty found for group {}",
            group->GetGUID().GetCounter()
        );
        return 0;
    }


    return fields[0].Get<uint8>();
}
