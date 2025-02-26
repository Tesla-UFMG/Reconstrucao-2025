#include "ui/dataPickerWindow.hpp"

void Window::dataPicker(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin("Selecionador de Dados", isOpen, flags)) {

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Arquivos")) {
                    if (ImGui::MenuItem("Abrir")) {
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::End();
        }
    }
}