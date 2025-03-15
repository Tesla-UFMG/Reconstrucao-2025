#include "ui/windows/w_DataPicker.hpp"

// TODO: carregar os dados somente quando carregar/deletar um arquivo

Window::DataPicker::DataPicker(bool* isOpen) : IWindow(isOpen) {
    title = "Selecionador de Dados";
    flags = ImGuiWindowFlags_MenuBar;
    // this->refreshData();
}

void Window::DataPicker::renderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Arquivos")) {
            if (ImGui::MenuItem("Carregar")) {
                DB::getInstance().loadCSVDialog();
                // this->refreshData();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::DataPicker::sendArchivePayload(const std::string& filename) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        std::string payload = filename;
        ImGui::SetDragDropPayload("FILE_NAME", payload.c_str(), payload.size() + 1);
        ImGui::Text("Arquivo: %s", filename.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::sendColumnPayload(const std::string& filename, const std::string& columnName) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        std::string payload = filename + ":" + columnName;
        ImGui::SetDragDropPayload("COLUMN_NAME", payload.c_str(), payload.size() + 1);
        ImGui::Text("%s", columnName.c_str());
        ImGui::EndDragDropSource();
    }
}

void Window::DataPicker::renderArchiveContextPopup(const std::filesystem::path& archivePath) {
    if (ImGui::BeginPopupContextItem((archivePath.filename().string() + "_popup").c_str())) {
        if (ImGui::MenuItem("Fechar")) {
            DB::getInstance().deleteCSV(archivePath);
            // this->refreshData();
        }
        ImGui::EndPopup();
    }
}

void Window::DataPicker::refreshData() {
    this->paths   = DB::getInstance().getCsvPaths();
    this->columns = DB::getInstance().getCsvColumns();
}

void Window::DataPicker::renderArchiveNode(const std::filesystem::path& archivePath, size_t index) {
    std::string filename = archivePath.filename().string();
    if (ImGui::TreeNode((filename + "##" + std::to_string(index)).c_str())) {
        this->sendArchivePayload(filename); // Inicia o payload de drag & drop para o arquivo

        // Renderiza cada coluna do arquivo
        const std::vector<std::string>& cols = this->columns[index];
        for (const std::string& colName : cols) {
            this->renderColumnItem(filename, colName);
        }
        ImGui::TreePop();
    }
}

void Window::DataPicker::renderColumnItem(const std::string& filename, const std::string& colName) {
    ImGui::Selectable(colName.c_str());
    this->sendColumnPayload(filename, colName);
}

void Window::DataPicker::render() {
    if (this->isOpen && *this->isOpen) {
        this->refreshData();
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        this->renderMenuBar();
        ImGui::BeginChild("##dataPicker", ImGui::GetContentRegionAvail(), true, ImGuiWindowFlags_HorizontalScrollbar);

        // Renderiza cada arquivo e seu respectivo menu de contexto
        for (size_t i = 0; i < this->paths.size(); i++) {
            this->renderArchiveNode(paths[i], i);
            this->renderArchiveContextPopup(paths[i]);
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
