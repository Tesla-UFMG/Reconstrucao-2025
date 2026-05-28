#include "ui/windows/w_Matrix.hpp"
#include "implot.h"

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

Window::Matrix::Matrix(const std::string& title) : IWindow() {
    this->title  = title;
    this->flags  = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;
    this->isOpen = &m_isOpen;

    // Solid default colors for safe/warning zones
    m_confLL.enabled = false;
    m_confL.enabled = false;
    m_confNormal.enabled = false;
    m_confH.enabled = false;
    m_confHH.enabled = false;

    m_confLL.bg[0] = 0.7f; m_confLL.bg[1] = 0.1f; m_confLL.bg[2] = 0.1f; m_confLL.bg[3] = 1.0f; // Solid Dark Red
    m_confL.bg[0]  = 0.7f; m_confL.bg[1]  = 0.4f; m_confL.bg[2]  = 0.0f; m_confL.bg[3]  = 1.0f; // Solid Orange
    m_confH.bg[0]  = 0.7f; m_confH.bg[1]  = 0.4f; m_confH.bg[2]  = 0.0f; m_confH.bg[3]  = 1.0f; // Solid Orange
    m_confHH.bg[0] = 0.7f; m_confHH.bg[1] = 0.1f; m_confHH.bg[2] = 0.1f; m_confHH.bg[3] = 1.0f; // Solid Dark Red

    m_confNormal.bg[0] = 0.0f; m_confNormal.bg[1] = 0.0f; m_confNormal.bg[2] = 0.0f; m_confNormal.bg[3] = 0.0f; // Transparent
    m_confNormal.fg[0] = 1.0f; m_confNormal.fg[1] = 1.0f; m_confNormal.fg[2] = 1.0f; m_confNormal.fg[3] = 1.0f; // White

    m_fontScale = 1.0f;
    m_showVariableName = true;

    m_suffix[0] = '\0';
    m_useFormula = false;
    m_multiplier = 1.0;
    m_offset = 0.0;
    m_useTranslation = false;
    m_translationRules.clear();
}

// ─────────────────────────────────────────────
// Colors Helper
// ─────────────────────────────────────────────

ImVec4 Window::Matrix::getCellColor(double val) const {
    float bg[4] = {0.15f, 0.15f, 0.15f, 1.0f};
    const float* targetBg = bg;
    float interpolatedBg[4] = {0.15f, 0.15f, 0.15f, 1.0f};

    if (m_colorMode > 0) {
        if (m_colorMode == 1) { // Por Faixas
            if (m_confHH.enabled && val > m_threshHH) {
                targetBg = m_confHH.bg;
            } else if (m_confH.enabled && val > m_threshH) {
                targetBg = m_confH.bg;
            } else if (m_confLL.enabled && val < m_threshLL) {
                targetBg = m_confLL.bg;
            } else if (m_confL.enabled && val < m_threshL) {
                targetBg = m_confL.bg;
            } else if (m_confNormal.enabled) {
                targetBg = m_confNormal.bg;
            }
        } else if (m_colorMode == 2) { // Valores Específicos
            for (const auto& rule : m_specificRules) {
                if (std::abs(rule.value - val) < 1e-5) {
                    targetBg = rule.bg;
                    break;
                }
            }
        } else if (m_colorMode == 3) { // Gradiente Dinâmico
            double t = 0.0;
            if (m_maxVal > m_minVal) {
                t = (val - m_minVal) / (m_maxVal - m_minVal);
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
            }
            interpolatedBg[0] = m_minColor[0] * (1.0f - t) + m_maxColor[0] * t;
            interpolatedBg[1] = m_minColor[1] * (1.0f - t) + m_maxColor[1] * t;
            interpolatedBg[2] = m_minColor[2] * (1.0f - t) + m_maxColor[2] * t;
            interpolatedBg[3] = 1.0f;
            targetBg = interpolatedBg;
        }
    }
    return ImVec4(targetBg[0], targetBg[1], targetBg[2], targetBg[3]);
}

