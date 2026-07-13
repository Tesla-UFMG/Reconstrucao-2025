#include "ui/windows/w_HomePage.hpp"

Window::HomePage::HomePage(bool* isOpen) : IWindow(isOpen) {
    this->title = "Aviso";
    this->flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;
}

void Window::HomePage::render() {
    std::string text = "Abra ou crie um projeto";

    ImVec2 textSize   = ImGui::CalcTextSize(text.c_str());
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 textPos    = ImVec2((screenSize.x - textSize.x) / 2, (screenSize.y - textSize.y) / 2);

    ImGui::SetNextWindowPos(textPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->title.c_str(), nullptr, this->flags);
    ImGui::Text(text.c_str());
    ImGui::End();
}
