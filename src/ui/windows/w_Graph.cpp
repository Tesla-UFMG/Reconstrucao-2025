#include "ui/windows/w_Graph.hpp"
#include <iostream>

Window::Graph::Graph(const std::string& title) : IWindow() {
    this->title    = title;
    this->flags    = ImGuiWindowFlags_NoScrollbar;
    this->m_isOpen = true;
    this->setupVisibility(&this->m_isOpen);
    this->m_graph.config.id = 0;
}

void Window::Graph::render() {
    if (!this->isOpen || !*this->isOpen)
        return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Drag and drop target fills the whole window
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    if (m_graph.data.empty() && m_graph.textAnnotations.empty()) {

        // Perfect centered placeholder text
        std::string placeholder = "(Arraste colunas de dados aqui)";
        ImVec2      textSize    = ImGui::CalcTextSize(placeholder.c_str());
        float       x           = ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f;
        float       y           = ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::TextDisabled("%s", placeholder.c_str());
    } else {
        renderGraphPlot();
    }

    this->drawContextMenu();
    ImGui::End();
}

void Window::Graph::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        ImGui::EndDragDropTarget();
    }
}
void Window::Graph::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    if (fileType == "Text") {
        GraphTextAnnotation ann;
        ann.archiveName = fileName;
        ann.columnName  = columnName;

        bool exists = false;
        for (const auto& a : m_graph.textAnnotations) {
            if (a.archiveName == fileName && a.columnName == columnName) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_graph.textAnnotations.push_back(ann);
            LOG("INFO", "Anotação de texto '" + columnName + "' de '" + fileName + "' adicionada ao gráfico dinâmico.");
        }
        return;
    }

    // Verifica se a coluna já está no gráfico
    for (const GraphData& graphData : m_graph.data) {
        if (graphData.fileName == fileName && graphData.columnName == columnName) {
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já está no gráfico dinâmico.");
            return;
        }
    }

    GraphData graphData;
    graphData.columnName = columnName;
    graphData.fileName   = fileName;
    graphData.fileType   = fileType;

    if (fileType == "CSV") {
        graphData.y = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        graphData.y = &DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (!graphData.y) {
        Dialogs::showErrorDialog("Não é possível adicionar uma coluna inválida ao gráfico!");
        return;
    }

    graphData.buildXVector();
    m_graph.data.push_back(graphData);

    ImPlot::BustItemCache();
    LOG("DEBUG", "Coluna " + columnName + " adicionada ao gráfico dinâmico '" + this->title + "'.");
}

void Window::Graph::removeColumn(size_t columnIndex) {
    if (columnIndex >= m_graph.data.size()) {
        LOG("ERROR", "Não foi possível remover a coluna do gráfico, índice inválido.");
        return;
    }
    m_graph.data.erase(m_graph.data.begin() + columnIndex);
    ImPlot::BustItemCache();
    LOG("DEBUG", "Coluna removida do gráfico dinâmico '" + this->title + "'.");
}

