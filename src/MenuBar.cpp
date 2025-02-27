#include "MenuBar.hpp"

void MenuBar::renderCurrentTime() {
    std::time_t t   = std::time(nullptr);
    std::tm*    now = std::localtime(&t);
    char        buffer[64];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S  %d-%m-%Y", now);
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer)[0] * 1.1);
    ImGui::Text("%s", buffer);
}

void MenuBar::renderProgramName() {
    std::string programName = "Fórmula Tesla";
    float       windowWidth = ImGui::GetWindowWidth();
    float       textWidth   = ImGui::CalcTextSize(programName.c_str())[0];
    float       textOffsetX = (windowWidth - textWidth) / 2.0f;
    ImGui::SetCursorPosX(textOffsetX);
    ImGui::Text("%s", programName.c_str());
}

void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {

        Menu::Tesla();

        if (DB::getInstance().getCurrentProject().empty() == false) {
            Menu::Windows();
        }

        Menu::Help();

        MenuBar::renderCurrentTime();
        MenuBar::renderProgramName();

        ImGui::EndMainMenuBar();
    }
}