#include "ui/windows/w_Numeric.hpp"

Window::Numeric::Numeric(const std::string& title) : IWindow() {
    this->title = title;
    this->flags = ImGuiWindowFlags_NoScrollbar;
    this->m_isOpen = true;
    this->setupVisibility(&this->m_isOpen);
}

void Window::Numeric::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Área inteira da janela como Drag and Drop target
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    // Menu de contexto no clique com o botão direito
    if (ImGui::BeginPopupContextWindow()) {
        ImGui::Text("Dados Selecionados:");
        ImGui::Separator();
        if (!m_hasData) {
            ImGui::TextDisabled("(Nenhum dado carregado)");
        } else {
            if (ImGui::Button("Remover Dados (X)")) {
                m_hasData = false;
                m_loadedData.data = nullptr;
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::SameLine();
                std::string label = m_loadedData.archive + ": " + m_loadedData.column;
                ImGui::TextUnformatted(label.c_str());
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
        }
        ImGui::EndPopup();
    }

    if (!m_hasData || !m_loadedData.data || m_loadedData.data->empty()) {
        // Exibe mensagem centralizada pedindo drag and drop
        std::string placeholder = "(Arraste uma coluna de dados aqui)";
        ImVec2 textSize = ImGui::CalcTextSize(placeholder.c_str());
        float x = ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f;
        float y = ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::TextDisabled("%s", placeholder.c_str());
    } else {
        // Obtém o valor de acordo com a métrica selecionada
        double val = 0.0;
        std::string colName = m_loadedData.column;

        if (m_currentMetric == MetricType::LAST) {
            val = m_loadedData.data->back();
        } else if (m_currentMetric == MetricType::AVERAGE) {
            double sum = 0.0;
            for (double x : *m_loadedData.data) {
                sum += x;
            }
            val = sum / m_loadedData.data->size();
            colName += " (Média)";
        } else if (m_currentMetric == MetricType::MIN) {
            val = *std::min_element(m_loadedData.data->begin(), m_loadedData.data->end());
            colName += " (Mínimo)";
        } else if (m_currentMetric == MetricType::MAX) {
            val = *std::max_element(m_loadedData.data->begin(), m_loadedData.data->end());
            colName += " (Máximo)";
        }

        // 1. Mostrar o nome da coluna no topo centralizado
        float colTextWidth = ImGui::CalcTextSize(colName.c_str()).x;
        
        // 2. Mostrar o valor centralizado
        char valText[32];
        snprintf(valText, sizeof(valText), "%.4f", val);
        float valTextWidth = ImGui::CalcTextSize(valText).x;

        float textHeight = ImGui::GetTextLineHeightWithSpacing();
        float totalTextHeight = textHeight * 2.0f + 5.0f; // Espaçamento de 5px entre eles
        
        // Centraliza verticalmente o conjunto completo de textos relativo ao content region
        float localStartX = ImGui::GetWindowContentRegionMin().x;
        float localStartY = ImGui::GetWindowContentRegionMin().y;

        float startY = localStartY + std::max(0.0f, (avail.y - totalTextHeight) * 0.5f);

        // Renderiza o título da coluna
        ImGui::SetCursorPos(ImVec2(localStartX + std::max(0.0f, (avail.x - colTextWidth) * 0.5f), startY));
        ImGui::TextUnformatted(colName.c_str());

        // Renderiza o valor numérico
        ImGui::SetCursorPos(ImVec2(localStartX + std::max(0.0f, (avail.x - valTextWidth) * 0.5f), startY + textHeight + 5.0f));
        ImGui::TextUnformatted(valText);
    }

    ImGui::End();
}

void Window::Numeric::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Numeric::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    m_loadedData.archive = fileName;
    m_loadedData.column = columnName;
    m_loadedData.fileType = fileType;
    
    if (fileType == "CSV") {
        m_loadedData.data = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        m_loadedData.data = &DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (m_loadedData.data && !m_loadedData.data->empty()) {
        m_hasData = true;
        LOG("INFO", "[Numérico] Carregado dados da coluna '" + columnName + "' de '" + fileName + "'. Total de registros: " + std::to_string(m_loadedData.data->size()));
    } else {
        m_hasData = false;
        m_loadedData.data = nullptr;
        LOG("ERROR", "[Numérico] Falha ao carregar dados da coluna '" + columnName + "' de '" + fileName + "'.");
    }
}
