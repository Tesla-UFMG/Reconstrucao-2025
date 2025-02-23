#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

// C++
#include <fstream>
#include <sstream>
#include <string>

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "Window.hpp"
#include "SDLWrapper.hpp"

namespace MenuBar {
    void showWindowVisibility(const std::string& windowName, bool* windowVisibility);
    void windowsTab();
    void configurationTab();
    void helpTab();

    void renderCurrentTime();
    void renderProgramName();
    void render();

} // namespace MenuBar

#endif // MENU_BAR