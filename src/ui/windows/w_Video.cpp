#include "ui/windows/w_Video.hpp"

void Window::Video(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Vídeo", isOpen, flags);
        ImGui::End();
    }
}