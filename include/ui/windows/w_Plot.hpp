#ifndef PLOT_WINDOW_HPP
#define PLOT_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Plot : public IWindow {
        public:
            explicit Plot(bool* isOpen = nullptr);
            void         MenuBar();
            virtual void render() override;
    };

} // namespace Window

#endif