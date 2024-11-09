#include "fightscene.h"

#include "Systems/oneventcomponent.h"

#include "UI/ttftext.h" 

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <SDL_opengles2.h>
// #include <SDL_opengl_glext.h>
#include <GLES2/gl2.h>
// #include <GLFW/glfw3.h>
#else
#ifdef __linux__
#include <SDL2/SDL.h>
#elif _WIN32
#include <SDL.h>
#endif
#include <GL/gl.h>
#endif

namespace pg
{
    void FightSystem::onEvent(const EnemyHit& event)
    {
        if (enemies.size() <= event.id)
        {
            LOG_ERROR("Fight System", "Wrong id received");
            return;
        }

        enemies[event.id].health -= event.damage;

        LOG_INFO("Fight System", "Remaining hp [" << event.id << "] is: " << enemies[event.id].health);

        ecsRef->sendEvent(FightSystemUpdate{});
    }

    void FightScene::init()
    {
        fightSys = ecsRef->getSystem<FightSystem>();

        float xEnemyName = 80;

        auto& enemyNames = uiElements["Enemy Names"];
        auto& enemyHealths = uiElements["Enemy Health"];

        size_t i = 0;

        for (auto enemy : fightSys->enemies)
        {
            auto enemyText = makeTTFText(this, xEnemyName, 20, "res/font/Inter/static/Inter_28pt-Light.ttf", enemy.name, 0.4);

            attach<MouseLeftClickComponent>(enemyText.entity, makeCallable<EnemyHit>(i, 5.0f));

            enemyNames.push_back(enemyText.entity);

            auto enemyHealth = makeTTFText(this, xEnemyName, 50, "res/font/Inter/static/Inter_28pt-Light.ttf", std::to_string(static_cast<int>(enemy.health)), 0.4);

            enemyHealths.push_back(enemyHealth.entity);

            xEnemyName += 100;

            ++i;
        }

        float xPlayerName = 80; 

        for (auto player : fightSys->players)
        {
            auto playerText = makeTTFText(this, xPlayerName, 120, "res/font/Inter/static/Inter_28pt-Light.ttf", player.name, 0.4);

            xPlayerName += 100;
        }

        listenToEvent<FightSystemUpdate>([this](const FightSystemUpdate&) {
            auto& enemyHealths = uiElements["Enemy Health"];

            for (size_t i = 0; i < fightSys->enemies.size(); ++i)
            {
                enemyHealths[i].get<TTFText>()->setText(std::to_string(static_cast<int>(fightSys->enemies[i].health)));

                LOG_INFO("Fight Scene", "Health: " << enemyHealths[i].get<TTFText>()->text);
            }
        });

    }
}