#include "menu/TeslaMenu.hpp"

void Menu::Tesla() {
    if (ImGui::BeginMenu("Tesla")) {

        if (ImGui::MenuItem("Novo", "CTRL + C")) {
            DB::getInstance().createProjectDialog();
        }

        if (ImGui::MenuItem("Carregar", "CTRL + N")) {
            DB::getInstance().loadProjectDialog();
        }

        if (DB::getInstance().getCurrentProject().empty() == false) {
            if (ImGui::MenuItem("Salvar", "CTRL + S")) {
                DB::getInstance().saveProjectDialog();
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Sair", "ALT + F4")) {
            SDLWrapper::raiseExitEvent();
        }
        ImGui::EndMenu();
    }
}