#ifndef INITIAL_WINDOW_HPP
#define INITIAL_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class HomePage : public IWindow {
        public:
            explicit HomePage(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif
