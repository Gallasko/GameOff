#pragma once

#include "ECS/system.h"

#include "UI/listview.h"

#include "Scene/scenemanager.h"

namespace pg
{
    // ElementLess (Neutral) = 0, Water, Earth, Air, Fire, Light, Dark
    inline constexpr size_t NbElements = 7;

    enum class Element : uint8_t
    {
        ElementLess = 0,
        Water,
        Earth,
        Air,
        Fire,
        Light,
        Dark
    };

    enum class DamageType : uint8_t
    {
        Physical = 0,
        Magical
    };

    struct Spell
    {
        std::string name = "Unknown";

        float baseDmg = 1;

        Element elementType = Element::ElementLess;
        DamageType damageType = DamageType::Physical;
    };

    enum class CharacterType : uint8_t
    {
        Player = 0,
        Enemy
    };

    struct Character
    {
        std::string name = "Unknown";

        CharacterType type = CharacterType::Player;

        float health = 100;

        float physicalAttack = 10;
        float magicalAttack = 10;

        float physicalDefense = 1;
        float magicalDefense = 1;

        float speed = 100;

        // In percentage
        float critChance = 5;
        float critDamage = 150;
        float evasionRate = 1;

        float elementalRes[NbElements] = {0.0f};

        float speedUnits = 0;

        std::vector<Spell> spells = {};
    };

    struct EnemyHit
    {
        EnemyHit(size_t id, Spell *spell) : id(id), spell(spell) {}

        size_t id;
        
        Spell *spell;
    };

    struct FightSystemUpdate {};

    struct FightSceneUpdate {};

    struct FightSystem : public System<Listener<FightSceneUpdate>, Listener<EnemyHit>, StoragePolicy>
    {
        virtual void onEvent(const EnemyHit& event) override;
        virtual void onEvent(const FightSceneUpdate& event) override;

        void calculateNextPlayingCharacter();

        Character* findNextPlayingCharacter();

        std::vector<Character> characters;
    };

    struct FightScene : public Scene
    {
        virtual void init() override;

        FightSystem *fightSys;

        std::unordered_map<std::string, std::vector<EntityRef>> uiElements;

        Spell currentCastedSpell;

        CompRef<ListView> spellView;
    };
}