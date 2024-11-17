#pragma once

#include "ECS/system.h"

#include "Scene/scenemanager.h"

#include "UI/listview.h"

#include "character.h"
#include "skilltree.h"

namespace pg
{
    struct PlayerCharacter : public Ctor
    {
        Character character;

        std::vector<SkillTree> learnedSkillTree = { NoneSkillTree{} };

        std::vector<SkillTree*> skillTreeInUse = { nullptr, nullptr, nullptr };

        // Todo add equipment

        virtual void onCreation(EntityRef entity) override;
    };

    struct NewPlayerCreated { EntityRef entity; };

    struct PlayerHandlingSystem : public System<Own<PlayerCharacter>, StoragePolicy>
    {
        size_t lastGivenId = 0;
    };

    enum class CurrentCharacterTab : uint8_t
    {
        Stat = 0,
        Equipment,
        Job,
    };

    struct PlayerCustomizationScene : public Scene
    {
        virtual void init() override;
        virtual void execute() override;

        void updateCharacterList();

        void makeStatUi();
        void showStat();

        void makeSkillTreeUi();
        void showSkillTree();

        bool newCharacterCreated = false;
        EntityRef newlyCreatedCharacter;

        CompRef<ListView> characterList;

        Character *currentPlayer;

        bool menuShown = false;

        CurrentCharacterTab tab = CurrentCharacterTab::Stat;

        std::unordered_map<_unique_id, Character*> ttfTextIdToCharacter;

        EntityRef characterName;

        std::unordered_map<std::string, EntityRef> characterStatUi;
        std::unordered_map<std::string, EntityRef> skillTreeUi;
    };
}