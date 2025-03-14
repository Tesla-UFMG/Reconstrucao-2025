#include "menu/m_Help.hpp"

void Menu::Help() {
    if (ImGui::BeginMenu("Ajuda")) {
        Menu::showWindowVisibility("Sobre", &WindowManager::visibility.showAbout);
        ImGui::EndMenu();
    }
}
