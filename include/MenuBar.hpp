#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

// C++
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "SDLWrapper.hpp"
#include "Window.hpp"

#include "menu/HelpMenu.hpp"
#include "menu/TeslaMenu.hpp"
#include "menu/WindowsMenu.hpp"
namespace MenuBar {
    void renderCurrentTime();
    void renderProgramName();
    void render();

} // namespace MenuBar

#endif // MENU_BAR