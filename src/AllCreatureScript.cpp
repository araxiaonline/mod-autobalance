#include "ScriptMgr.h"


class AutoBalance_AllCreatureScript : public AllCreatureScript
{
public:
    AutoBalance_AllCreatureScript()
        : AllCreatureScript("AutoBalance_AllCreatureScript")
    {
    }

    void Creature_SelectLevel(const CreatureTemplate* /*creatureTemplate*/, Creature* creature) override
    {
        if (creature->GetMap()->IsDungeon())
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::Creature_SelectLevel(): {} ({})", creature->GetName(), creature->GetLevel());

        // add the creature to the map's tracking list
        sAutoBalancer->AddCreatureToMapData(creature, true, nullptr, false);

        // do an initial modification of the creature
        ModifyCreatureAttributes(creature);

    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        if (creature->GetMap()->IsDungeon())
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::OnCreatureAddWorld(): {} ({})", creature->GetName(), creature->GetLevel());
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature->GetMap()->IsDungeon())
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::OnCreatureRemoveWorld(): {} ({})", creature->GetName(), creature->GetLevel());

        // remove the creature from the map's tracking list, if present
        sAutoBalancer->RemoveCreatureFromMapData(creature);
    }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        // If the config is out of date and the creature was reset, run modify against it
        if (ResetCreatureIfNeeded(creature))
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::OnAllCreatureUpdate(): Creature {} ({}) is reset to its original stats.", creature->GetName(), creature->GetLevel());

            // Update the map's level if it is out of date
            sAutoBalancer->UpdateMapLevelIfNeeded(creature->GetMap());

            ModifyCreatureAttributes(creature);
        }
    }

    // Reset the passed creature to stock if the config has changed
    bool ResetCreatureIfNeeded(Creature* creature)
    {
        // make sure we have a creature and that it's assigned to a map
        if (!creature || !creature->GetMap())
            return false;


        // if this isn't a dungeon or a battleground, make no changes
        if (!(creature->GetMap()->IsDungeon() || creature->GetMap()->IsRaid()))
            return false;

        // if this is a pet or summon controlled by the player, make no changes
        if ((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer())
            return false;

        // if this is a non-relevant creature, skip
        if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
            return false;

        // get (or create) the creature and map's info
        AutoBalanceCreatureInfo *creatureABInfo=creature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");
        AutoBalanceMapInfo *mapABInfo=creature->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        // if this creature is below 85% of the minimum level for the map, make no changes
        if (creatureABInfo->UnmodifiedLevel < (float)mapABInfo->lfgMinLevel * .85f)
        {
            if (creatureABInfo->configTime == 0)
                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ResetCreatureIfNeeded(): {} ({}) is below 85% of the LFG min level for the map, do not reset or modify.", creature->GetName(), creatureABInfo->UnmodifiedLevel);

            creatureABInfo->configTime = sAutoBalancer->lastConfigTime;
            return false;
        }

        // if this creature is above 115% of the maximum level for the map, make no changes
        if (creatureABInfo->UnmodifiedLevel > (float)mapABInfo->lfgMaxLevel * 1.15f)
        {
            if (creatureABInfo->configTime == 0)
                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ResetCreatureIfNeeded(): {} ({}) is above 115% of the LFG max level for the map, do not reset or modify.", creature->GetName(), creatureABInfo->UnmodifiedLevel);

            creatureABInfo->configTime = sAutoBalancer->lastConfigTime;
            return false;
        }

        // if creature is dead and configTime is 0, skip
        if (creature->isDead() && creatureABInfo->configTime == 0)
        {
            return false;
        }
        // if the creature is dead but configTime is NOT 0, we set it to 0 so that it will be recalculated if revived
        // also remember that this creature was once alive but is now dead
        else if (creature->isDead())
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ResetCreatureIfNeeded(): {} ({}) is dead and configTime is not 0 - prime for reset if revived.", creature->GetName(), creature->GetLevel());
            creatureABInfo->configTime = 0;
            creatureABInfo->wasAliveNowDead = true;
            return false;
        }

        // if the config is outdated, reset the creature
        if (creatureABInfo->configTime != sAutoBalancer->lastConfigTime)
        {
            // before updating the creature, we should update the map level if needed
            sAutoBalancer->UpdateMapLevelIfNeeded(creature->GetMap());

            // retain some values
            uint8 unmodifiedLevel = creatureABInfo->UnmodifiedLevel;
            bool isActive = creatureABInfo->isActive;
            bool wasAliveNowDead = creatureABInfo->wasAliveNowDead;
            bool isInCreatureList = creatureABInfo->isInCreatureList;

            // reset AutoBalance modifiers
            creature->CustomData.Erase("AutoBalanceCreatureInfo");
            AutoBalanceCreatureInfo *creatureABInfo=creature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");

            // grab the creature's template and the original creature's stats
            CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();

            // set the creature's level
            creature->SetLevel(unmodifiedLevel);
            creatureABInfo->UnmodifiedLevel = unmodifiedLevel;

            // get the creature's base stats
            CreatureBaseStats const* origCreatureStats = sObjectMgr->GetCreatureBaseStats(unmodifiedLevel, creatureTemplate->unit_class);

            // health
            float currentHealthPercent = (float)creature->GetHealth() / (float)creature->GetMaxHealth();
            creature->SetMaxHealth(origCreatureStats->GenerateHealth(creatureTemplate));
            creature->SetHealth((float)origCreatureStats->GenerateHealth(creatureTemplate) * currentHealthPercent);

            // mana
            if (creature->getPowerType() == POWER_MANA && creature->GetPower(POWER_MANA) >= 0 && creature->GetMaxPower(POWER_MANA) > 0)
            {
                float currentManaPercent = creature->GetPower(POWER_MANA) / creature->GetMaxPower(POWER_MANA);
                creature->SetMaxPower(POWER_MANA, origCreatureStats->GenerateMana(creatureTemplate));
                creature->SetPower(POWER_MANA, creature->GetMaxPower(POWER_MANA) * currentManaPercent);
            }

            // armor
            creature->SetArmor(origCreatureStats->GenerateArmor(creatureTemplate));

            // restore the saved data
            creatureABInfo->isActive = isActive;
            creatureABInfo->wasAliveNowDead = wasAliveNowDead;
            creatureABInfo->isInCreatureList = isInCreatureList;

            // damage and ccduration are handled using AutoBalanceCreatureInfo data only

            // return true to indicate that the creature was reset
            return true;
        }

        // creature was not reset, return false
        return false;

    }

    void ModifyCreatureAttributes(Creature* creature)
    {
        // make sure we have a creature and that it's assigned to a map
        if (!creature || !creature->GetMap())
            return;

        // if this isn't a dungeon or a battleground, make no changes
        if (!(creature->GetMap()->IsDungeon() || creature->GetMap()->IsRaid()))
            return;

        // if this is a pet or summon controlled by the player, make no changes
        if (((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer()))
            return;

        // if this is a non-relevant creature, make no changes
        if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
            return;

        // grab creature and map data
        AutoBalanceCreatureInfo *creatureABInfo=creature->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo");
        AutoBalanceMapInfo *mapABInfo=creature->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        // mark the creature as updated using the current settings if needed
        if (creatureABInfo->configTime != sAutoBalancer->lastConfigTime)
            creatureABInfo->configTime = sAutoBalancer->lastConfigTime;

        // check to make sure that the creature's map is enabled for scaling
        if (!mapABInfo->enabled || !sAutoBalancer->EnableGlobal)
            return;

        // if this creature is below 85% of the minimum LFG level for the map, make no changes
        if (creatureABInfo->UnmodifiedLevel < (float)mapABInfo->lfgMinLevel * .85f)
            return;

        // if this creature is above 115% of the maximum LFG level for the map, make no changes
        if (creatureABInfo->UnmodifiedLevel > (float)mapABInfo->lfgMaxLevel * 1.15f)
            return;

        // if the creature was dead (but this function is being called because they are being revived), reset it and allow modifications
        if (creatureABInfo->wasAliveNowDead)
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes(): {} ({}) was dead but appears to be alive now, reset wasAliveNowDead flag.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
            // if the creature was dead, reset it
            creatureABInfo->wasAliveNowDead = false;
        }
        // if the creature is dead and wasn't marked as dead by this script, simply skip
        else if (creature->isDead())
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes(): {} ({}) is dead, do not modify.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
            return;
        }

        CreatureTemplate const *creatureTemplate = creature->GetCreatureTemplate();

        InstanceMap* instanceMap = ((InstanceMap*)sMapMgr->FindMap(creature->GetMapId(), creature->GetInstanceId()));
        uint32 mapId = instanceMap->GetEntry()->MapID;
        if (sAutoBalancer->perDungeonScalingEnabled() && !sAutoBalancer->isEnabledDungeon(mapId))
        {
            return;
        }
        uint32 maxNumberOfPlayers = instanceMap->GetMaxPlayers();
        int forcedNumPlayers = sAutoBalancer->GetForcedNumPlayers(creatureTemplate->Entry);

        if (forcedNumPlayers > 0)
            maxNumberOfPlayers = forcedNumPlayers; // Force maxNumberOfPlayers to be changed to match the Configuration entries ForcedID2, ForcedID5, ForcedID10, ForcedID20, ForcedID25, ForcedID40
        else if (forcedNumPlayers == 0)
            return; // forcedNumPlayers 0 means that the creature is contained in DisabledID -> no scaling

        uint32 curCount=mapABInfo->playerCount + sAutoBalancer->PlayerCountDifficultyOffset;
        if (sAutoBalancer->perDungeonScalingEnabled())
        {
            curCount = adjustCurCount(curCount, mapId);
        }
        creatureABInfo->instancePlayerCount = curCount;

        if (!creatureABInfo->instancePlayerCount) // no players in map, do not modify attributes
            return;

        if (!sABScriptMgr->OnBeforeModifyAttributes(creature, creatureABInfo->instancePlayerCount))
            return;

        // only scale levels if level scaling is enabled and the instance's average creature level is not within the skip range
        if (sAutoBalancer->LevelScaling &&
             ((mapABInfo->avgCreatureLevel > mapABInfo->highestPlayerLevel + mapABInfo->levelScalingSkipHigherLevels || mapABInfo->levelScalingSkipHigherLevels == 0) ||
              (mapABInfo->avgCreatureLevel < mapABInfo->highestPlayerLevel - mapABInfo->levelScalingSkipLowerLevels || mapABInfo->levelScalingSkipLowerLevels == 0))
           )
        {
            uint8 selectedLevel;

            // if we're using dynamic scaling, calculate the creature's level based relative to the highest player level in the map
            if (sAutoBalancer->LevelScalingMethod == AUTOBALANCE_SCALING_DYNAMIC)
            {
                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Creature {} ({}) dynamic scaling floor: {}, ceiling: {}.", creature->GetName(), creatureABInfo->UnmodifiedLevel, mapABInfo->levelScalingDynamicFloor, mapABInfo->levelScalingDynamicCeiling);

                // calculate the creature's new level
                selectedLevel = (mapABInfo->highestPlayerLevel + mapABInfo->levelScalingDynamicCeiling) - (mapABInfo->highestCreatureLevel - creatureABInfo->UnmodifiedLevel);

                // check to be sure that the creature's new level is at least the dynamic scaling floor
                if (selectedLevel < (mapABInfo->highestPlayerLevel - mapABInfo->levelScalingDynamicFloor))
                {
                    selectedLevel = mapABInfo->highestPlayerLevel - mapABInfo->levelScalingDynamicFloor;
                }

                // check to be sure that the creature's new level is no higher than the dynamic scaling ceiling
                if (selectedLevel > (mapABInfo->highestPlayerLevel + mapABInfo->levelScalingDynamicCeiling))
                {
                    selectedLevel = mapABInfo->highestPlayerLevel + mapABInfo->levelScalingDynamicCeiling;
                }

                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Creature {} ({}) scaled to {} via dynamic scaling.", creature->GetName(), creatureABInfo->UnmodifiedLevel, selectedLevel);
            }
            // otherwise we're using "fixed" scaling and should use the highest player level in the map
            else
            {
                selectedLevel = mapABInfo->highestPlayerLevel;
                LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Creature {} ({}) scaled to {} via fixed scaling.", creature->GetName(), creatureABInfo->UnmodifiedLevel, selectedLevel);
            }

            // Overwrite levels for harder difficulties
            if (mapABInfo->customDifficulty == GROUP_DIFFICULTY_MYTHIC  )
            {
                selectedLevel = 83;
            }
            if (mapABInfo->customDifficulty == GROUP_DIFFICULTY_LEGENDARY || mapABInfo->customDifficulty == GROUP_DIFFICULTY_ASCENDANT)
            {
                selectedLevel = 85;
            }


            creatureABInfo->selectedLevel = selectedLevel;
            creature->SetLevel(creatureABInfo->selectedLevel);
        }
        else
        {
            LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Creature {} ({}) not level scaled due to level scaling being disabled or the instance's average creature level being outside the skip range.", creature->GetName(), creatureABInfo->UnmodifiedLevel);
            creatureABInfo->selectedLevel = creatureABInfo->UnmodifiedLevel;
        }

        creatureABInfo->entry = creature->GetEntry();

        CreatureBaseStats const* origCreatureStats = sObjectMgr->GetCreatureBaseStats(creatureABInfo->UnmodifiedLevel, creatureTemplate->unit_class);
        CreatureBaseStats const* creatureStats = sObjectMgr->GetCreatureBaseStats(creatureABInfo->selectedLevel, creatureTemplate->unit_class);

        uint32 baseMana = origCreatureStats->GenerateMana(creatureTemplate);
        uint32 scaledHealth = 0;
        uint32 scaledMana = 0;

        // Note: InflectionPoint handle the number of players required to get 50% health.
        //       you'd adjust this to raise or lower the hp modifier for per additional player in a non-whole group.
        //
        //       diff modify the rate of percentage increase between
        //       number of players. Generally the closer to the value of 1 you have this
        //       the less gradual the rate will be. For example in a 5 man it would take 3
        //       total players to face a mob at full health.
        //
        //       The +1 and /2 values raise the TanH function to a positive range and make
        //       sure the modifier never goes above the value or 1.0 or below 0.
        //
        //       curveFloor and curveCeiling squishes the curve by adjusting the curve start and end points.
        //       This allows for better control over high and low player count scaling.

        float defaultMultiplier;
        float curveFloor;
        float curveCeiling;

        //
        // Inflection Point
        //
        float inflectionValue  = (float)maxNumberOfPlayers;

        if (instanceMap->IsHeroic())
        {
            switch (maxNumberOfPlayers)
            {
			    case 1:
			    case 2:
			    case 3:
			    case 4:
			    case 5:
                    inflectionValue *= sAutoBalancer->InflectionPointHeroic;
                    curveFloor = sAutoBalancer->InflectionPointHeroicCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointHeroicCurveCeiling;
                    break;
                case 10:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid10MHeroic;
                    curveFloor = sAutoBalancer->InflectionPointRaid10MHeroicCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid10MHeroicCurveCeiling;
                    break;
                case 25:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid25MHeroic;
                    curveFloor = sAutoBalancer->InflectionPointRaid25MHeroicCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid25MHeroicCurveCeiling;
                    break;
                default:
                    inflectionValue *= sAutoBalancer->InflectionPointRaidHeroic;
                    curveFloor = sAutoBalancer->InflectionPointRaidHeroicCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaidHeroicCurveCeiling;
            }
        }
        else
        {
            switch (maxNumberOfPlayers)
            {
			    case 1:
			    case 2:
			    case 3:
			    case 4:
			    case 5:
                    inflectionValue *= sAutoBalancer->InflectionPoint;
                    curveFloor = sAutoBalancer->InflectionPointCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointCurveCeiling;
                    break;
                case 10:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid10M;
                    curveFloor = sAutoBalancer->InflectionPointRaid10MCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid10MCurveCeiling;
                    break;
                case 15:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid15M;
                    curveFloor = sAutoBalancer->InflectionPointRaid15MCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid15MCurveCeiling;
                    break;
                case 20:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid20M;
                    curveFloor = sAutoBalancer->InflectionPointRaid20MCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid20MCurveCeiling;
                    break;
                case 25:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid25M;
                    curveFloor = sAutoBalancer->InflectionPointRaid25MCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid25MCurveCeiling;
                    break;
                case 40:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid40M;
                    curveFloor = sAutoBalancer->InflectionPointRaid40MCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaid40MCurveCeiling;
                    break;
                default:
                    inflectionValue *= sAutoBalancer->InflectionPointRaid;
                    curveFloor = sAutoBalancer->InflectionPointRaidCurveFloor;
                    curveCeiling = sAutoBalancer->InflectionPointRaidCurveCeiling;
            }
        }

        // Per map ID overrides alter the above settings, if set
        if (sAutoBalancer->hasDungeonOverride(mapId))
        {
            AutoBalanceInflectionPointSettings* myInflectionPointOverrides = &sAutoBalancer->dungeonOverrides[mapId];

            // Alter the inflectionValue according to the override, if set
            if (myInflectionPointOverrides->value != -1)
            {
                inflectionValue  = (float)maxNumberOfPlayers; // Starting over
                inflectionValue *= myInflectionPointOverrides->value;
            }

            if (myInflectionPointOverrides->curveFloor != -1)   { curveFloor =    myInflectionPointOverrides->curveFloor;   }
            if (myInflectionPointOverrides->curveCeiling != -1) { curveCeiling =  myInflectionPointOverrides->curveCeiling; }
        }

        //
        // Boss Inflection Point
        //
        if (creature->IsDungeonBoss()) {

            float bossInflectionPointMultiplier;

            // Determine the correct boss inflection multiplier
            if (instanceMap->IsHeroic())
            {
                switch (maxNumberOfPlayers)
                {
			        case 1:
			        case 2:
			        case 3:
			        case 4:
			        case 5:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointHeroicBoss;
                        break;
                    case 10:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid10MHeroicBoss;
                        break;
                    case 25:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid25MHeroicBoss;
                        break;
                    default:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaidHeroicBoss;
                }
            }
            else
            {
                switch (maxNumberOfPlayers)
                {
			        case 1:
			        case 2:
			        case 3:
			        case 4:
			        case 5:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointBoss;
                        break;
                    case 10:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid10MBoss;
                        break;
                    case 15:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid15MBoss;
                        break;
                    case 20:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid20MBoss;
                        break;
                    case 25:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid25MBoss;
                        break;
                    case 40:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaid40MBoss;
                        break;
                    default:
                        bossInflectionPointMultiplier = sAutoBalancer->InflectionPointRaidBoss;
                }
            }

            // Per map ID overrides alter the above settings, if set
            if (sAutoBalancer->hasBossOverride(mapId))
            {
                AutoBalanceInflectionPointSettings* myBossOverrides = &sAutoBalancer->bossOverrides[mapId];

                // If set, alter the inflectionValue according to the override
                if (myBossOverrides->value != -1)
                {
                    inflectionValue *= myBossOverrides->value;
                }
                // Otherwise, calculate using the value determined by instance type
                else
                {
                    inflectionValue *= bossInflectionPointMultiplier;
                }
            }
            // No override, use the value determined by the instance type
            else
            {
                inflectionValue *= bossInflectionPointMultiplier;
            }
        }

        //
        // Stat Modifiers
        //

        // Calculate stat modifiers
        float statMod_global, statMod_health, statMod_mana, statMod_armor, statMod_damage, statMod_ccDuration;
        float statMod_boss_global, statMod_boss_health, statMod_boss_mana, statMod_boss_armor, statMod_boss_damage, statMod_boss_ccDuration;

        // Apply the per-instance-type modifiers first
		if (instanceMap->IsHeroic())
		{
			switch (maxNumberOfPlayers)
			{
			    case 1:
			    case 2:
			    case 3:
			    case 4:
			    case 5:
			        statMod_global = sAutoBalancer->StatModifierHeroic_Global;
			        statMod_health = sAutoBalancer->StatModifierHeroic_Health;
			        statMod_mana = sAutoBalancer->StatModifierHeroic_Mana;
			        statMod_armor = sAutoBalancer->StatModifierHeroic_Armor;
			        statMod_damage = sAutoBalancer->StatModifierHeroic_Damage;
			        statMod_ccDuration = sAutoBalancer->StatModifierHeroic_CCDuration;

			        statMod_boss_global = sAutoBalancer->StatModifierHeroic_Boss_Global;
			        statMod_boss_health = sAutoBalancer->StatModifierHeroic_Boss_Health;
			        statMod_boss_mana = sAutoBalancer->StatModifierHeroic_Boss_Mana;
			        statMod_boss_armor = sAutoBalancer->StatModifierHeroic_Boss_Armor;
			        statMod_boss_damage = sAutoBalancer->StatModifierHeroic_Boss_Damage;
			        statMod_boss_ccDuration = sAutoBalancer->StatModifierHeroic_Boss_CCDuration;
			        break;
			    case 10:
                    statMod_global = sAutoBalancer->StatModifierRaid10MHeroic_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid10MHeroic_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid10MHeroic_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid10MHeroic_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid10MHeroic_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid10MHeroic_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid10MHeroic_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid10MHeroic_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid10MHeroic_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid10MHeroic_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid10MHeroic_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid10MHeroic_Boss_CCDuration;
			        break;
			    case 25:
                    statMod_global = sAutoBalancer->StatModifierRaid25MHeroic_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid25MHeroic_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid25MHeroic_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid25MHeroic_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid25MHeroic_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid25MHeroic_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid25MHeroic_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid25MHeroic_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid25MHeroic_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid25MHeroic_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid25MHeroic_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid25MHeroic_Boss_CCDuration;
                    break;
			    default:
                    statMod_global = sAutoBalancer->StatModifierRaidHeroic_Global;
                    statMod_health = sAutoBalancer->StatModifierRaidHeroic_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaidHeroic_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaidHeroic_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaidHeroic_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaidHeroic_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaidHeroic_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaidHeroic_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaidHeroic_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaidHeroic_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaidHeroic_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaidHeroic_Boss_CCDuration;
			}
		}
		else
		{
			switch (maxNumberOfPlayers)
			{
			    case 1:
			    case 2:
			    case 3:
			    case 4:
			    case 5:
			        statMod_global = sAutoBalancer->StatModifier_Global;
			        statMod_health = sAutoBalancer->StatModifier_Health;
			        statMod_mana = sAutoBalancer->StatModifier_Mana;
			        statMod_armor = sAutoBalancer->StatModifier_Armor;
			        statMod_damage = sAutoBalancer->StatModifier_Damage;
			        statMod_ccDuration = sAutoBalancer->StatModifier_CCDuration;

			        statMod_boss_global = sAutoBalancer->StatModifier_Boss_Global;
			        statMod_boss_health = sAutoBalancer->StatModifier_Boss_Health;
			        statMod_boss_mana = sAutoBalancer->StatModifier_Boss_Mana;
			        statMod_boss_armor = sAutoBalancer->StatModifier_Boss_Armor;
			        statMod_boss_damage = sAutoBalancer->StatModifier_Boss_Damage;
			        statMod_boss_ccDuration = sAutoBalancer->StatModifier_Boss_CCDuration;
			        break;
			    case 10:
                    statMod_global = sAutoBalancer->StatModifierRaid10M_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid10M_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid10M_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid10M_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid10M_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid10M_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid10M_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid10M_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid10M_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid10M_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid10M_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid10M_Boss_CCDuration;
                    break;
			    case 15:
                    statMod_global = sAutoBalancer->StatModifierRaid15M_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid15M_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid15M_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid15M_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid15M_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid15M_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid15M_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid15M_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid15M_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid15M_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid15M_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid15M_Boss_CCDuration;
                    break;
			    case 20:
                    statMod_global = sAutoBalancer->StatModifierRaid20M_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid20M_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid20M_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid20M_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid20M_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid20M_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid20M_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid20M_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid20M_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid20M_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid20M_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid20M_Boss_CCDuration;
                    break;
			    case 25:
                    statMod_global = sAutoBalancer->StatModifierRaid25M_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid25M_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid25M_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid25M_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid25M_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid25M_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid25M_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid25M_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid25M_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid25M_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid25M_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid25M_Boss_CCDuration;
                    break;
			    case 40:
                    statMod_global = sAutoBalancer->StatModifierRaid40M_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid40M_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid40M_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid40M_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid40M_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid40M_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid40M_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid40M_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid40M_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid40M_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid40M_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid40M_Boss_CCDuration;
                    break;
			    default:
                    statMod_global = sAutoBalancer->StatModifierRaid_Global;
                    statMod_health = sAutoBalancer->StatModifierRaid_Health;
                    statMod_mana = sAutoBalancer->StatModifierRaid_Mana;
                    statMod_armor = sAutoBalancer->StatModifierRaid_Armor;
                    statMod_damage = sAutoBalancer->StatModifierRaid_Damage;
                    statMod_ccDuration = sAutoBalancer->StatModifierRaid_CCDuration;

                    statMod_boss_global = sAutoBalancer->StatModifierRaid_Boss_Global;
                    statMod_boss_health = sAutoBalancer->StatModifierRaid_Boss_Health;
                    statMod_boss_mana = sAutoBalancer->StatModifierRaid_Boss_Mana;
                    statMod_boss_armor = sAutoBalancer->StatModifierRaid_Boss_Armor;
                    statMod_boss_damage = sAutoBalancer->StatModifierRaid_Boss_Damage;
                    statMod_boss_ccDuration = sAutoBalancer->StatModifierRaid_Boss_CCDuration;
			}
		}

        // Boss modifiers
        if (creature->IsDungeonBoss())
        {
            // Start with the settings determined above
            // AutoBalance.StatModifier*.Boss.<stat>
            if (creature->IsDungeonBoss())
            {
                statMod_global = statMod_boss_global;
                statMod_health = statMod_boss_health;
                statMod_mana = statMod_boss_mana;
                statMod_armor = statMod_boss_armor;
                statMod_damage = statMod_boss_damage;
                statMod_ccDuration = statMod_boss_ccDuration;
            }

            // Per-instance boss overrides
            // AutoBalance.StatModifier.Boss.PerInstance
            if (creature->IsDungeonBoss() && sAutoBalancer->hasStatModifierBossOverride(mapId))
            {
                AutoBalanceStatModifiers* myStatModifierBossOverrides = &sAutoBalancer->statModifierBossOverrides[mapId];

                if (myStatModifierBossOverrides->global != -1)      { statMod_global =      myStatModifierBossOverrides->global;      }
                if (myStatModifierBossOverrides->health != -1)      { statMod_health =      myStatModifierBossOverrides->health;      }
                if (myStatModifierBossOverrides->mana != -1)        { statMod_mana =        myStatModifierBossOverrides->mana;        }
                if (myStatModifierBossOverrides->armor != -1)       { statMod_armor =       myStatModifierBossOverrides->armor;       }
                if (myStatModifierBossOverrides->damage != -1)      { statMod_damage =      myStatModifierBossOverrides->damage;      }
                if (myStatModifierBossOverrides->ccduration != -1)  { statMod_ccDuration =  myStatModifierBossOverrides->ccduration;  }
            }
        }
        // Non-boss modifiers
        else
        {
            // Per-instance non-boss overrides
            // AutoBalance.StatModifier.PerInstance
            if (sAutoBalancer->hasStatModifierOverride(mapId))
            {
                AutoBalanceStatModifiers* myStatModifierOverrides = &sAutoBalancer->statModifierOverrides[mapId];

                if (myStatModifierOverrides->global != -1)      { statMod_global =      myStatModifierOverrides->global;      }
                if (myStatModifierOverrides->health != -1)      { statMod_health =      myStatModifierOverrides->health;      }
                if (myStatModifierOverrides->mana != -1)        { statMod_mana =        myStatModifierOverrides->mana;        }
                if (myStatModifierOverrides->armor != -1)       { statMod_armor =       myStatModifierOverrides->armor;       }
                if (myStatModifierOverrides->damage != -1)      { statMod_damage =      myStatModifierOverrides->damage;      }
                if (myStatModifierOverrides->ccduration != -1)  { statMod_ccDuration =  myStatModifierOverrides->ccduration;  }
            }
        }

        // Per-creature modifiers applied last
        // AutoBalance.StatModifier.PerCreature
        if (sAutoBalancer->hasStatModifierCreatureOverride(creatureTemplate->Entry))
        {
            AutoBalanceStatModifiers* myCreatureOverrides = &sAutoBalancer->statModifierCreatureOverrides[creatureTemplate->Entry];

            if (myCreatureOverrides->global != -1)      { statMod_global =      myCreatureOverrides->global;      }
            if (myCreatureOverrides->health != -1)      { statMod_health =      myCreatureOverrides->health;      }
            if (myCreatureOverrides->mana != -1)        { statMod_mana =        myCreatureOverrides->mana;        }
            if (myCreatureOverrides->armor != -1)       { statMod_armor =       myCreatureOverrides->armor;       }
            if (myCreatureOverrides->damage != -1)      { statMod_damage =      myCreatureOverrides->damage;      }
            if (myCreatureOverrides->ccduration != -1)  { statMod_ccDuration =  myCreatureOverrides->ccduration;  }
        }

        // #maththings
        float diff = ((float)maxNumberOfPlayers/5)*1.5f;

        // For math reasons that I do not understand, curveCeiling needs to be adjusted to bring the actual multiplier
        // closer to the curveCeiling setting. Create an adjustment based on how much the ceiling should be changed at
        // the max players multiplier.
        float curveCeilingAdjustment = curveCeiling / (((tanh(((float)maxNumberOfPlayers - inflectionValue) / diff) + 1.0f) / 2.0f) * (curveCeiling - curveFloor) + curveFloor);

        // Adjust the multiplier based on the configured floor and ceiling values, plus the ceiling adjustment we just calculated
        defaultMultiplier = ((tanh(((float)creatureABInfo->instancePlayerCount - inflectionValue) / diff) + 1.0f) / 2.0f) * (curveCeiling * curveCeilingAdjustment - curveFloor) + curveFloor;

        if (!sABScriptMgr->OnAfterDefaultMultiplier(creature, defaultMultiplier))
            return;

        //
        //  Health Scaling
        //

        float healthMultiplier = defaultMultiplier * statMod_global * statMod_health;

        if (healthMultiplier <= sAutoBalancer->MinHPModifier)
            healthMultiplier = sAutoBalancer->MinHPModifier;

        float hpStatsRate  = 1.0f;
        float originalHealth = origCreatureStats->GenerateHealth(creatureTemplate);

        float newBaseHealth;

        // The database holds multiple values for base health, one for each expansion
        // This code will smooth transition between the different expansions based on the highest player level in the instance
        // Only do this if level scaling is enabled

        if (sAutoBalancer->LevelScaling)
        {
            float vanillaHealth = creatureStats->BaseHealth[0];
            float bcHealth = creatureStats->BaseHealth[1];
            float wotlkHealth = creatureStats->BaseHealth[2];

            // vanilla health
            if (mapABInfo->highestPlayerLevel <= 60)
            {
                newBaseHealth = vanillaHealth;
            }
            // transition from vanilla to BC health
            else if (mapABInfo->highestPlayerLevel < 63)
            {
                float vanillaMultiplier = (63 - mapABInfo->highestPlayerLevel) / 3.0f;
                float bcMultiplier = 1.0f - vanillaMultiplier;

                newBaseHealth = (vanillaHealth * vanillaMultiplier) + (bcHealth * bcMultiplier);
            }
            // BC health
            else if (mapABInfo->highestPlayerLevel <= 70)
            {
                newBaseHealth = bcHealth;
            }
            // transition from BC to WotLK health
            else if (mapABInfo->highestPlayerLevel < 73)
            {
                float bcMultiplier = (73 - mapABInfo->highestPlayerLevel) / 3.0f;
                float wotlkMultiplier = 1.0f - bcMultiplier;

                newBaseHealth = (bcHealth * bcMultiplier) + (wotlkHealth * wotlkMultiplier);
            }
            // WotLK health
            else
            {
                newBaseHealth = wotlkHealth;

                // special increase for end-game content
                if (sAutoBalancer->LevelScalingEndGameBoost)
                    if (mapABInfo->highestPlayerLevel >= 75 && creatureABInfo->UnmodifiedLevel < 75)
                    {
                        newBaseHealth *= (float)(mapABInfo->highestPlayerLevel-70) * 0.3f;
                    }
            }

            float newHealth = newBaseHealth * creatureTemplate->ModHealth;
            hpStatsRate = newHealth / originalHealth;

            healthMultiplier *= hpStatsRate;
        }

        creatureABInfo->HealthMultiplier = healthMultiplier;
        scaledHealth = round(originalHealth * creatureABInfo->HealthMultiplier);

        //
        //  Mana Scaling
        //
        float manaStatsRate  = 1.0f;
        float newMana = creatureStats->GenerateMana(creatureTemplate);
            manaStatsRate = newMana/float(baseMana);

        // check to be sure that manaStatsRate is not nan
        if (manaStatsRate != manaStatsRate)
        {
            creatureABInfo->ManaMultiplier = 0.0f;
        }
        else
        {
            creatureABInfo->ManaMultiplier =  defaultMultiplier * manaStatsRate * statMod_global * statMod_mana;

            if (creatureABInfo->ManaMultiplier <= sAutoBalancer->MinManaModifier)
            {
                creatureABInfo->ManaMultiplier = sAutoBalancer->MinManaModifier;
            }
        }

        scaledMana = round(baseMana * creatureABInfo->ManaMultiplier);

        //
        //  Armor Scaling
        //
        creatureABInfo->ArmorMultiplier = defaultMultiplier * statMod_global * statMod_armor;
        uint32 newBaseArmor = round(creatureABInfo->ArmorMultiplier * (sAutoBalancer->LevelScaling ? creatureStats->GenerateArmor(creatureTemplate) : origCreatureStats->GenerateArmor(creatureTemplate)));

        //
        //  Damage Scaling
        //
        float damageMul = defaultMultiplier * statMod_global * statMod_damage;

        // Can not be less than MinDamageModifier
        if (damageMul <= sAutoBalancer->MinDamageModifier)
        {
            damageMul = sAutoBalancer->MinDamageModifier;
        }

        // Calculate the new base damage
        float origDmgBase = origCreatureStats->GenerateBaseDamage(creatureTemplate);
        float newDmgBase = 0;

        float vanillaDamage = creatureStats->BaseDamage[0];
        float bcDamage = creatureStats->BaseDamage[1];
        float wotlkDamage = creatureStats->BaseDamage[2];

        // The database holds multiple values for base damage, one for each expansion
        // This code will smooth transition between the different expansions based on the highest player level in the instance
        // Only do this if level scaling is enabled

        if (sAutoBalancer->LevelScaling)
            {
            // vanilla damage
            if (mapABInfo->highestPlayerLevel <= 60)
            {
                newDmgBase=vanillaDamage;
            }
            // transition from vanilla to BC damage
            else if (mapABInfo->highestPlayerLevel < 63)
            {
                float vanillaMultiplier = (63 - mapABInfo->highestPlayerLevel) / 3.0;
                float bcMultiplier = 1.0f - vanillaMultiplier;

                newDmgBase=(vanillaDamage * vanillaMultiplier) + (bcDamage * bcMultiplier);
            }
            // BC damage
            else if (mapABInfo->highestPlayerLevel <= 70)
            {
                newDmgBase=bcDamage;
            }
            // transition from BC to WotLK damage
            else if (mapABInfo->highestPlayerLevel < 73)
            {
                float bcMultiplier = (73 - mapABInfo->highestPlayerLevel) / 3.0;
                float wotlkMultiplier = 1.0f - bcMultiplier;

                newDmgBase=(bcDamage * bcMultiplier) + (wotlkDamage * wotlkMultiplier);
            }
            // WotLK damage
            else
            {
                newDmgBase=wotlkDamage;

                // special increase for end-game content
                if (sAutoBalancer->LevelScalingEndGameBoost && maxNumberOfPlayers <= 5) {
                    if (mapABInfo->highestPlayerLevel >= 75 && creatureABInfo->UnmodifiedLevel < 75)
                        newDmgBase *= float(mapABInfo->highestPlayerLevel-70) * 0.3f;
                }
            }

            damageMul *= newDmgBase/origDmgBase;
        }

        //
        // Crowd Control Debuff Duration Scaling
        //
        float ccDurationMul;
        if (statMod_ccDuration != -1.0f)
        {
            ccDurationMul = defaultMultiplier * statMod_ccDuration;

            // Min/Max checking
            if (ccDurationMul < sAutoBalancer->MinCCDurationModifier)
            {
                ccDurationMul = sAutoBalancer->MinCCDurationModifier;
            }
            else if (ccDurationMul > sAutoBalancer->MaxCCDurationModifier)
            {
                ccDurationMul = sAutoBalancer->MaxCCDurationModifier;
            }
        }
        else
        {
            ccDurationMul = 1.0f;
        }

        //
        //  Apply New Values
        //
        if (!sABScriptMgr->OnBeforeUpdateStats(creature, scaledHealth, scaledMana, damageMul, newBaseArmor))
            return;

        uint32 prevMaxHealth = creature->GetMaxHealth();
        uint32 prevMaxPower = creature->GetMaxPower(POWER_MANA);
        uint32 prevHealth = creature->GetHealth();
        uint32 prevPower = creature->GetPower(POWER_MANA);

        Powers pType= creature->getPowerType();

        creature->SetArmor(newBaseArmor);
        creature->SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, (float)newBaseArmor);
        creature->SetCreateHealth(scaledHealth);
        creature->SetMaxHealth(scaledHealth);
        creature->ResetPlayerDamageReq();
        creature->SetCreateMana(scaledMana);
        creature->SetMaxPower(POWER_MANA, scaledMana);
        creature->SetModifierValue(UNIT_MOD_ENERGY, BASE_VALUE, (float)100.0f);
        creature->SetModifierValue(UNIT_MOD_RAGE, BASE_VALUE, (float)100.0f);
        creature->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, (float)scaledHealth);
        creature->SetModifierValue(UNIT_MOD_MANA, BASE_VALUE, (float)scaledMana);
        creatureABInfo->DamageMultiplier = damageMul;
        creatureABInfo->CCDurationMultiplier = ccDurationMul;

        uint32 scaledCurHealth=prevHealth && prevMaxHealth ? float(scaledHealth)/float(prevMaxHealth)*float(prevHealth) : 0;
        uint32 scaledCurPower=prevPower && prevMaxPower  ? float(scaledMana)/float(prevMaxPower)*float(prevPower) : 0;

        creature->SetHealth(scaledCurHealth);
        if (pType == POWER_MANA)
            creature->SetPower(POWER_MANA, scaledCurPower);
        else
            creature->setPowerType(pType); // fix creatures with different power types

        //
        // Reward Scaling
        //

        // calculate the average multiplier after level scaling is applied
        float averageMultiplierAfterLevelScaling;
        // use health and damage to calculate the average multiplier
        averageMultiplierAfterLevelScaling = (creatureABInfo->HealthMultiplier + creatureABInfo->DamageMultiplier) / 2.0f;

        // XP Scaling
        if (sAutoBalancer->RewardScalingXP)
        {
            if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_FIXED)
            {
                creatureABInfo->XPModifier = sAutoBalancer->RewardScalingXPModifier;
            }
            else if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_DYNAMIC)
            {
                creatureABInfo->XPModifier = averageMultiplierAfterLevelScaling * sAutoBalancer->RewardScalingXPModifier;
            }
        }

        // Money Scaling
        if (sAutoBalancer->RewardScalingMoney)
        {
            //LOG_DEBUG("module.AutoBalance", "AutoBalance_AllCreatureScript::ModifyCreatureAttributes: Creature {} ({}) has an average post-level-scaling modifier of {}.", creature->GetName(), creature->GetLevel(), averageMultiplierAfterLevelScaling);

            if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_FIXED)
            {
                creatureABInfo->MoneyModifier = sAutoBalancer->RewardScalingMoneyModifier;
            }
            else if (sAutoBalancer->RewardScalingMethod == AUTOBALANCE_SCALING_DYNAMIC)
            {
                creatureABInfo->MoneyModifier = averageMultiplierAfterLevelScaling * sAutoBalancer->RewardScalingMoneyModifier;
            }
        }

        creature->UpdateAllStats();
    }

private:
    uint32 adjustCurCount(uint32 inputCount, uint32 dungeonId)
    {
        uint8 minPlayers = sAutoBalancer->enabledDungeonIds[dungeonId];
        return inputCount < minPlayers ? minPlayers : inputCount;
    }
};