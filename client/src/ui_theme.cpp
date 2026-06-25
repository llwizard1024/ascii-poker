#include "ui_theme.h"

#include <functional>

namespace poker::client::ui_theme {

using namespace ftxui;

Color panel_background()
{
    return Color::RGB(28, 32, 42);
}

Color card_background()
{
    return Color::RGB(36, 40, 52);
}

Color red_suit()
{
    return Color::RGB(210, 110, 110);
}

Color black_suit()
{
    return Color::RGB(205, 215, 225);
}

InputOption input_option()
{
    InputOption option;
    option.transform = [](InputState state) {
        Element element = state.element;
        if (state.is_placeholder) {
            element |= dim;
        }
        if (state.focused) {
            element = element | color(Color::RGB(240, 210, 120)) | bgcolor(Color::RGB(45, 50, 62));
        } else if (state.hovered) {
            element |= color(Color::GrayLight);
        }
        return element;
    };
    return option;
}

ButtonOption button_option()
{
    ButtonOption option;
    option.transform = [](const EntryState& state) {
        Element element = text(state.label);
        if (state.focused) {
            element = element | color(Color::RGB(240, 210, 120)) | bold | bgcolor(Color::RGB(45, 50, 62));
        } else if (state.active) {
            element = element | color(Color::White);
        } else {
            element |= dim;
        }
        return element;
    };
    return option;
}

Component make_button(std::string* label, std::function<void()> on_click)
{
    return Button(label, std::move(on_click), button_option());
}

Component make_input(std::string* value, std::string placeholder)
{
    return Input(value, std::move(placeholder), input_option());
}

Component make_password_input(std::string* value, std::string placeholder, Ref<bool> password_mode)
{
    InputOption option = input_option();
    option.password = password_mode;
    return Input(value, std::move(placeholder), option);
}

} // namespace poker::client::ui_theme
