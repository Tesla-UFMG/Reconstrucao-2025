#include "ui/videoWindow.hpp"

void Window::video(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Vídeo", isOpen, flags);
        ImGui::End();
    }
}