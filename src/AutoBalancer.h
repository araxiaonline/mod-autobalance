#ifndef AUTOBALANCER_H
#define AUTOBALANCER_H

#include <vector>

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
    static std::map<int, int> forcedCreatureIds;
    static std::map<uint32, uint8> enabledDungeonIds;
    static std::map<uint32, AutoBalanceInflectionPointSettings> dungeonOverrides;
    static std::map<uint32, AutoBalanceInflectionPointSettings> bossOverrides;
    static std::map<uint32, AutoBalanceStatModifiers> statModifierOverrides;
    static std::map<uint32, AutoBalanceStatModifiers> statModifierBossOverrides;
    static std::map<uint32, AutoBalanceStatModifiers> statModifierCreatureOverrides;
    static std::map<uint8, AutoBalanceLevelScalingDynamicLevelSettings> levelScalingDynamicLevelOverrides;
    static std::map<uint32, uint32> levelScalingDistanceCheckOverrides;
    // cheaphack for difficulty server-wide.
    // Another value TODO in player class for the party leader's value to determine dungeon difficulty.
    static int8 PlayerCountDifficultyOffset;
    static float MythicMultiplier, LegendaryMultiplier, AscendantMultiplier;
    static bool LevelScaling;
    static int8 LevelScalingSkipHigherLevels, LevelScalingSkipLowerLevels;
    static int8 LevelScalingDynamicLevelCeilingDungeons, LevelScalingDynamicLevelFloorDungeons, LevelScalingDynamicLevelCeilingRaids, LevelScalingDynamicLevelFloorRaids;
    static int8 LevelScalingDynamicLevelCeilingHeroicDungeons, LevelScalingDynamicLevelFloorHeroicDungeons, LevelScalingDynamicLevelCeilingHeroicRaids, LevelScalingDynamicLevelFloorHeroicRaids;
    static ScalingMethod LevelScalingMethod;
    static uint32 rewardRaid;
    static uint32 rewardDungeon;
    static uint32 MinPlayerReward;
    static bool Announcement;
    static bool LevelScalingEndGameBoost, PlayerChangeNotify, rewardEnabled;
    static float MinHPModifier, MinManaModifier, MinDamageModifier, MinCCDurationModifier, MaxCCDurationModifier;

    // RewardScaling.*
    static ScalingMethod RewardScalingMethod;
    static bool RewardScalingXP, RewardScalingMoney;
    static float RewardScalingXPModifier, RewardScalingMoneyModifier;

    // Track the last time the config was reloaded
    static uint64_t lastConfigTime;

    // Enable.*
    static bool EnableGlobal;
    static bool Enable5M, Enable10M, Enable15M, Enable20M, Enable25M, Enable40M;
    static bool Enable5MHeroic, Enable10MHeroic, Enable25MHeroic;
    static bool EnableOtherNormal, EnableOtherHeroic;

    // InflectionPoint*
    static float InflectionPoint, InflectionPointCurveFloor, InflectionPointCurveCeiling, InflectionPointBoss;
    static float InflectionPointHeroic, InflectionPointHeroicCurveFloor, InflectionPointHeroicCurveCeiling, InflectionPointHeroicBoss;
    static float InflectionPointRaid, InflectionPointRaidCurveFloor, InflectionPointRaidCurveCeiling, InflectionPointRaidBoss;
    static float InflectionPointRaidHeroic, InflectionPointRaidHeroicCurveFloor, InflectionPointRaidHeroicCurveCeiling, InflectionPointRaidHeroicBoss;

    static float InflectionPointRaid10M, InflectionPointRaid10MCurveFloor, InflectionPointRaid10MCurveCeiling, InflectionPointRaid10MBoss;
    static float InflectionPointRaid10MHeroic, InflectionPointRaid10MHeroicCurveFloor, InflectionPointRaid10MHeroicCurveCeiling, InflectionPointRaid10MHeroicBoss;
    static float InflectionPointRaid15M, InflectionPointRaid15MCurveFloor, InflectionPointRaid15MCurveCeiling, InflectionPointRaid15MBoss;
    static float InflectionPointRaid20M, InflectionPointRaid20MCurveFloor, InflectionPointRaid20MCurveCeiling, InflectionPointRaid20MBoss;
    static float InflectionPointRaid25M, InflectionPointRaid25MCurveFloor, InflectionPointRaid25MCurveCeiling, InflectionPointRaid25MBoss;
    static float InflectionPointRaid25MHeroic, InflectionPointRaid25MHeroicCurveFloor, InflectionPointRaid25MHeroicCurveCeiling, InflectionPointRaid25MHeroicBoss;
    static float InflectionPointRaid40M, InflectionPointRaid40MCurveFloor, InflectionPointRaid40MCurveCeiling, InflectionPointRaid40MBoss;

    // StatModifier*
    static float StatModifier_Global, StatModifier_Health, StatModifier_Mana, StatModifier_Armor, StatModifier_Damage, StatModifier_SpellDamage, StatModifier_DoTDamage, StatModifier_CCDuration;
    static float StatModifierHeroic_Global, StatModifierHeroic_Health, StatModifierHeroic_Mana, StatModifierHeroic_Armor, StatModifierHeroic_Damage, StatModifierHeroic_CCDuration;
    static float StatModifierRaid_Global, StatModifierRaid_Health, StatModifierRaid_Mana, StatModifierRaid_Armor, StatModifierRaid_Damage, StatModifierRaid_CCDuration;
    static float StatModifierRaidHeroic_Global, StatModifierRaidHeroic_Health, StatModifierRaidHeroic_Mana, StatModifierRaidHeroic_Armor, StatModifierRaidHeroic_Damage, StatModifierRaidHeroic_CCDuration;

    static float StatModifierRaid10M_Global, StatModifierRaid10M_Health, StatModifierRaid10M_Mana, StatModifierRaid10M_Armor, StatModifierRaid10M_Damage, StatModifierRaid10M_CCDuration;
    static float StatModifierRaid10MHeroic_Global, StatModifierRaid10MHeroic_Health, StatModifierRaid10MHeroic_Mana, StatModifierRaid10MHeroic_Armor, StatModifierRaid10MHeroic_Damage, StatModifierRaid10MHeroic_CCDuration;
    static float StatModifierRaid15M_Global, StatModifierRaid15M_Health, StatModifierRaid15M_Mana, StatModifierRaid15M_Armor, StatModifierRaid15M_Damage, StatModifierRaid15M_CCDuration;
    static float StatModifierRaid20M_Global, StatModifierRaid20M_Health, StatModifierRaid20M_Mana, StatModifierRaid20M_Armor, StatModifierRaid20M_Damage, StatModifierRaid20M_CCDuration;
    static float StatModifierRaid25M_Global, StatModifierRaid25M_Health, StatModifierRaid25M_Mana, StatModifierRaid25M_Armor, StatModifierRaid25M_Damage, StatModifierRaid25M_CCDuration;
    static float StatModifierRaid25MHeroic_Global, StatModifierRaid25MHeroic_Health, StatModifierRaid25MHeroic_Mana, StatModifierRaid25MHeroic_Armor, StatModifierRaid25MHeroic_Damage, StatModifierRaid25MHeroic_CCDuration;
    static float StatModifierRaid40M_Global, StatModifierRaid40M_Health, StatModifierRaid40M_Mana, StatModifierRaid40M_Armor, StatModifierRaid40M_Damage, StatModifierRaid40M_CCDuration;

    // StatModifier* (Boss)
    static float StatModifier_Boss_Global, StatModifier_Boss_Health, StatModifier_Boss_Mana, StatModifier_Boss_Armor, StatModifier_Boss_Damage, StatModifier_Boss_CCDuration;
    static float StatModifierHeroic_Boss_Global, StatModifierHeroic_Boss_Health, StatModifierHeroic_Boss_Mana, StatModifierHeroic_Boss_Armor, StatModifierHeroic_Boss_Damage, StatModifierHeroic_Boss_CCDuration;
    static float StatModifierRaid_Boss_Global, StatModifierRaid_Boss_Health, StatModifierRaid_Boss_Mana, StatModifierRaid_Boss_Armor, StatModifierRaid_Boss_Damage, StatModifierRaid_Boss_CCDuration;
    static float StatModifierRaidHeroic_Boss_Global, StatModifierRaidHeroic_Boss_Health, StatModifierRaidHeroic_Boss_Mana, StatModifierRaidHeroic_Boss_Armor, StatModifierRaidHeroic_Boss_Damage, StatModifierRaidHeroic_Boss_CCDuration;

    static float StatModifierRaid10M_Boss_Global, StatModifierRaid10M_Boss_Health, StatModifierRaid10M_Boss_Mana, StatModifierRaid10M_Boss_Armor, StatModifierRaid10M_Boss_Damage, StatModifierRaid10M_Boss_CCDuration;
    static float StatModifierRaid10MHeroic_Boss_Global, StatModifierRaid10MHeroic_Boss_Health, StatModifierRaid10MHeroic_Boss_Mana, StatModifierRaid10MHeroic_Boss_Armor, StatModifierRaid10MHeroic_Boss_Damage, StatModifierRaid10MHeroic_Boss_CCDuration;
    static float StatModifierRaid15M_Boss_Global, StatModifierRaid15M_Boss_Health, StatModifierRaid15M_Boss_Mana, StatModifierRaid15M_Boss_Armor, StatModifierRaid15M_Boss_Damage, StatModifierRaid15M_Boss_CCDuration;
    static float StatModifierRaid20M_Boss_Global, StatModifierRaid20M_Boss_Health, StatModifierRaid20M_Boss_Mana, StatModifierRaid20M_Boss_Armor, StatModifierRaid20M_Boss_Damage, StatModifierRaid20M_Boss_CCDuration;
    static float StatModifierRaid25M_Boss_Global, StatModifierRaid25M_Boss_Health, StatModifierRaid25M_Boss_Mana, StatModifierRaid25M_Boss_Armor, StatModifierRaid25M_Boss_Damage, StatModifierRaid25M_Boss_CCDuration;
    static float StatModifierRaid25MHeroic_Boss_Global, StatModifierRaid25MHeroic_Boss_Health, StatModifierRaid25MHeroic_Boss_Mana, StatModifierRaid25MHeroic_Boss_Armor, StatModifierRaid25MHeroic_Boss_Damage, StatModifierRaid25MHeroic_Boss_CCDuration;
    static float StatModifierRaid40M_Boss_Global, StatModifierRaid40M_Boss_Health, StatModifierRaid40M_Boss_Mana, StatModifierRaid40M_Boss_Armor, StatModifierRaid40M_Boss_Damage, StatModifierRaid40M_Boss_CCDuration;

    static AutoBalancer * getInstance() {
        static AutoBalancer instance;
        return &instance;
    }
};

#define sAutoBalancer AutoBalancer::getInstance()

#endif