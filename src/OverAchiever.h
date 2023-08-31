#ifndef MODULE_OVERACHIEVER_H
#define MODULE_OVERACHIEVER_H

#include "Config.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "CharacterDatabase.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "GameTime.h"

#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

struct OverAchieverPlayerInfo {
    uint32 achievementPoints;
    std::vector<uint32> rewardIndexes;
};

class OverAchieverPlayerScript : PlayerScript {
public:
    OverAchieverPlayerScript() : PlayerScript("OverAchieverPlayerScript") {}

private:
    void OnLogin(Player* /*player*/) override;
    void OnUpdate(Player* /*player*/, uint32 /*p_time*/) override;

    uint32 GetAchievementPointsFromDB(Player* /*player*/);
    void UpdatePointsForPlayer(Player* /*player*/, uint32 /*points*/);
    uint32 GetPointsForPlayer(Player* /*player*/);

    bool HasRewardedIndex(Player* /*player*/, uint32 /*rewardIndex*/);
    void SetRewardedIndex(Player* /*player*/, uint32 /*rewardIndex*/);
    void ResetRewardedIndexes(Player* /*player*/);

    std::unordered_map<uint64, OverAchieverPlayerInfo> playerInfos;
};

class OverAchieverCommandScript : CommandScript {
public:
    OverAchieverCommandScript() : CommandScript("OverAchieverCommandScript") { }
private:
    ChatCommandTable GetCommands() const override;
    static bool HandleOADebugCommand(ChatHandler* handler);
};

#endif // MODULE_OVERACHIEVER_H
