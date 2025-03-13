#include "ui/LogWindow.hpp"

void Window::Log(bool* isOpen) {
    if (*isOpen) {
        ImGui::Begin("Log", isOpen, ImGuiWindowFlags_NoScrollbar);

        {
            ImVec2 available_size  = ImGui::GetContentRegionAvail();
            available_size.y      -= ImGui::GetFrameHeightWithSpacing(); // Subtraindo o tamanho do botão
            ImGui::BeginChild("LogScroll", available_size, true, ImGuiWindowFlags_HorizontalScrollbar);
            std::vector<std::string> lines = Log::getInstance().getLog();
            for (const std::string& line : lines) {
                ImGui::TextUnformatted(line.c_str());
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }

            ImGui::EndChild();
        }

        if (ImGui::Button("Limpar")) {
            Log::getInstance().clearLog();
        }
        ImGui::SetItemTooltip("Limpa o Log\nCTRL + L");
        ImGui::End();
    }
}