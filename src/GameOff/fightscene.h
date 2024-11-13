#pragma once

#include "ECS/system.h"

#include "UI/listview.h"

#include "Scene/scenemanager.h"

#include "character.h"
namespace pg
{
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
        SelectedSpell(Spell *spell) : spell(spell) {}

        Spell *spell;  
    };

    struct FightSystemUpdate {};

    struct StartFight {};

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
        Nothing = 0, // Used only to advance the state machine (when no spell is casted or the enemy couldn't target anyone !)
        Hit,
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

    struct FightSystem : public System<Listener<StartFight>, Listener<SpellCasted>, Listener<PlayFightAnimationDone>, Listener<EnemyNextTurn>>
    {
        virtual void onEvent(const StartFight& event) override;
        virtual void onEvent(const EnemyNextTurn& event) override;
        virtual void onEvent(const PlayFightAnimationDone& event) override;
        virtual void onEvent(const SpellCasted& event) override;

        virtual void execute() override;

        void resolveSpell(size_t casterId, size_t receiverId, Spell* spell);

        void processEnemyNextTurn(Character *chara);

        void addCharacter(Character character);

        void calculateNextPlayingCharacter();

        void sendNextTurn(Character* character);

        void tickDownPassives(Character* character);

        Character* findNextPlayingCharacter();

        Character* currentPlayingCharacter = nullptr;

        std::vector<Character> characters;

        bool needToProcessEnemyNextTurn = false;

        bool spellToBeResolved = false;
        SpellCasted spellToResolve; 
    };

    struct FightMessageEvent
    {
        FightMessageEvent(const std::string& message) : message(message) {}

        std::string message;
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

        void castSpell();

        void writeInLog(const std::string& message);

        FightSystem *fightSys;

        std::unordered_map<std::string, std::vector<EntityRef>> uiElements;

        Spell *currentCastedSpell = nullptr;

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