#include "ui/plotWindow.hpp"

void Window::plot(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Plot", isOpen, flags);
        ImGui::End();
    }
}
