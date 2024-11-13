#pragma once

#include "ECS/system.h"

#include "Scene/scenemanager.h"

namespace pg
{
    struct PlayerHandlingSystem : public System<StoragePolicy>
    {

    };

    struct PlayerCustomizationScene : public Scene
    {
        virtual void init() override;
    };
}