#pragma once

#include "ECS/system.h"

#include "Scene/scenemanager.h"

namespace pg
{
    struct Spell
    {
        float baseDmg;
    };

    struct Enemy
    {
        std::string name;

        float health;

        std::vector<Spell> spells;
    };

    struct Player
    {
        std::string name;

        float health;

        std::vector<Spell> spells;
    };

    struct FightSystem : public System<StoragePolicy>
    {
        std::vector<Enemy> enemies;
        std::vector<Player> players;
    };

    struct FightScene : public Scene
    {
        virtual void init() override;


        FightSystem *fightSys;
    };
}