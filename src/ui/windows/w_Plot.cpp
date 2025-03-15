#include "ui/windows/w_Plot.hpp"

// Variáveis globais para os gráficos e configurações
static std::vector<GraphData> graphs;
static float                  plotHeight = 200.0f;
static bool                   autoFit    = true;

Window::Plot::Plot(bool* isOpen) : IWindow(isOpen) {
    title = "Plot";
    flags = ImGuiWindowFlags_MenuBar;
}

void Window::Plot::drawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Novo Gráfico")) {
            graphs.emplace_back(GraphData());
        }
        if (ImGui::BeginMenu("Configurações")) {
            ImGui::MenuItem("Auto Fit", nullptr, &autoFit);
            if (ImGui::BeginMenu("Altura")) {
                ImGui::SliderFloat("Altura", &plotHeight, 50.0f, 400.0f, "%.2f px");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::Plot::processColumnDragDrop(GraphData& graphData) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       archiveName, columnName;
            if (std::getline(ss, archiveName, ':') && std::getline(ss, columnName, ':')) {
                std::vector<double> y = DB::getInstance().getCSVData(archiveName, columnName);
                graphData.y.push_back(y);

                std::vector<double> x(y.size());
                for (size_t i = 0; i < y.size(); ++i) {
                    x[i] = static_cast<double>(i);
                }
                graphData.x.push_back(x);

                graphData.columns.push_back(columnName);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Plot::drawLegendPopup(GraphData& graphData, int graphIndex, int& graphToRemove) {
    if (!graphData.columns.empty()) {
        const char* tipos[] = {"Linha", "Barra", "Scatter", "Preenchido"};
        if (ImPlot::BeginLegendPopup(graphData.columns[0].c_str())) {
            int currentType = static_cast<int>(graphData.type);
            if (ImGui::Combo("##Tipo", &currentType, tipos, IM_ARRAYSIZE(tipos))) {
                graphData.type = static_cast<GraphType>(currentType);
            }
            if (ImGui::Button("Remover Gráfico")) {
                graphToRemove = graphIndex;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Remover Eixos", &graphData.hideAxes);
            ImGui::SeparatorText("Colunas");
            for (size_t j = 0; j < graphData.columns.size(); j++) {
                std::string btnLabel = "X##" + std::to_string(j);
                if (ImGui::Button(btnLabel.c_str())) {
                    graphData.columns.erase(graphData.columns.begin() + j);
                    break;
                }
                ImGui::SameLine();
                ImGui::Text("%s", graphData.columns[j].c_str());
            }
            ImPlot::EndLegendPopup();
        }
    }
}

void Window::Plot::renderGraph(size_t graphIndex, int& graphToRemove) {
    GraphData& graphData = graphs[graphIndex];
    ImGui::PushID(static_cast<int>(graphIndex));

    std::string plotID = "##Plot " + std::to_string(graphIndex);
    if (ImPlot::BeginPlot(plotID.c_str(), ImVec2(-1, plotHeight), ImPlotFlags_NoFrame)) {

        // Configura os eixos com base nas opções de exibição
        ImPlotAxisFlags axisFlags = 0;
        if (graphData.hideAxes) {
            axisFlags |= ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
        }
        if (autoFit) {
            axisFlags |= ImPlotAxisFlags_AutoFit;
        }
        ImPlot::SetupAxes(nullptr, nullptr, axisFlags, axisFlags);

        // Plotagem de cada coluna conforme o tipo do gráfico
        for (size_t i = 0; i < graphData.columns.size(); i++) {
            const std::string&         col       = graphData.columns[i];
            const std::vector<double>& x         = graphData.x[i];
            const std::vector<double>& y         = graphData.y[i]; // Corrigido: usa os dados de y
            int                        numPoints = static_cast<int>(x.size());

            switch (graphData.type) {
                case GRAPH_LINE:
                    ImPlot::PlotLine(col.c_str(), x.data(), y.data(), numPoints);
                    break;
                case GRAPH_BAR:
                    ImPlot::PlotBars(col.c_str(), x.data(), y.data(), numPoints, 0.8f);
                    break;
                case GRAPH_SCATTER:
                    ImPlot::PlotScatter(col.c_str(), x.data(), y.data(), numPoints);
                    break;
                case GRAPH_FILLED_LINE:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotShaded(col.c_str(), x.data(), y.data(), numPoints);
                    ImPlot::PlotLine(col.c_str(), x.data(), y.data(), numPoints);
                    ImPlot::PopStyleVar();
                    break;
                default:
                    break;
            }
        }

        // Renderiza o popup de legenda para controle do gráfico
        drawLegendPopup(graphData, static_cast<int>(graphIndex), graphToRemove);
        ImPlot::EndPlot();
    }

    // Processa o payload de drag & drop para as colunas
    processColumnDragDrop(graphData);
    ImGui::PopID();
}

void Window::Plot::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        drawMenuBar();

        int graphToRemove = -1;

        if (ImPlot::BeginAlignedPlots("AlignedGroup")) {
            for (size_t i = 0; i < graphs.size(); i++) {
                renderGraph(i, graphToRemove);
            }
            ImPlot::EndAlignedPlots();
        }

        if (graphToRemove != -1) {
            graphs.erase(graphs.begin() + graphToRemove);
        }

        ImGui::End();
    }
}
