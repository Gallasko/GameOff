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

        ecsRef->sendEvent(NewPlayerCreated{this});
    }

    struct CreateNewPlayerButtonPressed {};

    void PlayerCustomizationScene::init()
    {
        auto createNewPlayer = makeTTFText(this, 10, 10, "res/font/Inter/static/Inter_28pt-Light.ttf", "Create new player", 0.6);

        attach<MouseLeftClickComponent>(createNewPlayer.entity, makeCallable<CreateNewPlayerButtonPressed>());

        listenToEvent<CreateNewPlayerButtonPressed>([this](const CreateNewPlayerButtonPressed&) {
            auto player = createEntity();

            attach<PlayerCharacter>(player);
        });

        listenToEvent<NewPlayerCreated>([this](const NewPlayerCreated& event) {
            auto player = event.character;
            
            auto ttf = makeTTFText(this, 0, 0, "res/font/Inter/static/Inter_28pt-Light.ttf", player->character.name, 0.4);
            auto ttfUi = ttf.get<UiComponent>();
            ttfUi->setVisibility(false);

            attach<MouseLeftClickComponent>(ttf.entity, makeCallable<CharacterLeftClicked>(&player->character));

            characterList->addEntity(ttfUi);

            characterListPosToId.push_back(player->character.id);
        });

        listenToStandardEvent("CharaNameChange", [this](const StandardEvent& event) {
            auto returnText = event.values.at("return").toString();

            LOG_INFO("CharaNameChange", "Name changed to: " << returnText);
        });

        auto listView = makeListView(ecsRef, 100, 250, 300, 120);

        characterList = listView.get<ListView>();

        auto charaName = makeTTFTextInput(this, 300.0f, 0.0f, StandardEvent("CharaNameChange"), "res/font/Inter/static/Inter_28pt-Light.ttf", "Character 1");
    }
}