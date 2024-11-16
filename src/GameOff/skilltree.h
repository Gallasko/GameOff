#pragma once

#include <string>

namespace pg
{
    constexpr static size_t MAXLEVEL = 100;

    struct LevelProgression
    {

    };

    struct StatIncrease
    {

    };

    struct LevelIncrease
    {
        StatIncrease stats;
    };

    struct SkillTree
    {
        std::string name;

        size_t currentLevel = 0;

        size_t maxLevel = 0;

        size_t currentXp = 0;

        LevelProgression requiredXpForNextLevel;

        LevelIncrease levelGains[MAXLEVEL];
    };
}