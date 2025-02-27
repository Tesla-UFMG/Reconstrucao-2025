#include "ui/PlotWindow.hpp"

void Window::Plot(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Plot", isOpen, flags);
        ImGui::End();
    }
}
