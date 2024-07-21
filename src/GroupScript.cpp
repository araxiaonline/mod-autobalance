
#include "ScriptMgr.h"
#include "Player.h"

// this handles updating custom group difficulties used in auto balancing mobs and
// scripts that enable buffs on mobs randomly
class AutoBalance_GroupScript : public GroupScript
{
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

void AddGroupScripts()
{
    new AutoBalance_GroupScript();
}