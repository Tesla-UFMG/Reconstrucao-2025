#include "ui/windows/w_Reconstruction.hpp"

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) { this->title = "Reconstrução de Pista"; }

void Window::Reconstruction::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, flags);

        ImGui::End();
    }
}