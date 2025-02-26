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

void MenuBar::teslaTab() {
    if (ImGui::BeginMenu("Tesla")) {

        if (ImGui::MenuItem("Novo")) {
            const char* filters[] = {"*.tesla", NULL};
            char*       filepath =
                tinyfd_saveFileDialog("Novo Projeto", "./project.tesla", 1, filters, ".tesla (Tesla Project)");
        }

        if (ImGui::MenuItem("Salvar")) {
            const char* filters[] = {"*.tesla", NULL};
            char*       filepath =
                tinyfd_saveFileDialog("Salvar Projeto", "./project.tesla", 1, filters, ".tesla (Tesla Project)");
        }

        if (ImGui::MenuItem("Carregar")) {
            const char* filters[] = {"*.tesla", NULL};
            char*       filepath =
                tinyfd_openFileDialog("Carregar Projeto", "./project.tesla", 1, filters, ".tesla (Tesla Project)", 0);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Sair", "ALT + F4")) {
            SDL_Event event = {SDL_QUIT};
            SDL_PushEvent(&event);
        }
        ImGui::EndMenu();
    }
}

void MenuBar::windowsTab() {
    if (ImGui::BeginMenu("Janelas")) {
        if (ImGui::MenuItem("Tela Cheia", "F11", SDLWrapper::getIsFullscreen())) {
            SDLWrapper::changeFullscreen();
        }
        ImGui::Separator();

        if (ImGui::BeginMenu("Salvar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("CTRL + F" + std::to_string(i)).c_str())) {
                    Window::saveWindowVisibility("./data/layouts/.visibility_" + std::to_string(i));
                    ImGuiWrapper::saveLayout("./data/layouts/.layout_" + std::to_string(i));
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Carregar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("F" + std::to_string(i)).c_str())) {
                    Window::loadWindowVisibility("./data/layouts/.visibility_" + std::to_string(i));
                    ImGuiWrapper::loadLayout("./data/layouts/.layout_" + std::to_string(i));
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        MenuBar::showWindowVisibility("Playback", &Window::visibility.showPlayback);
        MenuBar::showWindowVisibility("Selecionador de Dados", &Window::visibility.showDataPicker);
        MenuBar::showWindowVisibility("Reconstrução de Pista", &Window::visibility.showReconstruction);
        MenuBar::showWindowVisibility("Video", &Window::visibility.showVideo);
        MenuBar::showWindowVisibility("Plot", &Window::visibility.showPlot);

        ImGui::Separator();

        if (ImGui::BeginMenu("Desenvolvedor")) {
            MenuBar::showWindowVisibility("Log", &Window::visibility.showLog);
            MenuBar::showWindowVisibility("ImGui Demo", &Window::visibility.showImGuiDemo);
            MenuBar::showWindowVisibility("ImPlot Demo", &Window::visibility.showImPlotDemo);
            ImGui::EndMenu();
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

void MenuBar::showWindowVisibility(const std::filesystem::path& windowName, bool* windowVisibility) {
    if (windowVisibility == nullptr) {
        LOG("ERROR", "Ponteiro nulo ao tentar acessar a visibilidade da janela '" + windowName.string() + "'.");
        return;
    }

    if (ImGui::MenuItem(windowName.string().c_str(), nullptr, *windowVisibility)) {
        Window::changeWindowVisibility(windowName.string(), windowVisibility);
    }
}

void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {

        MenuBar::teslaTab();
        MenuBar::windowsTab();
        MenuBar::helpTab();

        MenuBar::renderCurrentTime();
        MenuBar::renderProgramName();
        ImGui::EndMainMenuBar();
    }
}