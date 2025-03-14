#include "ui/windows/w_Demo.hpp"

Window::ImGuiDemo::ImGuiDemo(bool* isOpen) : IWindow(isOpen) {}

void Window::ImGuiDemo::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::ShowDemoWindow(this->isOpen);
    }
}

Window::ImPlotDemo::ImPlotDemo(bool* isOpen) : IWindow(isOpen) {}

void Window::ImPlotDemo::render() {
    if (this->isOpen && *this->isOpen) {
        ImPlot::ShowDemoWindow(this->isOpen);
    }
}
