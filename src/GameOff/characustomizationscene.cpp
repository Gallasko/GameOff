#include "characustomizationscene.h"

#include "UI/ttftext.h" 

#include "UI/textinput.h"

namespace pg
{
    void PlayerCharacter::onCreation(EntityRef entity)
    {
        auto ecsRef = entity->world();

        auto sys = ecsRef->getSystem<PlayerHandlingSystem>();

        character.id = sys->lastGivenId++;
    }

    struct CreateNewPlayerButtonPressed {};

    void PlayerCustomizationScene::init()
    {
        auto createNewPlayer = makeTTFText(this, 10, 10, "res/font/Inter/static/Inter_28pt-Light.ttf", "Create new player", 0.6);

        attach<MouseLeftClickComponent>(createNewPlayer.entity, makeCallable<CreateNewPlayerButtonPressed>());

        listenToEvent<CreateNewPlayerButtonPressed>([this](const CreateNewPlayerButtonPressed&) {
            auto player = createEntity();

            attach<PlayerCharacter>(player);

            ecsRef->sendEvent(NewPlayerCreated{player});

            // newCharacterCreated = true;
            // newlyCreatedCharacter = player;
        });

        listenToEvent<NewPlayerCreated>([this](const NewPlayerCreated& event) {
            auto player = event.entity.get<PlayerCharacter>();
            
            auto ttf = makeTTFText(this, 0, 0, "res/font/Inter/static/Inter_28pt-Light.ttf", player->character.name, 0.4);
            auto ttfUi = ttf.get<UiComponent>();
            ttfUi->setVisibility(false);

            attach<MouseLeftClickComponent>(ttf.entity, makeCallable<CharacterLeftClicked>(&player->character));

            characterList->addEntity(ttfUi);

            ttfTextIdToCharacter.emplace(ttf.entity.id, &player->character);
        });

        listenToEvent<CharacterLeftClicked>([this](const CharacterLeftClicked& event) {
            currentPlayer = event.chara;

            LOG_INFO("CharacterLeftClicked", "Current player id: " << currentPlayer->id);

            characterName.get<TTFText>()->setText(currentPlayer->name);
            characterName.get<TextInputComponent>()->text = currentPlayer->name;

            characterName.get<UiComponent>()->setVisibility(true);

            showStat();
        });

        listenToStandardEvent("CharaNameChange", [this](const StandardEvent& event) {
            auto returnText = event.values.at("return").toString();

            LOG_INFO("CharaNameChange", "Name changed to: " << returnText);

            LOG_INFO("CharacterLeftClicked", "Current player id: " << currentPlayer->id);

            currentPlayer->name = returnText;

            updateCharacterList();
        });

        auto listView = makeListView(this, 0, 150, 300, 120);

        characterList = listView.get<ListView>();

        auto charaName = makeTTFTextInput(this, 300.0f, 0.0f, StandardEvent("CharaNameChange"), "res/font/Inter/static/Inter_28pt-Light.ttf", "Character 1", 0.7);

        characterName = charaName.entity;

        charaName.get<UiComponent>()->setVisibility(false);
        
        charaName.get<TextInputComponent>()->clearTextAfterEnter = false;

        makeStatUi();
    }

    void PlayerCustomizationScene::execute()
    {
        // if (newCharacterCreated)
        // {
        //     ecsRef->sendEvent(NewPlayerCreated{newlyCreatedCharacter});
        //     newCharacterCreated = false;
        // }
    }

    void PlayerCustomizationScene::updateCharacterList()
    {
        LOG_INFO("CharaNameChange", "Updating character list...");

        for (auto& character : characterList->entities)
        {
            auto entity = ecsRef->getEntity(character.entityId);

            if (entity != nullptr and entity->has<TTFText>())
            {
                auto chara = ttfTextIdToCharacter.at(character.entityId);
                entity->get<TTFText>()->setText(chara->name);
            }
        }
    }

    void PlayerCustomizationScene::makeStatUi()
    {
        std::vector<std::string> statToDisplay = {"HP", "ATK", "MATK", "DEF", "MDEF", "SPEED", "CC", "CD", "EVA"};

        float baseX = 400;
        float baseY = 75;

        for (std::string stat : statToDisplay)
        {
            auto ttf = makeTTFText(this, baseX, baseY, "res/font/Inter/static/Inter_28pt-Light.ttf", stat, 0.4);
            auto ttfUi = ttf.get<UiComponent>();
            ttfUi->setVisibility(false);

            auto ttfNb = makeTTFText(this, baseX + 125, baseY, "res/font/Inter/static/Inter_28pt-Light.ttf", "0", 0.4);
            auto ttfUi2 = ttfNb.get<UiComponent>();
            ttfUi2->setVisibility(false);

            characterStatUi[stat] = ttf.entity;
            characterStatUi[stat + "nb"] = ttfNb.entity;

            baseY += 35;
        }
    }

    void PlayerCustomizationScene::showStat()
    {
        characterStatUi["HPnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.health));
        characterStatUi["ATKnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.physicalAttack));
        characterStatUi["DEFnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.physicalDefense));
        characterStatUi["MATKnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.magicalAttack));
        characterStatUi["MDEFnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.magicalDefense));
        characterStatUi["SPEEDnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.speed));
        characterStatUi["CCnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.critChance));
        characterStatUi["CDnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.critDamage));
        characterStatUi["EVAnb"].get<TTFText>()->setText(std::to_string(currentPlayer->stat.evasionRate));

        for (auto entity : characterStatUi)
        {
            entity.second.get<UiComponent>()->setVisibility(true);
        }
    }

    void PlayerCustomizationScene::makeSkillTreeUi()
    {
        float baseX = 400;
        float baseY = 450;

        for (size_t i = 0; i < 3; i++)
        {
            auto ttf = makeTTFText(this, baseX, baseY, "res/font/Inter/static/Inter_28pt-Light.ttf", "None", 0.4);
            auto ttfUi = ttf.get<UiComponent>();
            ttfUi->setVisibility(false);

            skillTreeUi["skill" + std::to_string(i)] = ttf.entity;

            baseX += 90;
        }


    }
}