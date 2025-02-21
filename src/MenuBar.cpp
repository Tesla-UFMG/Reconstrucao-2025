#include "MenuBar.hpp"

void MenuBar::renderCurrentTime() {
    std::time_t t   = std::time(nullptr);
    std::tm*    now = std::localtime(&t);
    char        buffer[64];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S  %d-%m-%Y", now);
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(buffer)[0] * 1.1);
    ImGui::Text("%s", buffer);
}

void MenuBar::renderProgramName(){
    std::string programName = "Fórmula Tesla";
    float windowWidth = ImGui::GetWindowWidth();
    float textWidth = ImGui::CalcTextSize(programName.c_str())[0];
    float textOffsetX = (windowWidth - textWidth) / 2.0f;
    ImGui::SetCursorPosX(textOffsetX);
    ImGui::Text("%s", programName.c_str());
}

void MenuBar::loadLayout() {
    std::ifstream inFile(".interface-layout", std::ios::binary);
    if (inFile.is_open()) {
        std::string iniData, line;
        while (std::getline(inFile, line))
            iniData += line + '\n';

        ImGui::LoadIniSettingsFromMemory(iniData.c_str(), iniData.size());
        inFile.close();
        Log::getInstance().message("TRACE", "Layout carregado com sucesso.");
    } else {
        Log::getInstance().message("ERROR", "Não foi possível carregar o layout.");
    }
}

void MenuBar::saveLayout() {
    size_t        size;
    const char*   iniData = ImGui::SaveIniSettingsToMemory(&size);
    std::ofstream outFile(".interface-layout", std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(iniData, size);
        outFile.close();
        Log::getInstance().message("TRACE", "Layout salvo com sucesso.");
    } else {
        Log::getInstance().message("ERROR", "Não foi possível salvar o layout.");
    }
}

void MenuBar::WindowsTab() {
    if (ImGui::BeginMenu("Janelas")) {
        // Layout Manager
        if (ImGui::BeginMenu("Layout")) {
            if (ImGui::MenuItem("Salvar Layout")) {
                MenuBar::saveLayout();
            }
            if (ImGui::MenuItem("Carregar Layout")) {
                MenuBar::loadLayout();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        ImGui::MenuItem("Janela 1", nullptr, &Pages::showAbout);
        ImGui::MenuItem("Janela 2", nullptr, &Pages::showAbout);
        ImGui::MenuItem("Janela 3", nullptr, &Pages::showAbout);
        ImGui::MenuItem("Janela 4", nullptr, &Pages::showAbout);
        ImGui::MenuItem("Janela 5", nullptr, &Pages::showAbout);
        ImGui::EndMenu();
    }
}

void MenuBar::HelpTab() {
    if (ImGui::BeginMenu("Ajuda")) {
        ImGui::MenuItem("Sobre", nullptr, &Pages::showAbout);
        ImGui::EndMenu();
    }
}

void MenuBar::ConfigurationTab() {
    if (ImGui::BeginMenu("Configurações")) {
        if (ImGui::MenuItem("Tela Cheia", "F11", SDLWrapper::getIsFullscreen())) {
            SDLWrapper::changeFullscreen();
        }
        ImGui::EndMenu();
    }
}

void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {

        MenuBar::ConfigurationTab();
        MenuBar::WindowsTab();
        MenuBar::HelpTab();

        MenuBar::renderCurrentTime();
        MenuBar::renderProgramName();
        ImGui::EndMainMenuBar();
    }
}