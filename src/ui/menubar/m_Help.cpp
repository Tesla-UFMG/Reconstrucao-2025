#include "ui/menubar/m_Help.hpp"

void MenuBar::Help() {
    WindowManager& vw = WindowManager::getInstance();
    if (ImGui::BeginMenu("Ajuda")) {
        MenuBar::changeWindowVisibility("Sobre", &vw.visibility.showAbout);
        MenuBar::changeWindowVisibility("Atualizações", &vw.showUpdates);
        ImGui::EndMenu();
    }
}
