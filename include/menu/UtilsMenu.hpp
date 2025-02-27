#ifndef UTILS_MENU_HPP
#define UTILS_MENU_HPP

// C++
#include <filesystem>

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "Window.hpp"

namespace Menu {
    void showWindowVisibility(const std::filesystem::path& windowName, bool* isOpen);
}

#endif