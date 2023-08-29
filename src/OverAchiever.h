#ifndef MODULE_OVERACHIEVER_H
#define MODULE_OVERACHIEVER_H

#include "Config.h"
#include "Chat.h"
#include "CharacterDatabase.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <unordered_map>
#include <vector>

class OverAchieverPlayerScript : PlayerScript {
public:
    OverAchieverPlayerScript() : PlayerScript("OverAchieverPlayerScript")
    {
        currentFrequencyMS = 0;
    }

private:
    void OnLogin(Player* /*player*/) override;
    void OnUpdate(Player* /*player*/, uint32 /*p_time*/) override;

    uint32 GetAchievementPointsFromDB(Player* /*player*/);
    void UpdatePointsForPlayer(Player* /*player*/, uint32 /*points*/);
    uint32 GetPointsForPlayer(Player* /*player*/);

    uint32 currentFrequencyMS;
    std::unordered_map<uint64, uint32> achievementPoints;
};

#endif // MODULE_OVERACHIEVER_H