void Window::Graph::renderGraphPlot() {
    GraphConfig& graphConfig = m_graph.config;

    // Se uma coluna foi selecionada como eixo X, obtém seus dados.
    std::vector<double> customX;
    bool                useCustomX = false;
    if (!graphConfig.xColumn.empty()) {
        for (const GraphData& graphData : m_graph.data) {
            if (graphData.columnName == graphConfig.xColumn) {
                customX    = graphData.getYData();
                useCustomX = true;
                break;
            }
        }
    }

    std::string plotTitle = useCustomX ? "Gráfico vs " + graphConfig.xColumn : "";
    ImVec2      avail     = ImGui::GetContentRegionAvail();

    if (ImPlot::BeginPlot((plotTitle + "##Plot_" + this->title).c_str(), ImVec2(-1, avail.y), ImPlotFlags_NoFrame)) {
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
        } else if (!m_graph.data.empty()) {
            axisLength = m_graph.data[0].getYData().size();
            for (const GraphData& graphData : m_graph.data) {
                if (graphData.getYData().size() > axisLength) {
                    axisLength = graphData.getYData().size();
                }
            }
        }
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);

        // Plota cada coluna
        for (size_t j = 0; j < m_graph.data.size(); j++) {
            GraphData& graphData = m_graph.data[j];
            // Se a coluna for a do eixo X, pula
            if (!graphConfig.xColumn.empty() && graphData.columnName == graphConfig.xColumn)
                continue;

            const std::vector<double>& y = graphData.getYData();

            bool canUseDirectPointers = false;
            if (useCustomX) {
                if (customX.size() == y.size())
                    canUseDirectPointers = true;
            } else {
                if (axisLength == y.size())
                    canUseDirectPointers = true;
            }

            const std::vector<double>* finalX = nullptr;
            const std::vector<double>* finalY = nullptr;

            if (canUseDirectPointers) {
                if (graphData.multiplier == 1.0) {
                    finalY = &y;
                } else {
                    graphData.buildMultipliedY();
                    finalY = &graphData.multipliedY;
                }

                if (useCustomX) {
                    finalX = &customX;
                } else {
                    graphData.buildXVector();
                    finalX = &graphData.x;
                }

                graphData.lastYSize         = y.size();
                graphData.lastMultiplier    = graphData.multiplier;
                graphData.lastUseCustomX    = useCustomX;
                graphData.lastAlignmentMode = graphConfig.xyAlignmentMode;
                graphData.lastAxisLength    = axisLength;
                if (useCustomX) {
                    graphData.lastCustomXColumn = graphConfig.xColumn;
                    graphData.lastCustomXSize   = customX.size();
                }
            } else {
                bool cacheInvalid = false;
                if (graphData.cachedY.empty() || graphData.lastYSize != y.size() ||
                    graphData.lastMultiplier != graphData.multiplier || graphData.lastUseCustomX != useCustomX ||
                    graphData.lastAlignmentMode != graphConfig.xyAlignmentMode ||
                    (!useCustomX && graphData.lastAxisLength != axisLength) ||
                    (useCustomX && (graphData.lastCustomXColumn != graphConfig.xColumn ||
                                    graphData.lastCustomXSize != customX.size())) ||
                    graphData.fileName == "PLAYBACK") {
                    cacheInvalid = true;
                }

                if (cacheInvalid) {
                    std::vector<double> yData;
                    if (graphData.multiplier == 1.0) {
                        yData = y;
                    } else {
                        yData.reserve(y.size());
                        for (size_t k = 0; k < y.size(); ++k) {
                            yData.push_back(y[k] * graphData.multiplier);
                        }
                    }

                    if (useCustomX) {
                        auto aligned      = alignVectors(customX, yData, graphConfig.xyAlignmentMode);
                        graphData.cachedX = std::move(aligned.first);
                        graphData.cachedY = std::move(aligned.second);
                    } else {
                        std::vector<double> baseGrid;
                        baseGrid.reserve(axisLength);
                        for (size_t i = 0; i < axisLength; ++i) {
                            baseGrid.push_back(static_cast<double>(i));
                        }
                        auto aligned      = alignVectors(baseGrid, yData, graphConfig.xyAlignmentMode);
                        graphData.cachedX = std::move(aligned.first);
                        graphData.cachedY = std::move(aligned.second);
                    }

                    graphData.lastYSize         = y.size();
                    graphData.lastMultiplier    = graphData.multiplier;
                    graphData.lastUseCustomX    = useCustomX;
                    graphData.lastAlignmentMode = graphConfig.xyAlignmentMode;
                    graphData.lastAxisLength    = axisLength;
                    if (useCustomX) {
                        graphData.lastCustomXColumn = graphConfig.xColumn;
                        graphData.lastCustomXSize   = customX.size();
                    }
                }

                finalX = &graphData.cachedX;
                finalY = &graphData.cachedY;
            }

            int safeSize  = static_cast<int>(std::min(finalX->size(), finalY->size()));
            int colStart  = graphConfig.followTheEnd ? std::max(0, safeSize - graphConfig.numPoints) : 0;
            int numPoints = graphConfig.followTheEnd ? std::min(safeSize, graphConfig.numPoints) : safeSize;

            const double* xPtr = finalX->data() + colStart;
            const double* yPtr = finalY->data() + colStart;

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

            if (m_graph.config.showValueOnYAxis) {
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

                ImVec4 lineColor    = ImPlot::GetColormapColor(j);
                double currentValue = 0.0;
                if (!finalY->empty()) {
                    currentValue = finalY->back();
                }

                ImPlot::TagY(currentValue, lineColor, fmt, currentValue);
            }

            if (m_graph.config.showCursorOnYAxis) {
                if (ImPlot::IsPlotHovered()) {
                    ImPlotPoint mouse   = ImPlot::GetPlotMousePos();
                    double      cursorX = mouse.x;
                    double      cursorY = mouse.y;

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

                    ImPlot::TagY(cursorY, HI(1), fmt, cursorY);
                    ImPlot::TagX(cursorX, HI(1), fmt, cursorX);
                }
            }
        }

        // Renderizar anotações textuais (Comentários e Avisos)
        if (!m_graph.textAnnotations.empty()) {
            // Localizar datas para sincronização temporal
            const std::vector<double>* plotDates = nullptr;
            for (const auto& tf : DB::getInstance().getProject().getTelemetryFiles()) {
                if (!tf.getNumericDate().empty()) {
                    plotDates = &tf.getNumericDate();
                    break;
                }
            }

            if (plotDates && !plotDates->empty()) {
                ImPlotRect limits = ImPlot::GetPlotLimits();

                for (const auto& ann : m_graph.textAnnotations) {
                    const TextFile* targetTF = nullptr;
                    for (const auto& tf : DB::getInstance().getProject().getTextFiles()) {
                        if (tf.getName() == ann.archiveName) {
                            targetTF = &tf;
                            break;
                        }
                    }

                    if (targetTF) {
                        const auto& dates = targetTF->getNumericDates();
                        const auto& data  = targetTF->getData();

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
                                        double commentTime = dates[row];

                                        // Achar o índice numérico mais próximo no traçado do gráfico
                                        int    closestIdx = -1;
                                        double minDiff    = std::numeric_limits<double>::max();
                                        for (size_t i = 0; i < plotDates->size(); ++i) {
                                            double t    = (*plotDates)[i];
                                            double diff = std::abs(t - commentTime);
                                            if (diff < minDiff) {
                                                minDiff    = diff;
                                                closestIdx = static_cast<int>(i);
                                            }
                                        }

                                        if (closestIdx != -1) {
                                            // Se followTheEnd está ligado, só renderiza se estiver na janela final
                                            if (m_graph.config.followTheEnd) {
                                                int maxIdx      = static_cast<int>(plotDates->size());
                                                int windowStart = std::max(0, maxIdx - m_graph.config.numPoints);
                                                if (closestIdx < windowStart) {
                                                    continue; // Pula essa anotação para não forçar o zoom-out
                                                }
                                            }

                                            // Valor no eixo X (índice ou valor customizado)
                                            double xVal = static_cast<double>(closestIdx);
                                            if (useCustomX && closestIdx < static_cast<int>(customX.size())) {
                                                xVal = customX[closestIdx];
                                            }

                                            // Desenhar linha vertical e caixa de texto correspondente
                                            ImPlot::PushStyleColor(
                                                ImPlotCol_Line, ImVec4(1.0f, 1.0f, 0.0f, 0.6f)); // Amarelo translúcido
                                            ImPlot::PlotInfLines("##vLineAnn", &xVal, 1);
                                            ImPlot::PopStyleColor();

                                            double yVal =
                                                limits.Y.Max - (limits.Y.Max - limits.Y.Min) * 0.12 -
                                                (row % 3) * (limits.Y.Max - limits.Y.Min) * 0.08; // Distribuir no topo

                                            ImPlot::PlotText(text.c_str(), xVal, yVal, ImVec2(0, 0));
                                        }
                                    } catch (...) {
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        ImPlot::EndPlot();
    }
}

void Window::Graph::drawContextMenu() {
    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::BeginMenu("Dados & Exibição")) {
            if (m_graph.data.empty() && m_graph.textAnnotations.empty()) {
                ImGui::TextDisabled("(Nenhum dado carregado)");
            } else {
                ImGui::Text("Colunas Carregadas:");
                for (size_t columnIndex = 0; columnIndex < m_graph.data.size(); columnIndex++) {
                    GraphData& graphData = m_graph.data[columnIndex];
                    ImGui::PushID(static_cast<int>(columnIndex));
                    std::string label = graphData.fileName + ": " + graphData.columnName;
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##removeData")) {
                        this->removeColumn(columnIndex);
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopID();
                }

                ImGui::Separator();
                if (ImGui::Button("Limpar Todas as Colunas")) {
                    m_graph.data.clear();
                    m_isOpen = false;
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Textos")) {
            if (m_graph.textAnnotations.empty()) {
                ImGui::TextDisabled("(Nenhuma anotação carregada)");
            } else {
                if (ImGui::BeginTable("TabelaTextos", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Anotação", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();
                    for (size_t i = 0; i < m_graph.textAnnotations.size(); ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        if (ImGui::Button(("X##txt" + std::to_string(i)).c_str())) {
                            m_graph.textAnnotations.erase(m_graph.textAnnotations.begin() + i);
                            break;
                        }
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(m_graph.textAnnotations[i].columnName.c_str());
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Fórmula Matemática")) {
            if (m_graph.data.empty()) {
                ImGui::TextDisabled("(Nenhum dado carregado)");
            } else {
                for (size_t i = 0; i < m_graph.data.size(); ++i) {
                    GraphData& graphData = m_graph.data[i];
                    if (graphData.columnName == m_graph.config.xColumn)
                        continue;

                    ImGui::PushID(static_cast<int>(i));
                    ImGui::TextUnformatted(graphData.columnName.c_str());
                    ImGui::PushItemWidth(120.0f);
                    ImGui::InputDouble("Multiplicador (A)", &graphData.multiplier, 0.1, 1.0, "%.4f");
                    ImGui::InputDouble("Soma/Offset (B)", &graphData.offset, 0.1, 1.0, "%.4f");
                    ImGui::PopItemWidth();
                    ImGui::Separator();
                    ImGui::PopID();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Customização de Cores")) {
            ImPlotContext&  gp       = *GImPlot;
            ImPlotColormap& colormap = gp.Style.Colormap;

            if (ImPlot::ColormapButton(ImPlot::GetColormapName(colormap), ImVec2(225, 0), colormap)) {
                colormap = (colormap + 1) % ImPlot::GetColormapCount();
                ImPlot::BustItemCache();
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImPlot::ShowColormapSelector("##");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Configuração do Gráfico")) {
            const char* graphTypes[] = {"Linha", "Barra", "Scatter", "Preenchido"};
            ImGui::SeparatorText("Tipo de Gráfico");
            int currentType = static_cast<int>(m_graph.config.type);
            if (ImGui::Combo("##Tipo", &currentType, graphTypes, IM_ARRAYSIZE(graphTypes))) {
                m_graph.config.type = static_cast<GraphType>(currentType);
                LOG("DEBUG", "Gráfico dinâmico alterado para " + std::string(graphTypes[currentType]) + ".");
            }

            ImGui::SeparatorText("Exibição");
            ImGui::Checkbox("Auto Fit", &m_graph.config.autoFit);
            ImGui::SameLine();
            ImGui::Checkbox("Seguir o final", &m_graph.config.followTheEnd);
            if (m_graph.config.followTheEnd) {
                ImGui::InputInt("Pontos", &m_graph.config.numPoints, 1, 10);
            }
            ImGui::Checkbox("Exibir Valor no Eixo Y", &m_graph.config.showValueOnYAxis);

            ImGui::SeparatorText("Eixos");
            ImGui::Checkbox("Eixo X", &m_graph.config.showXAxis);
            ImGui::SameLine();
            ImGui::Checkbox("Eixo Y", &m_graph.config.showYAxis);

            if (ImGui::BeginCombo("##EixoX",
                                  m_graph.config.xColumn.empty() ? "Nenhuma" : m_graph.config.xColumn.c_str())) {
                if (ImGui::Selectable("Nenhuma", m_graph.config.xColumn.empty())) {
                    m_graph.config.xColumn.clear();
                }
                for (const std::string& col : m_graph.getColumnNames()) {
                    if (ImGui::Selectable(col.c_str(), m_graph.config.xColumn == col)) {
                        m_graph.config.xColumn = col;
                        ImPlot::BustItemCache();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Alinhamento Avançado")) {
            ImGui::SeparatorText("Alinhamento Temporal / XY");
            const char* alignmentModes[] = {"Tamanho Mínimo", "Proximidade Temporal", "Interpolação Linear (Técnico)"};
            int         currentMode      = static_cast<int>(m_graph.config.xyAlignmentMode);
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::Combo("Alinhamento##XY", &currentMode, alignmentModes, IM_ARRAYSIZE(alignmentModes))) {
                m_graph.config.xyAlignmentMode = static_cast<XYAlignmentMode>(currentMode);
                ImPlot::BustItemCache();
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}
