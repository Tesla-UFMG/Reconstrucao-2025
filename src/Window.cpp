#include "Window.hpp"

Window::VisibilityFlags Window::visibility;

void Window::about() {
    if (Window::visibility.showAbout) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
        ImGui::Begin("Sobre", &Window::visibility.showAbout, flags);
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

void Window::playback() {
    static float current        = 0;
    static int   selectedButton = 0;

    if (Window::visibility.showPlayback) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("Playback", &Window::visibility.showPlayback, flags);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##playbackSlider", &current, 0.00f, 10.00f, "%.2f");

        if (ImGui::Button("<")) {
            if (current > 0)
                current -= 0.01;
        }
        ImGui::SameLine();
        ImGui::RadioButton("Parar", &selectedButton, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Iniciar", &selectedButton, 1);
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            current += 0.01;
        }
        ImGui::SameLine();
        ImGui::Text("%.2f/10.00s", current);

        ImGui::End();
    }
}

void Window::dataPicker() {
    if (Window::visibility.showDataPicker) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Selecionador de Dados", &Window::visibility.showDataPicker, flags);
        ImGui::End();
    }
}

void Window::circuitReconstruction() {
    if (Window::visibility.showReconstruction) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Reconstrução de Pista", &Window::visibility.showReconstruction, flags);
        ImGui::End();
    }
}

void Window::video() {
    if (Window::visibility.showVideo) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Video", &Window::visibility.showVideo, flags);
        ImGui::End();
    }
}

void Window::plot() {
    if (Window::visibility.showPlot) {
        ImGuiWindowFlags flags = 0;
        ImGui::Begin("Plot", &Window::visibility.showPlot, flags);
        ImGui::End();
    }
}

void Window::log() {
    if (Window::visibility.showLog) {
        std::vector<std::string> lines = Log::getInstance().getLog();
        ImGui::Begin("Log", &Window::visibility.showLog);

        ImGui::BeginChild("LogScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const std::string& line : lines) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();
    }
}

void Window::ImGuiDemo() {
    if (Window::visibility.showImGuiDemo) {
        ImGui::ShowDemoWindow(&Window::visibility.showImGuiDemo);
    }
}

void Window::ImPlotDemo() {
    if (Window::visibility.showImPlotDemo) {
        ImPlot::ShowDemoWindow(&Window::visibility.showImPlotDemo);
    }
}

void Window::changeWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    *windowVisibility = !(*windowVisibility);
    std::string message =
        *windowVisibility ? "Foi aberta a janela '" + windowName + "'." : "Foi fechada a janela '" + windowName + "'.";
    LOG("TRACE", message);
}

void Window::render() {
    Window::about();
    Window::playback();
    Window::dataPicker();
    Window::circuitReconstruction();
    Window::video();
    Window::plot();
    Window::log();
    Window::ImGuiDemo();
    Window::ImPlotDemo();
}
