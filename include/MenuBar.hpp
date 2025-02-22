#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

// C++
#include <fstream>
#include <sstream>
#include <string>

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "Pages.hpp"
#include "SDLWrapper.hpp"

namespace MenuBar {
    void windowsTab();
    void layoutTab();
    void configurationTab();
    void helpTab();
    void openOrCloseWindow(std::string windowName, bool* isOpen);

    void saveLayout();
    void loadLayout();

    void renderCurrentTime();
    void renderProgramName();
    void render();

} // namespace MenuBar

#endif // MENU_BAR