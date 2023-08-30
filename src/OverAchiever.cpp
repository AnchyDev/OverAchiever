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

void OverAchieverPlayerScript::OnUpdate(Player* player, uint32 p_time)
{
    if (!sConfigMgr->GetOption<bool>("OverAchiever.Enabled", false))
    {
        return;
    }

    if (!player)
    {
        return;
    }

    if (player->isAFK())
    {
        return;
    }

    auto checkFrequencySeconds = sConfigMgr->GetOption<uint32>("OverAchiever.RewardFrequencySeconds", 3600);

    auto loginTime = player->m_logintime;
    auto currentTime = GameTime::GetGameTime().count();
    auto timePassed = currentTime - loginTime;

    //LOG_INFO("module", "Epoch | Login Time: {}, CurrentTime: {}, Passed: {}", loginTime, currentTime, timePassed);

    if (timePassed < checkFrequencySeconds)
    {
        return;
    }

    uint32 rewardIndex = timePassed / checkFrequencySeconds;

    if ((timePassed % checkFrequencySeconds) != 0)
    {
        return;
    }

    //LOG_INFO("module", "Reward Index: {}", rewardIndex);
    if (HasRewardedIndex(player, rewardIndex))
    {
        //LOG_INFO("module", "Already rewarded for index {}.", rewardIndex);
        return;
    }

    SetRewardedIndex(player, rewardIndex);
    //LOG_INFO("module", "Saved reward for index {}.", rewardIndex);

    uint32 points = GetPointsForPlayer(player);
    float constant = 100;
    float multi = sConfigMgr->GetOption<float>("OverAchiever.RewardMultiplier", 5);
    float amount = 1 + points / constant * multi;
    uint32 reward = sConfigMgr->GetOption<uint32>("OverAchiever.RewardId", 37711);

    player->AddItem(reward, amount);
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

void SC_AddOverAchieverScripts()
{
    new OverAchieverPlayerScript();
}
