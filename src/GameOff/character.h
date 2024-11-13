#pragma once

#include <string>
#include <functional>
#include <map>

namespace pg
{
    // Type definitions
    struct Character;

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

        float baseManaCost = 0;

        size_t baseCooldown = 1;

        Element elementType = Element::ElementLess;
        DamageType damageType = DamageType::Physical;

        bool selfOnly = false;

        size_t nbTargets = 1;

        bool canTargetSameCharacterMultipleTimes = false;

        // In combat charac
        
        size_t numberOfTurnsSinceLastUsed = 0;
    };

    enum class TriggerType : uint8_t
    {
        TurnStart = 0,
        TurnEnd,
        OnHit,
        OnDamageDealt,
        StatBoost
    };

    enum class PassiveType : uint8_t
    {
        CharacterEffect = 0,
        SpellEffect,
        TurnEffect,
    };

    struct Passive
    {
        PassiveType type;

        TriggerType trigger;

        std::string name = "Passive"; 
        
        // -1 means permanent, once it reach 0, the passive is removed
        int32_t remainingTurns = -1;

        // 0 means that it is active everytime the trigger is triggered, any other value means that it need multiple triggers before activation
        size_t numberOfTriggerBeforeActivation = 0;
        size_t currentNbOfTriggerSinceLastActivation = 0;

        /** Flag indicating if this passive is hidden from view of the player */
        bool hidden = false;

        /** Keep track of the number of time this passive was activated */
        size_t nbSuccesfulActivation = 0;

        // Function to use and define when passiveType == CharacterEffect and trigger == StatBoost
        std::function<void(Character&)> applyOnCharacter;
        std::function<void(Character&)> removeFromCharacter;
    };

    enum class PlayerBoostType : uint8_t
    {
        Health = 0,
        PAtk,
        MAtk,
        PDef,
        MDef,
        Speed,
        CChance,
        CDamage,
        Evasion,
        Res
    };

    /**
     * @brief Factory function creating simple Player boost passive
     * 
     * @param type Type of player boost to apply
     * @param value Value of player boost to apply
     * @param duration Number of turn that the boost last (-1 to go infinite)
     * @param name Name of the boost
     * 
     * @return Passive A Passive that gives the "value" amount to a given character
     */
    Passive makeSimplePlayerBoostPassive(PlayerBoostType type, float value, int32_t duration = -1, std::string name = "PlayerBoost");

    enum class CharacterType : uint8_t
    {
        Player = 0,
        Enemy
    };

    enum class PlayingStatus : uint8_t
    {
        Alive = 0,
        Dead
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

        // Todo create addSpell also

        std::vector<Passive> passives = {};

        void addPassive(const Passive& passive);

        // In combat charac

        /** Map containing the aggro of all the character relative to this one.
         * Key: id of the character
         * Value: Value of the aggro of the said character
         * 
         * Aggro increase with all the damage dealt
         * Base aggro = character.physicalAttack + character.magicalAttack
         * 
         * Todo make sure that this map is ordered with the higher aggro first and the worst aggro last
        */
        std::map<size_t, float> aggroMap = {};

        PlayingStatus playingStatus = PlayingStatus::Alive;

        size_t id = 0;
    };

}