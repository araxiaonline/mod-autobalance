#include "AutoBalancer.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"

class AutoBalance_WorldScript : public WorldScript
{
    public:
    AutoBalance_WorldScript()
        : WorldScript("AutoBalance_WorldScript")
    {
    }

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        SetInitialWorldSettings();
        sAutoBalancer->lastConfigTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void OnStartup() override
    {
    }

    void SetInitialWorldSettings()
    {
        sAutoBalancer->forcedCreatureIds.clear();
        sAutoBalancer->enabledDungeonIds.clear();
        sAutoBalancer->dungeonOverrides.clear();
        sAutoBalancer->bossOverrides.clear();
        sAutoBalancer->statModifierOverrides.clear();
        sAutoBalancer->statModifierBossOverrides.clear();
        sAutoBalancer->statModifierCreatureOverrides.clear();
        sAutoBalancer->levelScalingDynamicLevelOverrides.clear();
        sAutoBalancer->levelScalingDistanceCheckOverrides.clear();
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.ForcedID40", ""), 40);
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.ForcedID25", ""), 25);
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.ForcedID10", ""), 10);
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.ForcedID5", ""), 5);
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.ForcedID2", ""), 2);
        sAutoBalancer->LoadForcedCreatureIdsFromString(sConfigMgr->GetOption<std::string>("AutoBalance.DisabledID", ""), 0);
        sAutoBalancer->LoadEnabledDungeons(sConfigMgr->GetOption<std::string>("AutoBalance.PerDungeonPlayerCounts", ""));

        // Overrides
        if (sConfigMgr->GetOption<float>("AutoBalance.PerDungeonScaling", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.PerDungeonScaling` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
            sAutoBalancer->dungeonOverrides = sAutoBalancer->LoadInflectionPointOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.InflectionPoint.PerInstance",sConfigMgr->GetOption<std::string>("AutoBalance.PerDungeonScaling", "", false), false)
        ); // `AutoBalance.PerDungeonScaling` for backwards compatibility

        if (sConfigMgr->GetOption<float>("AutoBalance.PerDungeonBossScaling", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.PerDungeonBossScaling` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
            sAutoBalancer->bossOverrides = sAutoBalancer->LoadInflectionPointOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.InflectionPoint.Boss.PerInstance", sConfigMgr->GetOption<std::string>("AutoBalance.PerDungeonBossScaling", "", false), false)
        ); // `AutoBalance.PerDungeonBossScaling` for backwards compatibility

            sAutoBalancer->statModifierOverrides = sAutoBalancer->LoadStatModifierOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.StatModifier.PerInstance", "", false)
        );

        sAutoBalancer->statModifierBossOverrides = sAutoBalancer->LoadStatModifierOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.StatModifier.Boss.PerInstance", "", false)
        );

