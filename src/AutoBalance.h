#ifndef MOD_AUTOBALANCE_H
#define MOD_AUTOBALANCE_H

#include "ScriptMgr.h"
#include "Creature.h"
#include "AutoBalancer.h"

// Manages registration, loading, and execution of scripts.
class ABScriptMgr
{
    public: /* Initialization */

        static ABScriptMgr* instance();
        // called at the start of ModifyCreatureAttributes method
        // it can be used to add some condition to skip autobalancing system for example
        bool OnBeforeModifyAttributes(Creature* creature, uint32 & instancePlayerCount);
        // called right after default multiplier has been set, you can use it to change
        // current scaling formula based on number of players or just skip modifications
        bool OnAfterDefaultMultiplier(Creature* creature, float &defaultMultiplier);
        // called before change creature values, to tune some values or skip modifications
        bool OnBeforeUpdateStats(Creature* creature, uint32 &scaledHealth, uint32 &scaledMana, float &damageMultiplier, uint32 &newBaseArmor);
};

#define sABScriptMgr ABScriptMgr::instance()

/*
* Dedicated hooks for Autobalance Module
* Can be used to extend/customize this system
*/
class ABModuleScript : public ModuleScript
{
    protected:

        ABModuleScript(const char* name);

    public:
        virtual bool OnBeforeModifyAttributes(Creature* /*creature*/, uint32 & /*instancePlayerCount*/) { return true; }
        virtual bool OnAfterDefaultMultiplier(Creature* /*creature*/, float & /*defaultMultiplier*/) { return true; }
        virtual bool OnBeforeUpdateStats(Creature* /*creature*/, uint32 &/*scaledHealth*/, uint32 &/*scaledMana*/, float &/*damageMultiplier*/, uint32 &/*newBaseArmor*/) { return true; }
};

template class ScriptRegistry<ABModuleScript>;

#endif

#ifndef AUTOBALANCE_H
#define AUTOBALANCE_H

class AutoBalanceStatModifiers : public DataMap::Base
{
public:
    AutoBalanceStatModifiers() {}
    AutoBalanceStatModifiers(float global, float health, float mana, float armor, float damage, float ccduration) :
        global(global), health(health), mana(mana), armor(armor), damage(damage), ccduration(ccduration) {}
    float global;
    float health;
    float mana;
    float armor;
    float damage;
    float ccduration;

    std::time_t configTime;
};

class AutoBalanceInflectionPointSettings : public DataMap::Base
{
public:
    AutoBalanceInflectionPointSettings() {}
    AutoBalanceInflectionPointSettings(float value, float curveFloor, float curveCeiling) :
        value(value), curveFloor(curveFloor), curveCeiling(curveCeiling) {}
    float value;
    float curveFloor;
    float curveCeiling;
};

class AutoBalanceLevelScalingDynamicLevelSettings: public DataMap::Base
{
public:
    AutoBalanceLevelScalingDynamicLevelSettings() {}
    AutoBalanceLevelScalingDynamicLevelSettings(int skipHigher, int skipLower, int ceiling, int floor) :
        skipHigher(skipHigher), skipLower(skipLower), ceiling(ceiling), floor(floor) {}
    int skipHigher;
    int skipLower;
    int ceiling;
    int floor;
};

enum ScalingMethod {
    AUTOBALANCE_SCALING_FIXED,
    AUTOBALANCE_SCALING_DYNAMIC
};

enum GroupDifficulty {
    GROUP_DIFFICULTY_NORMAL    = 0,
    GROUP_DIFFICULTY_HEROIC    = 1,
    GROUP_DIFFICULTY_MYTHIC    = 2,
    GROUP_DIFFICULTY_LEGENDARY = 3,
    GROUP_DIFFICULTY_ASCENDANT = 4
};

class AutoBalanceCreatureInfo : public DataMap::Base
{
public:
    AutoBalanceCreatureInfo() {}

    uint64_t configTime;

    uint32 instancePlayerCount = 0;
    uint8 selectedLevel = 0;
    // this is used to detect creatures that update their entry
    uint32 entry = 0;
    float DamageMultiplier = 1.0f;
    float HealthMultiplier = 1.0f;
    float ManaMultiplier = 1.0f;
    float ArmorMultiplier = 1.0f;
    float CCDurationMultiplier = 1.0f;

    float XPModifier = 1.0f;
    float MoneyModifier = 1.0f;

    uint8 UnmodifiedLevel = 0;

    bool isActive = false;
    bool wasAliveNowDead = false;
    bool isInCreatureList = false;
};

class AutoBalanceMapInfo : public DataMap::Base
{
public:
    AutoBalanceMapInfo() {}

    uint64_t configTime;

    uint32 playerCount = 0;

    uint8 mapLevel = 0;
    uint8 lowestPlayerLevel = 0;
    uint8 highestPlayerLevel = 0;

    uint8 lfgMinLevel = 0;
    uint8 lfgTargetLevel = 80;
    uint8 lfgMaxLevel = 80;

    bool enabled = false;

    std::vector<Creature*> allMapCreatures;
    uint8 highestCreatureLevel = 0;
    uint8 lowestCreatureLevel = 0;
    float avgCreatureLevel;
    uint32 activeCreatureCount = 0;

    bool isLevelScalingEnabled;
    int levelScalingSkipHigherLevels, levelScalingSkipLowerLevels;
    int levelScalingDynamicCeiling, levelScalingDynamicFloor;

    int customDifficulty = 0;
};

#endif
