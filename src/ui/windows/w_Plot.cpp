#include "ui/windows/w_Plot.hpp"
#include "ui/menubar/m_Utils.hpp"

Window::Plot::Plot(bool* isOpen) : IWindow(isOpen) {
    title = "Plot";
    flags = ImGuiWindowFlags_MenuBar;
}

void Window::Plot::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->drawMenuBar();

        if (ImPlot::BeginAlignedPlots("AlignedGroup")) {
            for (size_t graphIndex = 0; graphIndex < this->graphs.size(); graphIndex++) {
                renderGraph(graphIndex);
                if (this->showResizeButton) {
                    renderResizeButton(graphIndex);
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
            this->addNewGraph();
        }

        if (graphs.size() > 0 && ImGui::BeginMenu("Remover Gráfico")) {
            for (size_t graphIndex = 0; graphIndex < graphs.size(); graphIndex++) {
                size_t graphId = graphs[graphIndex].config.id;
                if (ImGui::MenuItem(("Gráfico " + std::to_string(graphId)).c_str())) {
                    this->removeGraph(graphIndex);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Configurações")) {
            if (ImGui::MenuItem("Auto Fit", nullptr, &this->autoFit)) {
                for (::Graph& graph : this->graphs) {
                    graph.config.autoFit = this->autoFit;
                }
                LOG("DEBUG", "Botão Auto Fit gráfico " + std::string(this->autoFit ? "ativado." : "desativado."));
            }

            if (ImGui::MenuItem("Redimensionar", nullptr, &this->showResizeButton)) {
                LOG("DEBUG", "Botão de redimensionamento gráfico " +
                                 std::string(this->showResizeButton ? "ativado." : "desativado."));
            } 

            if (ImGui::MenuItem("Exibir Valor no Eixo Y", nullptr, &this->showValueOnYAxis)) {
                for (::Graph& graph : this->graphs) {
                    graph.config.showValueOnYAxis = this->showValueOnYAxis;
                }
                LOG("DEBUG", "Botão de Exibir Valor no Eixo Y " +
                                 std::string(this->showValueOnYAxis ? "ativado." : "desativado."));
            }

            if (ImGui::MenuItem("Exibir Cursor no Eixo X/Y", nullptr, &this->showCursorOnYAxis)) {
                for (::Graph& graph : this->graphs) {
                    graph.config.showCursorOnYAxis = this->showCursorOnYAxis;
                }
                LOG("DEBUG", "Botão de Exibir Cursor no Eixo X/Y " +
                                 std::string(this->showCursorOnYAxis ? "ativado." : "desativado."));
            }


            if (ImGui::MenuItem("Modo Telemetria", nullptr, &this->telemetryMode)) {
                size_t numOfGraphs = this->graphs.size();
 
                for (size_t i = 0; i < numOfGraphs; ++i) {
                    GraphConfig& graphConfig = this->graphs[i].config;
                    graphConfig.followTheEnd = this->telemetryMode;
                    graphConfig.showXAxis    = false;
                    graphConfig.autoFit = this->telemetryMode;
                    if (i == numOfGraphs - 1) {
                        graphConfig.showXAxis    = true;
                        continue;
                    }
                }
                LOG("DEBUG", "Botão modo telemetria " +
                                 std::string(this->showValueOnYAxis ? "ativado." : "desativado."));
            }

            

            ImGui::Separator();
            MenuBar::changePlotColormap();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::Plot::addNewGraph() {
    ::Graph  newGraph;
    size_t graphId     = this->graphs.size() ? this->graphs.back().config.id + 1 : 0;
    newGraph.config.id = graphId; // Configura o ID
    this->graphs.push_back(newGraph);
    ImPlot::BustItemCache();
    LOG("INFO", "Gráfico " + std::to_string(graphId) + " criado.");
}

void Window::Plot::removeGraph(size_t graphIndex) {
    std::cout << graphIndex << std::endl;
    std::cout << this->graphs.size() << std::endl;
    if (graphIndex >= this->graphs.size()) {
        LOG("ERROR", "Não foi possível remover o gráfico, índice inválido.");
        return;
    }

    this->graphs.erase(this->graphs.begin() + graphIndex);
    ImPlot::BustItemCache();
    LOG("INFO", "Gráfico " + std::to_string(graphIndex) + " removido.");
}

void Window::Plot::processColumnDragDrop(::Graph& graph) {

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            this->addColumnToGraph(graph, columnPayload);
        } else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVE_NAME")) {
            const ArchivePayload* archivePayload = reinterpret_cast<const ArchivePayload*>(payload->Data);
            //  this->addColumnToGraph(graphData, columnPayload);
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Plot::addColumnToGraph(::Graph& graph, const ColumnPayload* payload) {
    std::string fileType   = payload->fileType;
    std::string fileName   = payload->fileName;
    std::string columnName = payload->columnName;

    if (fileType == "Text") {
        GraphTextAnnotation ann;
        ann.archiveName = fileName;
        ann.columnName = columnName;
        
        bool exists = false;
        for (const auto& a : graph.textAnnotations) {
            if (a.archiveName == fileName && a.columnName == columnName) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            graph.textAnnotations.push_back(ann);
            LOG("INFO", "Anotação de texto '" + columnName + "' de '" + fileName + "' adicionada ao gráfico.");
        }
        return;
    }

    // Verifica se o arquivo já está no gráfico
    for (const GraphData& graphData : graph.data) {
        if (graphData.fileName == fileName && graphData.columnName == columnName) {
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já está no gráfico.");
            return;
        }
    }

    // Cria um novo GraphData
    GraphData graphData;
    graphData.columnName = columnName;
    graphData.fileName   = fileName;
    graphData.fileType   = fileType;

    // Adiciona os eixos
    if (fileType == "CSV") {
        graphData.y = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        graphData.y = &DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (graphData.y->empty()) {
        Dialogs::showErrorDialog("Não é possível adicionar uma coluna vazia ao gráfico!");
        LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " está vazia.");
        return;
    }

    graphData.buildXVector();
    graph.data.push_back(graphData);

    ImPlot::BustItemCache();
    LOG("DEBUG", "Coluna " + columnName + " adicionada ao gráfico " + std::to_string(graph.config.id) + ".");
}

void Window::Plot::drawLegendPopup(::Graph& graph, size_t graphIndex) {
    if (!graph.data.empty()) {
        const char* graphTypes[] = {"Linha", "Barra", "Scatter", "Preenchido"};

        bool popupOpen = false;
        for (const std::string& col : graph.getColumnNames()) {
            if (ImPlot::BeginLegendPopup(col.c_str())) {
                popupOpen = true;
                break;
            }
        }

        if (popupOpen) {
            // Mudar tipo do gráfico
            ImGui::SeparatorText("Tipo de Gráfico");
            int currentType = static_cast<int>(graph.config.type);
            if (ImGui::Combo("##Tipo", &currentType, graphTypes, IM_ARRAYSIZE(graphTypes))) {
                graph.config.type = static_cast<GraphType>(currentType);
                LOG("DEBUG",
                    "Gráfico " + std::to_string(graph.config.id) + " alterado para " + graphTypes[currentType] + ".");
            }

            // Tipo de exibição
            ImGui::SeparatorText("Exibição");
            ImGui::Checkbox("Auto Fit", &graph.config.autoFit);
            ImGui::SameLine();
            ImGui::Checkbox("Seguir o final", &graph.config.followTheEnd);
            if (graph.config.followTheEnd) {
                ImGui::InputInt("Pontos", &graph.config.numPoints, 1, 10);
            }
            ImGui::Checkbox("Exibir Valor no Eixo Y", &graph.config.showValueOnYAxis);

            // Mostrar ou esconder os eixos
            ImGui::SeparatorText("Eixos");
            ImGui::Checkbox("Eixo X", &graph.config.showXAxis);
            ImGui::SameLine();
            ImGui::Checkbox("Eixo Y", &graph.config.showYAxis);

            // Escolher eixo X em função de uma coluna
            if (ImGui::BeginCombo("##EixoX", graph.config.xColumn.empty() ? "Nenhuma" : graph.config.xColumn.c_str())) {
                if (ImGui::Selectable("Nenhuma", graph.config.xColumn.empty())) {
                    graph.config.xColumn.clear();
                }
                for (const std::string& col : graph.getColumnNames()) {
                    if (ImGui::Selectable(col.c_str(), graph.config.xColumn == col)) {
                        graph.config.xColumn = col;
                        ImPlot::BustItemCache();
                    }
                }
                ImGui::EndCombo();
            }

            {
                ImGui::SeparatorText("Alinhamento Temporal / XY");
                const char* alignmentModes[] = {
                    "Tamanho Mínimo",
                    "Proximidade Temporal",
                    "Interpolação Linear (Técnico)"
                };
                int currentMode = static_cast<int>(graph.config.xyAlignmentMode);
                ImGui::SetNextItemWidth(180.0f);
                if (ImGui::Combo("Alinhamento##XY", &currentMode, alignmentModes, IM_ARRAYSIZE(alignmentModes))) {
                    graph.config.xyAlignmentMode = static_cast<XYAlignmentMode>(currentMode);
                    ImPlot::BustItemCache();
                }
            }

            // Configuração de colunas
            ImGui::SeparatorText("Colunas");
            if (ImGui::BeginTable("TabelaColunas", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Coluna", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Multiplicador", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (size_t columnIndex = 0; columnIndex < graph.data.size(); columnIndex++) {
                    GraphData& graphData = graph.data[columnIndex];
                    if (graphData.columnName == graph.config.xColumn)
                        continue;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    std::string btnLabel = "X##" + std::to_string(columnIndex);
                    if (ImGui::Button(btnLabel.c_str())) {
                        this->removeColumnFromGraph(graphIndex, columnIndex);
                        break;
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(graphData.columnName.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushItemWidth(120.0f);
                    ImGui::InputDouble(("##mult" + std::to_string(columnIndex)).c_str(), &graphData.multiplier, 0.001,
                                       100.0, "%.15gx");
                    ImGui::PopItemWidth();
                }
                ImGui::EndTable();
            }

            if (ImGui::Button("Remover Gráfico")) {
                this->removeGraph(graphIndex);
            }

            ImPlot::EndLegendPopup();
        }
    }
}

void Window::Plot::removeColumnFromGraph(size_t graphIndex, size_t columnIndex) {
    if (graphIndex >= this->graphs.size() || columnIndex >= this->graphs[graphIndex].data.size()) {
        LOG("ERROR", "Não foi possível remover a coluna do gráfico, índice inválido.");
        return;
    }
    this->graphs[graphIndex].data.erase(this->graphs[graphIndex].data.begin() + columnIndex);
    ImPlot::BustItemCache();
    LOG("DEBUG", "Coluna removida do gráfico " + std::to_string(graphIndex) + ".");
}

void Window::Plot::renderGraph(size_t graphIndex) {
    ::Graph&       graph       = this->graphs[graphIndex];
    GraphConfig& graphConfig = graph.config;

    // Se uma coluna foi selecionada como eixo X, obtém seus dados.
    std::vector<double> customX;
    bool                useCustomX = false;
    if (!graphConfig.xColumn.empty()) {
        for (const GraphData& graphData : graph.data) {
            if (graphData.columnName == graphConfig.xColumn) {
                customX    = graphData.getYData();
                useCustomX = true;
                break;
            }
        }
    }
    std::string title = useCustomX ? "Gráfico vs " + graphConfig.xColumn : "";
    if (ImPlot::BeginPlot((title + "##Plot " + std::to_string(graphIndex)).c_str(), ImVec2(-1, graphConfig.plotHeight),
                          ImPlotFlags_NoFrame)) {
 
        // Define as flags dos eixos
        ImPlotAxisFlags xAxisFlags = !graphConfig.showXAxis ? ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks |
                                                                  ImPlotAxisFlags_NoTickLabels
                                                            : ImPlotAxisFlags_None;
 
        ImPlotAxisFlags yAxisFlags = !graphConfig.showYAxis ? ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks |
                                                                  ImPlotAxisFlags_NoTickLabels
                                                            : ImPlotAxisFlags_None;
 
        yAxisFlags |= ImPlotAxisFlags_Opposite;
 
        if (graphConfig.autoFit) {
            xAxisFlags |= ImPlotAxisFlags_AutoFit;
            yAxisFlags |= ImPlotAxisFlags_AutoFit;
        }
 
        if (graphConfig.followTheEnd) {
            xAxisFlags |= ImPlotAxisFlags_AutoFit;
        }
 
        ImPlot::SetupAxes(nullptr, nullptr, xAxisFlags, yAxisFlags);
 
        size_t axisLength = 0;
        if (useCustomX) {
            axisLength = customX.size();
        } else if (!graph.data.empty()) {
            axisLength = graph.data[0].getYData().size();
            for (const GraphData& graphData : graph.data) {
                if (graphData.getYData().size() > axisLength) {
                    axisLength = graphData.getYData().size();
                }
            }
        }
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);
 
        // Plota cada coluna
        for (size_t j = 0; j < graph.data.size(); j++) {
            GraphData& graphData = graph.data[j];
            // Se a coluna for a do eixo X, pula
            if (!graphConfig.xColumn.empty() && graphData.columnName == graphConfig.xColumn)
                continue;
 
            const std::vector<double>& y = graphData.getYData();
            std::vector<double>        yData(y.size());
            if (graphData.multiplier == 1.0) {
                yData = y;
            } else {
                for (size_t j = 0; j < y.size(); ++j) {
                    yData[j] = y[j] * graphData.multiplier;
                }
            }

            std::vector<double> xData;
            std::vector<double> alignedY;

            if (useCustomX) {
                auto aligned = alignVectors(customX, yData, graphConfig.xyAlignmentMode);
                xData = aligned.first;
                alignedY = aligned.second;
            } else {
                std::vector<double> baseGrid(axisLength);
                for (size_t i = 0; i < axisLength; ++i) {
                    baseGrid[i] = static_cast<double>(i);
                }
                auto aligned = alignVectors(baseGrid, yData, graphConfig.xyAlignmentMode);
                xData = aligned.first;
                alignedY = aligned.second;
            }

            int safeSize = static_cast<int>(std::min(xData.size(), alignedY.size()));
            int colStart = graphConfig.followTheEnd ? std::max(0, safeSize - graphConfig.numPoints) : 0;
            int numPoints = graphConfig.followTheEnd ? std::min(safeSize, graphConfig.numPoints) : safeSize;

            const double* xPtr = xData.data() + colStart;
            const double* yPtr = alignedY.data() + colStart;

            // Plota
            const char* columnName = graphData.columnName.c_str();
            switch (graphConfig.type) {
                case GRAPH_LINE:
                    ImPlot::PlotLine(columnName, xPtr, yPtr, numPoints);
                    break;
                case GRAPH_BAR:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotBars(columnName, xPtr, yPtr, numPoints, 0.8f);
                    ImPlot::PopStyleVar();
                    break;
                case GRAPH_SCATTER:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotScatter(columnName, xPtr, yPtr, numPoints);
                    ImPlot::PopStyleVar();
                    break;
                case GRAPH_FILLED_LINE:
                    ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                    ImPlot::PlotShaded(columnName, xPtr, yPtr, numPoints);
                    ImPlot::PlotLine(columnName, xPtr, yPtr, numPoints);
                    ImPlot::PopStyleVar();
                    break;
                default:
                    break;
            }

            if (graph.config.showValueOnYAxis) {
                // Cria a formatação da TAG para evitar ficar balangando o gráfico
                ImPlotRect limits = ImPlot::GetPlotLimits();
                double     yMin   = limits.Y.Min;
                double     yMax   = limits.Y.Max;
                char       bufMin[32], bufMax[32];
                int        prec   = 2;
                int        lenMin = snprintf(bufMin, sizeof(bufMin), "%.*f", prec, yMin);
                int        lenMax = snprintf(bufMax, sizeof(bufMax), "%.*f", prec, yMax);
                int        maxLen = (lenMin > lenMax) ? lenMin : lenMax;
                char       fmt[16];
                snprintf(fmt, sizeof(fmt), "%%%d.%df", maxLen, prec);

                // Pega a cor e o ultimo valor
                ImVec4 lineColor    = ImPlot::GetColormapColor(j);
                double currentValue = yData.back();

                ImPlot::TagY(currentValue, // posição da tag
                             lineColor,    // cor
                             fmt,          // formatação
                             currentValue  // valor que será jogado para a formatação
                );
            }


            if (graph.config.showCursorOnYAxis) {
                if (ImPlot::IsPlotHovered()) {
                    ImPlotPoint mouse = ImPlot::GetPlotMousePos(); // coordenadas no sistema do plot
                    double cursorX = mouse.x;
                    double cursorY = mouse.y;

                    // formatação (mantive a sua lógica)
                    ImPlotRect limits = ImPlot::GetPlotLimits();
                    double     yMin   = limits.Y.Min;
                    double     yMax   = limits.Y.Max;
                    char       bufMin[32], bufMax[32];
                    int        prec   = 2;
                    int        lenMin = snprintf(bufMin, sizeof(bufMin), "%.*f", prec, yMin);
                    int        lenMax = snprintf(bufMax, sizeof(bufMax), "%.*f", prec, yMax);
                    int        maxLen = (lenMin > lenMax) ? lenMin : lenMax;
                    char       fmt[32];
                    snprintf(fmt, sizeof(fmt), "%%%d.%df", maxLen, prec);

                    ImVec4 lineColor = ImPlot::GetColormapColor(j);

                    // marca no eixo Y e no eixo X onde está o cursor
                    ImPlot::TagY(cursorY, HI(1), fmt, cursorY);
                    ImPlot::TagX(cursorX, HI(1), fmt, cursorX);
                }
            }
        }

        // Renderizar anotações textuais (Comentários e Avisos)
        if (!graph.textAnnotations.empty()) {
            // Localizar datas para sincronização temporal
            const std::vector<std::string>* plotDates = nullptr;
            for (const auto& tf : DB::getInstance().getProject().getTelemetryFiles()) {
                if (!tf.getDate().empty()) {
                    plotDates = &tf.getDate();
                    break;
                }
            }
            
            if (plotDates && !plotDates->empty()) {
                ImPlotRect limits = ImPlot::GetPlotLimits();
                
                for (const auto& ann : graph.textAnnotations) {
                    const TextFile* targetTF = nullptr;
                    for (const auto& tf : DB::getInstance().getProject().getTextFiles()) {
                        if (tf.getName() == ann.archiveName) {
                            targetTF = &tf;
                            break;
                        }
                    }
                    
                    if (targetTF) {
                        const auto& dates = targetTF->getDates();
                        const auto& data = targetTF->getData();
                        
                        // Localizar o índice da coluna
                        int colIdx = -1;
                        for (size_t c = 0; c < targetTF->getColumnNames().size(); ++c) {
                            if (targetTF->getColumnNames()[c] == ann.columnName) {
                                colIdx = static_cast<int>(c);
                                break;
                            }
                        }
                        
                        if (colIdx != -1 && colIdx < static_cast<int>(data.size())) {
                            const auto& colData = data[colIdx];
                            
                            for (size_t row = 0; row < dates.size(); ++row) {
                                if (row < colData.size() && !colData[row].empty()) {
                                    std::string text = colData[row];
                                    try {
                                        double commentTime = std::stod(dates[row]);
                                        
                                        // Achar o índice numérico mais próximo no traçado do gráfico
                                        int closestIdx = -1;
                                        double minDiff = std::numeric_limits<double>::max();
                                        for (size_t i = 0; i < plotDates->size(); ++i) {
                                            double t = std::stod((*plotDates)[i]);
                                            double diff = std::abs(t - commentTime);
                                            if (diff < minDiff) {
                                                minDiff = diff;
                                                closestIdx = static_cast<int>(i);
                                            }
                                        }
                                        
                                        if (closestIdx != -1) {
                                            // Desenhar linha vertical e caixa de texto correspondente
                                            double xVal = static_cast<double>(closestIdx);
                                            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 0.0f, 0.6f)); // Amarelo translúcido
                                            ImPlot::PlotInfLines("##vLineAnn", &xVal, 1);
                                            ImPlot::PopStyleColor();
                                            
                                            double yVal = limits.Y.Max - (limits.Y.Max - limits.Y.Min) * 0.12 - (row % 3) * (limits.Y.Max - limits.Y.Min) * 0.08; // Distribuir no topo
                                            
                                            ImPlot::PlotText(text.c_str(), xVal, yVal, ImVec2(0, 0));
                                        }
                                    } catch (...) {}
                                }
                            }
                        }
                    }
                }
            }
        }

        this->drawLegendPopup(graph, graphIndex);
        ImPlot::EndPlot();
    }

    this->processColumnDragDrop(graph);
}

void Window::Plot::renderResizeButton(size_t graphIndex) {
    ::Graph& graph = this->graphs[graphIndex];

    ImGui::Dummy(ImVec2(0, RESIZE_BAR_SIZE));
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::PushID(static_cast<int>(graphIndex));
    ImGui::Button("PlotResize", ImVec2(avail.x, RESIZE_BAR_SIZE));

    // Se a área estiver ativa e o mouse estiver sendo arrastado, atualiza a altura.
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float delta             = ImGui::GetMouseDragDelta(0).y;
        graph.config.plotHeight = std::max(
            MIN_GRAPH_SIZE, std::min(graph.config.plotHeight + delta,
                                     MAX_GRAPH_SIZE)); // Atualiza a altura, garantindo limites mínimo e máximo.
        ImGui::ResetMouseDragDelta();
    }

    ImGui::PopID();
}