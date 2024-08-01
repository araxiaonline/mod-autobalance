#ifndef AUTOBALANCER_H
#define AUTOBALANCER_H

#include "AutoBalance.h"
#include <vector>

// struct AutoBalancerStatModifier
// {
//     float GlobalVal;
//     float Health;
//     float Mana;
//     float Armor;
//     float Damage;
//     float CCDuration;
//     float DoTDamage;
//     float SpellDamage;

//     // Person(const std::string& n, int a) : name(n), age(a) {}
//     // AutoBalancerStatModifier(float g) : GlobalVal(g) { };
// };

// struct AutoBalancerInflectionPoint
// {
//     float InflectionPoint;
//     float CurveFloor;
//     float CurveCeiling;
//     float Boss;

// };

class AutoBalancer
{
private:

public:
    AutoBalancer(/* args */);
    ~AutoBalancer();

    void LoadEnabledDungeons(std::string dungeonIdString);

    std::map<uint32, AutoBalanceInflectionPointSettings> LoadInflectionPointOverrides(std::string dungeonIdString);

    std::map<uint32, AutoBalanceStatModifiers> LoadStatModifierOverrides(std::string dungeonIdString);

    std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> LoadDynamicLevelOverrides(std::string dungeonIdString);

    std::map<uint32, uint32> LoadDistanceCheckOverrides(std::string dungeonIdString);

    bool isEnabledDungeon(uint32 dungeonId);

    bool perDungeonScalingEnabled();

    bool hasDungeonOverride(uint32 dungeonId);

    bool hasBossOverride(uint32 dungeonId);

    bool hasStatModifierOverride(uint32 dungeonId);

    bool hasStatModifierBossOverride(uint32 dungeonId);

    bool hasStatModifierCreatureOverride(uint32 creatureId);

    bool hasDynamicLevelOverride(uint32 dungeonId);

    bool hasLevelScalingDistanceCheckOverride(uint32 dungeonId);

    bool ShouldMapBeEnabled(Map *map);

    void LoadMapSettings(Map *map);

    void AddCreatureToMapData(Creature *creature, bool addToCreatureList, Player *playerToExcludeFromChecks, bool forceRecalculation);

    void RemoveCreatureFromMapData(Creature *creature);

    void UpdateMapLevelIfNeeded(Map *map);

    void UpdateMapPlayerStats(Map *map);

    void LoadForcedCreatureIdsFromString(std::string creatureIds, int forcedPlayerCount);

    int GetForcedNumPlayers(int creatureId);

    void SetGroupDifficulty(Player *player, uint8 difficulty);

    uint8 GetGroupDifficulty(const Group *group);

    // The map values correspond with the .AutoBalance.XX.Name entries in the configuration file.
    std::map<int, int> forcedCreatureIds;
    std::map<uint32, uint8> enabledDungeonIds;
    std::map<uint32, AutoBalanceInflectionPointSettings> dungeonOverrides;
    std::map<uint32, AutoBalanceInflectionPointSettings> bossOverrides;
    std::map<uint32, AutoBalanceStatModifiers> statModifierOverrides;
    std::map<uint32, AutoBalanceStatModifiers> statModifierBossOverrides;
    std::map<uint32, AutoBalanceStatModifiers> statModifierCreatureOverrides;
    std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> levelScalingDynamicLevelOverrides;
    std::map<uint32, uint32> levelScalingDistanceCheckOverrides;
    // cheaphack for difficulty server-wide.
    // Another value TODO in player class for the party leader's value to determine dungeon difficulty.
    int8 PlayerCountDifficultyOffset;
    float MythicMultiplier, LegendaryMultiplier, AscendantMultiplier;
    bool LevelScaling;
    int8 LevelScalingSkipHigherLevels, LevelScalingSkipLowerLevels;
    int8 LevelScalingDynamicLevelCeilingDungeons, LevelScalingDynamicLevelFloorDungeons, LevelScalingDynamicLevelCeilingRaids, LevelScalingDynamicLevelFloorRaids;
    int8 LevelScalingDynamicLevelCeilingHeroicDungeons, LevelScalingDynamicLevelFloorHeroicDungeons, LevelScalingDynamicLevelCeilingHeroicRaids, LevelScalingDynamicLevelFloorHeroicRaids;
    ScalingMethod LevelScalingMethod;
    uint32 rewardRaid;
    uint32 rewardDungeon;
    uint32 MinPlayerReward;
    bool Announcement;
    bool LevelScalingEndGameBoost, PlayerChangeNotify, rewardEnabled;
    float MinHPModifier, MinManaModifier, MinDamageModifier, MinCCDurationModifier, MaxCCDurationModifier;