ImVec4 Window::Matrix::getCellTextColor(double val) const {
    float fg[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float* targetFg = fg;

    if (m_colorMode > 0) {
        if (m_colorMode == 1) { // Por Faixas
            if (m_confHH.enabled && val > m_threshHH) {
                targetFg = m_confHH.fg;
            } else if (m_confH.enabled && val > m_threshH) {
                targetFg = m_confH.fg;
            } else if (m_confLL.enabled && val < m_threshLL) {
                targetFg = m_confLL.fg;
            } else if (m_confL.enabled && val < m_threshL) {
                targetFg = m_confL.fg;
            } else if (m_confNormal.enabled) {
                targetFg = m_confNormal.fg;
            }
        } else if (m_colorMode == 2) { // Valores Específicos
            for (const auto& rule : m_specificRules) {
                if (std::abs(rule.value - val) < 1e-5) {
                    targetFg = rule.fg;
                    break;
                }
            }
        }
    }
    return ImVec4(targetFg[0], targetFg[1], targetFg[2], targetFg[3]);
}

// ─────────────────────────────────────────────
// Drag & Drop
// ─────────────────────────────────────────────

void Window::Matrix::processDragDrop() {
    if (!ImGui::BeginDragDropTarget()) return;

    // Drop ARCHIVE/ID -> Add column and auto-import all its variables into m_rowVariables
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVE_NAME")) {
        const ArchivePayload* ap = reinterpret_cast<const ArchivePayload*>(payload->Data);
        std::string ft   = ap->fileType;
        std::string name = ap->fileName;

        bool colExists = std::any_of(m_columns.begin(), m_columns.end(),
            [&](const MatrixColumn& c){ return c.archiveName == name; });

        if (!colExists) {
            MatrixColumn col;
            col.fileType    = ft;
            col.archiveName = name;
            m_columns.push_back(col);

            auto& project = DB::getInstance().getProject();
            std::vector<std::string> newVars;
            if (ft == "CSV") {
                for (const auto& f : project.csvFiles) {
                    if (f.getName() == name) { newVars = f.getColumnNames(); break; }
                }
            } else if (ft == "Telemetry") {
                for (const auto& f : project.telemetryFiles) {
                    if (f.getPacketId() == name) { newVars = f.getColumnNames(); break; }
                }
            }

            for (const auto& var : newVars) {
                if (std::find(m_rowVariables.begin(), m_rowVariables.end(), var) == m_rowVariables.end()) {
                    m_rowVariables.push_back(var);
                }
            }
            LOG("INFO", "[Matriz] Adicionado ID: " + name + " com suas variáveis.");
        }
    }
    // Drop COLUMN/variable -> Find or add column ID, and insert variable to rowVariables
    else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
        const ColumnPayload* cp = reinterpret_cast<const ColumnPayload*>(payload->Data);
        std::string ft   = cp->fileType;
        std::string name = cp->fileName;
        std::string var  = cp->columnName;

        bool colExists = std::any_of(m_columns.begin(), m_columns.end(),
            [&](const MatrixColumn& c){ return c.archiveName == name; });

        if (!colExists) {
            MatrixColumn col;
            col.fileType    = ft;
            col.archiveName = name;
            m_columns.push_back(col);
        }

        if (std::find(m_rowVariables.begin(), m_rowVariables.end(), var) == m_rowVariables.end()) {
            m_rowVariables.push_back(var);
        }
        LOG("INFO", "[Matriz] Adicionada variável '" + var + "' do ID: " + name);
    }

    ImGui::EndDragDropTarget();
}

// ─────────────────────────────────────────────
// Resolved Friendly Names Helper
// ─────────────────────────────────────────────

std::string Window::Matrix::getIDDisplayName(const std::string& archiveName, const std::string& fileType) const {
    if (fileType == "Telemetry") {
        const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
        for (const auto& f : telemetryFiles) {
            if (f.getPacketId() == archiveName) {
                return f.getName();
            }
        }
    } else if (fileType == "CSV") {
        const auto& csvFiles = DB::getInstance().getProject().getCSVFiles();
        for (const auto& f : csvFiles) {
            if (f.getName() == archiveName) {
                return f.getName();
            }
        }
    }
    return archiveName;
}

// ─────────────────────────────────────────────
// Matrix Grid Render
// ─────────────────────────────────────────────

