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

    void FightSystem::onEvent(const StartFight&)
    {
        for (auto& character : characters)
        {
            for (const auto& chara : characters)
            {
                if (character.id != chara.id)
                {
                    character.aggroMap[chara.id] = chara.stat.physicalAttack + chara.stat.magicalAttack;
                }
            }
        }

        calculateNextPlayingCharacter();
    }

    void FightSystem::onEvent(const EnemyNextTurn& event)
    {
        needToProcessEnemyNextTurn = true;

        currentPlayingCharacter = event.chara;
    }

    void FightSystem::onEvent(const PlayFightAnimationDone&)
    {
        calculateNextPlayingCharacter();
    }

    void FightSystem::onEvent(const SpellCasted& event)
    {
        spellToResolve = event;

        spellToBeResolved = true;
    }

    void FightSystem::execute()
    {
        if (needToProcessEnemyNextTurn)
        {
            processEnemyNextTurn(currentPlayingCharacter);

            ecsRef->sendEvent(FightSystemUpdate{});

            return;
        }

        if (not spellToBeResolved)
            return;

        LOG_INFO("Fight System", "Spell " << spellToResolve.spell->name << " casted on: ");

        for (auto id : spellToResolve.ids)
        {
            LOG_INFO("Fight System", "-> " << id->name);
            resolveSpell(spellToResolve.caster, id->id, spellToResolve.spell);
        }

        spellToBeResolved = false;

        ecsRef->sendEvent(FightSystemUpdate{});
    }

    void FightSystem::resolveSpell(size_t casterId, size_t receiverId, Spell* spell)
    {
        auto& caster = characters[casterId];
        auto& receiver = characters[receiverId];

        // Todo this is the heart of the fighting system

        float spellDamage = spell->baseDmg; 

        receiver.stat.health -= spellDamage;

        // Give more aggro to player who deal damage to the character
        receiver.aggroMap[casterId] += spellDamage;

        std::string message = caster.name + " dealt " + std::to_string(static_cast<int>(spellDamage)) + " to " + receiver.name;

        ecsRef->sendEvent(FightMessageEvent{message});

        if (receiver.stat.health <= 0.5f)
        {
            receiver.speedUnits = 0;
            receiver.playingStatus = PlayingStatus::Dead;

            message = receiver.name + " died !";

            ecsRef->sendEvent(FightMessageEvent{message});
        }

        ecsRef->sendEvent(PlayFightAnimation{receiverId, FightAnimationEffects::Hit});
    }

    void FightSystem::processEnemyNextTurn(Character *chara)
    {
        // Todo here need to resolve spell and enemy AI

        if (chara->spells.size() == 0)
        {
            LOG_ERROR("Fight Scene", "Character: " << chara->name << ", cannot cast any spell !");

            ecsRef->sendEvent(PlayFightAnimation{0, FightAnimationEffects::Nothing});
            
            return;
        }

        auto& spell = chara->spells[0];

        float currentTarget = 0;

        for (auto& aggro : chara->aggroMap)
        {
            auto& other = characters[aggro.first];

            if (other.type == CharacterType::Player)
            {
                resolveSpell(chara->id, aggro.first, &spell);

                ++currentTarget;
            }

            if (spell.nbTargets >= currentTarget)
            {
                break;
            }
        }

        // If the mob couldn't hit anyone we need to at least send one PlayFightAnimation to update the state machine
        if (currentTarget == 0)
            ecsRef->sendEvent(PlayFightAnimation{0, FightAnimationEffects::Nothing});

        needToProcessEnemyNextTurn = false;
    }

    void FightSystem::addCharacter(Character character)
    {
        character.id = characters.size();

        // Set up the base states of the spell at startup
        for (auto& spell : character.spells)
        {
            spell.numberOfTurnsSinceLastUsed = spell.baseCooldown;
        }

        characters.push_back(character);
    }

    void FightSystem::calculateNextPlayingCharacter()
    {
        Character *nextPlayingCharacter = findNextPlayingCharacter();

        if (nextPlayingCharacter)
        {
            sendNextTurn(nextPlayingCharacter);
            return;
        }

        while (true)
        {
            for (auto& chara : characters)
            {
                if (chara.playingStatus != PlayingStatus::Dead)
                    chara.speedUnits += chara.stat.speed;
            }

            nextPlayingCharacter = findNextPlayingCharacter();

            if (nextPlayingCharacter)
            {
                sendNextTurn(nextPlayingCharacter);
                return;
            }
        }
    }

    void FightSystem::sendNextTurn(Character* character)
    {
        character->speedUnits -= SPEEDUNITTHRESHOLD + 1;
        LOG_INFO("Fight System", "Next playing character: " << character->name);

        tickDownPassives(character);

        if (character->type == CharacterType::Player)
        {
            ecsRef->sendEvent(PlayerNextTurn{character});
        }
        else if (character->type == CharacterType::Enemy)
        {
            ecsRef->sendEvent(EnemyNextTurn{character});
        }
    }

    void FightSystem::tickDownPassives(Character* character)
    {
        bool passiveWasRemoved = false;

        // Iterate through all the character passives to tick down their turn count
        for (int i = character->passives.size() - 1; i >= 0; --i)
        {
            auto& passive = character->passives[i];

            // If the passive is not infinite (remainingTurn == -1) tick down a turn from it
            if (passive.remainingTurns != -1)
            {
                --passive.remainingTurns;

                // After ticking it down if it reaches 0 delete it
                if (passive.remainingTurns <= 0)
                {
                    // In case of a character boost we need to undo the boost first before deleting it
                    if (passive.type == PassiveType::CharacterEffect and passive.trigger == TriggerType::StatBoost)
                    {
                        passive.removeFromCharacter(*character);
                    }

                    std::string message = character->name + " is no longer under: " + passive.name;

                    ecsRef->sendEvent(FightMessageEvent{message});

                    character->passives.erase(character->passives.begin() + i);

                    passiveWasRemoved = true;
                }
            }
        }

        if (passiveWasRemoved)
        {
            ecsRef->sendEvent(FightSystemUpdate{});
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

    struct SpellDoneClicked {};

    void FightScene::init()
    {
        fightSys = ecsRef->getSystem<FightSystem>();

        auto currentSelectedSpellTextUit = makeTTFText(this, 450, 550, "res/font/Inter/static/Inter_28pt-Light.ttf", "No selection", 0.4);

        currentSelectedSpellTextUi = currentSelectedSpellTextUit.entity;

        auto doneUit = makeTTFText(this, 600, 150, "res/font/Inter/static/Inter_28pt-Light.ttf", "Done", 0.6, {255.0f, 0.0f, 0.0f, 255.0f});

        doneUit.get<UiComponent>()->setVisibility(false);

        attach<MouseLeftClickComponent>(doneUit.entity, makeCallable<SpellDoneClicked>());

        doneUi = doneUit.entity;

        float xEnemyName = 80;

        auto& enemyNames = uiElements["Enemy Names"];
        auto& enemyHealths = uiElements["Enemy Health"];

        auto& playerNames = uiElements["Player Names"];
        auto& playerHealths = uiElements["Player Health"];

        float xPlayerName = 80; 

        size_t j = 0, k = 0;

        for (size_t i = 0; i < fightSys->characters.size(); ++i)
        {
            auto& character = fightSys->characters[i];

            if (character.type == CharacterType::Enemy)
            {
                auto enemyText = makeTTFText(this, xEnemyName, 20, "res/font/Inter/static/Inter_28pt-Light.ttf", character.name, 0.4);

                attach<MouseLeftClickComponent>(enemyText.entity, makeCallable<CharacterLeftClicked>(&character));

                enemyNames.push_back(enemyText.entity);

                auto enemyHealth = makeTTFText(this, xEnemyName, 50, "res/font/Inter/static/Inter_28pt-Light.ttf", std::to_string(character.stat.health), 0.4);

                enemyHealths.push_back(enemyHealth.entity);

                xEnemyName += 100;

                ++j;
            }
            else if (character.type == CharacterType::Player)
            {
                auto playerText = makeTTFText(this, xPlayerName, 120, "res/font/Inter/static/Inter_28pt-Light.ttf", character.name, 0.4);

                attach<MouseLeftClickComponent>(playerText.entity, makeCallable<CharacterLeftClicked>(&character));

                playerNames.push_back(playerText.entity);

                auto playerHealth = makeTTFText(this, xPlayerName, 170, "res/font/Inter/static/Inter_28pt-Light.ttf", std::to_string(character.stat.health), 0.4);

                playerHealths.push_back(playerHealth.entity);

                xPlayerName += 100;

                ++k;
            }
            
        }

        auto listView = makeListView(this, 100, 250, 300, 120);

        spellView = listView.get<ListView>();

        auto listView2 = makeListView(this, 620, 120, 300, 400);

        logView = listView2.get<ListView>();

        logView->stickToBottom = true;

        listenToEvent<OnMouseClick>([this](const OnMouseClick& event) {
            if (event.button == SDL_BUTTON_RIGHT)
            {
                ecsRef->sendEvent(PlayFightAnimationDone{});
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
                    enemyHealths[j].get<TTFText>()->setText(std::to_string(chara.stat.health));

                    LOG_INFO("Fight Scene", "Health: " << enemyHealths[j].get<TTFText>()->text);

                    ++j;
                }
                else if (chara.type == CharacterType::Player)
                {
                    playerHealths[k].get<TTFText>()->setText(std::to_string(chara.stat.health));

                    LOG_INFO("Fight Scene", "Health: " << playerHealths[k].get<TTFText>()->text);

                    ++k;
                }
            }
        });

        listenToEvent<EnemyNextTurn>([this](const EnemyNextTurn& event) {
            LOG_INFO("Fight Scene", "Current enemy turn: " << event.chara->name);

            writeInLog("Starting turn of " + event.chara->name);
        });

        listenToEvent<PlayerNextTurn>([this](const PlayerNextTurn& event) {
            LOG_INFO("Fight Scene", "Current player turn: " << event.chara->name);

            writeInLog("Starting turn of " + event.chara->name);

            if (event.chara->type == CharacterType::Player)
            {
                inPlayableTurn = true;
            }
            else if (event.chara->type != CharacterType::Player)
            {
                return;
            }

            inTargetSelection = false;
   
            currentPlayerTurn = event.chara->id;

            spellView->clear();

            for (auto& spell : event.chara->spells)
            {
                if (spell.numberOfTurnsSinceLastUsed < spell.baseCooldown)
                {
                    ++spell.numberOfTurnsSinceLastUsed;
                }

                LOG_INFO("Fight Scene", "Spell: " << spell.name);
                auto sp = makeTTFText(this, 0, 0, "res/font/Inter/static/Inter_28pt-Light.ttf", spell.name, 0.4);

                // The spell can be cast
                if (spell.numberOfTurnsSinceLastUsed >= spell.baseCooldown)
                {
                    attach<MouseLeftClickComponent>(sp.entity, makeCallable<SelectedSpell>(&spell));
                }
                else
                {
                    sp.get<TTFText>()->colors = {255.0f, 0.0f, 0.0f, 255.0f};
                }

                auto ui = sp.get<UiComponent>();

                ui->setVisibility(false);

                spellView->addEntity(ui);
            }
        });

        listenToEvent<SelectedSpell>([this](const SelectedSpell& event) {
            currentCastedSpell = event.spell;

            currentSelectedSpellTextUi.get<TTFText>()->setText(event.spell->name);

            selectedTarget.clear();

            inTargetSelection = true;
        });

        listenToEvent<CharacterLeftClicked>([this](const CharacterLeftClicked& event) {
            if (not inPlayableTurn)
                return;

            if (inTargetSelection)
            {
                auto character = event.chara;

                const auto& id = character->id;

                if (currentCastedSpell->selfOnly and id != currentPlayerTurn)
                {
                    LOG_INFO("Fight Scene", "Cannot target someone else with a self only spell");
                    return;
                }

                if (not currentCastedSpell->canTargetSameCharacterMultipleTimes)
                {
                    const auto& it = std::find_if(selectedTarget.begin(), selectedTarget.end(), [id](Character* chara) { return chara->id == id; });

                    if (it != selectedTarget.end())
                    {
                        LOG_INFO("Fight Scene", "Cannot target someone multiple times with this spell");
                        return;
                    }
                }

                selectedTarget.push_back(character);

                if (selectedTarget.size() >= currentCastedSpell->nbTargets)
                {
                    castSpell();
                }
                else
                {
                    doneUi.get<UiComponent>()->setVisibility(true);
                }
            }
        });

        listenToEvent<PlayFightAnimation>([this](const PlayFightAnimation& event) {
            animationToDo.push_back(event);
        });

        listenToEvent<FightMessageEvent>([this](const FightMessageEvent& event) {
            writeInLog(event.message);
        });

        listenToEvent<SpellDoneClicked>([this](const SpellDoneClicked&) {
            if (not inPlayableTurn)
                return;

            castSpell();
        });
    }

    void FightScene::startUp()
    {
        ecsRef->sendEvent(StartFight{});
    }

    void FightScene::execute()
    {
        if (animationToDo.size() == 0)
            return;

        for (auto anim : animationToDo)
        {
            //Run animation
        }

        animationToDo.clear();

        ecsRef->sendEvent(PlayFightAnimationDone{});
    }

    void FightScene::castSpell()
    {
        currentCastedSpell->numberOfTurnsSinceLastUsed = 0;

        doneUi.get<UiComponent>()->setVisibility(false);

        inPlayableTurn = false;
        ecsRef->sendEvent(SpellCasted{currentPlayerTurn, selectedTarget, currentCastedSpell});
    }

    void FightScene::writeInLog(const std::string& message)
    {
        auto playerTurnText = makeTTFText(this, 0, 0, "res/font/Inter/static/Inter_28pt-Light.ttf", message, 0.4);

        auto ui = playerTurnText.get<UiComponent>();

        ui->setVisibility(false);

        logView->addEntity(ui);
    }
}