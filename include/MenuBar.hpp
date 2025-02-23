#ifndef MENU_BAR_HPP
#define MENU_BAR_HPP

// C++
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "SDLWrapper.hpp"
#include "Window.hpp"

namespace MenuBar {
    void showWindowVisibility(const std::filesystem::path& windowName, bool* windowVisibility);
    void windowsTab();
    void configurationTab();
    void helpTab();

    void renderCurrentTime();
    void renderProgramName();
    void render();

} // namespace MenuBar

#endif // MENU_BAR