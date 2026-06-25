#pragma once

#include "ftxui_all.hpp"

#include <functional>
#include <string>

namespace poker::client::ui_theme {

ftxui::Color panel_background();
ftxui::Color card_background();
ftxui::Color red_suit();
ftxui::Color black_suit();

ftxui::InputOption input_option();
ftxui::ButtonOption button_option();

ftxui::Component make_button(std::string* label, std::function<void()> on_click);
ftxui::Component make_input(std::string* value, std::string placeholder);
ftxui::Component make_password_input(std::string* value, std::string placeholder, ftxui::Ref<bool> password_mode);

} // namespace poker::client::ui_theme
