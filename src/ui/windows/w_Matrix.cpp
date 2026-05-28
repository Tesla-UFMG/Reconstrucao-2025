#include "ui/windows/w_Matrix.hpp"

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

Window::Matrix::Matrix(const std::string& title) : IWindow() {
    this->title  = title;
    this->flags  = ImGuiWindowFlags_MenuBar;
    this->isOpen = &m_isOpen;

    // Threshold color presets
    m_confLL.enabled = true;
    m_confLL.bg[0] = 0.6f; m_confLL.bg[1] = 0.0f; m_confLL.bg[2] = 0.0f; m_confLL.bg[3] = 1.0f;

    m_confL.enabled = true;
    m_confL.bg[0] = 0.6f; m_confL.bg[1] = 0.4f; m_confL.bg[2] = 0.0f; m_confL.bg[3] = 1.0f;

    m_confNormal.enabled = true;
    m_confNormal.bg[0] = 0.0f; m_confNormal.bg[1] = 0.45f; m_confNormal.bg[2] = 0.0f; m_confNormal.bg[3] = 1.0f;

    m_confH.enabled = true;
    m_confH.bg[0] = 0.6f; m_confH.bg[1] = 0.4f; m_confH.bg[2] = 0.0f; m_confH.bg[3] = 1.0f;

    m_confHH.enabled = true;
    m_confHH.bg[0] = 0.6f; m_confHH.bg[1] = 0.0f; m_confHH.bg[2] = 0.0f; m_confHH.bg[3] = 1.0f;
}

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

ImVec4 Window::Matrix::getCellColor(double val) const {
    if (m_colorMode == 1) { // Gradient
        double range = m_maxVal - m_minVal;
        double t = (std::abs(range) > 1e-6) ? (val - m_minVal) / range : 0.0;
        t = std::max(0.0, std::min(1.0, t));
        return ImVec4(
            m_minColor[0] * (1.0f - t) + m_maxColor[0] * t,
            m_minColor[1] * (1.0f - t) + m_maxColor[1] * t,
            m_minColor[2] * (1.0f - t) + m_maxColor[2] * t,
            1.0f
        );
    } else if (m_colorMode == 2) { // Thresholds
        auto pick = [](const ColorThresholdConfig& c) {
            return ImVec4(c.bg[0], c.bg[1], c.bg[2], c.bg[3]);
        };
        if (m_confLL.enabled && val < m_threshLL) return pick(m_confLL);
        if (m_confL.enabled  && val < m_threshL)  return pick(m_confL);
        if (m_confHH.enabled && val > m_threshHH) return pick(m_confHH);
        if (m_confH.enabled  && val > m_threshH)  return pick(m_confH);
        if (m_confNormal.enabled)                  return pick(m_confNormal);
    }
    return ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // default dark card
}

// ─────────────────────────────────────────────
// Drag & Drop
// ─────────────────────────────────────────────

void Window::Matrix::processDragDrop() {
    if (!ImGui::BeginDragDropTarget()) return;

    // Drop an ARCHIVE/ID → create a new column and auto-fill all its variables
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVE_NAME")) {
        const ArchivePayload* ap = reinterpret_cast<const ArchivePayload*>(payload->Data);
        std::string ft   = ap->fileType;
        std::string name = ap->fileName;

        // Don't duplicate columns
        bool exists = std::any_of(m_columns.begin(), m_columns.end(),
            [&](const MatrixColumn& c){ return c.archiveName == name; });

        if (!exists) {
            MatrixColumn col;
            col.fileType    = ft;
            col.archiveName = name;

            // Auto-populate all variables from this archive
            auto& project = DB::getInstance().getProject();
            if (ft == "CSV") {
                for (const auto& f : project.csvFiles) {
                    if (f.getName() == name) {
                        col.variables = f.getColumnNames();
                        break;
                    }
                }
            } else {
                for (const auto& f : project.telemetryFiles) {
                    if (f.getPacketId() == name) {
                        col.variables = f.getColumnNames();
                        break;
                    }
                }
            }
            m_columns.push_back(col);
            LOG("INFO", "[Matriz] Coluna criada para ID: " + name);
        }
    }

    // Drop a COLUMN/variable → find matching column or create one, add variable as row
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
        const ColumnPayload* cp = reinterpret_cast<const ColumnPayload*>(payload->Data);
        std::string ft   = cp->fileType;
        std::string name = cp->fileName;
        std::string var  = cp->columnName;

        // Find or create column
        MatrixColumn* col = nullptr;
        for (auto& c : m_columns) {
            if (c.archiveName == name) { col = &c; break; }
        }
        if (!col) {
            MatrixColumn newCol;
            newCol.fileType    = ft;
            newCol.archiveName = name;
            m_columns.push_back(newCol);
            col = &m_columns.back();
        }

        // Add variable if not already present
        if (std::find(col->variables.begin(), col->variables.end(), var) == col->variables.end()) {
            col->variables.push_back(var);
        }
    }

    ImGui::EndDragDropTarget();
}

