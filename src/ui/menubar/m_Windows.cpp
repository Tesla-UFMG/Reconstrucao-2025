#include "ui/menubar/m_Windows.hpp"

void MenuBar::Windows() {
    WindowManager& vw = WindowManager::getInstance();

    if (ImGui::BeginMenu("Janelas")) {

        MenuBar::changePlotColormap();

        ImGui::Separator();

        if (ImGui::BeginMenu("Salvar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("CTRL + F" + std::to_string(i)).c_str())) {
                    vw.saveWindowVisibility("./cache/layouts/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::saveLayout("./cache/layouts/.layout_" + std::to_string(i) + ".ini");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Carregar Layout")) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("F" + std::to_string(i)).c_str())) {
                    vw.loadWindowVisibility("./cache/layouts/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::loadLayout("./cache/layouts/.layout_" + std::to_string(i) + ".ini");
                }
            }
            ImGui::EndMenu();
        }

        ImGui::SeparatorText("Janelas Estáticas");

        MenuBar::changeWindowVisibility("Selecionador de Dados", &vw.visibility.showDataPicker);
        MenuBar::changeWindowVisibility("Telemetria", &vw.visibility.showTelemetry);
        MenuBar::changeWindowVisibility("Avisos", &vw.visibility.showWarnings);
        MenuBar::changeWindowVisibility("Reconstrução de Pista", &vw.visibility.showReconstruction);
        //MenuBar::changeWindowVisibility("Plot", &vw.visibility.showPlot);
        MenuBar::changeWindowVisibility("Estatísticas", &vw.visibility.showStatistics);
        MenuBar::changeWindowVisibility("Volante", &vw.visibility.showWheelControl);
        MenuBar::changeWindowVisibility("Pedais", &vw.visibility.showPedal);
        
        
        ImGui::SeparatorText("Janelas Dinâmicas");
        if (ImGui::MenuItem("Gráfico")) {
            vw.createGraphWindow();
        }
        if (ImGui::MenuItem("Numérico")) {
            vw.createNumericWindow();
        }
        if (ImGui::MenuItem("Barra")) {
            vw.createBarWindow();
        }
        if (ImGui::MenuItem("Matriz de Confusão")) {
            vw.createMatrixWindow();
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Desenvolvedor")) {
            MenuBar::changeWindowVisibility("Log", &vw.visibility.showLog);
            MenuBar::changeWindowVisibility("ImGui Demo", &vw.visibility.showImGuiDemo);
            MenuBar::changeWindowVisibility("ImPlot Demo", &vw.visibility.showImPlotDemo);
            MenuBar::changeWindowVisibility("ImPlot 3D Demo", &vw.visibility.showImPlot3dDemo);
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
}