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

        bool selfOnly = false;

        size_t nbTargets = 1;
    };

    enum class TriggerType : uint8_t
    {
        TurnStart = 0,
        TurnEnd,
        AnyTurnStart,
        AnyTurnEnd,
        OnHit,
        OnDamageDealt,
        StatBoost
    };

    struct Passiv
    {
        TriggerType trigger;
        
        // -1 means permanent, once it reach 0, the passiv is removed
        size_t remainingTurns = -1;

        // 0 means that it is active everytime the trigger is triggered, any other value means that it need multiple triggers before activation
        size_t numberOfTriggerBeforeActivation = 0;
        size_t currentNbOfTriggerSinceLastActivation = 0;
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

    struct SelectedSpell
    {
        Spell spell;  
    };

    struct FightSystemUpdate {};

    struct FightSceneUpdate {};

    struct PlayerNextTurn
    {
        Character* chara;
    };

    struct FightSystem : public System<Listener<FightSceneUpdate>, Listener<EnemyHit>, StoragePolicy>
    {
        virtual void onEvent(const EnemyHit& event) override;
        virtual void onEvent(const FightSceneUpdate& event) override;

        Character* calculateNextPlayingCharacter();

        Character* findNextPlayingCharacter();

        std::vector<Character> characters;
    };

    struct FightScene : public Scene
    {
        virtual void init() override;

        virtual void startUp() override;

        FightSystem *fightSys;

        std::unordered_map<std::string, std::vector<EntityRef>> uiElements;

        Spell currentCastedSpell;

        CompRef<ListView> spellView;
    };
}