        sAutoBalancer->statModifierCreatureOverrides = sAutoBalancer->LoadStatModifierOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.StatModifier.PerCreature", "", false)
        );

        sAutoBalancer->levelScalingDynamicLevelOverrides = sAutoBalancer->LoadDynamicLevelOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.LevelScaling.DynamicLevel.PerInstance", "", false)
        );

        sAutoBalancer->levelScalingDistanceCheckOverrides = sAutoBalancer->LoadDistanceCheckOverrides(
            sConfigMgr->GetOption<std::string>("AutoBalance.LevelScaling.DynamicLevel.DistanceCheck.PerInstance", "", false)
        );

        // AutoBalance.Enable.*
        // Deprecated setting warning
        if (sConfigMgr->GetOption<int>("AutoBalance.enable", -1, false) != -1)
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.enable` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");

        sAutoBalancer->EnableGlobal = sConfigMgr->GetOption<bool>("AutoBalance.Enable.Global", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false)); // `AutoBalance.enable` for backwards compatibility

        sAutoBalancer->Enable5M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.5M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable10M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.10M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable15M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.15M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable20M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.20M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable25M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.25M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable40M = sConfigMgr->GetOption<bool>("AutoBalance.Enable.40M", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->EnableOtherNormal = sConfigMgr->GetOption<bool>("AutoBalance.Enable.OtherNormal", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));

        sAutoBalancer->Enable5MHeroic = sConfigMgr->GetOption<bool>("AutoBalance.Enable.5MHeroic", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable10MHeroic = sConfigMgr->GetOption<bool>("AutoBalance.Enable.5MHeroic", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->Enable25MHeroic = sConfigMgr->GetOption<bool>("AutoBalance.Enable.5MHeroic", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));
        sAutoBalancer->EnableOtherHeroic = sConfigMgr->GetOption<bool>("AutoBalance.Enable.5MHeroic", sConfigMgr->GetOption<bool>("AutoBalance.enable", 1, false));

        // Deprecated setting warning
        if (sConfigMgr->GetOption<int>("AutoBalance.DungeonsOnly", -1, false) != -1)
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.DungeonsOnly` defined in `AutoBalance.conf`. This variable has been removed and has no effect. Please see `AutoBalance.conf.dist` for more details.");

        if (sConfigMgr->GetOption<int>("AutoBalance.levelUseDbValuesWhenExists", -1, false) != -1)
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.levelUseDbValuesWhenExists` defined in `AutoBalance.conf`. This variable has been removed and has no effect. Please see `AutoBalance.conf.dist` for more details.");

        // Misc Settings
        // TODO: Organize and standardize variable names

        sAutoBalancer->PlayerChangeNotify = sConfigMgr->GetOption<bool>("AutoBalance.PlayerChangeNotify", 1);

        sAutoBalancer->rewardEnabled = sConfigMgr->GetOption<bool>("AutoBalance.reward.enable", 1);
        sAutoBalancer->PlayerCountDifficultyOffset = sConfigMgr->GetOption<uint32>("AutoBalance.playerCountDifficultyOffset", 0);
        sAutoBalancer->rewardRaid = sConfigMgr->GetOption<uint32>("AutoBalance.reward.raidToken", 49426);
        sAutoBalancer->rewardDungeon = sConfigMgr->GetOption<uint32>("AutoBalance.reward.dungeonToken", 47241);
        sAutoBalancer->MinPlayerReward = sConfigMgr->GetOption<float>("AutoBalance.reward.MinPlayerReward", 1);

        // InflectionPoint*
        // warn the console if deprecated values are detected
        if (sConfigMgr->GetOption<float>("AutoBalance.BossInflectionMult", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.BossInflectionMult` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");

        sAutoBalancer->InflectionPoint =                           sConfigMgr->GetOption<float>("AutoBalance.InflectionPoint", 0.5f, false);
        sAutoBalancer->InflectionPointCurveFloor =                 sConfigMgr->GetOption<float>("AutoBalance.InflectionPoint.CurveFloor", 0.0f, false);
        sAutoBalancer->InflectionPointCurveCeiling =               sConfigMgr->GetOption<float>("AutoBalance.InflectionPoint.CurveCeiling", 1.0f, false);
        sAutoBalancer->InflectionPointBoss =                       sConfigMgr->GetOption<float>("AutoBalance.InflectionPoint.BossModifier", sConfigMgr->GetOption<float>("AutoBalance.BossInflectionMult", 1.0f, false), false); // `AutoBalance.BossInflectionMult` for backwards compatibility

        sAutoBalancer->InflectionPointHeroic =                     sConfigMgr->GetOption<float>("AutoBalance.InflectionPointHeroic", 0.5f, false);
        sAutoBalancer->InflectionPointHeroicCurveFloor =           sConfigMgr->GetOption<float>("AutoBalance.InflectionPointHeroic.CurveFloor", 0.0f, false);
        sAutoBalancer->InflectionPointHeroicCurveCeiling =         sConfigMgr->GetOption<float>("AutoBalance.InflectionPointHeroic.CurveCeiling", 1.0f, false);
        sAutoBalancer->InflectionPointHeroicBoss =                 sConfigMgr->GetOption<float>("AutoBalance.InflectionPointHeroic.BossModifier", sConfigMgr->GetOption<float>("AutoBalance.BossInflectionMult", 1.0f, false), false); // `AutoBalance.BossInflectionMult` for backwards compatibility

        sAutoBalancer->InflectionPointRaid =                       sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid", 0.5f, false);
        sAutoBalancer->InflectionPointRaidCurveFloor =             sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid.CurveFloor", 0.0f, false);
        sAutoBalancer->InflectionPointRaidCurveCeiling =           sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid.CurveCeiling", 1.0f, false);
        sAutoBalancer->InflectionPointRaidBoss =                   sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid.BossModifier", sConfigMgr->GetOption<float>("AutoBalance.BossInflectionMult", 1.0f, false), false); // `AutoBalance.BossInflectionMult` for backwards compatibility

        sAutoBalancer->InflectionPointRaidHeroic =                 sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaidHeroic", 0.5f, false);
        sAutoBalancer->InflectionPointRaidHeroicCurveFloor =       sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaidHeroic.CurveFloor", 0.0f, false);
        sAutoBalancer->InflectionPointRaidHeroicCurveCeiling =     sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaidHeroic.CurveCeiling", 1.0f, false);
        sAutoBalancer->InflectionPointRaidHeroicBoss =             sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaidHeroic.BossModifier", sConfigMgr->GetOption<float>("AutoBalance.BossInflectionMult", 1.0f, false), false); // `AutoBalance.BossInflectionMult` for backwards compatibility

        sAutoBalancer->InflectionPointRaid10M =                    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10M", sAutoBalancer->InflectionPointRaid, false);
        sAutoBalancer->InflectionPointRaid10MCurveFloor =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10M.CurveFloor", sAutoBalancer->InflectionPointRaidCurveFloor, false);
        sAutoBalancer->InflectionPointRaid10MCurveCeiling =        sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10M.CurveCeiling", sAutoBalancer->InflectionPointRaidCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid10MBoss =                sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10M.BossModifier", sAutoBalancer->InflectionPointRaidBoss, false);

        sAutoBalancer->InflectionPointRaid10MHeroic =              sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10MHeroic", sAutoBalancer->InflectionPointRaidHeroic, false);
        sAutoBalancer->InflectionPointRaid10MHeroicCurveFloor =    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10MHeroic.CurveFloor", sAutoBalancer->InflectionPointRaidHeroicCurveFloor, false);
        sAutoBalancer->InflectionPointRaid10MHeroicCurveCeiling =  sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10MHeroic.CurveCeiling", sAutoBalancer->InflectionPointRaidHeroicCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid10MHeroicBoss =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid10MHeroic.BossModifier", sAutoBalancer->InflectionPointRaidHeroicBoss, false);

        sAutoBalancer->InflectionPointRaid15M =                    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid15M", sAutoBalancer->InflectionPointRaid, false);
        sAutoBalancer->InflectionPointRaid15MCurveFloor =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid15M.CurveFloor", sAutoBalancer->InflectionPointRaidCurveFloor, false);
        sAutoBalancer->InflectionPointRaid15MCurveCeiling =        sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid15M.CurveCeiling", sAutoBalancer->InflectionPointRaidCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid15MBoss =                sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid15M.BossModifier", sAutoBalancer->InflectionPointRaidBoss, false);

        sAutoBalancer->InflectionPointRaid20M =                    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid20M", sAutoBalancer->InflectionPointRaid, false);
        sAutoBalancer->InflectionPointRaid20MCurveFloor =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid20M.CurveFloor", sAutoBalancer->InflectionPointRaidCurveFloor, false);
        sAutoBalancer->InflectionPointRaid20MCurveCeiling =        sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid20M.CurveCeiling", sAutoBalancer->InflectionPointRaidCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid20MBoss =                sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid20M.BossModifier", sAutoBalancer->InflectionPointRaidBoss, false);

        sAutoBalancer->InflectionPointRaid25M =                    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25M", sAutoBalancer->InflectionPointRaid, false);
        sAutoBalancer->InflectionPointRaid25MCurveFloor =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25M.CurveFloor", sAutoBalancer->InflectionPointRaidCurveFloor, false);
        sAutoBalancer->InflectionPointRaid25MCurveCeiling =        sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25M.CurveCeiling", sAutoBalancer->InflectionPointRaidCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid25MBoss =                sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25M.BossModifier", sAutoBalancer->InflectionPointRaidBoss, false);

        sAutoBalancer->InflectionPointRaid25MHeroic =              sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25MHeroic", sAutoBalancer->InflectionPointRaidHeroic, false);
        sAutoBalancer->InflectionPointRaid25MHeroicCurveFloor =    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25MHeroic.CurveFloor", sAutoBalancer->InflectionPointRaidHeroicCurveFloor, false);
        sAutoBalancer->InflectionPointRaid25MHeroicCurveCeiling =  sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25MHeroic.CurveCeiling", sAutoBalancer->InflectionPointRaidHeroicCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid25MHeroicBoss =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid25MHeroic.BossModifier", sAutoBalancer->InflectionPointRaidHeroicBoss, false);

        sAutoBalancer->InflectionPointRaid40M =                    sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid40M", sAutoBalancer->InflectionPointRaid, false);
        sAutoBalancer->InflectionPointRaid40MCurveFloor =          sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid40M.CurveFloor", sAutoBalancer->InflectionPointRaidCurveFloor, false);
        sAutoBalancer->InflectionPointRaid40MCurveCeiling =        sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid40M.CurveCeiling", sAutoBalancer->InflectionPointRaidCurveCeiling, false);
        sAutoBalancer->InflectionPointRaid40MBoss =                sConfigMgr->GetOption<float>("AutoBalance.InflectionPointRaid40M.BossModifier", sAutoBalancer->InflectionPointRaidBoss, false);

        // StatModifier*
        // warn the console if deprecated values are detected
        if (sConfigMgr->GetOption<float>("AutoBalance.rate.global", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.rate.global` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        if (sConfigMgr->GetOption<float>("AutoBalance.rate.health", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.rate.health` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        if (sConfigMgr->GetOption<float>("AutoBalance.rate.mana", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.rate.mana` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        if (sConfigMgr->GetOption<float>("AutoBalance.rate.armor", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.rate.armor` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        if (sConfigMgr->GetOption<float>("AutoBalance.rate.damage", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.rate.damage` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");

        // 5-player dungeons
        sAutoBalancer->StatModifier_Global =                       sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifier_Health =                       sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifier_Mana =                         sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifier_Armor =                        sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifier_Damage =                       sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifier_SpellDamage =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifier.SpellDamage", 1.0f, false);
        sAutoBalancer->StatModifier_DoTDamage =                    sConfigMgr->GetOption<float>("AutoBalance.StatModifier.DoTDamage", 1.0f, false);
        sAutoBalancer->StatModifier_CCDuration =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifier.CCDuration", -1.0f, false);

        sAutoBalancer->StatModifier_Boss_Global =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifier_Boss_Health =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifier_Boss_Mana =                    sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifier_Boss_Armor =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifier_Boss_Damage =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifier_Boss_CCDuration =              sConfigMgr->GetOption<float>("AutoBalance.StatModifier.Boss.CCDuration", -1.0f, false);

        // 5-player heroic dungeons
        sAutoBalancer->StatModifierHeroic_Global =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Health =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Mana =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Armor =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Damage =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_CCDuration =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.CCDuration", -1.0f, false);

        sAutoBalancer->StatModifierHeroic_Boss_Global =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Boss_Health =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Boss_Mana =              sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Boss_Armor =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Boss_Damage =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierHeroic_Boss_CCDuration =        sConfigMgr->GetOption<float>("AutoBalance.StatModifierHeroic.Boss.CCDuration", -1.0f, false);

        // Default for all raids
        sAutoBalancer->StatModifierRaid_Global =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Health =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Mana =                     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Armor =                    sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Damage =                   sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierRaid_CCDuration =               sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.CCDuration", -1.0f, false);

        sAutoBalancer->StatModifierRaid_Boss_Global =              sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Boss_Health =              sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Boss_Mana =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Boss_Armor =               sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Boss_Damage =              sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierRaid_Boss_CCDuration =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid.Boss.CCDuration", -1.0f, false);

        // Default for all heroic raids
        sAutoBalancer->StatModifierRaidHeroic_Global =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Health =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Mana =               sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Armor =              sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Damage =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_CCDuration =         sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.CCDuration", -1.0f, false);

        sAutoBalancer->StatModifierRaidHeroic_Boss_Global =        sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.Global", sConfigMgr->GetOption<float>("AutoBalance.rate.global", 1.0f, false), false); // `AutoBalance.rate.global` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Boss_Health =        sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.Health", sConfigMgr->GetOption<float>("AutoBalance.rate.health", 1.0f, false), false); // `AutoBalance.rate.health` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Boss_Mana =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.Mana", sConfigMgr->GetOption<float>("AutoBalance.rate.mana", 1.0f, false), false); // `AutoBalance.rate.mana` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Boss_Armor =         sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.Armor", sConfigMgr->GetOption<float>("AutoBalance.rate.armor", 1.0f, false), false); // `AutoBalance.rate.armor` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Boss_Damage =        sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.Damage", sConfigMgr->GetOption<float>("AutoBalance.rate.damage", 1.0f, false), false); // `AutoBalance.rate.damage` for backwards compatibility
        sAutoBalancer->StatModifierRaidHeroic_Boss_CCDuration =    sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaidHeroic.Boss.CCDuration", -1.0f, false);

        // 10-player raids
        sAutoBalancer->StatModifierRaid10M_Global =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Global", sAutoBalancer->StatModifierRaid_Global, false);
        sAutoBalancer->StatModifierRaid10M_Health =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Health", sAutoBalancer->StatModifierRaid_Health, false);
        sAutoBalancer->StatModifierRaid10M_Mana =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Mana", sAutoBalancer->StatModifierRaid_Mana, false);
        sAutoBalancer->StatModifierRaid10M_Armor =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Armor", sAutoBalancer->StatModifierRaid_Armor, false);
        sAutoBalancer->StatModifierRaid10M_Damage =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Damage", sAutoBalancer->StatModifierRaid_Damage, false);
        sAutoBalancer->StatModifierRaid10M_CCDuration =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.CCDuration", sAutoBalancer->StatModifierRaid_CCDuration, false);

        sAutoBalancer->StatModifierRaid10M_Boss_Global =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.Global", sAutoBalancer->StatModifierRaid_Boss_Global, false);
        sAutoBalancer->StatModifierRaid10M_Boss_Health =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.Health", sAutoBalancer->StatModifierRaid_Boss_Health, false);
        sAutoBalancer->StatModifierRaid10M_Boss_Mana =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.Mana", sAutoBalancer->StatModifierRaid_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid10M_Boss_Armor =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.Armor", sAutoBalancer->StatModifierRaid_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid10M_Boss_Damage =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.Damage", sAutoBalancer->StatModifierRaid_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid10M_Boss_CCDuration =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10M.Boss.CCDuration", sAutoBalancer->StatModifierRaid_Boss_CCDuration, false);

        // 10-player heroic raids
        sAutoBalancer->StatModifierRaid10MHeroic_Global =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Global", sAutoBalancer->StatModifierRaidHeroic_Global, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Health =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Health", sAutoBalancer->StatModifierRaidHeroic_Health, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Mana =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Mana", sAutoBalancer->StatModifierRaidHeroic_Mana, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Armor =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Armor", sAutoBalancer->StatModifierRaidHeroic_Armor, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Damage =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Damage", sAutoBalancer->StatModifierRaidHeroic_Damage, false);
        sAutoBalancer->StatModifierRaid10MHeroic_CCDuration =      sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.CCDuration", sAutoBalancer->StatModifierRaidHeroic_CCDuration, false);

        sAutoBalancer->StatModifierRaid10MHeroic_Boss_Global =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.Global", sAutoBalancer->StatModifierRaidHeroic_Boss_Global, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Boss_Health =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.Health", sAutoBalancer->StatModifierRaidHeroic_Boss_Health, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Boss_Mana =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.Mana", sAutoBalancer->StatModifierRaidHeroic_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Boss_Armor =      sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.Armor", sAutoBalancer->StatModifierRaidHeroic_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Boss_Damage =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.Damage", sAutoBalancer->StatModifierRaidHeroic_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid10MHeroic_Boss_CCDuration = sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid10MHeroic.Boss.CCDuration", sAutoBalancer->StatModifierRaidHeroic_Boss_CCDuration, false);

        // 15-player raids
        sAutoBalancer->StatModifierRaid15M_Global =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Global", sAutoBalancer->StatModifierRaid_Global, false);
        sAutoBalancer->StatModifierRaid15M_Health =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Health", sAutoBalancer->StatModifierRaid_Health, false);
        sAutoBalancer->StatModifierRaid15M_Mana =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Mana", sAutoBalancer->StatModifierRaid_Mana, false);
        sAutoBalancer->StatModifierRaid15M_Armor =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Armor", sAutoBalancer->StatModifierRaid_Armor, false);
        sAutoBalancer->StatModifierRaid15M_Damage =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Damage", sAutoBalancer->StatModifierRaid_Damage, false);
        sAutoBalancer->StatModifierRaid15M_CCDuration =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.CCDuration", sAutoBalancer->StatModifierRaid_CCDuration, false);

        sAutoBalancer->StatModifierRaid15M_Boss_Global =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.Global", sAutoBalancer->StatModifierRaid_Boss_Global, false);
        sAutoBalancer->StatModifierRaid15M_Boss_Health =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.Health", sAutoBalancer->StatModifierRaid_Boss_Health, false);
        sAutoBalancer->StatModifierRaid15M_Boss_Mana =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.Mana", sAutoBalancer->StatModifierRaid_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid15M_Boss_Armor =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.Armor", sAutoBalancer->StatModifierRaid_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid15M_Boss_Damage =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.Damage", sAutoBalancer->StatModifierRaid_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid15M_Boss_CCDuration =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid15M.Boss.CCDuration", sAutoBalancer->StatModifierRaid_Boss_CCDuration, false);

        // 20-player raids
        sAutoBalancer->StatModifierRaid20M_Global =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Global", sAutoBalancer->StatModifierRaid_Global, false);
        sAutoBalancer->StatModifierRaid20M_Health =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Health", sAutoBalancer->StatModifierRaid_Health, false);
        sAutoBalancer->StatModifierRaid20M_Mana =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Mana", sAutoBalancer->StatModifierRaid_Mana, false);
        sAutoBalancer->StatModifierRaid20M_Armor =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Armor", sAutoBalancer->StatModifierRaid_Armor, false);
        sAutoBalancer->StatModifierRaid20M_Damage =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Damage", sAutoBalancer->StatModifierRaid_Damage, false);
        sAutoBalancer->StatModifierRaid20M_CCDuration =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.CCDuration", sAutoBalancer->StatModifierRaid_CCDuration, false);

        sAutoBalancer->StatModifierRaid20M_Boss_Global =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.Global", sAutoBalancer->StatModifierRaid_Boss_Global, false);
        sAutoBalancer->StatModifierRaid20M_Boss_Health =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.Health", sAutoBalancer->StatModifierRaid_Boss_Health, false);
        sAutoBalancer->StatModifierRaid20M_Boss_Mana =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.Mana", sAutoBalancer->StatModifierRaid_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid20M_Boss_Armor =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.Armor", sAutoBalancer->StatModifierRaid_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid20M_Boss_Damage =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.Damage", sAutoBalancer->StatModifierRaid_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid20M_Boss_CCDuration =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid20M.Boss.CCDuration", sAutoBalancer->StatModifierRaid_Boss_CCDuration, false);

        // 25-player raids
        sAutoBalancer->StatModifierRaid25M_Global =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Global", sAutoBalancer->StatModifierRaid_Global, false);
        sAutoBalancer->StatModifierRaid25M_Health =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Health", sAutoBalancer->StatModifierRaid_Health, false);
        sAutoBalancer->StatModifierRaid25M_Mana =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Mana", sAutoBalancer->StatModifierRaid_Mana, false);
        sAutoBalancer->StatModifierRaid25M_Armor =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Armor", sAutoBalancer->StatModifierRaid_Armor, false);
        sAutoBalancer->StatModifierRaid25M_Damage =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Damage", sAutoBalancer->StatModifierRaid_Damage, false);
        sAutoBalancer->StatModifierRaid25M_CCDuration =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.CCDuration", sAutoBalancer->StatModifierRaid_CCDuration, false);

        sAutoBalancer->StatModifierRaid25M_Boss_Global =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.Global", sAutoBalancer->StatModifierRaid_Boss_Global, false);
        sAutoBalancer->StatModifierRaid25M_Boss_Health =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.Health", sAutoBalancer->StatModifierRaid_Boss_Health, false);
        sAutoBalancer->StatModifierRaid25M_Boss_Mana =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.Mana", sAutoBalancer->StatModifierRaid_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid25M_Boss_Armor =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.Armor", sAutoBalancer->StatModifierRaid_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid25M_Boss_Damage =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.Damage", sAutoBalancer->StatModifierRaid_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid25M_Boss_CCDuration =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25M.Boss.CCDuration", sAutoBalancer->StatModifierRaid_Boss_CCDuration, false);

        // 25-player heroic raids
        sAutoBalancer->StatModifierRaid25MHeroic_Global =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Global", sAutoBalancer->StatModifierRaidHeroic_Global, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Health =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Health", sAutoBalancer->StatModifierRaidHeroic_Health, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Mana =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Mana", sAutoBalancer->StatModifierRaidHeroic_Mana, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Armor =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Armor", sAutoBalancer->StatModifierRaidHeroic_Armor, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Damage =          sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Damage", sAutoBalancer->StatModifierRaidHeroic_Damage, false);
        sAutoBalancer->StatModifierRaid25MHeroic_CCDuration =      sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.CCDuration", sAutoBalancer->StatModifierRaidHeroic_CCDuration, false);

        sAutoBalancer->StatModifierRaid25MHeroic_Boss_Global =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.Global", sAutoBalancer->StatModifierRaidHeroic_Boss_Global, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Boss_Health =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.Health", sAutoBalancer->StatModifierRaidHeroic_Boss_Health, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Boss_Mana =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.Mana", sAutoBalancer->StatModifierRaidHeroic_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Boss_Armor =      sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.Armor", sAutoBalancer->StatModifierRaidHeroic_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Boss_Damage =     sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.Damage", sAutoBalancer->StatModifierRaidHeroic_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid25MHeroic_Boss_CCDuration = sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid25MHeroic.Boss.CCDuration", sAutoBalancer->StatModifierRaidHeroic_Boss_CCDuration, false);

        // 40-player raids
        sAutoBalancer->StatModifierRaid40M_Global =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Global", sAutoBalancer->StatModifierRaid_Global, false);
        sAutoBalancer->StatModifierRaid40M_Health =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Health", sAutoBalancer->StatModifierRaid_Health, false);
        sAutoBalancer->StatModifierRaid40M_Mana =                  sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Mana", sAutoBalancer->StatModifierRaid_Mana, false);
        sAutoBalancer->StatModifierRaid40M_Armor =                 sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Armor", sAutoBalancer->StatModifierRaid_Armor, false);
        sAutoBalancer->StatModifierRaid40M_Damage =                sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Damage", sAutoBalancer->StatModifierRaid_Damage, false);
        sAutoBalancer->StatModifierRaid40M_CCDuration =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.CCDuration", sAutoBalancer->StatModifierRaid_CCDuration, false);

        sAutoBalancer->StatModifierRaid40M_Boss_Global =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.Global", sAutoBalancer->StatModifierRaid_Boss_Global, false);
        sAutoBalancer->StatModifierRaid40M_Boss_Health =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.Health", sAutoBalancer->StatModifierRaid_Boss_Health, false);
        sAutoBalancer->StatModifierRaid40M_Boss_Mana =             sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.Mana", sAutoBalancer->StatModifierRaid_Boss_Mana, false);
        sAutoBalancer->StatModifierRaid40M_Boss_Armor =            sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.Armor", sAutoBalancer->StatModifierRaid_Boss_Armor, false);
        sAutoBalancer->StatModifierRaid40M_Boss_Damage =           sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.Damage", sAutoBalancer->StatModifierRaid_Boss_Damage, false);
        sAutoBalancer->StatModifierRaid40M_Boss_CCDuration =       sConfigMgr->GetOption<float>("AutoBalance.StatModifierRaid40M.Boss.CCDuration", sAutoBalancer->StatModifierRaid_Boss_CCDuration, false);

        // Modifier Min/Max
        sAutoBalancer->MinHPModifier = sConfigMgr->GetOption<float>("AutoBalance.MinHPModifier", 0.1f);
        sAutoBalancer->MinManaModifier = sConfigMgr->GetOption<float>("AutoBalance.MinManaModifier", 0.01f);
        sAutoBalancer->MinDamageModifier = sConfigMgr->GetOption<float>("AutoBalance.MinDamageModifier", 0.01f);
        sAutoBalancer->MinCCDurationModifier = sConfigMgr->GetOption<float>("AutoBalance.MinCCDurationModifier", 0.25f);
        sAutoBalancer->MaxCCDurationModifier = sConfigMgr->GetOption<float>("AutoBalance.MaxCCDurationModifier", 1.0f);

        // Advanced Difficulty Scaling
        sAutoBalancer->MythicMultiplier = sConfigMgr->GetOption<float>("AutoBalance.MythicMultiplier", 1.25f);
        sAutoBalancer->LegendaryMultiplier = sConfigMgr->GetOption<float>("AutoBalance.LegendaryMultiplier", 1.5f);
        sAutoBalancer->AscendantMultiplier = sConfigMgr->GetOption<float>("AutoBalance.AscendantMultiplier", 2.0f);

        // LevelScaling.*
        sAutoBalancer->LevelScaling = sConfigMgr->GetOption<bool>("AutoBalance.LevelScaling", true);

        std::string LevelScalingMethodString = sConfigMgr->GetOption<std::string>("AutoBalance.LevelScaling.Method", "dynamic", false);
        if (LevelScalingMethodString == "fixed")
        {
            sAutoBalancer->LevelScalingMethod = AUTOBALANCE_SCALING_FIXED;
        }
        else if (LevelScalingMethodString == "dynamic")
        {
            sAutoBalancer->LevelScalingMethod = AUTOBALANCE_SCALING_DYNAMIC;
        }
        else
        {
            LOG_ERROR("server.loading", "mod-autobalance: invalid value `{}` for `AutoBalance.LevelScaling.Method` defined in `AutoBalance.conf`. Defaulting to a value of `dynamic`.", LevelScalingMethodString);
            sAutoBalancer->LevelScalingMethod = AUTOBALANCE_SCALING_DYNAMIC;
        }

        if (sConfigMgr->GetOption<float>("AutoBalance.LevelHigherOffset", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.LevelHigherOffset` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        sAutoBalancer->LevelScalingSkipHigherLevels = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.SkipHigherLevels", sConfigMgr->GetOption<uint32>("AutoBalance.LevelHigherOffset", 3, false), true);
        if (sConfigMgr->GetOption<float>("AutoBalance.LevelLowerOffset", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.LevelLowerOffset` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        sAutoBalancer->LevelScalingSkipLowerLevels = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.SkipLowerLevels", sConfigMgr->GetOption<uint32>("AutoBalance.LevelLowerOffset", 5, false), true);

        sAutoBalancer->LevelScalingDynamicLevelCeilingDungeons = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Ceiling.Dungeons", 1);
        sAutoBalancer->LevelScalingDynamicLevelFloorDungeons = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Floor.Dungeons", 5);
        sAutoBalancer->LevelScalingDynamicLevelCeilingHeroicDungeons = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Ceiling.HeroicDungeons", 2);
        sAutoBalancer->LevelScalingDynamicLevelFloorHeroicDungeons = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Floor.HeroicDungeons", 5);
        sAutoBalancer->LevelScalingDynamicLevelCeilingRaids = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Ceiling.Raids", 3);
        sAutoBalancer->LevelScalingDynamicLevelFloorRaids = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Floor.Raids", 5);
        sAutoBalancer->LevelScalingDynamicLevelCeilingHeroicRaids = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Ceiling.HeroicRaids", 3);
        sAutoBalancer->LevelScalingDynamicLevelFloorHeroicRaids = sConfigMgr->GetOption<uint8>("AutoBalance.LevelScaling.DynamicLevel.Floor.HeroicRaids", 5);



        if (sConfigMgr->GetOption<float>("AutoBalance.LevelEndGameBoost", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.LevelEndGameBoost` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        sAutoBalancer->LevelScalingEndGameBoost = sConfigMgr->GetOption<bool>("AutoBalance.LevelScaling.EndGameBoost", sConfigMgr->GetOption<bool>("AutoBalance.LevelEndGameBoost", 1, false), true);

        // RewardScaling.*
        // warn the console if deprecated values are detected
        if (sConfigMgr->GetOption<float>("AutoBalance.DungeonScaleDownXP", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.DungeonScaleDownXP` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");
        if (sConfigMgr->GetOption<float>("AutoBalance.DungeonScaleDownMoney", false, false))
            LOG_WARN("server.loading", "mod-autobalance: deprecated value `AutoBalance.DungeonScaleDownMoney` defined in `AutoBalance.conf`. This variable will be removed in a future release. Please see `AutoBalance.conf.dist` for more details.");

        std::string RewardScalingMethodString = sConfigMgr->GetOption<std::string>("AutoBalance.RewardScaling.Method", "dynamic", false);
        if (RewardScalingMethodString == "fixed")
        {
            sAutoBalancer->RewardScalingMethod = AUTOBALANCE_SCALING_FIXED;
        }
        else if (RewardScalingMethodString == "dynamic")
        {
            sAutoBalancer->RewardScalingMethod = AUTOBALANCE_SCALING_DYNAMIC;
        }
        else
        {
            LOG_ERROR("server.loading", "mod-autobalance: invalid value `{}` for `AutoBalance.RewardScaling.Method` defined in `AutoBalance.conf`. Defaulting to a value of `dynamic`.", RewardScalingMethodString);
            sAutoBalancer->RewardScalingMethod = AUTOBALANCE_SCALING_DYNAMIC;
        }

        sAutoBalancer->RewardScalingXP = sConfigMgr->GetOption<bool>("AutoBalance.RewardScaling.XP", sConfigMgr->GetOption<bool>("AutoBalance.DungeonScaleDownXP", true, false));
        sAutoBalancer->RewardScalingXPModifier = sConfigMgr->GetOption<float>("AutoBalance.RewardScaling.XP.Modifier", 1.0f, false);

        sAutoBalancer->RewardScalingMoney = sConfigMgr->GetOption<bool>("AutoBalance.RewardScaling.Money", sConfigMgr->GetOption<bool>("AutoBalance.DungeonScaleDownMoney", true, false));
        sAutoBalancer->RewardScalingMoneyModifier = sConfigMgr->GetOption<float>("AutoBalance.RewardScaling.Money.Modifier", 1.0f, false);

        // Announcement
        sAutoBalancer->Announcement = sConfigMgr->GetOption<bool>("AutoBalanceAnnounce.enable", true);

    }
};

void AddWorldScripts()
{
    new AutoBalance_WorldScript();
}