#ifndef WHEEL_WINDOW_HPP
#define WHEEL_WINDOW_HPP

// Project
#include "AssetManager.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/iWindow.hpp"

#define VOLANTE_PATH "assets/volantetesla.png"

namespace Window {

    void WheelControl(bool* isOpen);
} // namespace Window

#endif