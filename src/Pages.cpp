#include "Pages.hpp"

namespace Pages {
    bool showAbout = false;
    bool showPlayback = false;
    bool showDataPicker = false;
    bool showReconstruction = false;
    bool showVideo = false;
    bool showPlot = false;
}

void Pages::about() {
    if (Pages::showAbout) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        ImGui::Begin("Sobre", &Pages::showAbout, flags);
        ImGui::SeparatorText("Resconstrução de Pista");
        ImGui::Text("Formula Tesla");
        ImGui::Text("Edição: 2025");
        ImGui::Text("Versão: 1.0.0");

        ImGui::SeparatorText("Desenvolvedores");
        ImGui::BulletText("Lucas Martins Rocha");
        ImGui::BulletText("Gabriel Matos");

        ImGui::SeparatorText("Contatos");
        ImGui::Text("Está precisando de uma ajuda?");
        ImGui::Text("Email: lucasrocha.png@gmail.com");

        ImGui::End();
    }
}

void Pages::playback() {
    static float current = 0;
    static int selectedButton = 0;

    if (Pages::showPlayback) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("Playback", &Pages::showPlayback, flags);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##playbackSlider", &current, 0.00f, 10.00f, "%.2f");


        if (ImGui::Button("<")) {
            current -= 0.01;
        } ImGui::SameLine();
        ImGui::RadioButton("Parar", &selectedButton, 0); ImGui::SameLine();
        ImGui::RadioButton("Iniciar", &selectedButton, 1); ImGui::SameLine();
        if (ImGui::Button(">")) {
            current += 0.01;
        } ImGui::SameLine();
        ImGui::Text("%.2f/10.00", current);

        ImGui::End();
    }
}

void Pages::dataPicker() {
    if (Pages::showDataPicker) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Selecionador de Dados", &Pages::showDataPicker, flags);
        ImGui::End();
    }
}

void Pages::circuitReconstruction() {
    if (Pages::showReconstruction) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Reconstrução de Pista", &Pages::showReconstruction, flags);
        ImGui::End();
    }
}

void Pages::video() {
    if (Pages::showVideo) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Video", &Pages::showVideo, flags);
        ImGui::End();
    }
}

void Pages::plot() {
    if (Pages::showPlot) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Plot", &Pages::showPlot, flags);
        ImGui::End();
    }
}

void Pages::render() { 
    Pages::about();
    Pages::playback();
    Pages::dataPicker();
    Pages::circuitReconstruction();
    Pages::video();
    Pages::plot();
}