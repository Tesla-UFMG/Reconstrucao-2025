#include "Pages.hpp"

namespace Pages {
    bool showAbout = false;
}

void Pages::mainMenuBarTime() {
    std::time_t t   = std::time(nullptr);
    std::tm*    now = std::localtime(&t);
    char        buffer[64];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S  %d-%m-%Y", now);
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer)[0] * 1.1);
    ImGui::Text("%s", buffer);
}

void Pages::mainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Arquivos")) {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Janelas")) {
            ImGui::MenuItem("Janela 1", nullptr, &Pages::showAbout);
            ImGui::MenuItem("Janela 2", nullptr, &Pages::showAbout);
            ImGui::MenuItem("Janela 3", nullptr, &Pages::showAbout);
            ImGui::MenuItem("Janela 4", nullptr, &Pages::showAbout);
            ImGui::MenuItem("Janela 5", nullptr, &Pages::showAbout);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Ajuda")) {
            ImGui::MenuItem("Sobre", nullptr, &Pages::showAbout);
            ImGui::EndMenu();
        }

        Pages::mainMenuBarTime();
        ImGui::EndMainMenuBar();
    }

    if (Pages::showAbout) {
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("Sobre", &Pages::showAbout, flags);
        ImGui::SeparatorText("Resconstrução de Pista");
        ImGui::Text("Formula Tesla");
        ImGui::Text("Edição: 2025");
        ImGui::Text("Versão: 1.0.0");
        ImGui::SeparatorText("Desenvolvedores");
        ImGui::Text("Lucas Martins Rocha");
        ImGui::Text("Gabriel Matos");
        ImGui::End();
    }
}