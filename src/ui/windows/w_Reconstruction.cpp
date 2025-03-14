#include "ui/windows/w_Reconstruction.hpp"

void Window::Reconstruction(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Reconstrução de Pista", isOpen, flags);
        ImGui::End();
    }
}