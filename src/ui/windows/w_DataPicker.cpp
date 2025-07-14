#include "ui/windows/w_DataPicker.hpp"

Window::DataPicker::DataPicker(bool* isOpen) : IWindow(isOpen) {
    title = "Selecionador de Dados";
    flags = ImGuiWindowFlags_MenuBar;
}

void Window::DataPicker::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Arquivos")) {
            if (ImGui::MenuItem("Carregar")) {
                DB::getInstance().loadCSVDialog();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::DataPicker::sendArchivePayload(const std::string& fileType, const std::string& fileName) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

        ArchivePayload payload{};
        std::strncpy(payload.fileType, fileType.c_str(), sizeof(payload.fileType));
        std::strncpy(payload.fileName, fileName.c_str(), sizeof(payload.fileName));

        ImGui::SetDragDropPayload("ARCHIVE_NAME", &payload, sizeof(ArchivePayload));
        ImGui::Text("%s", fileName.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::sendColumnPayload(const std::string& fileType, const std::string& fileName,
                                           const std::string& columnName) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

        ColumnPayload payload{};
        std::strncpy(payload.fileType, fileType.c_str(), sizeof(payload.fileType));
        std::strncpy(payload.fileName, fileName.c_str(), sizeof(payload.fileName));
        std::strncpy(payload.columnName, columnName.c_str(), sizeof(payload.columnName));

        ImGui::SetDragDropPayload("COLUMN_NAME", &payload, sizeof(ColumnPayload));
        ImGui::Text("%s", columnName.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::renderArchiveContextPopup(const GenericFile& file, int i) {
    const std::string& filename = file.getPath().filename().string();

    if (ImGui::BeginPopupContextItem((filename + "_popup##" + std::to_string(i)).c_str())) {
        if (ImGui::MenuItem("Fechar")) {
            if (DB::ConfirmationDialog("Você tem certeza que deseja fechar este arquivo?")) {
                // Se for do tipo CSV File....
                if (auto csvFile = dynamic_cast<const CSVFile*>(&file)) {
                    DB::getInstance().deleteCSV(csvFile->getPath());
                }

                // Se for do tipo Video File....
                else if (auto videoFile = dynamic_cast<const VideoFile*>(&file)) {
                }

                else if (auto telemetryFile = dynamic_cast<const TelemetryFile*>(&file)) {
                    DB::getInstance().getProject().removePacket(telemetryFile->getPacketId());
                }
            }
        }
        ImGui::EndPopup();
    }
}

void Window::DataPicker::renderArchiveNode(const GenericFile& file) {
    const std::string& fileType = file.getFileType();
    const std::string& fileName = file.getName();

    if (auto csvFile = dynamic_cast<const CSVFile*>(&file)) {
        const std::string& filepath = file.getPath().string();
        if (ImGui::TreeNode(fileName.c_str())) {
            this->sendArchivePayload(fileType, fileName);
            for (const std::string& colName : csvFile->getColumnNames()) {
                this->renderColumnItem(fileType, fileName, colName);
            }
            ImGui::TreePop();
        }
    }

    else if (auto telemetryFile = dynamic_cast<const TelemetryFile*>(&file)) {
        const std::string& packetId = telemetryFile->getPacketId();
        char               buf[packetId.size() + fileName.size() + 4];
        std::snprintf(buf, sizeof(buf), "[%s] %s", packetId.c_str(), fileName.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, HI(1));
        if (ImGui::TreeNode(buf)) {
            this->sendArchivePayload(fileType, packetId);
            for (const std::string& colName : telemetryFile->getColumnNames()) {
                this->renderColumnItem(fileType, packetId, colName);
            }
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
    }

    else if (auto videoFile = dynamic_cast<const VideoFile*>(&file)) {
    }
}

void Window::DataPicker::renderColumnItem(const std::string& fileType, const std::string& fileName,
                                          const std::string& colName) {
    ImGui::Selectable(colName.c_str());
    this->sendColumnPayload(fileType, fileName, colName);
}

void Window::DataPicker::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->renderMenuBar();
        ImGui::BeginChild("##dataPicker", ImGui::GetContentRegionAvail(), true, ImGuiWindowFlags_HorizontalScrollbar);

        const std::vector<TelemetryFile>& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
        int                               i              = 0;
        for (auto& telemetryFile : telemetryFiles) {
            this->renderArchiveNode(telemetryFile);
            this->renderArchiveContextPopup(telemetryFile, i);
            i++;
        }

        const std::vector<CSVFile>& csvFiles = DB::getInstance().getProject().getCSVFiles();
        for (auto& csvFile : csvFiles) {
            this->renderArchiveNode(csvFile);
            this->renderArchiveContextPopup(csvFile, i);
            i++;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}
