#pragma once

#include "ECS/entitysystem.h"

#include "ECS/callable.h"
namespace pg 
{
    enum class UiComponentType
    {
        BUTTON,
        TEXTURE,
        TEXT,
        TTFTEXT,
        TEXTINPUT,
        LIST,
        PREFAB // Todo to implement !
    };

    class UiComponent;
    class Input;

namespace editor
{
    struct OpenFile {};

    struct SaveFile {};

    struct ShowContextMenu 
    {
        ShowContextMenu(const Input* const inputHandler) : inputHandler(inputHandler) {}

        const Input* const inputHandler;
    };

    struct HideContextMenu {};

    struct CreateElement { CreateElement(const UiComponentType& type) : type(type) {} UiComponentType type; };

    // Todo make the context menu a generic Ui element !
    struct ContextMenu : System<Listener<ShowContextMenu>, Listener<HideContextMenu>, Listener<CreateElement>, Listener<OpenFile>, Listener<SaveFile>, InitSys>
    {
        ContextMenu();
        ~ContextMenu();

        virtual void init() override;

        void setContextList(const std::string& text, CallablePtr callable);

        template<typename... Args>
        void setContextList(const std::string& text, CallablePtr callable, const std::string& nText, CallablePtr nCallable, Args... args)
        {
            addItemInContextMenu(text, callable);

            setContextList(nText, nCallable, args...);
        }

        void addItemInContextMenu(const std::string& text, CallablePtr callable);

        inline void clear() { components.clear(); }

        void hide();

        virtual void onEvent(const ShowContextMenu& event) override;
        virtual void onEvent(const HideContextMenu& event) override;
        virtual void onEvent(const CreateElement& event) override;
        virtual void onEvent(const OpenFile& event) override;
        virtual void onEvent(const SaveFile& event) override;

        virtual void execute() override;

        EntityRef parent;
        CompRef<UiComponent> parentUi;

        CompRef<UiComponent> backgroundC;

        std::vector<CompRef<UiComponent>> components;

        float currentX = 0.0f, currentY = 0.0f;

        std::queue<ShowContextMenu> showEventQueue;
        std::queue<HideContextMenu> hideEventQueue;
        std::queue<CreateElement> elementQueue;
    };
}
}