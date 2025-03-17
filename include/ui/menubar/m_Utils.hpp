#ifndef UTILS_MENU_HPP
#define UTILS_MENU_HPP

// C++
#include <filesystem>

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "WindowManager.hpp"

namespace MenuBar {
    void changeWindowVisibility(const std::filesystem::path& windowName, bool* isOpen);
    void renderStatus();
    void renderProgramName();
    void changeColorMap();
} // namespace MenuBar

#endif