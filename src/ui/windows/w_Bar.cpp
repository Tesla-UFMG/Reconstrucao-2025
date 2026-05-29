#include "ui/windows/w_Bar.hpp"
#include "ImGuiWrapper.hpp"

Window::Bar::Bar(const std::string& title) : IWindow() {
    this->title    = title;
    this->flags    = ImGuiWindowFlags_NoScrollbar;
    this->m_isOpen = true;
    this->setupVisibility(&this->m_isOpen);

    // Initial threshold colors (solid colors to avoid alpha bugs)
    m_confLL.color[0] = 0.8f; m_confLL.color[1] = 0.1f; m_confLL.color[2] = 0.1f; m_confLL.color[3] = 1.0f; // Red
    m_confL.color[0]  = 0.8f; m_confL.color[1]  = 0.5f; m_confL.color[2]  = 0.0f; m_confL.color[3]  = 1.0f; // Orange
    m_confNormal.color[0] = 0.0f; m_confNormal.color[1] = 0.8f; m_confNormal.color[2] = 0.0f; m_confNormal.color[3] = 1.0f; // Green
    m_confH.color[0]  = 0.8f; m_confH.color[1]  = 0.5f; m_confH.color[2]  = 0.0f; m_confH.color[3]  = 1.0f; // Orange
    m_confHH.color[0] = 0.8f; m_confHH.color[1] = 0.1f; m_confHH.color[2] = 0.1f; m_confHH.color[3] = 1.0f; // Red

    m_fontScale = 1.0f;
    m_showPercentage = true;
    m_showValue = true;
    m_showColumnName = true;
    memset(m_stripPattern, 0, sizeof(m_stripPattern));
    memset(m_prefix, 0, sizeof(m_prefix));
    memset(m_suffix, 0, sizeof(m_suffix));
}

