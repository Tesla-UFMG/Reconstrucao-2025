#ifndef TERMINAL_WINDOW_HPP
#define TERMINAL_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Terminal : public IWindow {
        public:
            explicit Terminal(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif