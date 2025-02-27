#include "ui/InitialWindow.hpp"

void Window::Initial() {

    std::string text = "Abra ou crie um projeto";

    ImVec2 textSize   = ImGui::CalcTextSize(text.c_str());
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 textPos    = ImVec2((screenSize.x - textSize.x) / 2, (screenSize.y - textSize.y) / 2);

    ImGui::SetNextWindowPos(textPos, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
    if (ImGui::Begin("Aviso", nullptr, flags)) {
        ImGui::Text(text.c_str());
        ImGui::End();
    }
}