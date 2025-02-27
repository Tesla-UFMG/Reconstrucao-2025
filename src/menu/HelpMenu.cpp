#include "menu/HelpMenu.hpp"

void Menu::Help() {
    if (ImGui::BeginMenu("Ajuda")) {
        Menu::showWindowVisibility("Sobre", &Window::visibility.showAbout);
        ImGui::EndMenu();
    }
}
