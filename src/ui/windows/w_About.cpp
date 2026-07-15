#include "ui/windows/w_About.hpp"

Window::About::About(bool* isOpen) : IWindow(isOpen) {
    this->title = "Sobre";

    this->flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_UnsavedDocument;

    this->developers = {"Lucas Martins Rocha", "Gabriel Matos"};
}

void Window::About::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        ImGui::SeparatorText("Reconstrução de Pista");
        ImGui::Text("Formula Tesla");
        ImGui::Text("Edição: 2026");
        ImGui::Text("Versão: 3.0.0-dev");
        ImGui::Text("A cobra vai fumar!");
        ImGui::SeparatorText("Desenvolvedores");
        for (const std::string& developer : this->developers) {
            ImGui::BulletText(developer.c_str());
        }

        ImGui::SeparatorText("Contatos");
        ImGui::Text("Está precisando de uma ajuda?");
        ImGui::Text("Email: lucasrocha.png@gmail.com");

        ImGui::End();
    }
}
