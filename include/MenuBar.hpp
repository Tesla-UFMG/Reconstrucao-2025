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

#include "menu/m_Help.hpp"
#include "menu/m_Tesla.hpp"
#include "menu/m_Windows.hpp"

namespace MenuBar {
    void renderCurrentTime();
    void renderProgramName();
    void render();

} // namespace MenuBar

#endif // MENU_BAR