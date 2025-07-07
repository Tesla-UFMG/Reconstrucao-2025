#include "ui/windows/w_Plot.hpp"

// Variáveis globais para os gráficos e configurações
static std::vector<GraphData> graphs;
static bool                   autoFit          = true;
static bool                   showResizeButton = false;

Window::Plot::Plot(bool* isOpen) : IWindow(isOpen) {
    title = "Plot";
    flags = ImGuiWindowFlags_MenuBar;
}

void Window::Plot::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        drawMenuBar();

        if (ImPlot::BeginAlignedPlots("AlignedGroup")) {
            for (size_t i = 0; i < graphs.size(); i++) {
                renderGraph(i);
                if (showResizeButton) {
                    renderResizeButton(i);
                }
            }
            ImPlot::EndAlignedPlots();
        }

        ImGui::End();
    }
}

void Window::Plot::drawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Novo Gráfico")) {
            this->addGraph(graphs);
        }

        if (graphs.size() > 0 && ImGui::BeginMenu("Remover Gráfico")) {
            for (size_t i = 0; i < graphs.size(); i++) {
                if (ImGui::MenuItem(("Gráfico " + std::to_string(i)).c_str())) {
                    this->removeGraph(graphs, i);
                }
            }
            ImGui::EndMenu();
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

            MenuBar::changeColorMap();

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Window::Plot::addGraph(std::vector<GraphData>& graphs) {
    graphs.emplace_back(GraphData());
    ImPlot::BustItemCache();
    LOG("INFO", "Gráfico " + std::to_string(graphs.size() - 1) + " criado.");
}

void Window::Plot::removeGraph(std::vector<GraphData>& graphs, size_t graphIndex) {
    if (graphIndex >= graphs.size()) {
        LOG("ERROR", "Não foi possível remover o gráfico, índice inválido.");
        return;
    }

    graphs.erase(graphs.begin() + graphIndex);
    ImPlot::BustItemCache();
    LOG("INFO", "Gráfico " + std::to_string(graphIndex) + " removido.");
}

void Window::Plot::processColumnDragDrop(GraphData& graphData) {
    if (ImGui::BeginDragDropTarget()) {
        // Aceita o payload
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       filename, columnName;

            // Pega o nome do arquivo e a coluna
            if (std::getline(ss, filename, ':') && std::getline(ss, columnName, ':')) {
                this->addColumnToGraph(graphData, filename, columnName);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Plot::addColumnToGraph(GraphData& graphData, const std::string& filename, const std::string& columnName) {
    // Verifica se a coluna do arquivo já foi adicionada
    for (size_t i = 0; i < graphData.archives.size(); ++i) {
        if (graphData.archives[i] == filename && graphData.columns[i] == columnName) {
            LOG("WARN", "Gráfico " + std::to_string(i) + ": A coluna " + columnName + " do arquivo " + filename +
                            " já existe.");
            return;
        }
    }

    // Adiciona os eixos
    std::vector<double> y = DB::getInstance().getCSVData(filename, columnName);
    if (y.size() == 0) {
        LOG("ERROR", "Não foi possível adicionar a coluna " + columnName + " do arquivo " + filename + " ao gráfico.");
        return;
    }
    graphData.y.push_back(y);

    std::vector<double> x(y.size());
    for (size_t i = 0; i < y.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
    graphData.x.push_back(x);

    // Adiciona a coluna, o nome do arquivo e o multiplicador padrão (1.0)
    graphData.columns.push_back(columnName);
    graphData.archives.push_back(filename);
    graphData.multiplier.push_back(1.0);

    ImPlot::BustItemCache();
    LOG("DEBUG", "Coluna " + columnName + " adicionada ao gráfico " + std::to_string(graphs.size() - 1) + ".");
}
void Window::Plot::drawLegendPopup(GraphData& graphData, int graphIndex) {
    if (!graphData.columns.empty()) {
        const char* tipos[] = {"Linha", "Barra", "Scatter", "Preenchido"};

        bool popupOpen = false;
        for (const auto& col : graphData.columns) {
            if (ImPlot::BeginLegendPopup(col.c_str())) {
                popupOpen = true;
                break;
            }
        }
        if (popupOpen) {

            // Tipo do gráfico
            ImGui::SeparatorText("Tipo de Gráfico");
            int currentType = static_cast<int>(graphData.type);
            if (ImGui::Combo("##Tipo", &currentType, tipos, IM_ARRAYSIZE(tipos))) {
                graphData.type = static_cast<GraphType>(currentType);
                LOG("DEBUG", "Gráfico " + std::to_string(graphIndex) + " alterado para " + tipos[currentType] + ".");
            }

            // Configurações de exibição
            ImGui::SeparatorText("Eixos");
            ImGui::Checkbox("Eixo X", &graphData.showXAxis);
            ImGui::SameLine();
            ImGui::Checkbox("Eixo Y", &graphData.showYAxis);

            // Seleção do eixo X (com opção "Nenhuma")
            if (ImGui::BeginCombo("##EixoX", graphData.xColumn.empty() ? "Nenhuma" : graphData.xColumn.c_str())) {
                if (ImGui::Selectable("Nenhuma", graphData.xColumn.empty())) {
                    graphData.xColumn.clear();
                }
                for (const auto& column : graphData.columns) {
                    if (ImGui::Selectable(column.c_str(), graphData.xColumn == column)) {
                        graphData.xColumn = column;
                        ImPlot::BustItemCache();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SeparatorText("Colunas");
            if (ImGui::BeginTable("TabelaColunas", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Coluna", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Multiplicador", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t j = 0; j < graphData.columns.size(); j++) {
                    if (graphData.columns[j] == graphData.xColumn)
                        continue;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    std::string btnLabel = "X##" + std::to_string(j);
                    if (ImGui::Button(btnLabel.c_str())) {
                        this->removeColumnFromGraph(graphData, j);
                        break;
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(graphData.columns[j].c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushItemWidth(120.0f);
                    ImGui::InputDouble(("##mult" + std::to_string(j)).c_str(), &graphData.multiplier[j], 0.001, 100.0,
                                       "%.15gx");
                    ImGui::PopItemWidth();
                }
                ImGui::EndTable();
            }

            if (ImGui::Button("Remover Gráfico")) {
                this->removeGraph(graphs, graphIndex);
            }

            ImPlot::EndLegendPopup();
        }
    }
}

void Window::Plot::removeColumnFromGraph(GraphData& graphData, int graphIndex) {
    if (graphIndex < 0 || graphIndex >= static_cast<int>(graphData.columns.size())) {
        LOG("ERROR", "Não foi possível remover o gráfico, índice inválido.");
        return;
    }

    graphData.archives.erase(graphData.archives.begin() + graphIndex);
    graphData.columns.erase(graphData.columns.begin() + graphIndex);
    graphData.x.erase(graphData.x.begin() + graphIndex);
    graphData.y.erase(graphData.y.begin() + graphIndex);
    graphData.multiplier.erase(graphData.multiplier.begin() + graphIndex);

    if (graphData.xColumn == graphData.columns[graphIndex]) {
        graphData.xColumn.clear();
    }

    ImPlot::BustItemCache();

    LOG("DEBUG", "Coluna removida do gráfico " + std::to_string(graphIndex) + ".");
}

void Window::Plot::renderGraph(size_t graphIndex) {
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

        // Se uma coluna foi selecionada como eixo X, obtém seus dados.
        std::vector<double> customX;
        bool                useCustomX = false;
        if (!graphData.xColumn.empty()) {
            auto it = std::find(graphData.columns.begin(), graphData.columns.end(), graphData.xColumn);
            if (it != graphData.columns.end()) {
                size_t index = std::distance(graphData.columns.begin(), it);
                if (index < graphData.y.size() && !graphData.y[index].empty()) {
                    customX    = graphData.y[index];
                    useCustomX = true;
                }
            }
        }

        // Plota cada série, exceto a coluna selecionada como eixo X.
        for (size_t i = 0; i < graphData.columns.size(); i++) {
            if (!graphData.xColumn.empty() && graphData.columns[i] == graphData.xColumn)
                continue;

            const std::string&         col       = graphData.columns[i];
            const std::vector<double>& y         = graphData.y[i];
            int                        numPoints = static_cast<int>(y.size());
            std::vector<double>        scaledY(numPoints);
            double                     multiplier = graphData.multiplier[i];
            for (int j = 0; j < numPoints; ++j) {
                scaledY[j] = y[j] * multiplier;
            }

            // Define qual vetor de X será usado.
            const std::vector<double>* xData = nullptr;
            if (useCustomX && customX.size() == y.size())
                xData = &customX;
            else
                xData = &graphData.x[i];

            // Plota a série usando o vetor de X selecionado.
            switch (graphData.type) {
                case GRAPH_LINE:
                    ImPlot::PlotLine(col.c_str(), xData->data(), scaledY.data(), numPoints);
                    break;
                case GRAPH_BAR:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotBars(col.c_str(), xData->data(), scaledY.data(), numPoints, 0.8f);
                    ImPlot::PopStyleVar();
                    break;
                case GRAPH_SCATTER:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotScatter(col.c_str(), xData->data(), scaledY.data(), numPoints);
                    ImPlot::PopStyleVar();
                    break;
                case GRAPH_FILLED_LINE:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotShaded(col.c_str(), xData->data(), scaledY.data(), numPoints);
                    ImPlot::PlotLine(col.c_str(), xData->data(), scaledY.data(), numPoints);
                    ImPlot::PopStyleVar();
                    break;
                default:
                    break;
            }
        }

        drawLegendPopup(graphData, static_cast<int>(graphIndex));
        ImPlot::EndPlot();
    }

    processColumnDragDrop(graphData);
    ImGui::PopID();
}

void Window::Plot::renderResizeButton(size_t graphIndex) {
    GraphData& graphData = graphs[graphIndex];

    ImGui::Dummy(ImVec2(0, RESIZE_BAR_SIZE));
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::PushID(static_cast<int>(graphIndex));
    ImGui::Button("PlotResize", ImVec2(avail.x, RESIZE_BAR_SIZE));

    // Se a área estiver ativa e o mouse estiver sendo arrastado, atualiza a altura.
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float delta = ImGui::GetMouseDragDelta(0).y;
        // Atualiza a altura, garantindo limites mínimo e máximo.
        graphData.plotHeight = std::max(MIN_GRAPH_SIZE, std::min(graphData.plotHeight + delta, MAX_GRAPH_SIZE));
        ImGui::ResetMouseDragDelta();
    }

    ImGui::PopID();
}