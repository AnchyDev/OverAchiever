#include "OverAchiever.h"

void OverAchieverPlayerScript::OnLogin(Player* player)
{
    if (!player)
    {
        return;
    }

    auto points = GetAchievementPointsFromDB(player);
    UpdatePointsForPlayer(player, points);
}

void OverAchieverPlayerScript::OnUpdate(Player* player, uint32 p_time)
{
    if (!sConfigMgr->GetOption<bool>("OverAchiever.Enabled", false))
    {
        return;
    }

    auto checkFrequency = sConfigMgr->GetOption<uint32>("OverAchiever.RewardFrequencySeconds", 30);
    currentFrequencyMS += p_time;

    if ((currentFrequencyMS / 1000) < checkFrequency)
    {
        return;
    }

    currentFrequencyMS = 0;

    if (!player)
    {
        return;
    }

    if (player->isAFK())
    {
        return;
    }

    uint32 points = GetPointsForPlayer(player);
    float constant = 100;
    float multi = sConfigMgr->GetOption<float>("OverAchiever.RewardMultiplier", 0.1);
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

        do
        {
            Field* fields = qResult->Fetch();
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
    auto ptr = achievementPoints.find(guid);

    if (ptr == achievementPoints.end())
    {
        achievementPoints.emplace(guid, points);
    }
    else
    {
        ptr->second = points;
    }
}

uint32 OverAchieverPlayerScript::GetPointsForPlayer(Player* player)
{
    if (!player)
    {
        return 0;
    }

    auto guid = player->GetGUID().GetRawValue();
    auto ptr = achievementPoints.find(guid);

    if (ptr == achievementPoints.end())
    {
        return 0;
    }
    else
    {
        return ptr->second;
    }
}

void SC_AddOverAchieverScripts()
{
    new OverAchieverPlayerScript();
}
