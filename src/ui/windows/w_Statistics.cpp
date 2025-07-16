#include "ui/windows/w_Statistics.hpp"
#include "DB.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

// --- ESTRUTURAS DE DADOS ---
struct TelemetryData {
        double timestamp;
        double speed_kmh;
        double battery_soc;
        double power_kw;
        double motor_temp_c;
        double battery_temp_c;
        double g_force_lateral;
        double g_force_long;
};

struct CustomMetric {
        std::string         unique_id;
        std::string         display_name;
        std::vector<double> values;
        double              min_val = 0.0;
        double              max_val = 1.0;

        // Controles individuais consistentes
        float  playback_speed        = 1.0f;
        float  y_axis_multiplier     = 1.0f; // Achatamento Vertical
        float  horizontal_multiplier = 1.0f; // Achatamento Horizontal
        double current_offset        = 0.0;
};

// --- VARIÁVEIS GLOBAIS DO ARQUIVO ---
static bool          showGraphs       = true;
static constexpr int HISTORY_SIZE     = 200;
static constexpr int BASE_WINDOW_SIZE = 300;

static std::vector<TelemetryData> telemetryHistory;
static double                     time_counter = 0.0;
static std::vector<CustomMetric>  customMetrics;

// --- FUNÇÕES AUXILIARES ---

void Sparkline(const char* id, const double* values, int count, double min_v, double max_v, const ImVec2& size,
               int offset) {
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
    if (ImPlot::BeginPlot(id, size, ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, count - 1, min_v, max_v, ImGuiCond_Always);
        ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(1));
        ImPlot::SetNextFillStyle(ImPlot::GetColormapColor(1), 0.25f);
        ImPlot::PlotLine(id, values, count, 1.0, 0, ImPlotLineFlags_Shaded, offset);
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleVar();
}

void update_telemetry_data() {
    time_counter += 0.05;
    TelemetryData data;
    data.timestamp       = time_counter;
    data.speed_kmh       = 60.0 + 30.0 * sin(time_counter * 0.5);
    data.battery_soc     = 85.0 - (time_counter * 0.1) + 0.5 * cos(time_counter * 0.5);
    data.power_kw        = 20.0 + 15.0 * cos(time_counter * 0.5) * sin(time_counter);
    data.motor_temp_c    = 70.0 + 10.0 * sin(time_counter * 0.2);
    data.battery_temp_c  = 45.0 + 5.0 * sin(time_counter * 0.1);
    data.g_force_lateral = 1.5 * sin(time_counter);
    data.g_force_long    = 1.0 * cos(time_counter * 0.5);
    telemetryHistory.push_back(data);
    if (telemetryHistory.size() > HISTORY_SIZE) {
        telemetryHistory.erase(telemetryHistory.begin());
    }
}

void RenderTelemetryRow(const char* label, double value, const char* unit, const std::vector<double>& history_data,
                        double min_val, double max_val) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.2f %s", value, unit);
    if (showGraphs) {
        ImGui::TableSetColumnIndex(2);
        ImGui::PushID(label);
        Sparkline("##spark", history_data.data(), history_data.size(), min_val, max_val, ImVec2(-1, 35), 0);
        ImGui::PopID();
    }
}

// --- IMPLEMENTAÇÃO DA CLASSE Window::Statistics ---

Window::Statistics::Statistics(bool* isOpen) : IWindow(isOpen) {
    this->title = "Painel de Telemetria";
    this->flags = ImGuiWindowFlags_MenuBar;
}

void Window::Statistics::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       filename, columnName;
            if (std::getline(ss, filename, ':') && std::getline(ss, columnName, ':')) {
                std::string metricIdentifier = filename + ":" + columnName;
                for (const auto& metric : customMetrics) {
                    if (metric.unique_id == metricIdentifier)
                        return;
                }
                std::vector<double> data = DB::getInstance().getCSVData(filename, columnName);
                if (data.empty())
                    return;
                CustomMetric new_metric;
                new_metric.unique_id    = metricIdentifier;
                new_metric.display_name = columnName;
                new_metric.values       = data;
                auto   minmax           = std::minmax_element(data.begin(), data.end());
                double range            = *minmax.second - *minmax.first;
                double margin           = (range == 0) ? 1.0 : range * 0.1;
                new_metric.min_val      = *minmax.first - margin;
                new_metric.max_val      = *minmax.second + margin;
                int default_window      = std::min(static_cast<int>(data.size()), BASE_WINDOW_SIZE);
                if (BASE_WINDOW_SIZE > 0) {
                    new_metric.horizontal_multiplier = static_cast<float>(default_window) / BASE_WINDOW_SIZE;
                }
                customMetrics.push_back(new_metric);
            }
        }
        ImGui::EndDragDropTarget();
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

