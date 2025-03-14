#ifndef WHEEL_WINDOW_HPP
#define WHEEL_WINDOW_HPP

// Project
#include "AssetManager.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

#define VOLANTE_PATH "assets/volantetesla.png"

namespace Window {
    class WheelControl : public IWindow {
        public:
            explicit WheelControl(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif