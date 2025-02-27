#include "ui/DatapickerWindow.hpp"

void Window::Datapicker(bool* isOpen) {
    if (*isOpen) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin("Selecionador de Dados", isOpen, flags)) {

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Arquivos")) {
                    if (ImGui::MenuItem("Carregar")) {
                        DB::getInstance().loadDataDialog();
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
                    if (ImGui::TreeNode(paths[i].filename().string().c_str())) {
                        std::vector<std::string> columns_ = columns[i];
                        for (const std::string& text : columns_) {
                            ImGui::TextUnformatted(text.c_str());
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