    // RewardScaling.*
    ScalingMethod RewardScalingMethod;
    bool RewardScalingXP, RewardScalingMoney;
    float RewardScalingXPModifier, RewardScalingMoneyModifier;

    // Track the last time the config was reloaded
    uint64_t lastConfigTime;

    // Enable.*
    bool EnableGlobal;
    bool Enable5M, Enable10M, Enable15M, Enable20M, Enable25M, Enable40M;
    bool Enable5MHeroic, Enable10MHeroic, Enable25MHeroic;
    bool EnableOtherNormal, EnableOtherHeroic;

    // InflectionPoint*
    float InflectionPoint, InflectionPointCurveFloor, InflectionPointCurveCeiling, InflectionPointBoss;
    float InflectionPointHeroic, InflectionPointHeroicCurveFloor, InflectionPointHeroicCurveCeiling, InflectionPointHeroicBoss;
    float InflectionPointRaid, InflectionPointRaidCurveFloor, InflectionPointRaidCurveCeiling, InflectionPointRaidBoss;
    float InflectionPointRaidHeroic, InflectionPointRaidHeroicCurveFloor, InflectionPointRaidHeroicCurveCeiling, InflectionPointRaidHeroicBoss;

    float InflectionPointRaid10M, InflectionPointRaid10MCurveFloor, InflectionPointRaid10MCurveCeiling, InflectionPointRaid10MBoss;
    float InflectionPointRaid10MHeroic, InflectionPointRaid10MHeroicCurveFloor, InflectionPointRaid10MHeroicCurveCeiling, InflectionPointRaid10MHeroicBoss;
    float InflectionPointRaid15M, InflectionPointRaid15MCurveFloor, InflectionPointRaid15MCurveCeiling, InflectionPointRaid15MBoss;
    float InflectionPointRaid20M, InflectionPointRaid20MCurveFloor, InflectionPointRaid20MCurveCeiling, InflectionPointRaid20MBoss;
    float InflectionPointRaid25M, InflectionPointRaid25MCurveFloor, InflectionPointRaid25MCurveCeiling, InflectionPointRaid25MBoss;
    float InflectionPointRaid25MHeroic, InflectionPointRaid25MHeroicCurveFloor, InflectionPointRaid25MHeroicCurveCeiling, InflectionPointRaid25MHeroicBoss;
    float InflectionPointRaid40M, InflectionPointRaid40MCurveFloor, InflectionPointRaid40MCurveCeiling, InflectionPointRaid40MBoss;

    // StatModifier*
    float StatModifier_Global, StatModifier_Health, StatModifier_Mana, StatModifier_Armor, StatModifier_Damage, StatModifier_SpellDamage, StatModifier_DoTDamage, StatModifier_CCDuration;
    float StatModifierHeroic_Global, StatModifierHeroic_Health, StatModifierHeroic_Mana, StatModifierHeroic_Armor, StatModifierHeroic_Damage, StatModifierHeroic_CCDuration;
    float StatModifierRaid_Global, StatModifierRaid_Health, StatModifierRaid_Mana, StatModifierRaid_Armor, StatModifierRaid_Damage, StatModifierRaid_CCDuration;
    float StatModifierRaidHeroic_Global, StatModifierRaidHeroic_Health, StatModifierRaidHeroic_Mana, StatModifierRaidHeroic_Armor, StatModifierRaidHeroic_Damage, StatModifierRaidHeroic_CCDuration;

    float StatModifierRaid10M_Global, StatModifierRaid10M_Health, StatModifierRaid10M_Mana, StatModifierRaid10M_Armor, StatModifierRaid10M_Damage, StatModifierRaid10M_CCDuration;
    float StatModifierRaid10MHeroic_Global, StatModifierRaid10MHeroic_Health, StatModifierRaid10MHeroic_Mana, StatModifierRaid10MHeroic_Armor, StatModifierRaid10MHeroic_Damage, StatModifierRaid10MHeroic_CCDuration;
    float StatModifierRaid15M_Global, StatModifierRaid15M_Health, StatModifierRaid15M_Mana, StatModifierRaid15M_Armor, StatModifierRaid15M_Damage, StatModifierRaid15M_CCDuration;
    float StatModifierRaid20M_Global, StatModifierRaid20M_Health, StatModifierRaid20M_Mana, StatModifierRaid20M_Armor, StatModifierRaid20M_Damage, StatModifierRaid20M_CCDuration;
    float StatModifierRaid25M_Global, StatModifierRaid25M_Health, StatModifierRaid25M_Mana, StatModifierRaid25M_Armor, StatModifierRaid25M_Damage, StatModifierRaid25M_CCDuration;
    float StatModifierRaid25MHeroic_Global, StatModifierRaid25MHeroic_Health, StatModifierRaid25MHeroic_Mana, StatModifierRaid25MHeroic_Armor, StatModifierRaid25MHeroic_Damage, StatModifierRaid25MHeroic_CCDuration;
    float StatModifierRaid40M_Global, StatModifierRaid40M_Health, StatModifierRaid40M_Mana, StatModifierRaid40M_Armor, StatModifierRaid40M_Damage, StatModifierRaid40M_CCDuration;

