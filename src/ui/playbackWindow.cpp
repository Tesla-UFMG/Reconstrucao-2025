#include "ui/PlaybackWindow.hpp"

void Window::Playback(bool* isOpen) {
    static float counter        = 0;
    static int   selectedButton = 0;

    if (*isOpen) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin("Playback", isOpen, flags)) {

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
}