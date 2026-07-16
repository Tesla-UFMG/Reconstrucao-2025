#include "ui/menubar/m_Windows.hpp"

void MenuBar::Windows() {
    WindowManager& vw = WindowManager::getInstance();

    if (ImGui::BeginMenu("Janelas")) {

        MenuBar::changePlotColormap();

        ImGui::Separator();

        std::string currentProject = DB::getInstance().getProject().currentProjectName;
        bool projectActive = !currentProject.empty();

        if (!projectActive) {
            ImGui::MenuItem("Layouts (Requer projeto ativo)", nullptr, false, false);
            ImGui::Separator();
        }

        if (ImGui::BeginMenu("Salvar Layout", projectActive)) {
            for (int i = 1; i <= 10; i++) {
                std::string layoutName = "Layout " + std::to_string(i);
                if (ImGui::MenuItem(layoutName.c_str(), ("CTRL + F" + std::to_string(i)).c_str())) {
                    vw.saveWindowVisibility("./cache/layouts/" + currentProject + "/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::saveLayout("./cache/layouts/" + currentProject + "/.layout_" + std::to_string(i) + ".ini");
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Carregar Layout", projectActive)) {
            for (int i = 1; i <= 10; i++) {
                std::string pathIni = "./cache/layouts/" + currentProject + "/.layout_" + std::to_string(i) + ".ini";
                bool exists = std::filesystem::exists(pathIni);
                std::string layoutName = "Layout " + std::to_string(i) + (exists ? "" : " (Vazio)");
                if (ImGui::MenuItem(layoutName.c_str(), ("F" + std::to_string(i)).c_str(), false, exists)) {
                    vw.loadWindowVisibility("./cache/layouts/" + currentProject + "/.visibility_" + std::to_string(i) + ".bin");
                    ImGuiWrapper::loadLayout(pathIni);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::SeparatorText("Janelas Estáticas");

        MenuBar::changeWindowVisibility("Selecionador de Dados", &vw.visibility.showDataPicker);
        MenuBar::changeWindowVisibility("Telemetria", &vw.visibility.showTelemetry);
        MenuBar::changeWindowVisibility("Avisos", &vw.visibility.showWarnings);
        MenuBar::changeWindowVisibility("Playback", &vw.visibility.showPlayback);
        if (vw.getPlaybackWindow()) {
            MenuBar::changeWindowVisibility("Comentários do Playback", &vw.getPlaybackWindow()->getShowCommentsWindow());
        }
        MenuBar::changeWindowVisibility("Reconstrução de Pista", &vw.visibility.showReconstruction);
        MenuBar::changeWindowVisibility("Vídeo", &vw.visibility.showVideo);
        MenuBar::changeWindowVisibility("Volante", &vw.visibility.showWheelControl);
        MenuBar::changeWindowVisibility("Pedais", &vw.visibility.showPedal);
        
        
        ImGui::SeparatorText("Janelas Dinâmicas");
        if (ImGui::MenuItem("Tabela")) {
            vw.createTabelaWindow();
        }
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