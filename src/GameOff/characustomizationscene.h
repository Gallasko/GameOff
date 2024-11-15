#pragma once

#include "ECS/system.h"

#include "Scene/scenemanager.h"

#include "UI/listview.h"

#include "character.h"

namespace pg
{
    struct PlayerCharacter : public Ctor
    {
        Character character;

        virtual void onCreation(EntityRef entity) override;
    };

    struct NewPlayerCreated { PlayerCharacter *character; };

    struct PlayerHandlingSystem : public System<Own<PlayerCharacter>, StoragePolicy>
    {
        size_t lastGivenId = 0;
    };

    struct PlayerCustomizationScene : public Scene
    {
        virtual void init() override;

        CompRef<ListView> characterList;

        std::vector<size_t> characterListPosToId;
    };
}