#include "OverAchiever.h"

void OverAchieverPlayerScript::OnLogin(Player* player)
{
    if (!player)
    {
        return;
    }

    ResetRewardedIndexes(player);
    auto points = GetAchievementPointsFromDB(player);
    UpdatePointsForPlayer(player, points);
}

void OverAchieverPlayerScript::OnUpdate(Player* player, uint32 /*p_time*/)
{
    if (!sConfigMgr->GetOption<bool>("OverAchiever.Enabled", false))
    {
        return;
    }

    if (!player)
    {
        return;
    }

    auto checkFrequencySeconds = sConfigMgr->GetOption<uint32>("OverAchiever.RewardFrequencySeconds", 3600);

    auto loginTime = player->m_logintime;
    auto currentTime = GameTime::GetGameTime().count();
    auto timePassed = currentTime - loginTime;

    if (timePassed < checkFrequencySeconds)
    {
        return;
    }

    uint32 rewardIndex = timePassed / checkFrequencySeconds;

    if ((timePassed % checkFrequencySeconds) != 0)
    {
        return;
    }

    if (HasRewardedIndex(player, rewardIndex))
    {
        return;
    }

    SetRewardedIndex(player, rewardIndex);

    // Skip this reward cycle, naughty.
    if (player->isAFK())
    {
        return;
    }

    uint32 points = GetPointsForPlayer(player);
    float constant = 100;
    float multi = sConfigMgr->GetOption<float>("OverAchiever.RewardMultiplier", 5);
    float amount = 1 + points / constant * multi;
    uint32 reward = sConfigMgr->GetOption<uint32>("OverAchiever.RewardId", 37711);

    player->AddItem(reward, amount);

    std::string message = sConfigMgr->GetOption<std::string>("OverAchiever.RewardMessage", "|cffffffffYou have received |cff00ff00{}|cffffffff token(s) for being active.|r");
    ChatHandler(player->GetSession()).SendSysMessage(Acore::StringFormatFmt(message, amount));
}

uint32 OverAchieverPlayerScript::GetAchievementPointsFromDB(Player* player)
{
    if (!player)
    {
        return 0;
    }

    try
    {
        std::vector<uint32> achievements;
        QueryResult qResult = CharacterDatabase.Query("SELECT achievement FROM character_achievement WHERE guid = {}", player->GetGUID().GetRawValue());

        if (!qResult ||
            qResult->GetRowCount() < 1)
        {
            return 0;
        }

        do
        {
            Field* fields = qResult->Fetch();
            if (fields->IsNull())
            {
                return 0;
            }

            achievements.push_back(fields[0].Get<uint32>());
        } while (qResult->NextRow());

        uint32 sum = 0;
        for (auto it = achievements.begin(); it != achievements.end(); ++it)
        {
            auto entry = sAchievementStore.LookupEntry(*it);
            sum += entry->points;
        }

        return sum;
    }
    catch (std::exception ex)
    {
        LOG_INFO("module", "Failed to load achievements from DB for player {}: {}", player->GetName(), ex.what());
        return 0;
    }

    return 0;
}

void OverAchieverPlayerScript::UpdatePointsForPlayer(Player* player, uint32 points)
{
    if (!player)
    {
        return;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto it = playerInfos.find(guid);

    if (it == playerInfos.end())
    {
        OverAchieverPlayerInfo pInfo;
        std::vector<uint32> rewards;

        pInfo.achievementPoints = points;
        pInfo.rewardIndexes = rewards;

        playerInfos.emplace(guid, pInfo);
    }
    else
    {
        it->second.achievementPoints = points;
    }
}

uint32 OverAchieverPlayerScript::GetPointsForPlayer(Player* player)
{
    if (!player)
    {
        return 0;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto it = playerInfos.find(guid);

    if (it == playerInfos.end())
    {
        return 0;
    }
    else
    {
        return it->second.achievementPoints;
    }
}

bool OverAchieverPlayerScript::HasRewardedIndex(Player* player, uint32 rewardIndex)
{
    if (!player)
    {
        return false;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto it = playerInfos.find(guid);

    if (it == playerInfos.end())
    {
        return false;
    }

    auto rewards = it->second.rewardIndexes;
    auto it2 = std::find(rewards.begin(), rewards.end(), rewardIndex);

    if (it2 == rewards.end())
    {
        return false;
    }

    return true;
}

void OverAchieverPlayerScript::SetRewardedIndex(Player* player, uint32 rewardIndex)
{
    if (!player)
    {
        return;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto it = playerInfos.find(guid);

    if (it == playerInfos.end())
    {
        return;
    }

    std::vector<uint32>* rewards = &it->second.rewardIndexes;

    for (auto it2 = rewards->begin(); it2 != rewards->end(); ++it2)
    {
        auto reward = *it2;
        if (reward == rewardIndex)
        {
            return;
        }
    }

    rewards->push_back(rewardIndex);
}

void OverAchieverPlayerScript::ResetRewardedIndexes(Player* player)
{
    if (!player)
    {
        return;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto it = playerInfos.find(guid);

    if (it == playerInfos.end())
    {
        return;
    }

    std::vector<uint32>* rewards = &it->second.rewardIndexes;
    if (!rewards)
    {
        return;
    }

    rewards->clear();
}

ChatCommandTable OverAchieverCommandScript::GetCommands() const
{
    static ChatCommandTable oaCommandTable =
    {
        { "debug", HandleOADebugCommand, SEC_ADMINISTRATOR, Console::No }
    };

    static ChatCommandTable commandTable =
    {
        { "oa", oaCommandTable }
    };

    return commandTable;
}

bool OverAchieverCommandScript::HandleOADebugCommand(ChatHandler* handler)
{
    if (!handler)
    {
        handler->SetSentErrorMessage(true);
        return false;
    }

    auto player = handler->GetPlayer();
    if (!player)
    {
        handler->SetSentErrorMessage(true);
        return false;
    }

    auto checkFrequencySeconds = sConfigMgr->GetOption<uint32>("OverAchiever.RewardFrequencySeconds", 3600);

    auto loginTime = player->m_logintime;
    auto currentTime = GameTime::GetGameTime().count();
    auto timePassed = currentTime - loginTime;

    uint32 rewardIndex = timePassed / checkFrequencySeconds;
    uint32 nextReward = checkFrequencySeconds - (timePassed % checkFrequencySeconds);

    handler->SendSysMessage(Acore::StringFormatFmt("[EPOCH] - Login: {}, Current: {}, Diff: {}", loginTime, currentTime, timePassed));
    handler->SendSysMessage(Acore::StringFormatFmt("[MISC] - IsAFK: {}, RewardIndex: {}, NextReward: {}", player->isAFK() ? "true" : "false", rewardIndex, nextReward));

    return true;
}

void SC_AddOverAchieverScripts()
{
    new OverAchieverPlayerScript();
    new OverAchieverCommandScript();
}
