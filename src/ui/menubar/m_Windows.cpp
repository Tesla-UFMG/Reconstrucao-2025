#include "ui/menubar/m_Windows.hpp"

void Menu::Windows() {
    if (ImGui::BeginMenu("Janelas")) {
        if (ImGui::MenuItem("Tela Cheia", "F11", SDLWrapper::getIsFullscreen())) {
            SDLWrapper::changeFullscreen();
        }
        ImGui::Separator();

        if (ImGui::BeginMenu("Salvar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("CTRL + F" + std::to_string(i)).c_str())) {
                    WindowManager::saveWindowVisibility("./cache/layouts/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::saveLayout("./cache/layouts/.layout_" + std::to_string(i) + ".ini");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Carregar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("F" + std::to_string(i)).c_str())) {
                    WindowManager::loadWindowVisibility("./cache/layouts/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::loadLayout("./cache/layouts/.layout_" + std::to_string(i) + ".ini");
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        Menu::showWindowVisibility("Playback", &WindowManager::visibility.showPlayback);
        Menu::showWindowVisibility("Selecionador de Dados", &WindowManager::visibility.showDataPicker);
        Menu::showWindowVisibility("Reconstrução de Pista", &WindowManager::visibility.showReconstruction);
        Menu::showWindowVisibility("Video", &WindowManager::visibility.showVideo);
        Menu::showWindowVisibility("Plot", &WindowManager::visibility.showPlot);
        Menu::showWindowVisibility("Volante", &WindowManager::visibility.showWheelControl);

        ImGui::Separator();

        if (ImGui::BeginMenu("Desenvolvedor")) {
            Menu::showWindowVisibility("Log", &WindowManager::visibility.showLog);
            Menu::showWindowVisibility("ImGui Demo", &WindowManager::visibility.showImGuiDemo);
            Menu::showWindowVisibility("ImPlot Demo", &WindowManager::visibility.showImPlotDemo);
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}