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

        bool canTargetSameCharacterMultipleTimes = false;
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

        size_t id = 0;
    };

    struct SpellCasted
    {
        SpellCasted() {}
        SpellCasted(size_t caster, std::vector<size_t> ids, Spell *spell) : caster(caster), ids(ids), spell(spell) {}

        size_t caster = 0;

        std::vector<size_t> ids = {0};
        
        Spell *spell = nullptr;
    };

    struct SelectedSpell
    {
        SelectedSpell(const Spell& spell) : spell(spell) {}

        Spell spell;  
    };

    struct FightSystemUpdate {};

    struct FightSceneUpdate {};

    struct PlayerNextTurn
    {
        Character* chara;
    };

    struct EnemyNextTurn
    {
        Character* chara;
    };

    enum class FightAnimationEffects : uint8_t
    {
        Hit = 0,
        Heal,
        StatusAilement,
        Counter,
        Dodge,
    };

    struct PlayFightAnimation
    {
        PlayFightAnimation(size_t id, const FightAnimationEffects& effect) : id(id), effect(effect) {}

        size_t id;

        FightAnimationEffects effect;
    };

    struct PlayFightAnimationDone {};

    struct FightSystem : public System<Listener<FightSceneUpdate>, Listener<SpellCasted>, Listener<PlayFightAnimationDone>, Listener<EnemyNextTurn>>
    {
        virtual void onEvent(const EnemyNextTurn& event) override;
        virtual void onEvent(const PlayFightAnimationDone& event) override;
        virtual void onEvent(const FightSceneUpdate& event) override;
        virtual void onEvent(const SpellCasted& event) override;

        virtual void execute() override;

        void resolveSpell(size_t casterId, size_t receiverId, Spell* spell);

        void addCharacter(Character character);

        Character* calculateNextPlayingCharacter();

        Character* findNextPlayingCharacter();

        std::vector<Character> characters;

        bool spellToBeResolved = false;
        SpellCasted spellToResolve; 
    };

    struct CharacterLeftClicked
    {
        CharacterLeftClicked(size_t id) : id(id) {}

        size_t id;
    };

    struct FightScene : public Scene
    {
        virtual void init() override;

        virtual void startUp() override;

        virtual void execute() override;

        void writeInLog(const std::string& message);

        FightSystem *fightSys;

        std::unordered_map<std::string, std::vector<EntityRef>> uiElements;

        Spell currentCastedSpell;

        CompRef<ListView> spellView;

        CompRef<ListView> logView;

        EntityRef currentSelectedSpellTextUi;

        EntityRef doneUi;

        size_t currentPlayerTurn = 0;

        bool inPlayableTurn = false;

        bool inTargetSelection = false;

        std::vector<size_t> selectedTarget;

        std::vector<PlayFightAnimation> animationToDo;
    };
}