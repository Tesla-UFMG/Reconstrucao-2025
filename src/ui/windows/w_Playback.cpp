#include "ui/windows/w_Playback.hpp"

Window::Playback::Playback(bool* isOpen) : IWindow(isOpen) {
    this->title = "Playback";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::Playback::render() {
    static float counter        = 0;
    static int   selectedButton = 0;

    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##playbackSlider", &counter, 0.00f, 10.00f, "%.2f");

        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
            counter -= 0.01;
        }
        ImGui::SameLine();
        ImGui::RadioButton("Parar", &selectedButton, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Iniciar", &selectedButton, 1);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
            counter += 0.01;
        }
        ImGui::SameLine();
        ImGui::Text("%.2f/10.00s", counter);

        ImGui::End();
    }
}