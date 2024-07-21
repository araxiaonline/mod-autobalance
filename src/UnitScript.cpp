#include "ScriptMgr.h"
#include "Player.h"
#include "AutoBalancer.h"

class AutoBalance_UnitScript : public UnitScript
{
    public:
    AutoBalance_UnitScript()
        : UnitScript("AutoBalance_UnitScript", true)
    {
    }

    uint32 DealDamage(Unit* AttackerUnit, Unit *playerVictim, uint32 damage, DamageEffectType /*damagetype*/) override
    {
        return _Modifer_DealDamage(playerVictim, AttackerUnit, damage);
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        uint32 newdamage = _Modifer_DealDamage(target, attacker, damage);

        if(target->IsPlayer() && !attacker->IsPlayer())
        {
            Group* group = target->ToPlayer()->GetGroup();
            if(group)
            {
                uint8 difficulty = sAutoBalancer->GetGroupDifficulty(group);
                if(difficulty == 2) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_DoTDamage * 0.50;
                }
                else if(difficulty == 3) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_DoTDamage * 0.75;
                }
                else if(difficulty == 4) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_DoTDamage * 1.20;
                }
                else {
                    newdamage = newdamage * sAutoBalancer->StatModifier_DoTDamage;
                }
            }
        }

        damage = newdamage;
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* /*spellInfo*/) override
    {
        uint32 newdamage = _Modifer_DealDamage(target, attacker, damage);

        if(target->IsPlayer() && !attacker->IsPlayer())
        {
            Group* group = target->ToPlayer()->GetGroup();
            if(group)
            {
                uint8 difficulty = sAutoBalancer->GetGroupDifficulty(group);
                if(difficulty == 2) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_SpellDamage * 0.80;
                }
                else if(difficulty == 3) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_SpellDamage * 1.0;
                }
                else if(difficulty == 4) {
                    newdamage = newdamage * sAutoBalancer->StatModifier_SpellDamage * 1.20;
                }
                else {
                    newdamage = newdamage * sAutoBalancer->StatModifier_SpellDamage;
                }
            }
        }

        damage = newdamage;
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {

        uint32 newdamage = _Modifer_DealDamage(target, attacker, damage);

        if(target->IsPlayer() && !attacker->IsPlayer())
        {
            Group* group = target->ToPlayer()->GetGroup();
            if(group)
            {
                uint8 difficulty = sAutoBalancer->GetGroupDifficulty(group);
                if(difficulty == 2) {
                    newdamage = newdamage * 1.0;
                }
                else if(difficulty == 3) {
                    newdamage = newdamage * 1.30;
                }
                else if(difficulty == 4) {
                    newdamage = newdamage * 2.0;
                }
                else {
                    newdamage = newdamage;
                }
            }
        }

        damage = newdamage;
    }

    void ModifyHealReceived(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* /*spellInfo*/) override
    {
        damage = _Modifer_DealDamage(target, attacker, damage);
    }

    void OnAuraApply(Unit* unit, Aura* aura) override {
        // Only if this aura has a duration
        if (aura->GetDuration() > 0 || aura->GetMaxDuration() > 0)
        {
            uint32 auraDuration = _Modifier_CCDuration(unit, aura->GetCaster(), aura);

            // only update if we decided to change it
            if (auraDuration != (float)aura->GetDuration())
            {
                aura->SetMaxDuration(auraDuration);
                aura->SetDuration(auraDuration);
            }
        }
    }

    uint32 _Modifer_DealDamage(Unit* target, Unit* attacker, uint32 damage)
    {
        // check that we're enabled globally, else return the original damage
        if (!sAutoBalancer->EnableGlobal)
            return damage;

        // make sure we have an attacker, that its not a player, and that the attacker is in the world, else return the original damage
        if (!attacker || attacker->GetTypeId() == TYPEID_PLAYER || !attacker->IsInWorld())
            return damage;

        // make sure we're in an instance, else return the original damage
        if (
            !(
                (target->GetMap()->IsDungeon() && attacker->GetMap()->IsDungeon()) ||
                (target->GetMap()->IsRaid() && attacker->GetMap()->IsRaid())
            )
           )
            return damage;

        // get the map's info to see if we're enabled
        AutoBalanceMapInfo *targetMapInfo = target->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
        AutoBalanceMapInfo *attackerMapInfo = attacker->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");

        // if either the target or the attacker's maps are not enabled, return the original damage
        if (!targetMapInfo->enabled || !attackerMapInfo->enabled)
            return damage;

        // get the current creature's damage multiplier
        float damageMultiplier = attacker->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo")->DamageMultiplier;

        // if it's the default of 1.0, return the original damage
        if (damageMultiplier == 1)
            return damage;

        // if the attacker is under the control of the player, return the original damage
        if ((attacker->IsHunterPet() || attacker->IsPet() || attacker->IsSummon()) && attacker->IsControlledByPlayer())
            return damage;

        // we are good to go, return the original damage times the multiplier
        return damage * damageMultiplier;
    }

    uint32 _Modifier_CCDuration(Unit* target, Unit* caster, Aura* aura)
    {
        // store the original duration of the aura
        float originalDuration = (float)aura->GetDuration();

        // check that we're enabled globally, else return the original duration
        if (!sAutoBalancer->EnableGlobal)
            return originalDuration;

        // ensure that both the target and the caster are defined
        if (!target || !caster)
            return originalDuration;

        // if the aura wasn't cast just now, don't change it
        if (aura->GetDuration() != aura->GetMaxDuration())
            return originalDuration;

        // if the target isn't a player or the caster is a player, return the original duration
        if (!target->IsPlayer() || caster->IsPlayer())
            return originalDuration;

        // make sure we're in an instance, else return the original duration
        if (
            !(
                (target->GetMap()->IsDungeon() && caster->GetMap()->IsDungeon()) ||
                (target->GetMap()->IsRaid() && caster->GetMap()->IsRaid())
            )
           )
            return originalDuration;

        // get the current creature's CC duration multiplier
        float ccDurationMultiplier = caster->CustomData.GetDefault<AutoBalanceCreatureInfo>("AutoBalanceCreatureInfo")->CCDurationMultiplier;

        // if it's the default of 1.0, return the original damage
        if (ccDurationMultiplier == 1)
            return originalDuration;

        // if the aura was cast by a pet or summon, return the original duration
        if ((caster->IsHunterPet() || caster->IsPet() || caster->IsSummon()) && caster->IsControlledByPlayer())
            return originalDuration;

        // only if this aura is a CC
        if (
            aura->HasEffectType(SPELL_AURA_MOD_CHARM)          ||
            aura->HasEffectType(SPELL_AURA_MOD_CONFUSE)        ||
            aura->HasEffectType(SPELL_AURA_MOD_DISARM)         ||
            aura->HasEffectType(SPELL_AURA_MOD_FEAR)           ||
            aura->HasEffectType(SPELL_AURA_MOD_PACIFY)         ||
            aura->HasEffectType(SPELL_AURA_MOD_POSSESS)        ||
            aura->HasEffectType(SPELL_AURA_MOD_SILENCE)        ||
            aura->HasEffectType(SPELL_AURA_MOD_STUN)           ||
            aura->HasEffectType(SPELL_AURA_MOD_SPEED_SLOW_ALL)
            )
        {
            return originalDuration * ccDurationMultiplier;
        }
        else
        {
            return originalDuration;
        }
    }
};

void AddUnitScripts()
{
    new AutoBalance_UnitScript();
}