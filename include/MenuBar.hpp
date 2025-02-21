#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

// C++
#include <fstream>
#include <string>
#include <sstream>

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "Pages.hpp"
#include "Log.hpp"

namespace MenuBar {
    void WindowsTab();
    void ConfigurationTab();
    void HelpTab();

    void saveLayout();
    void loadLayout();

    void renderCurrentTime();
    void render();

} // App

#endif // MENU_BAR