#include "menu/UtilsMenu.hpp"

void Menu::showWindowVisibility(const std::filesystem::path& windowName, bool* isOpen) {
    if (isOpen == nullptr) {
        LOG("ERROR", "Ponteiro nulo ao tentar acessar a visibilidade da janela '" + windowName.string() + "'.");
        return;
    }

    if (ImGui::MenuItem(windowName.string().c_str(), nullptr, *isOpen)) {
        Window::changeWindowVisibility(windowName.string(), isOpen);
    }
}