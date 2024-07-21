/*
* Copyright (C) 2018 AzerothCore <http://www.azerothcore.org>
* Copyright (C) 2012 CVMagic <http://www.trinitycore.org/f/topic/6551-vas-autobalance/>
* Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
* Copyright (C) 1985-2010 KalCorp  <http://vasserver.dyndns.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/*
* Script Name: AutoBalance
* Original Authors: KalCorp and Vaughner
* Maintainer(s): AzerothCore
* Original Script Name: AutoBalance
* Description: This script is intended to scale based on number of players,
* instance mobs & world bosses' level, health, mana, and damage.
*/

#include "Configuration/Config.h"
#include "Unit.h"
#include "Chat.h"
#include "Creature.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "MapMgr.h"
#include "World.h"
#include "Map.h"
#include "ScriptMgr.h"
#include "Language.h"
#include <vector>
#include "AutoBalance.h"
#include "ScriptMgrMacros.h"
#include "Group.h"
#include "Log.h"
#include <chrono>

#include "AutoBalancer.h"
#include "WorldScript.cpp"
#include "PlayerScript.cpp"
#include "UnitScript.cpp"
#include "AllMapScript.cpp"
#include "AllCreatureScript.cpp"
#include "CommandScript.cpp"
#include "GlobalScript.cpp"

using namespace Acore::ChatCommands;

ABScriptMgr* ABScriptMgr::instance()
{
    static ABScriptMgr instance;
    return &instance;
}

bool ABScriptMgr::OnBeforeModifyAttributes(Creature *creature, uint32 & instancePlayerCount)
{
    auto ret = IsValidBoolScript<ABModuleScript>([&](ABModuleScript* script)
    {
        return !script->OnBeforeModifyAttributes(creature, instancePlayerCount);
    });

    if (ret && *ret)
    {
        return false;
    }

    return true;
}

bool ABScriptMgr::OnAfterDefaultMultiplier(Creature *creature, float& defaultMultiplier)
{
    auto ret = IsValidBoolScript<ABModuleScript>([&](ABModuleScript* script)
    {
        return !script->OnAfterDefaultMultiplier(creature, defaultMultiplier);
    });

    if (ret && *ret)
    {
        return false;
    }

    return true;
}

bool ABScriptMgr::OnBeforeUpdateStats(Creature* creature, uint32& scaledHealth, uint32& scaledMana, float& damageMultiplier, uint32& newBaseArmor)
{
    auto ret = IsValidBoolScript<ABModuleScript>([&](ABModuleScript* script)
    {
        return !script->OnBeforeUpdateStats(creature, scaledHealth, scaledMana, damageMultiplier, newBaseArmor);
    });

    if (ret && *ret)
    {
        return false;
    }

    return true;
}

ABModuleScript::ABModuleScript(const char* name)
    : ModuleScript(name)
{
    ScriptRegistry<ABModuleScript>::AddScript(this);
}

// this handles updating custom group difficulties used in auto balancing mobs and
// scripts that enable buffs on mobs randomly
class AutoBalance_GroupScript : public GroupScript {
public:
    AutoBalance_GroupScript() : GroupScript("AutoBalance_GroupScript") { }

    void OnCreate(Group* group, Player* leader) override {
        if (!group) {
            return;
        }

        if(!leader) {
            return;
        }

        // default difficulty is whatever the player currently has it set as
        uint8 difficulty = leader->GetDifficulty(false);

        CharacterDatabase.DirectExecute("INSERT INTO group_difficulty (guid, difficulty) VALUES ({}, {}) ON DUPLICATE KEY UPDATE difficulty = {}",
            group->GetGUID().GetCounter(), difficulty, difficulty);
    }

    void OnDisband(Group* group) override {
        if (!group) {
            return;
        }

        CharacterDatabase.DirectExecute("DELETE FROM group_difficulty WHERE guid = {}", group->GetGUID().GetCounter());
    }
};

// this adds in the custom difficulties configurations for mythic, legendary, and ascendant
class AutoBalance_DifficultyScript : public ABModuleScript {
public:
    AutoBalance_DifficultyScript() : ABModuleScript("AutoBalance_DifficultyScript") { }

    bool OnAfterDefaultMultiplier(Creature* creature, float &defaultMultiplier)
    {

        AutoBalanceMapInfo *mapABInfo = creature->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
        if (!mapABInfo)
            return true;

        if (mapABInfo->customDifficulty != 0)
        {
            switch (mapABInfo->customDifficulty)
            {
                case GROUP_DIFFICULTY_MYTHIC:
                    defaultMultiplier *= sAutoBalancer->MythicMultiplier;
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: MythicMultiplier being applied {}", sAutoBalancer->MythicMultiplier);
                    break;
                case GROUP_DIFFICULTY_LEGENDARY:
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: LegendaryMultiplier being applied {}", sAutoBalancer->LegendaryMultiplier);
                    defaultMultiplier *= sAutoBalancer->LegendaryMultiplier;
                    break;
                case GROUP_DIFFICULTY_ASCENDANT:
                    defaultMultiplier *= sAutoBalancer->AscendantMultiplier;
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: AscendantMultiplier being applied {}", sAutoBalancer->AscendantMultiplier);
                    break;
                default:
                    break;
            }
        }

        return true;
    }

     virtual bool OnBeforeUpdateStats(Creature* creature, uint32 &scaledHealth, uint32 &scaledMana, float &damageMultiplier, uint32 &newBaseArmor) {

        AutoBalanceMapInfo *mapABInfo = creature->GetMap()->CustomData.GetDefault<AutoBalanceMapInfo>("AutoBalanceMapInfo");
        if (!mapABInfo)
            return true;

        if (mapABInfo->customDifficulty != 0)
        {
            switch (mapABInfo->customDifficulty)
            {
                case GROUP_DIFFICULTY_MYTHIC:
                    scaledHealth *= sAutoBalancer->MythicMultiplier;
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: MythicMultiplier being applied {}", sAutoBalancer->MythicMultiplier);
                    break;
                case GROUP_DIFFICULTY_LEGENDARY:
                    scaledHealth *= sAutoBalancer->LegendaryMultiplier;
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: LegendaryMultiplier being applied {}", sAutoBalancer->LegendaryMultiplier);

                    break;
                case GROUP_DIFFICULTY_ASCENDANT:
                    scaledHealth *= sAutoBalancer->AscendantMultiplier;
                    LOG_INFO("server", "> OnAfterDefaultMultiplier: AscendantMultiplier being applied {}", sAutoBalancer->AscendantMultiplier);
                    break;
                default:
                    break;
            }
        }

        return true;
     }
};

void AddAutoBalanceScripts()
{
    new AutoBalance_WorldScript();
    new AutoBalance_PlayerScript();
    new AutoBalance_UnitScript();
    new AutoBalance_AllCreatureScript();
    new AutoBalance_AllMapScript();
    new AutoBalance_CommandScript();
    new AutoBalance_GlobalScript();
    new AutoBalance_GroupScript();
}
