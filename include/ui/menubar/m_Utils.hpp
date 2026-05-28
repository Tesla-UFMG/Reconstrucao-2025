#ifndef UTILS_MENU_HPP
#define UTILS_MENU_HPP

// C++
#include <filesystem>

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "Log.hpp"

namespace MenuBar {
    void changeWindowVisibility(const std::filesystem::path& windowName, bool* isOpen);
    void renderStatus();
    void renderProgramName();
    void changePlotColormap();
    void changeAppStyleTheme();
} // namespace MenuBar

#endif