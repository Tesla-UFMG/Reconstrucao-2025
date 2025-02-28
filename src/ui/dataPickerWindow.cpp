#include "ui/DatapickerWindow.hpp"
#include "DB.hpp"
#include "imgui.h"
#include <string>

void Window::Datapicker(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin("Selecionador de Dados", isOpen, flags)) {

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

            if (ImGui::BeginChild("##dataPicker", ImGui::GetContentRegionAvail(), true,
                                  ImGuiWindowFlags_HorizontalScrollbar)) {

                const std::vector<std::filesystem::path>&    paths   = DB::getInstance().getCsvPaths();
                const std::vector<std::vector<std::string>>& columns = DB::getInstance().getCsvColumns();

                for (size_t i = 0; i < paths.size(); i++) {
                    std::string filename = paths[i].filename().string();
                    if (ImGui::TreeNode(filename.c_str())) {

                        // Payload filename
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                            std::string payload = filename;
                            ImGui::SetDragDropPayload("FILE_NAME", payload.c_str(), payload.size() + 1);
                            ImGui::Text("Arquivo: %s", filename.c_str());
                            ImGui::EndDragDropSource();
                        }

                        const std::vector<std::string>& cols = columns[i];
                        for (const std::string& colName : cols) {
                            ImGui::Selectable(colName.c_str());

                            // Payload column name
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                                ImGui::SetDragDropPayload("COLUMN_NAME", colName.c_str(), colName.size() + 1);
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
}
