#include "ui/windows/w_Statistics.hpp"

Window::Statistics::Statistics(bool* isOpen) : IWindow(isOpen) {
    this->title = "Estatísticas";
    this->flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;
}

static bool showGraphs = false;

void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset, const ImVec4& col,
               const ImVec2& size) {
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
    if (ImPlot::BeginPlot(id, size, ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, count - 1, min_v, max_v, ImGuiCond_Always);
        ImPlot::SetNextLineStyle(col);
        ImPlot::SetNextFillStyle(col, 0.25);
        ImPlot::PlotLine(id, values, count, 1, 0, ImPlotLineFlags_Shaded, offset);
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
}

template <typename T> inline T RandomRange(T min, T max) {
    T scale = rand() / (T)RAND_MAX;
    return min + scale * (max - min);
}

void Window::Statistics::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Configurações")) {
            if (ImGui::BeginMenu("Gráfico")) {

                if (ImGui::MenuItem("Mostrar Gráfico", nullptr, &showGraphs)) {
                    LOG("DEBUG", "Botão de mostrar gráfico nas estatísticas " +
                                     std::string(showGraphs ? "ativado." : "desativado."));
                }

                MenuBar::changeColorMap();

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::Statistics::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        // Aceita o payload
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       archiveName, columnName;

            // Pega o nome do arquivo e a coluna
            if (std::getline(ss, archiveName, ':') && std::getline(ss, columnName, ':')) {
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Statistics::renderTable() {
    static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg |
                                   ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
    static int offset = 0;
    offset            = (offset + 1) % 100;

    if (ImGui::BeginTable("##table", 3, flags, ImVec2(-1, 0))) {
        ImGui::TableSetupColumn("Electrode", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Voltage", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("EMG Signal");
        ImGui::TableHeadersRow();
        for (int row = 0; row < 10; row++) {
            ImGui::TableNextRow();
            static float data[100];
            srand(row);
            for (int i = 0; i < 100; ++i)
                data[i] = RandomRange(0.0f, 10.0f);
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("EMG %d", row);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f V", data[offset]);
            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(row);
            Sparkline("##spark", data, 100, 0, 11.0f, offset, ImPlot::GetColormapColor(row), ImVec2(-1, 35));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    this->processColumnDragDrop();
}

void Window::Statistics::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->renderMenuBar();
        this->renderTable();

        ImGui::End();
    }
}