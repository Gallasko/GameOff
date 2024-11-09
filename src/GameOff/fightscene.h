#pragma once

#include "ECS/system.h"

#include "Scene/scenemanager.h"

namespace pg
{
    // ElementLess (Neutral) = 0, Water, Earth, Air, Fire, Light, Dark
    inline constexpr size_t NbElements = 7;

    struct Spell
    {
        float baseDmg;
    };

    struct Character
    {
        std::string name = "Unknown";

        float health = 100;

        float physicalAttack = 10;
        float magicalAttack = 10;

        float physicalDefense = 1;
        float magicalDefense = 1;

        float speed = 10;

        // In percentage
        float critChance = 5;
        float critDamage = 150;
        float evasionRate = 1;

        float elementalRes[NbElements] = {0.0f};

        std::vector<Spell> spells = {};
    };

    struct EnemyHit
    {
        EnemyHit(size_t id, float damage) : id(id), damage(damage) {}

        size_t id;
        
        float damage;
    };

    struct FightSystemUpdate {};

    struct FightSystem : public System<Listener<EnemyHit>, StoragePolicy>
    {
        virtual void onEvent(const EnemyHit& event) override;

        std::vector<Character> enemies;
        std::vector<Character> players;
    };

    struct FightScene : public Scene
    {
        virtual void init() override;

        FightSystem *fightSys;

        std::unordered_map<std::string, std::vector<EntityRef>> uiElements;
    };
}