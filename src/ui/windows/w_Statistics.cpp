#include "ui/windows/w_Statistics.hpp"

Window::Statistics::Statistics(const std::string& title) : IWindow() {
    this->title    = title;
    this->flags    = 0;
    this->m_isOpen = true;
    this->setupVisibility(&this->m_isOpen);
}

void Window::Statistics::render() {
    if (!this->isOpen || !*this->isOpen) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (metrics.empty()) {
        ImGui::Dummy(avail);
        this->processColumnDragDrop();
        ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

        std::string placeholder = "(Arraste colunas de dados aqui)";
        ImVec2      textSize    = ImGui::CalcTextSize(placeholder.c_str());
        float       x           = ImGui::GetWindowContentRegionMin().x + (avail.x - textSize.x) * 0.5f;
        float       y           = ImGui::GetWindowContentRegionMin().y + (avail.y - textSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::TextDisabled("%s", placeholder.c_str());
    } else {
        ImGui::Dummy(avail);
        this->processColumnDragDrop();
        ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
        this->renderTable();
    }

    ImGui::End();
}


void Window::Statistics::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    if (fileType == "Text") {
        return;
    }

    Metric new_metric;
    new_metric.display_name = columnName;
    new_metric.unique_id    = fileName + ":" + columnName;
    new_metric.fileName     = fileName;
    new_metric.fileType     = fileType;

    for (const auto& metric : metrics) {
        if (metric.unique_id == new_metric.unique_id) {
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já está na tabela.");
            return;
        }
    }

    metrics.push_back(new_metric);
    LOG("INFO", "A coluna " + columnName + " do arquivo " + fileName + " foi adicionada à tabela.");
}

void Window::Statistics::processColumnDragDrop() {
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

void Window::Statistics::renderTable() {
    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
    int metric_to_remove = -1;

    if (ImGui::BeginTable("##telemetry_table", 3, flags)) {
        ImGui::TableSetupColumn("Métrica", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Valor Atual", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30.0f);
        ImGui::TableHeadersRow();

        if (!metrics.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            for (size_t i = 0; i < metrics.size(); ++i) {
                const Metric& metric = metrics[i];
                ImGui::TableNextRow();

                // Nome
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", metric.display_name.c_str());

                // Valores
                ImGui::TableSetColumnIndex(1);
                const std::vector<double>* data = nullptr;
                if (metric.fileType == "CSV") {
                    data = &DB::getInstance().getCSVData(metric.fileName, metric.display_name);
                } else if (metric.fileType == "Telemetry") {
                    data = &DB::getInstance().getTelemetryData(metric.fileName, metric.display_name);
                }

                if (data && !data->empty()) {
                    double value = data->back();
                    ImGui::Text("%.3f", value);
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(i);
                if (ImGui::Button("X", ImVec2(22.0f, 0))) {
                    metric_to_remove = i;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    if (metric_to_remove != -1) {
        metrics.erase(metrics.begin() + metric_to_remove);
    }
}