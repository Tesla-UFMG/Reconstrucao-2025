#include "ui/menubar/MenuBar.hpp"

void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {
        MenuBar::Tesla();
        if (DB::getInstance().getProject().currentProject.empty() == false) {
            MenuBar::Windows();
        }

        MenuBar::Help();
        MenuBar::renderProgramName();
        MenuBar::renderStatus();
        ImGui::EndMainMenuBar();
    }
}