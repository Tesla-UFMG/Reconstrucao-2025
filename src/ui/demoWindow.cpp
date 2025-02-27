#include "ui/DemoWindow.hpp"

void Window::ImGuiDemo(bool* isOpen) {
    if (*isOpen) {
        ImGui::ShowDemoWindow(isOpen);
    }
}

void Window::ImPlotDemo(bool* isOpen) {
    if (*isOpen) {
        ImPlot::ShowDemoWindow(isOpen);
    }
}