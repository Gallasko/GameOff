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
    namespace
    {
        constexpr float SPEEDUNITTHRESHOLD = 999;
    }

    void FightSystem::onEvent(const EnemyHit& event)
    {
        if (characters.size() <= event.id)
        {
            LOG_ERROR("Fight System", "Wrong id received");
            return;
        }

        characters[event.id].health -= event.spell->baseDmg;

        LOG_INFO("Fight System", "Remaining hp [" << event.id << "] is: " << characters[event.id].health);

        ecsRef->sendEvent(FightSystemUpdate{});
    }

    void FightSystem::onEvent(const FightSceneUpdate&)
    {
        auto chara = calculateNextPlayingCharacter();

        if (chara->type == CharacterType::Player)
        {
            ecsRef->sendEvent(PlayerNextTurn{chara});
        }
    }

    Character* FightSystem::calculateNextPlayingCharacter()
    {
        Character *nextPlayingCharacter = findNextPlayingCharacter();

        if (nextPlayingCharacter)
        {
            nextPlayingCharacter->speedUnits -= SPEEDUNITTHRESHOLD + 1;
            LOG_INFO("Fight System", "Next playing character: " << nextPlayingCharacter->name);
            return nextPlayingCharacter;
        }

        while (true)
        {
            for (auto& chara : characters)
            {
                chara.speedUnits += chara.speed;
            }

            nextPlayingCharacter = findNextPlayingCharacter();

            if (nextPlayingCharacter)
            {
                nextPlayingCharacter->speedUnits -= SPEEDUNITTHRESHOLD + 1;
                LOG_INFO("Fight System", "Next playing character: " << nextPlayingCharacter->name);
                return nextPlayingCharacter;
            }
        }
    }

    Character* FightSystem::findNextPlayingCharacter()
    {
        Character* chara;
        bool characterTurn = false;

        // Character get a turn once their speed unit 
        float higherSpeedUnit = SPEEDUNITTHRESHOLD;

        for (auto& player : characters)
        {
            if (player.speedUnits > higherSpeedUnit)
            {
                LOG_INFO("Fight System", "Found character that need to play: " << player.name);
                characterTurn = true;
                higherSpeedUnit = player.speedUnits;

                chara = &player;
            }
        }

        if (characterTurn)
            return chara;
        else
            return nullptr;
    }

    void FightScene::init()
    {
        fightSys = ecsRef->getSystem<FightSystem>();

        auto currentSelectedSpellTextUit = makeTTFText(this, 450, 550, "res/font/Inter/static/Inter_28pt-Light.ttf", "No selection", 0.4);

        currentSelectedSpellTextUi = currentSelectedSpellTextUit.entity;

        float xEnemyName = 80;

        auto& enemyNames = uiElements["Enemy Names"];
        auto& enemyHealths = uiElements["Enemy Health"];

        auto& playerNames = uiElements["Player Names"];
        auto& playerHealths = uiElements["Player Health"];

        float xPlayerName = 80; 

        size_t j = 0, k = 0;

        for (size_t i = 0; i < fightSys->characters.size(); ++i)
        {
            auto& character =  fightSys->characters[i];

            if (character.type == CharacterType::Enemy)
            {
                auto enemyText = makeTTFText(this, xEnemyName, 20, "res/font/Inter/static/Inter_28pt-Light.ttf", character.name, 0.4);

                attach<MouseLeftClickComponent>(enemyText.entity, makeCallable<EnemyHit>(i, &currentCastedSpell));

                enemyNames.push_back(enemyText.entity);

                auto enemyHealth = makeTTFText(this, xEnemyName, 50, "res/font/Inter/static/Inter_28pt-Light.ttf", std::to_string(static_cast<int>(character.health)), 0.4);

                enemyHealths.push_back(enemyHealth.entity);

                xEnemyName += 100;

                ++j;
            }
            else if (character.type == CharacterType::Player)
            {
                auto playerText = makeTTFText(this, xPlayerName, 120, "res/font/Inter/static/Inter_28pt-Light.ttf", character.name, 0.4);

                playerNames.push_back(playerText.entity);

                auto playerHealth = makeTTFText(this, xPlayerName, 170, "res/font/Inter/static/Inter_28pt-Light.ttf", std::to_string(static_cast<int>(character.health)), 0.4);

                playerHealths.push_back(playerHealth.entity);

                xPlayerName += 100;

                ++k;
            }
            
        }

        auto listView = makeListView(ecsRef, 100, 250, 300, 300);

        spellView = listView.get<ListView>();

        listenToEvent<OnMouseClick>([this](const OnMouseClick& event) {
            if (event.button == SDL_BUTTON_RIGHT)
            {
                ecsRef->sendEvent(FightSceneUpdate{});
            }
        });

        listenToEvent<FightSystemUpdate>([this](const FightSystemUpdate&) {
            auto& enemyHealths = uiElements["Enemy Health"];
            auto& playerHealths = uiElements["Player Health"];

            size_t j = 0, k = 0;

            for (size_t i = 0; i < fightSys->characters.size(); ++i)
            {
                auto& chara = fightSys->characters[i];

                if (chara.type == CharacterType::Enemy)
                {
                    enemyHealths[j].get<TTFText>()->setText(std::to_string(static_cast<int>(chara.health)));

                    LOG_INFO("Fight Scene", "Health: " << enemyHealths[j].get<TTFText>()->text);

                    ++j;
                }
                else if (chara.type == CharacterType::Player)
                {
                    playerHealths[k].get<TTFText>()->setText(std::to_string(static_cast<int>(chara.health)));

                    LOG_INFO("Fight Scene", "Health: " << playerHealths[k].get<TTFText>()->text);

                    ++k;
                }
            }
        });

        listenToEvent<PlayerNextTurn>([this](const PlayerNextTurn& event) {
            LOG_INFO("Fight Scene", "Current player turn: " << event.chara->name);

            spellView->clear();

            for (auto& spell : event.chara->spells)
            {
                LOG_INFO("Fight Scene", "Spell: " << spell.name);
                auto sp = makeTTFText(this, 0, 0, "res/font/Inter/static/Inter_28pt-Light.ttf", spell.name, 0.4);

                attach<MouseLeftClickComponent>(sp.entity, makeCallable<SelectedSpell>(spell));

                auto ui = sp.get<UiComponent>();

                ui->setVisibility(false);

                spellView->addEntity(ui);
            }

        });

        listenToEvent<SelectedSpell>([this](const SelectedSpell& event) {
            currentCastedSpell = event.spell;

            currentSelectedSpellTextUi.get<TTFText>()->setText(event.spell.name);
        });
    }

    void FightScene::startUp()
    {
        ecsRef->sendEvent(FightSceneUpdate{});
    }
}