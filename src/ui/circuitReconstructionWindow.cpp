#include "ui/circuitReconstructionWindow.hpp"

void Window::circuitReconstruction(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Reconstrução de Pista", isOpen, flags);
        ImGui::End();
    }
}