void Window::Bar::render() {
    if (!this->isOpen || !*this->isOpen)
        return;

    // 1. Calculate values and limits
    double val = 0.0;
    bool hasVal = false;
    double maxL = m_maxVal;
    double minL = m_minVal;

    if (m_hasData && m_loadedData.data && !m_loadedData.data->empty()) {
        hasVal = true;
        if (m_currentMetric == MetricType::LAST) {
            val = m_loadedData.data->back();
        } else if (m_currentMetric == MetricType::AVERAGE) {
            double sum = 0.0;
            for (double x : *m_loadedData.data) {
                sum += x;
            }
            val = sum / m_loadedData.data->size();
        } else if (m_currentMetric == MetricType::MIN) {
            val = *std::min_element(m_loadedData.data->begin(), m_loadedData.data->end());
        } else if (m_currentMetric == MetricType::MAX) {
            val = *std::max_element(m_loadedData.data->begin(), m_loadedData.data->end());
        }

        if (!m_useManualLimits) {
            double dataMin = *std::min_element(m_loadedData.data->begin(), m_loadedData.data->end());
            double dataMax = *std::max_element(m_loadedData.data->begin(), m_loadedData.data->end());
            minL = dataMin;
            maxL = (dataMax == dataMin) ? (dataMin + 1.0) : dataMax;
        }
    }

    double range = maxL - minL;
    if (std::abs(range) < 1e-6) {
        range = 1.0;
    }
    double pct = (val - minL) / range;
    pct = std::clamp(pct, 0.0, 1.0);

    // Determine bar color from mode
    float* targetColor = m_barColor;
    float interpolatedColor[4] = {0.0f, 0.8f, 0.0f, 1.0f};

    if (hasVal) {
        if (m_colorBarMode == 1) { // Gradient — uses dedicated grad limits
            double gradRange = m_gradMaxVal - m_gradMinVal;
            double t = (std::abs(gradRange) > 1e-6) ? (val - m_gradMinVal) / gradRange : 0.0;
            t = std::clamp(t, 0.0, 1.0);
            interpolatedColor[0] = m_gradMinColor[0] * (1.0f - t) + m_gradMaxColor[0] * t;
            interpolatedColor[1] = m_gradMinColor[1] * (1.0f - t) + m_gradMaxColor[1] * t;
            interpolatedColor[2] = m_gradMinColor[2] * (1.0f - t) + m_gradMaxColor[2] * t;
            interpolatedColor[3] = m_gradMinColor[3] * (1.0f - t) + m_gradMaxColor[3] * t;
            targetColor = interpolatedColor;
        } else if (m_colorBarMode == 2) { // Thresholds
            if (m_confHH.enabled && val > m_threshHH) {
                targetColor = m_confHH.color;
            } else if (m_confH.enabled && val > m_threshH) {
                targetColor = m_confH.color;
            } else if (m_confLL.enabled && val < m_threshLL) {
                targetColor = m_confLL.color;
            } else if (m_confL.enabled && val < m_threshL) {
                targetColor = m_confL.color;
            } else if (m_confNormal.enabled) {
                targetColor = m_confNormal.color;
            }
        }
    }

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Full-screen child for clean rendering bounds
    ImGui::BeginChild("##bar_child", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    // Context Menu
    if (ImGui::BeginPopupContextWindow()) {
        
        // 1. Submenu: Dados & Exibição
        if (ImGui::BeginMenu("Dados & Exibição")) {
            if (!m_hasData) {
                ImGui::TextDisabled("(Nenhum dado carregado)");
            } else {
                std::string label = m_loadedData.archive + ": " + m_loadedData.column;
                ImGui::TextUnformatted(label.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("X##removeData")) {
                    m_hasData         = false;
                    m_loadedData.data = nullptr;
                    ImGui::CloseCurrentPopup();
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

                ImGui::Separator();
                ImGui::Checkbox("Exibir Nome da Coluna", &m_showColumnName);
                ImGui::Checkbox("Exibir Porcentagem", &m_showPercentage);
                ImGui::Checkbox("Exibir Valor Numérico", &m_showValue);
                
                ImGui::Separator();
                ImGui::PushItemWidth(120.0f);
                ImGui::InputText("Remover do Nome", m_stripPattern, sizeof(m_stripPattern));
                ImGui::PopItemWidth();
            }
            ImGui::EndMenu();
        }

        // 2. Submenu: Orientação & Escala
        if (ImGui::BeginMenu("Orientação & Escala")) {
            ImGui::Text("Orientação:");
            ImGui::RadioButton("Vertical", &m_orientation, 0);
            ImGui::RadioButton("Horizontal", &m_orientation, 1);

            ImGui::Separator();
            ImGui::Checkbox("Limites Manuais", &m_useManualLimits);
            if (m_useManualLimits) {
                ImGui::PushItemWidth(100.0f);
                ImGui::InputDouble("Mínimo", &m_minVal, 1.0, 10.0, "%.2f");
                ImGui::InputDouble("Máximo", &m_maxVal, 1.0, 10.0, "%.2f");
                ImGui::PopItemWidth();
            } else {
                ImGui::TextDisabled("Auto-escala ativa (0.0 até Máx)");
            }
            ImGui::EndMenu();
        }

        // 3. Submenu: Customização de Cores (unified)
        if (ImGui::BeginMenu("Customização de Cores")) {
            ImGui::Text("Modo de Cores:");
            if (ImGui::RadioButton("Nenhuma", m_colorBarMode == 0)) {
                m_colorBarMode = 0;
            }
            if (ImGui::RadioButton("Gradiente Dinâmico", m_colorBarMode == 1)) {
                m_colorBarMode = 1;
            }
            if (ImGui::RadioButton("Faixas (LL / L / Normal / H / HH)", m_colorBarMode == 2)) {
                m_colorBarMode = 2;
            }

            if (m_colorBarMode == 1) {
                ImGui::Separator();
                ImGui::Text("Limites do Gradiente:");
                ImGui::PushItemWidth(120.0f);
                ImGui::InputDouble("Valor Mínimo##bar", &m_gradMinVal, 0.1, 1.0, "%.2f");
                ImGui::SameLine();
                ImGui::ColorEdit4("##gradMinColor_bar", m_gradMinColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::InputDouble("Valor Máximo##bar", &m_gradMaxVal, 0.1, 1.0, "%.2f");
                ImGui::SameLine();
                ImGui::ColorEdit4("##gradMaxColor_bar", m_gradMaxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::PopItemWidth();
            } else if (m_colorBarMode == 2) {
                ImGui::Separator();
                auto renderBarThreshConf = [](const char* label, double* thresh, BarThresholdConfig& conf, bool hasThresh = true) {
                    ImGui::PushID(label);
                    ImGui::Checkbox("Habilitar", &conf.enabled);
                    if (conf.enabled) {
                        if (conf.color[3] < 0.01f) {
                            conf.color[0] = 0.0f; conf.color[1] = 0.8f; conf.color[2] = 0.0f; conf.color[3] = 1.0f;
                        }
                        if (hasThresh && thresh) {
                            ImGui::PushItemWidth(100.0f);
                            ImGui::InputDouble(label, thresh, 0.1, 1.0, "%.2f");
                            ImGui::PopItemWidth();
                        } else {
                            ImGui::TextUnformatted(label);
                        }
                        ImGui::ColorEdit4("Cor##col", conf.color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    } else {
                        ImGui::TextDisabled("%s (Desativado)", label);
                    }
                    ImGui::PopID();
                    ImGui::Separator();
                };

                renderBarThreshConf("LL (Muito Baixo)", &m_threshLL, m_confLL);
                renderBarThreshConf("L (Baixo)",        &m_threshL,  m_confL);
                renderBarThreshConf("Normal (Seguro)",  nullptr,     m_confNormal, false);
                renderBarThreshConf("H (Alto)",         &m_threshH,  m_confH);
                renderBarThreshConf("HH (Muito Alto)",  &m_threshHH, m_confHH);
            }
            ImGui::EndMenu();
        }

        // 4. Submenu: Texto (Prefixo/Sufixo)
        if (ImGui::BeginMenu("Texto (Prefixo/Sufixo)")) {
            ImGui::PushItemWidth(120.0f);
            ImGui::InputText("Prefixo", m_prefix, sizeof(m_prefix));
            ImGui::InputText("Sufixo", m_suffix, sizeof(m_suffix));
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        // 5. Submenu: Tamanho do Texto
        if (ImGui::BeginMenu("Tamanho do Texto")) {
            ImGui::PushItemWidth(120.0f);
            ImGui::SliderFloat("Escala do Texto", &m_fontScale, 0.5f, 5.0f, "%.1fx");
            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    if (!m_hasData || !m_loadedData.data || m_loadedData.data->empty()) {
        std::string placeholder = "(Arraste uma coluna de dados aqui)";
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
        // Strip column name prefix if any
        std::string colName = m_loadedData.column;
        if (strlen(m_stripPattern) > 0) {
            std::string stripStr(m_stripPattern);
            size_t pos = colName.find(stripStr);
            while (pos != std::string::npos) {
                colName.erase(pos, stripStr.length());
                pos = colName.find(stripStr);
            }
        }
        if (m_currentMetric == MetricType::AVERAGE) {
            colName += " (Média)";
        } else if (m_currentMetric == MetricType::MIN) {
            colName += " (Mínimo)";
        } else if (m_currentMetric == MetricType::MAX) {
            colName += " (Máximo)";
        }

        // Build string for value
        char valTextBuf[64];
        snprintf(valTextBuf, sizeof(valTextBuf), "%.4f", val);
        std::string finalValText = std::string(m_prefix) + valTextBuf + std::string(m_suffix);

        // Build percentage string
        char pctTextBuf[16];
        snprintf(pctTextBuf, sizeof(pctTextBuf), "%.0f%%", pct * 100.0);
        std::string pctText = pctTextBuf;

        // Colors
        ImU32 barColU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(targetColor[0], targetColor[1], targetColor[2], targetColor[3]));
        ImU32 bgColU32  = ImGui::ColorConvertFloat4ToU32(ImVec4(m_bgColor[0], m_bgColor[1], m_bgColor[2], m_bgColor[3]));

        ImVec4 textColor = ImVec4(m_fgColor[0], m_fgColor[1], m_fgColor[2], m_fgColor[3]);
        if (ImGuiWrapper::currentTheme == LIGHT) {
            textColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        float localStartX = ImGui::GetWindowContentRegionMin().x;
        float localStartY = ImGui::GetWindowContentRegionMin().y;

        float headerHeight = 0.0f;
        if (m_showColumnName) {
            headerHeight = ImGui::GetTextLineHeightWithSpacing() * m_fontScale;
            // Render column name centered at the top
            float colNameWidth = ImGui::CalcTextSize(colName.c_str()).x * m_fontScale;
            ImGui::SetCursorPos(ImVec2(localStartX + std::max(0.0f, (avail.x - colNameWidth) * 0.5f), localStartY));
            ImGui::SetWindowFontScale(m_fontScale);
            ImGui::PushStyleColor(ImGuiCol_Text, textColor);
            ImGui::TextUnformatted(colName.c_str());
            ImGui::PopStyleColor(1);
            ImGui::SetWindowFontScale(1.0f);
        }

        float margin = 8.0f * m_fontScale;
        float spacing = 4.0f * m_fontScale;

        // Render Bar
        if (m_orientation == 0) {
            // --- VERTICAL BAR ---
            float footerHeight = 0.0f;
            std::string labelText = "";
            if (m_showValue && m_showPercentage) {
                labelText = finalValText + " (" + pctText + ")";
            } else if (m_showValue) {
                labelText = finalValText;
            } else if (m_showPercentage) {
                labelText = pctText;
            }

            if (!labelText.empty()) {
                footerHeight = ImGui::GetTextLineHeightWithSpacing() * m_fontScale;
            }

            // Occupy maximum available height and width leaving a clean margin
            float barHeight = avail.y - headerHeight - footerHeight - (m_showColumnName ? spacing : margin) - (footerHeight > 0.0f ? spacing : margin);
            barHeight = std::max(10.0f, barHeight);

            float barWidth = avail.x - margin * 2.0f;
            barWidth = std::max(10.0f, barWidth);

            float barX = localStartX + margin;
            float barY = localStartY + headerHeight + (m_showColumnName ? spacing : margin);

            ImVec2 barMin = ImVec2(ImGui::GetWindowPos().x + barX, ImGui::GetWindowPos().y + barY);
            ImVec2 barMax = ImVec2(barMin.x + barWidth, barMin.y + barHeight);

            // Draw Background
            ImGui::GetWindowDrawList()->AddRectFilled(barMin, barMax, bgColU32, ImGui::GetStyle().FrameRounding);

            // Draw Fill (bottom up)
            float fillHeight = barHeight * pct;
            if (fillHeight > 0.0f) {
                ImVec2 fillMin = ImVec2(barMin.x, barMax.y - fillHeight);
                ImVec2 fillMax = barMax;
                ImGui::GetWindowDrawList()->AddRectFilled(fillMin, fillMax, barColU32, ImGui::GetStyle().FrameRounding);
            }

            // Draw threshold stripe indicators if m_useThresholds is active
            if (m_useThresholds) {
                auto drawStripe = [&](double threshVal, const float* col) {
                    double ratio = (threshVal - minL) / range;
                    if (ratio >= 0.0 && ratio <= 1.0) {
                        ImU32 stripeCol = ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], col[3]));
                        float thickness = 1.2f * m_fontScale;
                        float stripeY = barMax.y - barHeight * ratio;
                        stripeY = std::clamp(stripeY, barMin.y, barMax.y);
                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2(barMin.x, stripeY),
                            ImVec2(barMax.x, stripeY),
                            stripeCol,
                            thickness
                        );
                    }
                };

                if (m_confLL.enabled) drawStripe(m_threshLL, m_confLL.color);
                if (m_confL.enabled)  drawStripe(m_threshL,  m_confL.color);
                if (m_confH.enabled)  drawStripe(m_threshH,  m_confH.color);
                if (m_confHH.enabled) drawStripe(m_threshHH, m_confHH.color);
            }

            // Draw Label below the bar
            if (!labelText.empty()) {
                float labelWidth = ImGui::CalcTextSize(labelText.c_str()).x * m_fontScale;
                float labelY = barY + barHeight + spacing;
                ImGui::SetCursorPos(ImVec2(localStartX + std::max(0.0f, (avail.x - labelWidth) * 0.5f), labelY));
                ImGui::SetWindowFontScale(m_fontScale);
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                ImGui::TextUnformatted(labelText.c_str());
                ImGui::PopStyleColor(1);
                ImGui::SetWindowFontScale(1.0f);
            }

        } else {
            // --- HORIZONTAL BAR ---
            // Occupy maximum available height and width leaving a clean margin
            float barWidth = avail.x - margin * 2.0f;
            barWidth = std::max(10.0f, barWidth);

            float barHeight = avail.y - headerHeight - (m_showColumnName ? spacing : margin) - margin;
            barHeight = std::max(10.0f, barHeight);

            float barX = localStartX + margin;
            float barY = localStartY + headerHeight + (m_showColumnName ? spacing : margin);

            ImVec2 barMin = ImVec2(ImGui::GetWindowPos().x + barX, ImGui::GetWindowPos().y + barY);
            ImVec2 barMax = ImVec2(barMin.x + barWidth, barMin.y + barHeight);

            // Draw Background
            ImGui::GetWindowDrawList()->AddRectFilled(barMin, barMax, bgColU32, ImGui::GetStyle().FrameRounding);

            // Draw Fill (left to right)
            float fillWidth = barWidth * pct;
            if (fillWidth > 0.0f) {
                ImVec2 fillMin = barMin;
                ImVec2 fillMax = ImVec2(barMin.x + fillWidth, barMax.y);
                ImGui::GetWindowDrawList()->AddRectFilled(fillMin, fillMax, barColU32, ImGui::GetStyle().FrameRounding);
            }

            // Draw threshold stripe indicators if m_useThresholds is active
            if (m_useThresholds) {
                auto drawStripe = [&](double threshVal, const float* col) {
                    double ratio = (threshVal - minL) / range;
                    if (ratio >= 0.0 && ratio <= 1.0) {
                        ImU32 stripeCol = ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], col[3]));
                        float thickness = 1.2f * m_fontScale;
                        float stripeX = barMin.x + barWidth * ratio;
                        stripeX = std::clamp(stripeX, barMin.x, barMax.x);
                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2(stripeX, barMin.y),
                            ImVec2(stripeX, barMax.y),
                            stripeCol,
                            thickness
                        );
                    }
                };

                if (m_confLL.enabled) drawStripe(m_threshLL, m_confLL.color);
                if (m_confL.enabled)  drawStripe(m_threshL,  m_confL.color);
                if (m_confH.enabled)  drawStripe(m_threshH,  m_confH.color);
                if (m_confHH.enabled) drawStripe(m_threshHH, m_confHH.color);
            }

            // Overlay label inside the middle of the bar
            std::string labelText = "";
            if (m_showValue && m_showPercentage) {
                labelText = finalValText + " (" + pctText + ")";
            } else if (m_showValue) {
                labelText = finalValText;
            } else if (m_showPercentage) {
                labelText = pctText;
            }

            if (!labelText.empty()) {
                ImVec2 textSize = ImGui::CalcTextSize(labelText.c_str());
                textSize.x *= m_fontScale;
                textSize.y *= m_fontScale;
                
                float textX = barX + (barWidth - textSize.x) * 0.5f;
                float textY = barY + (barHeight - textSize.y) * 0.5f;

                ImGui::SetCursorPos(ImVec2(textX, textY));
                ImGui::SetWindowFontScale(m_fontScale);
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                ImGui::TextUnformatted(labelText.c_str());
                ImGui::PopStyleColor(1);
                ImGui::SetWindowFontScale(1.0f);
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

void Window::Bar::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Bar::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    if (fileType == "Text") {
        return;
    }

    m_loadedData.archive  = fileName;
    m_loadedData.column   = columnName;
    m_loadedData.fileType = fileType;

    if (fileType == "CSV") {
        m_loadedData.data = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        m_loadedData.data = &DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (m_loadedData.data && !m_loadedData.data->empty()) {
        m_hasData = true;
        LOG("INFO", "[Barra] Carregado dados da coluna '" + columnName + "' de '" + fileName + "'. Total de registros: " + std::to_string(m_loadedData.data->size()));
    } else {
        m_hasData         = false;
        m_loadedData.data = nullptr;
        LOG("ERROR", "[Barra] Falha ao carregar dados da coluna '" + columnName + "' de '" + fileName + "'.");
    }
}




