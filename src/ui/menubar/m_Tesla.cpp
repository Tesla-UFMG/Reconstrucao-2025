#include "ui/menubar/m_Tesla.hpp"

void MenuBar::Tesla() {
    if (ImGui::BeginMenu("Tesla")) {
        if (ImGui::MenuItem("Tela Cheia", "F11", SDLWrapper::getIsFullscreen())) {
            SDLWrapper::changeFullscreen();
        }

        MenuBar::changeAppStyleTheme();

        ImGui::Separator();

        if (ImGui::MenuItem("Novo", "CTRL + C")) {
            DB::getInstance().createProjectDialog();
        }

        if (ImGui::MenuItem("Carregar", "CTRL + N")) {
            DB::getInstance().loadProjectDialog();
        }

        if (DB::getInstance().getProject().currentProjectName.empty() == false) {
            if (ImGui::MenuItem("Salvar", "CTRL + S")) {
                DB::getInstance().quickSaveProject();
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Sair", "ALT + F4")) {
            SDLWrapper::raiseExitEvent();
        }
        ImGui::EndMenu();
    }
}