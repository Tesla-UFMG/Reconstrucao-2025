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

void Window::DataPicker::sendArchivePayload(const std::string& filepath) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        std::string payload = filepath;
        ImGui::SetDragDropPayload("FILE_NAME", payload.c_str(), payload.size() + 1);
        ImGui::Text("Arquivo: %s", filepath.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::sendColumnPayload(const std::string& filepath, const std::string& columnName) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        std::string payload = filepath + ":" + columnName;
        ImGui::SetDragDropPayload("COLUMN_NAME", payload.c_str(), payload.size() + 1);
        ImGui::Text("%s", columnName.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::renderArchiveContextPopup(const GenericFile& file) {
    const std::string& filename = file.getPath().filename().string();

    if (ImGui::BeginPopupContextItem((filename + "_popup").c_str())) {
        if (ImGui::MenuItem("Fechar")) {

            // Se for do tipo CSV File....
            if (auto csvFile = dynamic_cast<const CSVFile*>(&file)) {
                DB::getInstance().deleteCSV(csvFile->getPath());
            }

            // Se for do tipo Video File....
            if (auto videoFile = dynamic_cast<const VideoFile*>(&file)) {
            }
        }
        ImGui::EndPopup();
    }
}

void Window::DataPicker::renderArchiveNode(const GenericFile& file, int index) {
    const std::string& filename = file.getPath().filename().string();
    const std::string& filepath = file.getPath().string();

    // Se for do tipo CSV File...
    if (auto csvFile = dynamic_cast<const CSVFile*>(&file)) {
        if (ImGui::TreeNode((filename + "##" + std::to_string(index)).c_str())) {
            this->sendArchivePayload(filename); // Inicia o payload de drag & drop para o arquivo
            // Renderiza cada coluna do arquivo
            const std::vector<std::string>& cols = csvFile->getDocument()->GetColumnNames();
            for (const std::string& colName : cols) {
                this->renderColumnItem(filename, colName);
            }
            ImGui::TreePop();
        }
    }
}

void Window::DataPicker::renderColumnItem(const std::string& filepath, const std::string& colName) {
    ImGui::Selectable(colName.c_str());
    this->sendColumnPayload(filepath, colName);
}

void Window::DataPicker::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->renderMenuBar();
        ImGui::BeginChild("##dataPicker", ImGui::GetContentRegionAvail(), true, ImGuiWindowFlags_HorizontalScrollbar);

        // Renderiza cada arquivo e seu respectivo menu de contexto
        const std::vector<CSVFile>& csvFiles = DB::getInstance().getProject().csvFiles;
        for (size_t i = 0; i < csvFiles.size(); i++) {
            this->renderArchiveNode(csvFiles[i], i);
            this->renderArchiveContextPopup(csvFiles[i]);
        }

        ImGui::EndChild();
        ImGui::End();
    }
}
