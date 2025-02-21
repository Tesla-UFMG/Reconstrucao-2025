#include "Pages.hpp"

namespace Pages {
    bool showAbout = false;
}

void Pages::aboutPage() {
    if (Pages::showAbout) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        if (ImGui::Begin("Sobre", &Pages::showAbout, flags)) {
            ImGui::SeparatorText("Resconstrução de Pista");
            ImGui::Text("Formula Tesla");
            ImGui::Text("Edição: 2025");
            ImGui::Text("Versão: 1.0.0");

            ImGui::SeparatorText("Desenvolvedores");
            ImGui::BulletText("Lucas Martins Rocha");
            ImGui::BulletText("Gabriel Matos");

            ImGui::SeparatorText("Contatos");
            ImGui::Text("Está precisando de uma ajuda?");
            ImGui::Text("Email: lucasrocha.png@gmail.com");

            ImGui::End();
        }
    }
}

void Pages::render() { Pages::aboutPage(); }