void Window::Matrix::renderGrid() {
    if (m_columns.empty() || m_rowVariables.empty()) {
        const char* placeholder = "(Arraste IDs ou colunas de dados aqui)";
        ImVec2 avail    = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize(placeholder);
        textSize.x     *= m_fontScale;
        textSize.y     *= m_fontScale;
        float x = ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f;
        float y = ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::SetWindowFontScale(m_fontScale);
        ImGui::TextDisabled("%s", placeholder);
        ImGui::SetWindowFontScale(1.0f);
        return;
    }

    int numCols = static_cast<int>(m_columns.size());
    int numRows = static_cast<int>(m_rowVariables.size());

    std::vector<std::string> xStrings;
    xStrings.reserve(numCols);
    for (int c = 0; c < numCols; ++c) {
        xStrings.push_back(getIDDisplayName(m_columns[c].archiveName, m_columns[c].fileType));
    }
    std::vector<const char*> xPtrs;
    xPtrs.reserve(numCols);
    for (int c = 0; c < numCols; ++c) {
        xPtrs.push_back(xStrings[c].c_str());
    }

    std::vector<const char*> yPtrs;
    if (m_showVariableName) {
        yPtrs.reserve(numRows);
        for (int r = 0; r < numRows; ++r) {
            yPtrs.push_back(m_rowVariables[r].c_str());
        }
    }

    // Build custom tick positions to bypass SetupAxisTicks clamp-to-2 bug when size is 1
    std::vector<double> xTicks;
    xTicks.reserve(numCols);
    for (int c = 0; c < numCols; ++c) {
        xTicks.push_back((c + 0.5) / numCols);
    }

    std::vector<double> yTicks;
    yTicks.reserve(numRows);
    for (int r = 0; r < numRows; ++r) {
        yTicks.push_back(1.0 - (r + 0.5) / numRows);
    }

    ImPlotAxisFlags xFlags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;
    ImPlotAxisFlags yFlags = m_showVariableName ? (ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks) : (ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);

    ImGui::SetWindowFontScale(m_fontScale);

    ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    if (ImPlot::BeginPlot("##ConfusionMatrixPlot", ImVec2(-1, -1), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes(nullptr, nullptr, xFlags, yFlags);
        ImPlot::SetupAxisTicks(ImAxis_X1, xTicks.data(), numCols, xPtrs.data());
        if (m_showVariableName) {
            ImPlot::SetupAxisTicks(ImAxis_Y1, yTicks.data(), numRows, yPtrs.data());
        }

        ImPlot::SetupAxesLimits(0.0, 1.0, 0.0, 1.0, ImPlotCond_Always);

        for (int r = 0; r < numRows; ++r) {
            std::string varName = m_rowVariables[r];
            for (int c = 0; c < numCols; ++c) {
                const auto& col = m_columns[c];

                const std::vector<double>* data = nullptr;
                if (col.fileType == "CSV") {
                    data = &DB::getInstance().getCSVData(col.archiveName, varName);
                } else if (col.fileType == "Telemetry") {
                    data = &DB::getInstance().getTelemetryData(col.archiveName, varName);
                }

                bool hasData = (data && !data->empty());
                double val = hasData ? data->back() : 0.0;
                double computedVal = m_useFormula ? (val * m_multiplier + m_offset) : val;

                ImVec4 bgCol = hasData ? getCellColor(computedVal) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
                ImVec4 fgCol = hasData ? getCellTextColor(computedVal) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

                double x_min = (double)c / numCols;
                double x_max = (double)(c + 1) / numCols;
                double y_min = 1.0 - (double)(r + 1) / numRows;
                double y_max = 1.0 - (double)r / numRows;

                ImVec2 pMin = ImPlot::PlotToPixels(ImPlotPoint(x_min, y_min));
                ImVec2 pMax = ImPlot::PlotToPixels(ImPlotPoint(x_max, y_max));

                ImU32 bgU32 = ImGui::ColorConvertFloat4ToU32(bgCol);
                ImPlot::GetPlotDrawList()->AddRectFilled(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMin.y), bgU32);

                std::string cellText = "";
                if (hasData) {
                    bool translated = false;
                    if (m_useTranslation) {
                        for (const auto& rule : m_translationRules) {
                            if (std::abs(rule.value - computedVal) < 1e-5) {
                                cellText = rule.text;
                                translated = true;
                                break;
                            }
                        }
                    }
                    if (!translated) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.4f", computedVal);
                        cellText = buf;
                    }
                    cellText += m_suffix;
                } else {
                    cellText = "N/D";
                }

                double cellX = x_min + 0.5 / numCols;
                double cellY = y_min + 0.5 / numRows;

                ImPlot::PushStyleColor(ImPlotCol_InlayText, fgCol);
                ImPlot::PlotText(cellText.c_str(), cellX, cellY);
                ImPlot::PopStyleColor();

                if (ImPlot::IsPlotHovered()) {
                    ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
                    if (mousePos.x >= x_min && mousePos.x <= x_max && mousePos.y >= y_min && mousePos.y <= y_max) {
                        ImGui::BeginTooltip();
                        std::string displayName = getIDDisplayName(col.archiveName, col.fileType);
                        ImGui::Text("[%s] %s - %s", col.archiveName.c_str(), displayName.c_str(), varName.c_str()));
                        if (hasData) {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Valor original: %.6f", val);
                            if (m_useFormula) {
                                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "Valor pós-fórmula: %.6f", computedVal);
                            }
                            if (m_useTranslation) {
                                ImGui::Text("Tradução: %s", cellText.c_str());
                            }
                        } else {
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Sem dados carregados (N/D)");
                        }
                        ImGui::EndTooltip();
                    }
                }
            }
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor(2);
    ImGui::SetWindowFontScale(1.0f);
}

