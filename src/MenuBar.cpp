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

void MenuBar::windowsTab() {
    if (ImGui::BeginMenu("Janelas")) {
        if (ImGui::MenuItem("Tela Cheia", "F11", SDLWrapper::getIsFullscreen())) {
            SDLWrapper::changeFullscreen();
        }
        ImGui::Separator();

        if (ImGui::MenuItem("Salvar Layout")) {
            ImGuiWrapper::saveLayout(".interface-layout");
        }
        if (ImGui::MenuItem("Carregar Layout")) {
            ImGuiWrapper::loadLayout(".interface-layout");
        }

        ImGui::Separator();
        
        MenuBar::showWindowVisibility("Playback", &Window::visibility.showPlayback);
        MenuBar::showWindowVisibility("Selecionador de Dados", &Window::visibility.showDataPicker);
        MenuBar::showWindowVisibility("Reconstrução de Pista", &Window::visibility.showReconstruction);
        MenuBar::showWindowVisibility("Video", &Window::visibility.showVideo);
        MenuBar::showWindowVisibility("Plot", &Window::visibility.showPlot);
        MenuBar::showWindowVisibility("Log", &Window::visibility.showLog);

        ImGui::Separator();

        if (ImGui::MenuItem("Desenvolvedor")) {
            MenuBar::showWindowVisibility("ImGui Demo", &Window::visibility.showImGuiDemo);
            MenuBar::showWindowVisibility("ImPlot Demo", &Window::visibility.showImPlotDemo);    
        }
        
        ImGui::EndMenu();
    }
}

void MenuBar::helpTab() {
    if (ImGui::BeginMenu("Ajuda")) {
        MenuBar::showWindowVisibility("Sobre", &Window::visibility.showAbout);
        ImGui::EndMenu();
    }
}

void MenuBar::configurationTab() {
    if (ImGui::BeginMenu("Configurações")) {
        
        ImGui::EndMenu();
    }
}

void MenuBar::showWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    if (windowVisibility == nullptr) {
        Log::getInstance().message("ERROR", "Ponteiro nulo ao tentar acessar a visibilidade da janela '" + windowName + "'.");
        return;
    }

    if (ImGui::MenuItem(windowName.c_str(), nullptr, *windowVisibility)) {
        Window::changeWindowVisibility(windowName, windowVisibility);
    }
}



void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {

        MenuBar::configurationTab();
        MenuBar::windowsTab();
        MenuBar::helpTab();

        MenuBar::renderCurrentTime();
        MenuBar::renderProgramName();
        ImGui::EndMainMenuBar();
    }
}