#ifndef DEMO_WINDOW_HPP
#define DEMO_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class ImGuiDemo : public IWindow {
        public:
            explicit ImGuiDemo(bool* isOpen = nullptr);
            virtual void render() override;
    };

    class ImPlotDemo : public IWindow {
        public:
            explicit ImPlotDemo(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif