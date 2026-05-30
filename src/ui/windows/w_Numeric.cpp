#include "ui/windows/w_Numeric.hpp"

Window::Numeric::Numeric(const std::string& title) : IWindow() {
    this->title    = title;
    this->flags    = ImGuiWindowFlags_NoScrollbar;
    this->m_isOpen = true;
    this->setupVisibility(&this->m_isOpen);

    // Default color threshold configurations
    m_confLL.enabled = false;
    m_confL.enabled = false;
    m_confNormal.enabled = false;
    m_confH.enabled = false;
    m_confHH.enabled = false;

    // Solid default colors for warnings to avoid invisible/transparent bug
    m_confLL.bg[0] = 0.7f; m_confLL.bg[1] = 0.1f; m_confLL.bg[2] = 0.1f; m_confLL.bg[3] = 1.0f; // Solid Dark Red
    m_confL.bg[0]  = 0.7f; m_confL.bg[1]  = 0.4f; m_confL.bg[2]  = 0.0f; m_confL.bg[3]  = 1.0f; // Solid Orange
    m_confH.bg[0]  = 0.7f; m_confH.bg[1]  = 0.4f; m_confH.bg[2]  = 0.0f; m_confH.bg[3]  = 1.0f; // Solid Orange
    m_confHH.bg[0] = 0.7f; m_confHH.bg[1] = 0.1f; m_confHH.bg[2] = 0.1f; m_confHH.bg[3] = 1.0f; // Solid Dark Red

    // Normal safe zone defaults to transparent background (default panel style) and white text
    m_confNormal.bg[0] = 0.0f; m_confNormal.bg[1] = 0.0f; m_confNormal.bg[2] = 0.0f; m_confNormal.bg[3] = 0.0f;
    m_confNormal.fg[0] = 1.0f; m_confNormal.fg[1] = 1.0f; m_confNormal.fg[2] = 1.0f; m_confNormal.fg[3] = 1.0f;

    m_fontScale = 1.0f;
    m_showColumnName = true;
    memset(m_stripPattern, 0, sizeof(m_stripPattern));
    memset(m_customLabel, 0, sizeof(m_customLabel));
}

void Window::Numeric::render() {
    if (!this->isOpen || !*this->isOpen)
        return;

    // Calculate value and styling overrides before ImGui::Begin so background color applies correctly
    double val = 0.0;
    bool hasVal = false;
    std::string hoveredColumnName = "";
    std::string hoveredArchiveName = "";

    bool anyData = false;
    for (const auto& col : m_loadedColumns) {
        if (col.data && !col.data->empty()) {
            anyData = true;
            break;
        }
    }

    if (m_hasData && anyData) {
        hasVal = true;
        if (m_currentMetric == MetricType::LAST) {
            val = m_loadedColumns.back().data->back();
        } else {
            if (m_statModeAll) {
                if (m_currentMetric == MetricType::MIN) {
                    double minVal = std::numeric_limits<double>::max();
                    std::string bestCol = "";
                    std::string bestArch = "";
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            double colMin = *std::min_element(col.data->begin(), col.data->end());
                            if (colMin < minVal) {
                                minVal = colMin;
                                bestCol = col.column;
                                bestArch = col.archive;
                            }
                        }
                    }
                    val = minVal;
                    hoveredColumnName = bestCol;
                    hoveredArchiveName = bestArch;
                } else if (m_currentMetric == MetricType::MAX) {
                    double maxVal = -std::numeric_limits<double>::max();
                    std::string bestCol = "";
                    std::string bestArch = "";
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            double colMax = *std::max_element(col.data->begin(), col.data->end());
                            if (colMax > maxVal) {
                                maxVal = colMax;
                                bestCol = col.column;
                                bestArch = col.archive;
                            }
                        }
                    }
                    val = maxVal;
                    hoveredColumnName = bestCol;
                    hoveredArchiveName = bestArch;
                } else if (m_currentMetric == MetricType::AVERAGE) {
                    double sum = 0.0;
                    size_t count = 0;
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            for (double x : *col.data) {
                                sum += x;
                                count++;
                            }
                        }
                    }
                    val = (count > 0) ? (sum / count) : 0.0;
                }
            } else {
                if (m_currentMetric == MetricType::MIN) {
                    double minVal = std::numeric_limits<double>::max();
                    std::string bestCol = "";
                    std::string bestArch = "";
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            double lastVal = col.data->back();
                            if (lastVal < minVal) {
                                minVal = lastVal;
                                bestCol = col.column;
                                bestArch = col.archive;
                            }
                        }
                    }
                    if (minVal != std::numeric_limits<double>::max()) {
                        val = minVal;
                        hoveredColumnName = bestCol;
                        hoveredArchiveName = bestArch;
                    }
                } else if (m_currentMetric == MetricType::MAX) {
                    double maxVal = -std::numeric_limits<double>::max();
                    std::string bestCol = "";
                    std::string bestArch = "";
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            double lastVal = col.data->back();
                            if (lastVal > maxVal) {
                                maxVal = lastVal;
                                bestCol = col.column;
                                bestArch = col.archive;
                            }
                        }
                    }
                    if (maxVal != -std::numeric_limits<double>::max()) {
                        val = maxVal;
                        hoveredColumnName = bestCol;
                        hoveredArchiveName = bestArch;
                    }
                } else if (m_currentMetric == MetricType::AVERAGE) {
                    double sum = 0.0;
                    size_t count = 0;
                    for (const auto& col : m_loadedColumns) {
                        if (col.data && !col.data->empty()) {
                            sum += col.data->back();
                            count++;
                        }
                    }
                    val = (count > 0) ? (sum / count) : 0.0;
                }
            }
        }
    }

    double computedVal = m_useFormula ? (val * m_multiplier + m_offset) : val;

    float* targetBg = nullptr;
    float* targetFg = nullptr;
    float interpolatedBg[4] = {0.15f, 0.15f, 0.15f, 1.0f};
    float interpolatedFg[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    if (hasVal && m_colorMode > 0) {
        if (m_colorMode == 1) {
            if (m_confHH.enabled && computedVal > m_threshHH) {
                targetBg = m_confHH.bg;
                targetFg = m_confHH.fg;
            } else if (m_confH.enabled && computedVal > m_threshH) {
                targetBg = m_confH.bg;
                targetFg = m_confH.fg;
            } else if (m_confLL.enabled && computedVal < m_threshLL) {
                targetBg = m_confLL.bg;
                targetFg = m_confLL.fg;
            } else if (m_confL.enabled && computedVal < m_threshL) {
                targetBg = m_confL.bg;
                targetFg = m_confL.fg;
            } else if (m_confNormal.enabled) {
                targetBg = m_confNormal.bg;
                targetFg = m_confNormal.fg;
            }
        } else if (m_colorMode == 2) {
            for (const auto& rule : m_specificRules) {
                if (std::abs(rule.value - computedVal) < 1e-5) {
                    targetBg = const_cast<float*>(rule.bg);
                    targetFg = const_cast<float*>(rule.fg);
                    break;
                }
            }
        } else if (m_colorMode == 3) {
            double t = 0.0;
            if (m_gradMaxVal > m_gradMinVal) {
                t = (computedVal - m_gradMinVal) / (m_gradMaxVal - m_gradMinVal);
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
            }
            interpolatedBg[0] = m_gradMinColor[0] * (1.0f - t) + m_gradMaxColor[0] * t;
            interpolatedBg[1] = m_gradMinColor[1] * (1.0f - t) + m_gradMaxColor[1] * t;
            interpolatedBg[2] = m_gradMinColor[2] * (1.0f - t) + m_gradMaxColor[2] * t;
            interpolatedBg[3] = m_gradMinColor[3] * (1.0f - t) + m_gradMaxColor[3] * t;
            targetBg = interpolatedBg;
            targetFg = interpolatedFg;
        }
    }

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Begin a full-screen child window that natively holds the content
    ImGui::BeginChild("##numeric_child", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

    // Custom background drawing directly into child window draw list (fully covering the child area)
    if (targetBg && targetBg[3] > 0.01f) {
        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(targetBg[0], targetBg[1], targetBg[2], targetBg[3]));
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 rectMin = windowPos;
        ImVec2 rectMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
        ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, bgColor, ImGui::GetStyle().WindowRounding);
    }

    // Área inteira da janela como Drag and Drop target
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    // Menu de contexto no clique com o botão direito (organizado em submenus estilo Windows)
    if (ImGui::BeginPopupContextWindow()) {
        
        // 1. Submenu: Dados & Exibição
        if (ImGui::BeginMenu("Dados & Exibição")) {
            if (m_loadedColumns.empty()) {
                ImGui::TextDisabled("(Nenhum dado carregado)");
            } else {
                ImGui::Text("Colunas Carregadas:");
                for (size_t i = 0; i < m_loadedColumns.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));
                    std::string label = m_loadedColumns[i].archive + ": " + m_loadedColumns[i].column;
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("X##removeData")) {
                        m_loadedColumns.erase(m_loadedColumns.begin() + i);
                        if (m_loadedColumns.empty()) {
                            m_hasData = false;
                        }
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopID();
                }
                ImGui::Separator();
                if (ImGui::Button("Limpar Todas as Colunas")) {
                    m_loadedColumns.clear();
                    m_hasData = false;
                }

                ImGui::Separator();
                ImGui::Text("Exibição:");
                if (ImGui::RadioButton("Último Valor", m_currentMetric == MetricType::LAST)) {
                    m_currentMetric = MetricType::LAST;
                }
                if (ImGui::RadioButton("Média", m_currentMetric == MetricType::AVERAGE)) {
                    m_currentMetric = MetricType::AVERAGE;
                }
                if (ImGui::RadioButton("Mínimo", m_currentMetric == MetricType::MIN)) {
                    m_currentMetric = MetricType::MIN;
                }
                if (ImGui::RadioButton("Máximo", m_currentMetric == MetricType::MAX)) {
                    m_currentMetric = MetricType::MAX;
                }

                if (m_currentMetric != MetricType::LAST) {
                    ImGui::Separator();
                    ImGui::Text("Calcular estatísticas sobre:");
                    if (ImGui::RadioButton("Todos os valores existentes", m_statModeAll)) {
                        m_statModeAll = true;
                    }
                    if (ImGui::RadioButton("Últimos valores de cada coluna", !m_statModeAll)) {
                        m_statModeAll = false;
                    }
                }
            }
            ImGui::EndMenu();
        }


        // 3. Submenu: Textos
        if (ImGui::BeginMenu("Textos")) {
            ImGui::PushItemWidth(150.0f);
            
            // Sufixo
            ImGui::InputText("Sufixo", m_suffix, sizeof(m_suffix));
            
            ImGui::Separator();
            
            // Rótulo Personalizado e Exibição do Nome
            ImGui::Checkbox("Exibir Nome da Coluna", &m_showColumnName);
            if (m_showColumnName) {
                ImGui::InputText("Rótulo Personalizado", m_customLabel, sizeof(m_customLabel));
            }
            
            ImGui::Separator();
            
            // Tamanho / Escala do Texto
            ImGui::SliderFloat("Escala do Texto", &m_fontScale, 0.5f, 5.0f, "%.1fx");
            
            ImGui::PushItemWidth(120.0f); // Restore default push width style
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        // 4. Submenu: Fórmula Matemática
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

        // 5. Submenu: Tradução de Valores
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

        // 6. Submenu: Customização de Cores
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
                ImGui::InputDouble("Valor Mínimo##num", &m_gradMinVal, 0.1, 1.0, "%.2f");
                ImGui::SameLine();
                ImGui::ColorEdit4("##gradMinColor_num", m_gradMinColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::InputDouble("Valor Máximo##num", &m_gradMaxVal, 0.1, 1.0, "%.2f");
                ImGui::SameLine();
                ImGui::ColorEdit4("##gradMaxColor_num", m_gradMaxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::PopItemWidth();
            }
            ImGui::EndMenu();
        }

        // (Tamanho do Texto unificado no menu "Textos")

        ImGui::EndPopup();
    }

    bool anyDataToRender = false;
    for (const auto& col : m_loadedColumns) {
        if (col.data && !col.data->empty()) {
            anyDataToRender = true;
            break;
        }
    }

    if (!m_hasData || m_loadedColumns.empty() || !anyDataToRender) {
        // Exibe mensagem centralizada pedindo drag and drop
        std::string placeholder = "(Arraste colunas de dados aqui)";
        ImVec2      textSize    = ImGui::CalcTextSize(placeholder.c_str());
        textSize.x             *= m_fontScale;
        textSize.y             *= m_fontScale;
        float       x           = ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f;
        float       y           = ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::SetWindowFontScale(m_fontScale);
        ImGui::TextDisabled("%s", placeholder.c_str());
        ImGui::SetWindowFontScale(1.0f);
    } else {
        std::string colName = "";
        if (strlen(m_customLabel) > 0) {
            colName = m_customLabel;
        } else {
            for (size_t i = 0; i < m_loadedColumns.size(); ++i) {
                std::string cName = m_loadedColumns[i].column;
                if (i > 0) {
                    colName += " + ";
                }
                colName += cName;
            }

            if (m_currentMetric == MetricType::AVERAGE) {
                colName += m_statModeAll ? " (Média Geral)" : " (Média dos Últimos)";
            } else if (m_currentMetric == MetricType::MIN) {
                colName += m_statModeAll ? " (Mínimo Geral)" : " (Mínimo dos Últimos)";
            } else if (m_currentMetric == MetricType::MAX) {
                colName += m_statModeAll ? " (Máximo Geral)" : " (Máximo dos Últimos)";
            } else if (m_currentMetric == MetricType::LAST) {
                colName += " (Último)";
            }
        }

        // Apply translations
        std::string valStr = "";
        bool translated = false;
        if (m_useTranslation) {
            for (const auto& rule : m_translationRules) {
                if (std::abs(rule.value - computedVal) < 1e-5) {
                    valStr = rule.text;
                    translated = true;
                    break;
                }
            }
        }

        if (!translated) {
            char valTextBuf[64];
            snprintf(valTextBuf, sizeof(valTextBuf), "%.4f", computedVal);
            valStr = valTextBuf;
        }

        std::string finalValText = valStr + std::string(m_suffix);

        // 1. Mostrar o nome da coluna no topo centralizado
        float colTextWidth = ImGui::CalcTextSize(colName.c_str()).x * m_fontScale;

        // 2. Mostrar o valor centralizado
        float valTextWidth = ImGui::CalcTextSize(finalValText.c_str()).x * m_fontScale;

        float textHeight      = ImGui::GetTextLineHeightWithSpacing() * m_fontScale;
        float totalTextHeight = m_showColumnName ? (textHeight * 2.0f + 5.0f * m_fontScale) : textHeight;

        // Centraliza verticalmente o conjunto completo de textos relativo ao content region
        float localStartX = ImGui::GetWindowContentRegionMin().x;
        float localStartY = ImGui::GetWindowContentRegionMin().y;

        float startY = localStartY + std::max(0.0f, (avail.y - totalTextHeight) * 0.5f);

        // Scope color pushes strictly to inner telemetry text labels to avoid bleed
        bool pushedFg = false;
        if (targetFg && targetFg[3] > 0.01f) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(targetFg[0], targetFg[1], targetFg[2], targetFg[3]));
            pushedFg = true;
        }

        ImGui::SetWindowFontScale(m_fontScale);

        if (m_showColumnName) {
            // Renderiza o título da coluna
            ImGui::SetCursorPos(ImVec2(localStartX + std::max(0.0f, (avail.x - colTextWidth) * 0.5f), startY));
            ImGui::TextUnformatted(colName.c_str());

            // Renderiza o valor numérico abaixo
            ImGui::SetCursorPos(
                ImVec2(localStartX + std::max(0.0f, (avail.x - valTextWidth) * 0.5f), startY + textHeight + 5.0f * m_fontScale));
            ImGui::TextUnformatted(finalValText.c_str());
        } else {
            // Renderiza apenas o valor numérico centralizado
            ImGui::SetCursorPos(
                ImVec2(localStartX + std::max(0.0f, (avail.x - valTextWidth) * 0.5f), startY));
            ImGui::TextUnformatted(finalValText.c_str());
        }

        ImGui::SetWindowFontScale(1.0f);

        if (pushedFg) {
            ImGui::PopStyleColor(1);
        }
    }

    if (m_loadedColumns.size() > 1 && (m_currentMetric == MetricType::MIN || m_currentMetric == MetricType::MAX)) {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
            std::string actualFile = hoveredArchiveName;
            // Resolve packetId to actual file name if it's a telemetry packet
            const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
            for (const auto& tf : telemetryFiles) {
                if (tf.getPacketId() == hoveredArchiveName) {
                    actualFile = tf.getName();
                    break;
                }
            }

            ImGui::BeginTooltip();
            ImGui::Text("Valor originado de:");
            ImGui::Text("Coluna: %s", hoveredColumnName.c_str());
            ImGui::Text("Arquivo: %s", actualFile.c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

void Window::Numeric::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVE_NAME")) {
            const ArchivePayload* archivePayload = reinterpret_cast<const ArchivePayload*>(payload->Data);
            std::string fileType = archivePayload->fileType;
            std::string fileName = archivePayload->fileName;

            if (fileType == "CSV") {
                const auto& csvFiles = DB::getInstance().getProject().getCSVFiles();
                for (const auto& csvFile : csvFiles) {
                    if (csvFile.getName() == fileName) {
                        for (const std::string& colName : csvFile.getColumnNames()) {
                            this->addColumn(fileType, fileName, colName);
                        }
                        break;
                    }
                }
            } else if (fileType == "Telemetry") {
                const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
                for (const auto& telemetryFile : telemetryFiles) {
                    if (telemetryFile.getPacketId() == fileName) {
                        for (const std::string& colName : telemetryFile.getColumnNames()) {
                            this->addColumn(fileType, fileName, colName);
                        }
                        break;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Numeric::addColumn(const std::string& fileType, const std::string& fileName,
                                const std::string& columnName) {
    if (fileType == "Text") {
        return;
    }

    NumericData colData;
    colData.archive  = fileName;
    colData.column   = columnName;
    colData.fileType = fileType;

    if (fileType == "CSV") {
        colData.data = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        colData.data = &DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (colData.data && !colData.data->empty()) {
        m_loadedColumns.push_back(colData);
        m_hasData = true;
        LOG("INFO", "[Numérico] Adicionado dados da coluna '" + columnName + "' de '" + fileName +
                        "'. Total de registros: " + std::to_string(colData.data->size()));
    } else {
        LOG("ERROR", "[Numérico] Falha ao carregar dados da coluna '" + columnName + "' de '" + fileName + "'.");
    }
}


