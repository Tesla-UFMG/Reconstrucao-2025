#include "ui/windows/w_Video.hpp"

Window::Video::Video(bool* isOpen) : IWindow(isOpen) {
    this->title = "Vídeo";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::Video::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        ImGui::End();
    }
}