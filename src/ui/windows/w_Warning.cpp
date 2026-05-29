#include "ui/windows/w_Warning.hpp"
#include "Dialogs.hpp"

Window::Warning* Window::Warning::s_instance = nullptr;

Window::Warning::Warning(bool* isOpen) : IWindow(isOpen) {
    this->title = "Avisos";
    this->flags = ImGuiWindowFlags_MenuBar;
    s_instance = this;
}

Window::Warning::~Warning() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

Window::Warning* Window::Warning::getInstance() {
    return s_instance;
}

void Window::Warning::render() {
    if (!this->isOpen || !*this->isOpen)
        return;

    // 1. Evaluate rules against new data points
    evaluateRules();

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Get loaded files
    auto& project = DB::getInstance().getProject();
    const auto& csvFiles = project.csvFiles;
    const auto& telemetryFiles = project.telemetryFiles;

    // Build list of unified files dynamically
    struct FileSource {
        std::string type;         // "CSV" or "Telemetry"
        std::string name;         // original filename or packetId
        std::string displayName;  // e.g. "[CSV] run.csv" or "[Telemetria] packet1"
    };

    std::vector<FileSource> sources;
    for (const auto& file : csvFiles) {
        sources.push_back({ "CSV", file.getName(), "[CSV] " + file.getName() });
    }
    for (const auto& file : telemetryFiles) {
        sources.push_back({ "Telemetry", file.getPacketId(), "[Telemetria] " + file.getPacketId() });
    }

    bool openAddRule = false;
    bool openActiveRules = false;

    // ==========================================
    // MENU BAR
    // ==========================================
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Regras")) {
            if (ImGui::MenuItem("Adicionar Nova Regra...")) {
                openAddRule = true;
            }
            if (ImGui::MenuItem("Visualizar Regras Ativas...")) {
                openActiveRules = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Relatório")) {
            if (ImGui::MenuItem("Limpar Relatório")) {
                clearLogs();
            }
            if (ImGui::MenuItem("Exportar Manual (CSV)...")) {
                char* filepath = Dialogs::showSaveFileDialog("Exportar Avisos", "warning_report", "*.csv");
                if (filepath) {
                    exportToCSV(filepath);
                }
            }
            ImGui::MenuItem("Exportar Auto ao Ocorrer", nullptr, &m_autoExport);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (openAddRule) {
        ImGui::OpenPopup("Adicionar Regra");
    }
    if (openActiveRules) {
        ImGui::OpenPopup("Regras Ativas");
    }

    // ==========================================
    // POPUP: ADICIONAR REGRA
    // ==========================================
    if (ImGui::BeginPopupModal("Adicionar Regra", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Configurar Nova Regra de Alerta");
        ImGui::Separator();

        if (sources.empty()) {
            ImGui::TextDisabled("(Nenhum arquivo ou pacote carregado no momento)");
            ImGui::Spacing();
            if (ImGui::Button("Fechar", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            // Clamp selected file index
            if (m_selectedFileIdx < 0 || m_selectedFileIdx >= static_cast<int>(sources.size())) {
                m_selectedFileIdx = 0;
            }

            // Combobox para Fonte de Dados
            if (ImGui::BeginCombo("Fonte / Arquivo", sources[m_selectedFileIdx].displayName.c_str())) {
                for (int i = 0; i < static_cast<int>(sources.size()); i++) {
                    bool isSelected = (m_selectedFileIdx == i);
                    if (ImGui::Selectable(sources[i].displayName.c_str(), isSelected)) {
                        m_selectedFileIdx = i;
                        m_selectedColIdx = 0; // reset column index
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Obter colunas para a fonte selecionada
            const auto& src = sources[m_selectedFileIdx];
            std::vector<std::string> colNames;
            if (src.type == "CSV") {
                for (const auto& file : csvFiles) {
                    if (file.getName() == src.name) {
                        colNames = file.getColumnNames();
                        break;
                    }
                }
            } else {
                for (const auto& file : telemetryFiles) {
                    if (file.getPacketId() == src.name) {
                        colNames = file.getColumnNames();
                        break;
                    }
                }
            }

            if (colNames.empty()) {
                ImGui::TextDisabled("(Nenhuma coluna encontrada nesta fonte)");
            } else {
                if (m_selectedColIdx < 0 || m_selectedColIdx >= static_cast<int>(colNames.size())) {
                    m_selectedColIdx = 0;
                }

                // Combobox para Variável
                if (ImGui::BeginCombo("Variável / Coluna", colNames[m_selectedColIdx].c_str())) {
                    for (int i = 0; i < static_cast<int>(colNames.size()); i++) {
                        bool isSelected = (m_selectedColIdx == i);
                        if (ImGui::Selectable(colNames[i].c_str(), isSelected)) {
                            m_selectedColIdx = i;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Condição do Alerta:");
            const char* conditions[] = { 
                "Igual a (Especifico)", 
                "Fora da Faixa (Min/Max)", 
                "Dentro da Faixa (Min/Max)", 
                "Maior que (>) ", 
                "Menor que (<) " 
            };
            if (ImGui::BeginCombo("##condition_combo", conditions[m_selectedCondType])) {
                for (int i = 0; i < 5; i++) {
                    bool isSelected = (m_selectedCondType == i);
                    if (ImGui::Selectable(conditions[i], isSelected)) {
                        m_selectedCondType = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // Inputs dinâmicos para limites
            ImGui::Spacing();
            if (m_selectedCondType == 0 || m_selectedCondType == 3 || m_selectedCondType == 4) {
                ImGui::InputDouble("Valor Alvo", &m_tempTargetValue, 1.0, 10.0, "%.3f");
            } else {
                ImGui::InputDouble("Valor Mínimo (Min)", &m_tempMinVal, 1.0, 10.0, "%.3f");
                ImGui::InputDouble("Valor Máximo (Max)", &m_tempMaxVal, 1.0, 10.0, "%.3f");
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Rótulo / Descrição Customizada:");
            ImGui::InputText("##desc_input", m_tempDesc, sizeof(m_tempDesc));

            ImGui::Spacing();
            ImGui::TextUnformatted("Cor de Destaque da Linha:");
            ImGui::SameLine();
            ImGui::ColorEdit4("##alert_color", m_tempColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::Separator();
            if (ImGui::Button("Adicionar Regra (+)", ImVec2(160, 0))) {
                if (!colNames.empty()) {
                    WarningRule rule;
                    rule.fileType = src.type;
                    rule.fileName = src.name;
                    rule.columnName = colNames[m_selectedColIdx];
                    rule.conditionType = m_selectedCondType;
                    rule.targetValue = m_tempTargetValue;
                    rule.minVal = m_tempMinVal;
                    rule.maxVal = m_tempMaxVal;
                    rule.description = m_tempDesc;
                    std::copy(std::begin(m_tempColor), std::end(m_tempColor), std::begin(rule.alertColor));
                    rule.lastProcessedIndex = -1;
                    rule.wasTriggered = false;

                    addRule(rule);

                    // Limpa campo temporário de descrição
                    memset(m_tempDesc, 0, sizeof(m_tempDesc));
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    // ==========================================
    // POPUP: REGRAS ATIVAS
    // ==========================================
    if (ImGui::BeginPopupModal("Regras Ativas", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Gerenciar Regras de Avisos Ativas");
        ImGui::Separator();

        if (m_rules.empty()) {
            ImGui::TextDisabled("Nenhuma regra configurada no momento.");
            ImGui::Spacing();
        } else {
            ImGui::BeginChild("##active_rules_popup_child", ImVec2(550, 200), true);
            for (size_t i = 0; i < m_rules.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                
                ImVec4 alertColVec = ImVec4(m_rules[i].alertColor[0], m_rules[i].alertColor[1], m_rules[i].alertColor[2], m_rules[i].alertColor[3]);
                ImGui::TextColored(alertColVec, "%s: %s", m_rules[i].fileName.c_str(), m_rules[i].columnName.c_str());
                ImGui::SameLine();
                
                std::string condDesc = "";
                if (m_rules[i].conditionType == 0) condDesc = "== " + std::to_string(m_rules[i].targetValue);
                else if (m_rules[i].conditionType == 1) condDesc = "Fora de [" + std::to_string(m_rules[i].minVal) + ", " + std::to_string(m_rules[i].maxVal) + "]";
                else if (m_rules[i].conditionType == 2) condDesc = "Dentro de [" + std::to_string(m_rules[i].minVal) + ", " + std::to_string(m_rules[i].maxVal) + "]";
                else if (m_rules[i].conditionType == 3) condDesc = "> " + std::to_string(m_rules[i].targetValue);
                else if (m_rules[i].conditionType == 4) condDesc = "< " + std::to_string(m_rules[i].targetValue);

                ImGui::Text("| %s", condDesc.c_str());
                
                if (!m_rules[i].description.empty()) {
                    ImGui::SameLine();
                    ImGui::Text("| Descrição: %s", m_rules[i].description.c_str());
                }

                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    removeRule(i);
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        if (ImGui::Button("Fechar", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ==========================================
    // LOGGED WARNINGS TABLE (Main body)
    // ==========================================
    if (ImGui::BeginTable("##warnings_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Data/Hora", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Variável", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Valor Atingido", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Restrição", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupColumn("Descrição / Rótulo", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (int i = static_cast<int>(m_logs.size()) - 1; i >= 0; i--) { // Show newest first
            const auto& log = m_logs[i];
            ImGui::TableNextRow();

            // Set custom visual highlighting color for the alert row with 15% opacity
            ImU32 rowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(log.color[0], log.color[1], log.color[2], 0.15f));
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowColor);

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(log.timestamp.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s (%s)", log.variableName.c_str(), log.archiveName.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.4f", log.valueReached);

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(log.conditionText.c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(log.description.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void Window::Warning::evaluateRules() {
    for (auto& rule : m_rules) {
        const std::vector<double>* data = nullptr;
        if (rule.fileType == "CSV") {
            data = &DB::getInstance().getCSVData(rule.fileName, rule.columnName);
        } else if (rule.fileType == "Telemetry") {
            data = &DB::getInstance().getTelemetryData(rule.fileName, rule.columnName);
        }

        if (!data || data->empty()) {
            rule.lastProcessedIndex = -1; // Reset if empty
            continue;
        }

        int currentSize = static_cast<int>(data->size());
        
        // Handle database clear or load-reloads safely
        if (rule.lastProcessedIndex >= currentSize) {
            rule.lastProcessedIndex = -1;
        }

        if (rule.lastProcessedIndex == -1) {
            // Initialize scanner to current tail, evaluating incoming data dynamically
            rule.lastProcessedIndex = currentSize - 1;
        } else if (rule.lastProcessedIndex < currentSize - 1) {
            // Process all new telemetry data points since last update
            for (int idx = rule.lastProcessedIndex + 1; idx < currentSize; ++idx) {
                double val = (*data)[idx];
                bool triggered = false;

                if (rule.conditionType == 0) { // Specific Value
                    triggered = (std::abs(val - rule.targetValue) < 1e-5);
                } else if (rule.conditionType == 1) { // Out of Range
                    triggered = (val < rule.minVal || val > rule.maxVal);
                } else if (rule.conditionType == 2) { // Inside Range
                    triggered = (val >= rule.minVal && val <= rule.maxVal);
                } else if (rule.conditionType == 3) { // Greater Than
                    triggered = (val > rule.targetValue);
                } else if (rule.conditionType == 4) { // Less Than
                    triggered = (val < rule.targetValue);
                }

                if (triggered) {
                    if (!rule.wasTriggered) {
                        triggerWarning(rule, val);
                        rule.wasTriggered = true;
                    }
                } else {
                    rule.wasTriggered = false;
                }
            }
            rule.lastProcessedIndex = currentSize - 1;
        }
    }
}

void Window::Warning::triggerWarning(const WarningRule& rule, double value) {
    LoggedWarning logEntry;
    
    // Get formatted local timestamp YYYY-MM-DD HH:MM:SS
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_struct = *std::localtime(&now_time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
    logEntry.timestamp = oss.str();
    
    logEntry.variableName = rule.columnName;
    logEntry.archiveName = rule.fileName;
    logEntry.valueReached = value;
    logEntry.description = rule.description;
    
    // Copy visual highlight color
    std::copy(std::begin(rule.alertColor), std::end(rule.alertColor), std::begin(logEntry.color));
    
    // Format restriction description
    if (rule.conditionType == 0) {
        logEntry.conditionText = "Igual a " + std::to_string(rule.targetValue);
    } else if (rule.conditionType == 1) {
        logEntry.conditionText = "Fora de [" + std::to_string(rule.minVal) + ", " + std::to_string(rule.maxVal) + "]";
    } else if (rule.conditionType == 2) {
        logEntry.conditionText = "Dentro de [" + std::to_string(rule.minVal) + ", " + std::to_string(rule.maxVal) + "]";
    } else if (rule.conditionType == 3) {
        logEntry.conditionText = "Maior que " + std::to_string(rule.targetValue);
    } else if (rule.conditionType == 4) {
        logEntry.conditionText = "Menor que " + std::to_string(rule.targetValue);
    }
    m_logs.push_back(logEntry);
    
    // Registra o aviso disparado no TextFile "Avisos"
    {
        auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string epochStr = std::to_string(timestamp_ms);

        std::vector<std::string> rowData(5, "");
        std::string warnText = rule.columnName + " de " + rule.fileName + " atingiu " + std::to_string(value) + " (" + logEntry.conditionText + ")";
        if (rule.conditionType >= 0 && rule.conditionType < 5) {
            rowData[rule.conditionType] = warnText;
        }

        for (auto& tf : DB::getInstance().getProject().textFiles) {
            if (tf.getName() == "Avisos") {
                tf.addRow(epochStr, rowData);
                break;
            }
        }
    }

    LOG("WARN", "[Aviso] " + rule.columnName + " de " + rule.fileName + " atingiu " + std::to_string(value) + " (" + logEntry.conditionText + ")");
    
    // Automatic CSV Export if toggled
    if (m_autoExport) {
        std::filesystem::create_directories("telemetry");
        exportToCSV("telemetry/warning_report.csv");
    }
}

void Window::Warning::addRule(const WarningRule& rule) {
    m_rules.push_back(rule);
}

void Window::Warning::removeRule(size_t index) {
    if (index < m_rules.size()) {
        m_rules.erase(m_rules.begin() + index);
    }
}

void Window::Warning::clearLogs() {
    m_logs.clear();
}

void Window::Warning::exportToCSV(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        LOG("ERROR", "[Avisos] Falha ao abrir arquivo para exportação de relatório: " + filepath);
        return;
    }
    
    // CSV Header with UTF-8 BOM to keep Excel compatibility
    file << "\xEF\xBB\xBF";
    file << "Data/Hora,Variavel,Arquivo,Valor Atingido,Condicao,Descricao\n";
    
    for (const auto& log : m_logs) {
        file << log.timestamp << ","
             << log.variableName << ","
             << log.archiveName << ","
             << log.valueReached << ","
             << log.conditionText << ","
             << log.description << "\n";
    }
    
    LOG("INFO", "[Avisos] Relatório de avisos exportado com sucesso para '" + filepath + "'. Total de entradas: " + std::to_string(m_logs.size()));
}