    // StatModifier* (Boss)
    float StatModifier_Boss_Global, StatModifier_Boss_Health, StatModifier_Boss_Mana, StatModifier_Boss_Armor, StatModifier_Boss_Damage, StatModifier_Boss_CCDuration;
    float StatModifierHeroic_Boss_Global, StatModifierHeroic_Boss_Health, StatModifierHeroic_Boss_Mana, StatModifierHeroic_Boss_Armor, StatModifierHeroic_Boss_Damage, StatModifierHeroic_Boss_CCDuration;
    float StatModifierRaid_Boss_Global, StatModifierRaid_Boss_Health, StatModifierRaid_Boss_Mana, StatModifierRaid_Boss_Armor, StatModifierRaid_Boss_Damage, StatModifierRaid_Boss_CCDuration;
    float StatModifierRaidHeroic_Boss_Global, StatModifierRaidHeroic_Boss_Health, StatModifierRaidHeroic_Boss_Mana, StatModifierRaidHeroic_Boss_Armor, StatModifierRaidHeroic_Boss_Damage, StatModifierRaidHeroic_Boss_CCDuration;

    float StatModifierRaid10M_Boss_Global, StatModifierRaid10M_Boss_Health, StatModifierRaid10M_Boss_Mana, StatModifierRaid10M_Boss_Armor, StatModifierRaid10M_Boss_Damage, StatModifierRaid10M_Boss_CCDuration;
    float StatModifierRaid10MHeroic_Boss_Global, StatModifierRaid10MHeroic_Boss_Health, StatModifierRaid10MHeroic_Boss_Mana, StatModifierRaid10MHeroic_Boss_Armor, StatModifierRaid10MHeroic_Boss_Damage, StatModifierRaid10MHeroic_Boss_CCDuration;
    float StatModifierRaid15M_Boss_Global, StatModifierRaid15M_Boss_Health, StatModifierRaid15M_Boss_Mana, StatModifierRaid15M_Boss_Armor, StatModifierRaid15M_Boss_Damage, StatModifierRaid15M_Boss_CCDuration;
    float StatModifierRaid20M_Boss_Global, StatModifierRaid20M_Boss_Health, StatModifierRaid20M_Boss_Mana, StatModifierRaid20M_Boss_Armor, StatModifierRaid20M_Boss_Damage, StatModifierRaid20M_Boss_CCDuration;
    float StatModifierRaid25M_Boss_Global, StatModifierRaid25M_Boss_Health, StatModifierRaid25M_Boss_Mana, StatModifierRaid25M_Boss_Armor, StatModifierRaid25M_Boss_Damage, StatModifierRaid25M_Boss_CCDuration;
    float StatModifierRaid25MHeroic_Boss_Global, StatModifierRaid25MHeroic_Boss_Health, StatModifierRaid25MHeroic_Boss_Mana, StatModifierRaid25MHeroic_Boss_Armor, StatModifierRaid25MHeroic_Boss_Damage, StatModifierRaid25MHeroic_Boss_CCDuration;
    float StatModifierRaid40M_Boss_Global, StatModifierRaid40M_Boss_Health, StatModifierRaid40M_Boss_Mana, StatModifierRaid40M_Boss_Armor, StatModifierRaid40M_Boss_Damage, StatModifierRaid40M_Boss_CCDuration;

    // std::map<uint8, AutoBalancerInflectionPoint> InfPointNormal;
    // std::map<uint8, AutoBalancerInflectionPoint> InfPointHeroic;

    // std::map<uint8, AutoBalancerStatModifier> StatsNormal;
    // std::map<uint8, AutoBalancerStatModifier> StatsNormalBoss;
    // std::map<uint8, AutoBalancerStatModifier> StatsHeroic;
    // std::map<uint8, AutoBalancerStatModifier> StatsHeroicBoss;

    static AutoBalancer * getInstance() {
        AutoBalancer instance;
        return &instance;
    }
};

#define sAutoBalancer AutoBalancer::getInstance()

#endif