#ifndef PLOT_WINDOW_HPP
#define PLOT_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/iWindow.hpp"

namespace Window {

    namespace MenuBar {
        void Plot();
    }

    void Plot(bool* isOpen);
} // namespace Window

#endif