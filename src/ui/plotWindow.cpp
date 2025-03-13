#include "ui/PlotWindow.hpp"
#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

enum GraphType { GRAPH_LINE, GRAPH_BAR, GRAPH_SCATTER, GRAPH_FILLED_LINE };

struct GraphData {
        std::vector<std::string> columns;
        GraphType                type     = GRAPH_LINE;
        bool                     hideAxes = true;
};

static std::vector<GraphData> graphs;
static float                  plotHeight = 200.0f;
static bool                   autoFit    = true;

void Window::MenuBar::Plot() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Novo Gráfico")) {
            GraphData newGraph;
            graphs.push_back(newGraph);
        }
        if (ImGui::BeginMenu("Configurações")) {
            ImGui::MenuItem("Auto Fit", nullptr, &autoFit);
            if (ImGui::BeginMenu("Altura")) {
                ImGui::SliderFloat("Altura", &plotHeight, 100.0f, 400.0f, "%.2f px");
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void Window::Plot(bool* isOpen) {

    if (*isOpen) {
        ImGui::Begin("Plot", isOpen, ImGuiWindowFlags_MenuBar);

        Window::MenuBar::Plot();

        // Atualiza os limites comuns com base nos dados (no eixo X fixo e Y de acordo com a amplitude máxima)
        const int num_points = 100;

        int graphToRemove = -1;

        // Agrupa os plots para que os eixos fiquem alinhados
        if (ImPlot::BeginAlignedPlots("AlignedGroup")) {
            for (size_t i = 0; i < graphs.size(); i++) {
                GraphData& g = graphs[i];
                ImGui::PushID(static_cast<int>(i));

                // Gera dados simulados para o plot
                static float x[num_points], y[num_points];
                for (int j = 0; j < num_points; j++) {
                    x[j] = static_cast<float>(j);
                    y[j] = std::sin(j * 0.1f) * (i + 1);
                }

                std::string plotID = "##Plot " + std::to_string(i);
                if (ImPlot::BeginPlot(plotID.c_str(), ImVec2(-1, plotHeight), ImPlotFlags_NoFrame)) {

                    // Configuração dos eixos (opcional: sem decorações se g.hideAxes estiver ativo)
                    ImPlotAxisFlags flags = 0;
                    if (g.hideAxes) {
                        flags |= ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
                    }
                    if (autoFit) {
                        flags |= ImPlotAxisFlags_AutoFit;
                    }
                    ImPlot::SetupAxes(nullptr, nullptr, flags, flags);

                    // Plota cada coluna conforme o tipo de gráfico selecionado
                    for (const auto& col : g.columns) {
                        switch (g.type) {
                            case GRAPH_LINE:
                                ImPlot::PlotLine(col.c_str(), x, y, num_points);
                                break;
                            case GRAPH_BAR:
                                ImPlot::PlotBars(col.c_str(), x, y, num_points, 0.8f);
                                break;
                            case GRAPH_SCATTER:
                                ImPlot::PlotScatter(col.c_str(), x, y, num_points);
                                break;
                            case GRAPH_FILLED_LINE:
                                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                                ImPlot::PlotShaded(col.c_str(), x, y, num_points);
                                ImPlot::PlotLine(col.c_str(), x, y, num_points);
                                ImPlot::PopStyleVar();
                                break;
                        }
                    }

                    // Popup de legenda para controle do gráfico
                    if (!g.columns.empty()) {
                        const char* tipos[] = {"Linha", "Barra", "Scatter", "Filled Line"};
                        if (ImPlot::BeginLegendPopup(g.columns[0].c_str())) {
                            int currentType = static_cast<int>(g.type);
                            if (ImGui::Combo("##Tipo", &currentType, tipos, IM_ARRAYSIZE(tipos))) {
                                g.type = static_cast<GraphType>(currentType);
                            }
                            if (ImGui::Button("Remover Gráfico")) {
                                graphToRemove = static_cast<int>(i);
                            }
                            ImGui::SameLine();
                            ImGui::Checkbox("Remover Eixos", &g.hideAxes);
                            ImGui::SeparatorText("Colunas");
                            for (size_t j = 0; j < g.columns.size(); j++) {

                                std::string btnLabel = "X##" + std::to_string(j);
                                if (ImGui::Button(btnLabel.c_str())) {
                                    g.columns.erase(g.columns.begin() + j);
                                    break;
                                }
                                ImGui::SameLine();
                                ImGui::Text("%s", g.columns[j].c_str());
                            }
                            ImPlot::EndLegendPopup();
                        }
                    }
                    ImPlot::EndPlot();
                }

                // Suporte a drag & drop para colunas
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                        const char* col = (const char*)payload->Data;
                        g.columns.push_back(col);
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();
            }
            ImPlot::EndAlignedPlots();
        }

        if (graphToRemove != -1)
            graphs.erase(graphs.begin() + graphToRemove);

        ImGui::End();
    }
}
