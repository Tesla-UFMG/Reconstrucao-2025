#include "ui/windows/w_Plot.hpp"

// Variáveis globais para os gráficos e configurações
static std::vector<GraphData> graphs;
static bool                   autoFit          = true;
static bool                   showResizeButton = false;

Window::Plot::Plot(bool* isOpen) : IWindow(isOpen) {
    title = "Plot";
    flags = ImGuiWindowFlags_MenuBar;
}

void Window::Plot::drawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Novo Gráfico")) {
            graphs.emplace_back(GraphData());
            LOG("INFO", "Gráfico " + std::to_string(graphs.size() - 1) + " criado.");
        }

        if (ImGui::BeginMenu("Configurações")) {
            if (ImGui::MenuItem("Auto Fit", nullptr, &autoFit)) {
                LOG("DEBUG", "Botão Auto Fit gráfico " + std::string(autoFit ? "ativado." : "desativado."));
            }

            if (ImGui::MenuItem("Redimensionar", nullptr, &showResizeButton)) {
                LOG("DEBUG",
                    "Botão de redimensionamento gráfico " + std::string(showResizeButton ? "ativado." : "desativado."));
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Mudar Cores")) {
                ImPlotContext&  gp       = *GImPlot;
                ImPlotColormap& colormap = gp.Style.Colormap;

                if (ImPlot::ColormapButton(ImPlot::GetColormapName(colormap), ImVec2(225, 0), colormap)) {
                    colormap = (colormap + 1) % ImPlot::GetColormapCount();
                    ImPlot::BustItemCache();
                }

                ImPlot::ShowColormapSelector("##");

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Window::Plot::processColumnDragDrop(GraphData& graphData) {
    if (ImGui::BeginDragDropTarget()) {
        // Aceita o payload
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       archiveName, columnName;

            // Pega o nome do arquivo e a coluna
            if (std::getline(ss, archiveName, ':') && std::getline(ss, columnName, ':')) {

                // Verifica se o a coluna do arquivo ja foi adicionadda
                for (size_t i = 0; i < graphData.archives.size(); ++i) {
                    if (graphData.archives[i] == archiveName && graphData.columns[i] == columnName) {
                        LOG("WARN", "Gráfico " + std::to_string(i) + ": A coluna " + columnName + " do arquivo " + archiveName + " já existe.");
                        ImGui::EndDragDropTarget();
                        return;
                    }
                }
            
                // Adiciona a coluna e o nome do arquivo
                graphData.columns.push_back(columnName);
                graphData.archives.push_back(archiveName);
                

                // Adiciona os eixos
                std::vector<double> y = DB::getInstance().getCSVData(archiveName, columnName);
                graphData.y.push_back(y);

                std::vector<double> x(y.size());
                for (size_t i = 0; i < y.size(); ++i) {
                    x[i] = static_cast<double>(i);
                }
                graphData.x.push_back(x);

                LOG("DEBUG",
                    "Coluna " + columnName + " adicionada ao gráfico " + std::to_string(graphs.size() - 1) + ".");
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Plot::drawLegendPopup(GraphData& graphData, int graphIndex, int& graphToRemove) {
    if (!graphData.columns.empty()) {
        const char* tipos[] = {"Linha", "Barra", "Scatter", "Preenchido"};
        if (ImPlot::BeginLegendPopup(graphData.columns[0].c_str())) {

            // Tipo do gráfico
            int currentType = static_cast<int>(graphData.type);
            if (ImGui::Combo("##Tipo", &currentType, tipos, IM_ARRAYSIZE(tipos))) {
                graphData.type = static_cast<GraphType>(currentType);
                LOG("DEBUG", "Gráfico " + std::to_string(graphIndex) + " alterado para " + tipos[currentType] + ".");
            }

            // Remover gráfico
            if (ImGui::Button("Remover Gráfico")) {
                graphToRemove = graphIndex;
            }

            // Configurações de exibição
            ImGui::SeparatorText("Removevr Eixos");
            ImGui::Checkbox("Eixo X", &graphData.showXAxis);
            ImGui::SameLine();
            ImGui::Checkbox("Eixo Y", &graphData.showYAxis);

            // Colunas
            ImGui::SeparatorText("Remover Colunas");
            for (size_t j = 0; j < graphData.columns.size(); j++) {
                std::string btnLabel = "X##" + std::to_string(j);
                if (ImGui::Button(btnLabel.c_str())) {

                    // Remove com os respectivos indexes
                    graphData.archives.erase(graphData.archives.begin() + j);
                    graphData.columns.erase(graphData.columns.begin() + j);
                    graphData.x.erase(graphData.x.begin() + j);
                    graphData.y.erase(graphData.y.begin() + j);

                    LOG("DEBUG",
                        "Coluna " + graphData.columns[j] + " removida do gráfico " + std::to_string(graphIndex) + ".");

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
    if (ImPlot::BeginPlot(plotID.c_str(), ImVec2(-1, graphData.plotHeight), ImPlotFlags_NoFrame)) {

        ImPlotAxisFlags xAxisFlags = 0;
        ImPlotAxisFlags yAxisFlags = 0;
 
        if (!graphData.showXAxis) {
            xAxisFlags |= ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
        }
        if (!graphData.showYAxis) {
            yAxisFlags |= ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
        }
        if (autoFit) {
            xAxisFlags |= ImPlotAxisFlags_AutoFit;
            yAxisFlags |= ImPlotAxisFlags_AutoFit;
        }

        ImPlot::SetupAxes(nullptr, nullptr, xAxisFlags, yAxisFlags);

        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);

        // Plotagem de cada coluna conforme o tipo do gráfico
        for (size_t i = 0; i < graphData.columns.size(); i++) {
            const std::string&         col       = graphData.columns[i];
            const std::vector<double>& x         = graphData.x[i];
            const std::vector<double>& y         = graphData.y[i]; 
            int                        numPoints = static_cast<int>(x.size());

            switch (graphData.type) {
                case GRAPH_LINE:
                    ImPlot::PlotLine(col.c_str(), x.data(), y.data(), numPoints);
                    break;
                case GRAPH_BAR:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotBars(col.c_str(), x.data(), y.data(), numPoints, 0.8f);
                    break;
                case GRAPH_SCATTER:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
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

void Window::Plot::renderResizeButton(size_t graphIndex) {
    const float resizeAreaHeight = 5.0f;

    GraphData& graphData = graphs[graphIndex];

    ImGui::Dummy(ImVec2(0, resizeAreaHeight));
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::PushID(static_cast<int>(graphIndex));
    ImGui::Button("PlotResize", ImVec2(avail.x, resizeAreaHeight));

    // Se a área estiver ativa e o mouse estiver sendo arrastado, atualiza a altura.
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float delta = ImGui::GetMouseDragDelta(0).y;
        // Atualiza a altura, garantindo limites mínimo e máximo.
        graphData.plotHeight = std::max(50.0, std::min(graphData.plotHeight + delta, 1600.0));
        ImGui::ResetMouseDragDelta();
    }

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

                if (showResizeButton) {
                    renderResizeButton(i);
                }
            }
            ImPlot::EndAlignedPlots();
        }

        if (graphToRemove != -1) {
            graphs.erase(graphs.begin() + graphToRemove);
            LOG("INFO", "Gráfico " + std::to_string(graphToRemove) + " removido.");
        }

        ImGui::End();
    }
}
