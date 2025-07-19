#include "ui/windows/w_Statistics.hpp"

static bool showGraphs = true;

Window::Statistics::Statistics(bool* isOpen) : IWindow(isOpen) {
    this->title = "Estatísticas";
    this->flags = ImGuiWindowFlags_MenuBar;
}

void Window::Statistics::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->renderMenuBar();
        ImGui::Dummy(ImGui::GetContentRegionAvail());
        this->processColumnDragDrop();
        ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
        this->renderTable();
        ImGui::End();
    }
}

void Window::Statistics::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Opções")) {
            ImGui::MenuItem("Mostrar Gráficos", nullptr, &showGraphs);
            if (ImGui::BeginMenu("Cores do Gráfico")) {
                MenuBar::changePlotColormap();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::Statistics::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);

            std::string fileType   = columnPayload->fileType;
            std::string fileName   = columnPayload->fileName;
            std::string columnName = columnPayload->columnName;

            Metric new_metric;
            new_metric.display_name = columnName;
            new_metric.unique_id    = fileName + ":" + columnName;
            for (const auto& metric : metrics) {
                if (metric.unique_id == new_metric.unique_id) {
                    LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já está nas estatísticas.");
                    return;
                }
            }

            if (fileType == "CSV") {
                new_metric.data = &DB::getInstance().getCSVData(fileName, columnName);
            } else if (fileType == "Telemetry") {
                new_metric.data = &DB::getInstance().getTelemetryData(fileName, columnName);
            }

            metrics.push_back(new_metric);
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " foi adicionado às estatísticas.");
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Statistics::renderTable() {
    int                    column_count = showGraphs ? 4 : 3;
    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
    int metric_to_remove = -1;

    if (ImGui::BeginTable("##telemetry_table", column_count, flags)) {
        ImGui::TableSetupColumn("Métrica", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Valor Atual", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        if (showGraphs) {
            ImGui::TableSetupColumn("Gráfico", ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        if (!metrics.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            for (size_t i = 0; i < metrics.size(); ++i) {
                const Metric& metric = metrics[i];
                ImGui::TableNextRow();

                // Nome
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", metric.display_name.c_str());

                // Valores
                ImGui::TableSetColumnIndex(1);
                if (!metric.data->empty()) {
                    double value = metric.data->back();
                    ImGui::Text("%.3f", value);
                }

                if (showGraphs) {
                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushID(metric.unique_id.c_str());
                    if (!metric.data->empty()) {
                        this->renderGraph(metric, i);
                    }
                    ImGui::PopID();
                }

                ImGui::TableSetColumnIndex(showGraphs ? 3 : 2);
                ImGui::PushID(i);
                if (ImGui::Button("X")) {
                    metric_to_remove = i;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    if (metric_to_remove != -1) {
        metrics.erase(metrics.begin() + metric_to_remove);
    }
}

void Window::Statistics::renderGraph(const Metric& metric, size_t i) {
    std::string         id          = metric.unique_id;
    std::vector<double> y           = *(metric.data);
    size_t              ySize       = y.size();
    size_t              numOfPoints = std::min<size_t>(ySize, HISTORY_SIZE);
    size_t              begin       = ySize - numOfPoints;

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
    if (ImPlot::BeginPlot(id.c_str(), ImVec2(-1, 50), ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations,
                          ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_AutoFit);

        ImPlot::SetupAxisLimits(ImAxis_X1, (double)begin, (double)ySize, ImGuiCond_Always);
        ImVec4 color = ImPlot::GetColormapColor(static_cast<int>(i) % ImPlot::GetColormapSize());
        ImPlot::SetNextLineStyle(color, 1.0f);
        ImPlot::SetNextFillStyle(color, 0.25f);
        ImPlot::PlotLine(id.c_str(), y.data() + begin, (int)numOfPoints, 1.0, (double)begin, ImPlotLineFlags_Shaded);
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
}