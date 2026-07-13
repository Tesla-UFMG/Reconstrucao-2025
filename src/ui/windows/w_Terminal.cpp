#include "ui/windows/w_Terminal.hpp"

Window::Terminal::Terminal(bool* isOpen) : IWindow(isOpen) {
    this->title = "Terminal";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::Terminal::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        {
            ImVec2 available_size  = ImGui::GetContentRegionAvail();
            available_size.y      -= ImGui::GetFrameHeightWithSpacing(); // Subtraindo o tamanho do botão
            ImGui::BeginChild("LogScroll", available_size, true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::vector<std::string> lines = Log::getInstance().getLog();
                for (const std::string& line : lines) {
                    ImGui::TextUnformatted(line.c_str());
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();

            if (ImGui::Button("Limpar")) {
                Log::getInstance().clearLog();
            }
            ImGui::SetItemTooltip("Limpa o Log\nCTRL + L");
        }
        ImGui::End();
    }
}