void Window::Statistics::renderTable() {
    int                    column_count = showGraphs ? 4 : 3;
    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
    int metric_to_remove = -1;

    if (ImGui::BeginTable("##telemetry_table", column_count, flags)) {
        ImGui::TableSetupColumn("Métrica", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Valor Atual", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        if (showGraphs) {
            ImGui::TableSetupColumn("Histórico", ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableSetupColumn("Ações", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        if (!telemetryHistory.empty()) {
            const auto&         current_data = telemetryHistory.back();
            std::vector<double> history_values;
            auto                extract_history = [&](auto extractor) {
                history_values.clear();
                for (const auto& data : telemetryHistory) {
                    history_values.push_back(extractor(data));
                }
            };
            extract_history([](const TelemetryData& d) { return d.speed_kmh; });
            RenderTelemetryRow("Velocidade", current_data.speed_kmh, "km/h", history_values, 0.0, 120.0);
        }

        if (!customMetrics.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SeparatorText("Métricas Carregadas do DB");

            for (int i = 0; i < customMetrics.size(); ++i) {
                auto& metric = customMetrics[i];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", metric.display_name.c_str());

                ImGui::TableSetColumnIndex(1);
                if (!metric.values.empty()) {
                    int current_idx = static_cast<int>(metric.current_offset) % metric.values.size();
                    ImGui::Text("%.3f", metric.values[current_idx]);
                }

                if (showGraphs) {
                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushID(metric.unique_id.c_str());
                    if (!metric.values.empty()) {
                        double range_v     = metric.max_val - metric.min_val;
                        double center_v    = metric.min_val + range_v / 2.0;
                        double new_range_v = range_v * metric.y_axis_multiplier;
                        if (new_range_v <= 0)
                            new_range_v = 1.0;
                        double display_min = center_v - new_range_v / 2.0;
                        double display_max = center_v + new_range_v / 2.0;

                        int window_size = static_cast<int>(BASE_WINDOW_SIZE * metric.horizontal_multiplier);
                        window_size     = std::clamp(window_size, 10, static_cast<int>(metric.values.size()));

                        Sparkline("##spark_custom", metric.values.data(), window_size, display_min, display_max,
                                  ImVec2(-1, 35), static_cast<int>(metric.current_offset));
                    }
                    ImGui::PopID();
                }

                ImGui::TableSetColumnIndex(showGraphs ? 3 : 2);
                std::string popup_id = "ConfigPopup##" + metric.unique_id;
                ImGui::PushID(i);
                if (ImGui::Button("...")) {
                    ImGui::OpenPopup(popup_id.c_str());
                }

                if (ImGui::BeginPopup(popup_id.c_str())) {
                    ImGui::Text("Controles para: %s", metric.display_name.c_str());
                    ImGui::Separator();
                    ImGui::SliderFloat("Velocidade", &metric.playback_speed, 0.0f, 50.0f, "%.2f x");
                    ImGui::SliderFloat("Achatamento Vert.", &metric.y_axis_multiplier, 0.1f, 20.0f, "%.2f x");
                    ImGui::SliderFloat("Achatamento Horiz.", &metric.horizontal_multiplier, 10.0f, 100.0f, "%.2f x");
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    metric_to_remove = i;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    if (metric_to_remove != -1) {
        customMetrics.erase(customMetrics.begin() + metric_to_remove);
    }
}

void Window::Statistics::render() {
    if (this->isOpen && *this->isOpen) {
        update_telemetry_data();

        for (auto& metric : customMetrics) {
            metric.current_offset += metric.playback_speed;
        }

        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        this->renderMenuBar();

        ImGui::Dummy(ImGui::GetContentRegionAvail());
        this->processColumnDragDrop();

        ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

        this->renderTable();

        ImGui::End();
    }
}