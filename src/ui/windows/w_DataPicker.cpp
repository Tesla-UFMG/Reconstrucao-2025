#include "ui/windows/w_DataPicker.hpp"
#include "DataFiles.hpp"

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
            if (Dialogs::showConfirmationDialog("Você tem certeza que deseja fechar este arquivo?")) {
                // Se for do tipo CSV File....
                if (auto csvFile = dynamic_cast<const CSVFile*>(&file)) {
                    this->m_csvToRemove.push_back(csvFile->getPath());
                }

                // Se for do tipo Video File....
                else if (dynamic_cast<const VideoFile*>(&file)) {
                }

                else if (auto telemetryFile = dynamic_cast<const TelemetryFile*>(&file)) {
                    this->m_telemetryToRemove.push_back(telemetryFile->getPacketId());
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
        std::string        msg      = "[" + packetId + "] " + fileName;

        bool isComments = (packetId == "Comentários");
        bool isPlayback = (packetId == "PLAYBACK");
        if (isComments) {
            msg = fileName; // Exibe apenas "Comentários" (sem colchetes com ID)
            ImVec4 yellowColor = (ImGuiWrapper::currentTheme == LIGHT) 
                                 ? ImVec4(0.55f, 0.42f, 0.0f, 1.0f) // Amarelo escuro / dourado para tema claro
                                 : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Amarelo brilhante para tema escuro
            ImGui::PushStyleColor(ImGuiCol_Text, yellowColor);
        } else if (isPlayback) {
            ImVec4 purpleColor = (ImGuiWrapper::currentTheme == LIGHT)
                                 ? ImVec4(0.5f, 0.0f, 0.8f, 1.0f) // Roxo escuro
                                 : ImVec4(0.8f, 0.4f, 1.0f, 1.0f); // Roxo claro
            ImGui::PushStyleColor(ImGuiCol_Text, purpleColor);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, HI(1));
        }

        if (ImGui::TreeNode(msg.c_str())) {
            this->sendArchivePayload(fileType, packetId);
            for (const std::string& colName : telemetryFile->getColumnNames()) {
                this->renderColumnItem(fileType, packetId, colName);
            }
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
    }

    else if (dynamic_cast<const VideoFile*>(&file)) {
    }
    
    else if (auto textFile = dynamic_cast<const TextFile*>(&file)) {
        ImVec4 yellowColor = (ImGuiWrapper::currentTheme == LIGHT) 
                             ? ImVec4(0.55f, 0.42f, 0.0f, 1.0f) // Amarelo escuro / dourado para tema claro
                             : ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Amarelo brilhante para tema escuro
        ImGui::PushStyleColor(ImGuiCol_Text, yellowColor);
        if (ImGui::TreeNode(fileName.c_str())) {
            this->sendArchivePayload(fileType, fileName);
            for (const std::string& colName : textFile->getColumnNames()) {
                this->renderColumnItem(fileType, fileName, colName);
            }
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
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

        const std::vector<TextFile>& textFiles = DB::getInstance().getProject().getTextFiles();
        for (auto& textFile : textFiles) {
            this->renderArchiveNode(textFile);
            this->renderArchiveContextPopup(textFile, i);
            i++;
        }



        ImGui::EndChild();
        ImGui::End();
        
        // Executar remoções pendentes após os loops
        for (const auto& path : this->m_csvToRemove) {
            DB::getInstance().deleteCSV(path);
        }
        this->m_csvToRemove.clear();
        
        for (const auto& packetId : this->m_telemetryToRemove) {
            DB::getInstance().getProject().removePacket(packetId);
        }
        this->m_telemetryToRemove.clear();
    }
}