// ─────────────────────────────────────────────
// Color Settings (rendered inside menu bar menu)
// ─────────────────────────────────────────────

void Window::Matrix::renderColorSettings() {
    ImGui::Text("Modo de Cores:");
    ImGui::RadioButton("Nenhuma",             &m_colorMode, 0);
    ImGui::RadioButton("Gradiente Dinâmico",  &m_colorMode, 1);
    ImGui::RadioButton("Faixas (LL/L/N/H/HH)",&m_colorMode, 2);

    if (m_colorMode == 1) {
        ImGui::Separator();
        ImGui::PushItemWidth(120.0f);
        ImGui::InputDouble("Valor Mínimo##mat", &m_minVal, 0.1, 1.0, "%.2f");
        ImGui::SameLine();
        ImGui::ColorEdit4("##matGradMin", m_minColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::InputDouble("Valor Máximo##mat", &m_maxVal, 0.1, 1.0, "%.2f");
        ImGui::SameLine();
        ImGui::ColorEdit4("##matGradMax", m_maxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::PopItemWidth();
    } else if (m_colorMode == 2) {
        ImGui::Separator();
        auto renderConf = [](const char* label, double* thresh, ColorThresholdConfig& conf, bool hasThresh = true) {
            ImGui::PushID(label);
            ImGui::Checkbox("##en", &conf.enabled);
            ImGui::SameLine();
            if (hasThresh && thresh) {
                ImGui::PushItemWidth(80.0f);
                ImGui::InputDouble(label, thresh, 0.1, 1.0, "%.2f");
                ImGui::PopItemWidth();
            } else {
                ImGui::TextUnformatted(label);
            }
            ImGui::SameLine();
            ImGui::ColorEdit4("##bg", conf.bg, ImGuiColorEditFlags_NoInputs);
            ImGui::PopID();
        };
        renderConf("LL",     &m_threshLL, m_confLL);
        renderConf("L",      &m_threshL,  m_confL);
        renderConf("Normal", nullptr,     m_confNormal, false);
        renderConf("H",      &m_threshH,  m_confH);
        renderConf("HH",     &m_threshHH, m_confHH);
    }
}

// ─────────────────────────────────────────────
// Grid
// ─────────────────────────────────────────────

void Window::Matrix::renderGrid() {
    if (m_columns.empty()) {
        // Placeholder - same style as Numeric/Bar
        const char* msg = "(Arraste um ID ou variável aqui)";
        ImVec2 avail    = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize(msg);
        ImGui::SetCursorPos(ImVec2(
            ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f,
            ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f
        ));
        ImGui::TextDisabled("%s", msg);
        return;
    }

    int numCols = static_cast<int>(m_columns.size());

    // Count max rows
    int maxRows = 0;
    for (const auto& col : m_columns)
        maxRows = std::max(maxRows, static_cast<int>(col.variables.size()));

    ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders
                               | ImGuiTableFlags_SizingFixedFit
                               | ImGuiTableFlags_ScrollX
                               | ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable("##mat_table", numCols, tableFlags))
        return;

    // Setup columns with header = archive name
    for (int c = 0; c < numCols; c++) {
        float colW = std::max(70.0f, ImGui::GetContentRegionAvail().x / numCols);
        ImGui::TableSetupColumn(m_columns[c].archiveName.c_str(),
                                ImGuiTableColumnFlags_WidthFixed, colW);
    }
    ImGui::TableHeadersRow();

    // Right-click on headers to remove a column
    for (int c = 0; c < numCols; c++) {
        ImGui::TableSetColumnIndex(c);
        // Invisible button over header region for right-click
        ImGui::PushID(1000 + c);
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("##col_ctx");
        }
        if (ImGui::BeginPopup("##col_ctx")) {
            if (ImGui::MenuItem(("Remover coluna: " + m_columns[c].archiveName).c_str())) {
                m_removeColIdx = c;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    // Data rows
    for (int r = 0; r < maxRows; r++) {
        ImGui::TableNextRow(0, 42.0f);
        for (int c = 0; c < numCols; c++) {
            ImGui::TableSetColumnIndex(c);
            ImGui::PushID(r * 256 + c);

            const MatrixColumn& col = m_columns[c];
            bool hasVar  = (r < static_cast<int>(col.variables.size()));
            bool hasData = false;
            double val   = 0.0;
            std::string varName = hasVar ? col.variables[r] : "";

            if (hasVar) {
                const std::vector<double>* data = nullptr;
                if (col.fileType == "CSV") {
                    data = &DB::getInstance().getCSVData(col.archiveName, varName);
                } else {
                    data = &DB::getInstance().getTelemetryData(col.archiveName, varName);
                }
                if (data && !data->empty()) {
                    val = data->back();
                    hasData = true;
                }
            }

            // Cell colors
            ImVec4 bgCol = hasData ? getCellColor(val) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
            ImVec4 fgCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            if (!hasVar) {
                bgCol = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
                fgCol = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_Button,        bgCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(bgCol.x + 0.07f, bgCol.y + 0.07f, bgCol.z + 0.07f, bgCol.w));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(bgCol.x - 0.04f, bgCol.y - 0.04f, bgCol.z - 0.04f, bgCol.w));
            ImGui::PushStyleColor(ImGuiCol_Text, fgCol);

            // Build cell text
            std::string cellText;
            if (!hasVar) {
                cellText = "-";
            } else if (hasData) {
                std::ostringstream ss;
                ss << varName << "\n" << std::fixed << std::setprecision(2) << val;
                cellText = ss.str();
            } else {
                cellText = varName + "\nN/D";
            }

            ImGui::Button(cellText.c_str(), ImVec2(-1.0f, -1.0f));

            // Right-click to remove variable
            if (hasVar && ImGui::BeginPopupContextItem("##cell_ctx")) {
                if (ImGui::MenuItem(("Remover: " + varName).c_str())) {
                    m_removeColVarIdx = c;
                    m_removeVarIdx    = r;
                }
                ImGui::EndPopup();
            }

            // Tooltip
            if (hasVar && ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("ID: %s", col.archiveName.c_str());
                ImGui::Text("Variável: %s", varName.c_str());
                if (hasData)
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Valor: %.5f", val);
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Sem dados");
                ImGui::EndTooltip();
            }

            // Per-cell drag-drop (add variable to this column)
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                    const ColumnPayload* cp = reinterpret_cast<const ColumnPayload*>(payload->Data);
                    std::string dropVar = cp->columnName;
                    auto& vars = m_columns[c].variables;
                    if (std::find(vars.begin(), vars.end(), dropVar) == vars.end()) {
                        vars.push_back(dropVar);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopStyleColor(4);
            ImGui::PopID();
        }
    }

    ImGui::EndTable();

    // Deferred removals (safe to do after table render)
    if (m_removeColIdx >= 0 && m_removeColIdx < static_cast<int>(m_columns.size())) {
        m_columns.erase(m_columns.begin() + m_removeColIdx);
        m_removeColIdx = -1;
    }
    if (m_removeColVarIdx >= 0 && m_removeVarIdx >= 0
        && m_removeColVarIdx < static_cast<int>(m_columns.size())) {
        auto& vars = m_columns[m_removeColVarIdx].variables;
        if (m_removeVarIdx < static_cast<int>(vars.size())) {
            vars.erase(vars.begin() + m_removeVarIdx);
        }
        m_removeColVarIdx = -1;
        m_removeVarIdx    = -1;
    }
}

// ─────────────────────────────────────────────
// Main render
// ─────────────────────────────────────────────

void Window::Matrix::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Cores")) {
            renderColorSettings();
            ImGui::EndMenu();
        }
        if (!m_columns.empty() && ImGui::BeginMenu("Editar")) {
            if (ImGui::MenuItem("Limpar Matriz")) {
                m_columns.clear();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Full-area child so drag-drop covers the whole content region
    ImGui::BeginChild("##mat_child", ImVec2(0, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    renderGrid();

    ImGui::EndChild();
    ImGui::End();
}
