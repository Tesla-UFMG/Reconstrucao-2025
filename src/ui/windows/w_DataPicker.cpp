#include "ui/windows/w_DataPicker.hpp"
#include "DB.hpp"
#include "imgui.h"
#include <string>

void Window::MenuBar::Datapicker() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Arquivos")) {
            if (ImGui::MenuItem("Carregar")) {
                DB::getInstance().loadDataDialog();
            }

            if (ImGui::BeginMenu("Fechar")) {
                for (const std::filesystem::path& path : DB::getInstance().getCsvPaths()) {
                    if (ImGui::MenuItem(path.filename().string().c_str())) {
                        DB::getInstance().removeData(path);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Window::Datapicker(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        ImGui::Begin("Selecionador de Dados", isOpen, flags);

        Window::MenuBar::Datapicker();

        if (ImGui::BeginChild("##dataPicker", ImGui::GetContentRegionAvail(), true,
                              ImGuiWindowFlags_HorizontalScrollbar)) {

            const std::vector<std::filesystem::path>&    paths   = DB::getInstance().getCsvPaths();
            const std::vector<std::vector<std::string>>& columns = DB::getInstance().getCsvColumns();

            for (size_t i = 0; i < paths.size(); i++) {
                std::string filename = paths[i].filename().string();
                if (ImGui::TreeNode(filename.c_str())) {

                    // Payload nome do arquivo ----------------
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                        std::string payload = filename;
                        ImGui::SetDragDropPayload("FILE_NAME", payload.c_str(), payload.size() + 1);
                        ImGui::Text("Arquivo: %s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }

                    // Itera sobre cada coluna do arquivo
                    const std::vector<std::string>& cols = columns[i];
                    for (const std::string& colName : cols) {
                        ImGui::Selectable(colName.c_str());

                        // Payload nome da coluna ----------------
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                            std::string payload = filename + ":" + colName;
                            ImGui::SetDragDropPayload("COLUMN_NAME", payload.c_str(), payload.size() + 1);
                            ImGui::Text("%s", colName.c_str());
                            ImGui::EndDragDropSource();
                        }
                    }

                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }
}