// ─────────────────────────────────────────────
// Render
// ─────────────────────────────────────────────

void Window::Matrix::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Render Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Configurações")) {
            
            // 1. Submenu: Dados & Exibição
            if (ImGui::BeginMenu("Dados & Exibição")) {
                if (m_columns.empty() && m_rowVariables.empty()) {
                    ImGui::TextDisabled("(Nenhum dado carregado)");
                } else {
                    if (!m_columns.empty()) {
                        ImGui::Text("Colunas (IDs) Carregadas:");
                        for (size_t i = 0; i < m_columns.size(); ++i) {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::TextUnformatted(m_columns[i].archiveName.c_str());
                            ImGui::SameLine();
                            if (ImGui::SmallButton("X##removeCol")) {
                                m_columns.erase(m_columns.begin() + i);
                                ImGui::PopID();
                                break;
                            }
                            ImGui::PopID();
                        }
                    }
                    
                    if (!m_rowVariables.empty()) {
                        ImGui::Separator();
                        ImGui::Text("Linhas (Variáveis) Carregadas:");
                        for (size_t i = 0; i < m_rowVariables.size(); ++i) {
                            ImGui::PushID(static_cast<int>(100 + i));
                            ImGui::TextUnformatted(m_rowVariables[i].c_str());
                            ImGui::SameLine();
                            if (ImGui::SmallButton("X##removeVar")) {
                                m_rowVariables.erase(m_rowVariables.begin() + i);
                                ImGui::PopID();
                                break;
                            }
                            ImGui::PopID();
                        }
                    }
                    
                    ImGui::Separator();
                    if (ImGui::Button("Limpar Matriz")) {
                        m_columns.clear();
                        m_rowVariables.clear();
                    }
                }
                ImGui::EndMenu();
            }

            // 2. Submenu: Textos
            if (ImGui::BeginMenu("Textos")) {
                ImGui::PushItemWidth(150.0f);
                
                // Sufixo
                ImGui::InputText("Sufixo", m_suffix, sizeof(m_suffix));
                
                ImGui::Separator();
                
                // Exibir Nome da Variável / Linha
                ImGui::Checkbox("Exibir Nome da Variável", &m_showVariableName);
                
                ImGui::Separator();
                
                // Tamanho / Escala do Texto
                ImGui::SliderFloat("Escala do Texto", &m_fontScale, 0.5f, 5.0f, "%.1fx");
                
                ImGui::PopItemWidth();
                ImGui::EndMenu();
            }

            // 3. Submenu: Fórmula Matemática
            if (ImGui::BeginMenu("Fórmula Matemática")) {
                ImGui::Checkbox("Usar Fórmula (y = x * A + B)", &m_useFormula);
                if (m_useFormula) {
                    ImGui::Separator();
                    ImGui::PushItemWidth(120.0f);
                    ImGui::InputDouble("Multiplicador (A)", &m_multiplier, 0.1, 1.0, "%.4f");
                    ImGui::InputDouble("Soma/Offset (B)", &m_offset, 0.1, 1.0, "%.4f");
                    ImGui::PopItemWidth();
                }
                ImGui::EndMenu();
            }

            // 4. Submenu: Tradução de Valores
            if (ImGui::BeginMenu("Tradução de Valores")) {
                ImGui::Checkbox("Traduzir Valores", &m_useTranslation);
                if (m_useTranslation) {
                    ImGui::Separator();
                    ImGui::Text("Traduções (Valor -> Texto):");
                    for (size_t i = 0; i < m_translationRules.size(); i++) {
                        ImGui::PushID(static_cast<int>(i));
                        ImGui::PushItemWidth(60.0f);
                        ImGui::InputDouble("##val", &m_translationRules[i].value, 0.0, 0.0, "%.2f");
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::Text("->");
                        ImGui::SameLine();
                        char textBuf[64];
                        strncpy(textBuf, m_translationRules[i].text.c_str(), sizeof(textBuf));
                        ImGui::PushItemWidth(100.0f);
                        if (ImGui::InputText("##txt", textBuf, sizeof(textBuf))) {
                            m_translationRules[i].text = textBuf;
                        }
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        if (ImGui::Button("X##del")) {
                            m_translationRules.erase(m_translationRules.begin() + i);
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                    if (ImGui::Button("Adicionar Tradução")) {
                        m_translationRules.push_back({0.0, ""});
                    }
                }
                ImGui::EndMenu();
            }

            // 5. Submenu: Customização de Cores (Same as Numeric)
            if (ImGui::BeginMenu("Customização de Cores")) {
                ImGui::Text("Modo de Cores:");
                ImGui::RadioButton("Nenhuma", &m_colorMode, 0);
                ImGui::RadioButton("Por Faixas (L, LL, H, HH)", &m_colorMode, 1);
                ImGui::RadioButton("Valores Específicos", &m_colorMode, 2);
                ImGui::RadioButton("Gradiente Dinâmico", &m_colorMode, 3);

                if (m_colorMode == 1) {
                    ImGui::Separator();
                    auto renderThresholdConf = [](const char* label, double* thresh, ColorThresholdConfig& conf, bool hasThresh = true) {
                        ImGui::PushID(label);
                        ImGui::Checkbox("Habilitar", &conf.enabled);
                        if (conf.enabled) {
                            if (conf.bg[3] < 0.01f) {
                                conf.bg[0] = 0.15f; conf.bg[1] = 0.15f; conf.bg[2] = 0.15f; conf.bg[3] = 1.0f;
                            }
                            if (hasThresh && thresh) {
                                ImGui::PushItemWidth(100.0f);
                                ImGui::InputDouble(label, thresh, 0.1, 1.0, "%.2f");
                                ImGui::PopItemWidth();
                            } else {
                                ImGui::TextUnformatted(label);
                            }
                            ImGui::ColorEdit4("Bg##col", conf.bg, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            ImGui::SameLine();
                            ImGui::ColorEdit4("Fg##col", conf.fg, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        } else {
                            ImGui::TextDisabled("%s (Desativado)", label);
                        }
                        ImGui::PopID();
                        ImGui::Separator();
                    };

                    renderThresholdConf("LL (Muito Baixo)", &m_threshLL, m_confLL);
                    renderThresholdConf("L (Baixo)", &m_threshL, m_confL);
                    renderThresholdConf("Normal (Seguro)", nullptr, m_confNormal, false);
                    renderThresholdConf("H (Alto)", &m_threshH, m_confH);
                    renderThresholdConf("HH (Muito Alto)", &m_threshHH, m_confHH);
                } else if (m_colorMode == 2) {
                    ImGui::Separator();
                    ImGui::Text("Regras Específicas:");
                    for (size_t i = 0; i < m_specificRules.size(); i++) {
                        ImGui::PushID(static_cast<int>(i));
                        ImGui::PushItemWidth(60.0f);
                        ImGui::InputDouble("##val", &m_specificRules[i].value, 0.0, 0.0, "%.2f");
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::ColorEdit4("Bg##col", m_specificRules[i].bg, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SameLine();
                        ImGui::ColorEdit4("Fg##col", m_specificRules[i].fg, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        ImGui::SameLine();
                        if (ImGui::Button("X##del")) {
                            m_specificRules.erase(m_specificRules.begin() + i);
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                    if (ImGui::Button("Adicionar Regra")) {
                        m_specificRules.push_back({0.0, {0.15f, 0.15f, 0.15f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
                    }
                } else if (m_colorMode == 3) {
                    ImGui::Separator();
                    ImGui::Text("Limites do Gradiente:");
                    ImGui::PushItemWidth(120.0f);
                    ImGui::InputDouble("Valor Mínimo##num", &m_minVal, 0.1, 1.0, "%.2f");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("##gradMinColor_num", m_minColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    
                    ImGui::InputDouble("Valor Máximo##num", &m_maxVal, 0.1, 1.0, "%.2f");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("##gradMaxColor_num", m_maxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    ImGui::PopItemWidth();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Full-area child window so drag and drop targets the entire area
    ImGui::BeginChild("##mat_child", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    renderGrid();

    ImGui::EndChild();
    ImGui::End();
}
