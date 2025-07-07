#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Video.hpp"

Window::Playback::Playback(bool* isOpen, Video* video_window) : IWindow(isOpen), m_videoWindow(video_window) {
    this->title        = "Playback";
    this->flags        = ImGuiWindowFlags_NoScrollbar;
    this->m_sliderTime = 0.0f;
}

void Window::Playback::render() {
    if (!this->isOpen || !*this->isOpen) {
        return;
    }

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    if (m_videoWindow && m_videoWindow->isLoaded()) {

        static float jumpStep  = 1.0f;
        const float  maxTime   = static_cast<float>(m_videoWindow->getDuration());
        bool         isPlaying = m_videoWindow->isPlaying();

        if (!ImGui::IsItemActive()) {
            m_sliderTime = static_cast<float>(m_videoWindow->getCurrentTime());
        }

        ImGui::SliderFloat("##Tempo", &m_sliderTime, 0.0f, maxTime, "%.2f s");

        if (ImGui::IsItemActive()) {
            m_videoWindow->seek(m_sliderTime);
        }

        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
            m_videoWindow->seek(m_videoWindow->getCurrentTime() - jumpStep);
        }
        ImGui::SameLine();

        if (isPlaying) {
            if (ImGui::Button("Pausar")) {
                m_videoWindow->pause();
            }
        } else {
            if (ImGui::Button("Iniciar")) {
                m_videoWindow->play();
            }
        }
        ImGui::SameLine();

        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {

            printf("PLAYBACK: Botão >> clicado. Tempo Atual = %.3f, Passo = %.3f, Buscando por = %.3f\n",
                   m_videoWindow->getCurrentTime(), jumpStep, m_videoWindow->getCurrentTime() + jumpStep);

            m_videoWindow->seek(m_videoWindow->getCurrentTime() + jumpStep);
        }
        ImGui::SameLine();

        ImGui::Text("%.2f s / %.2f s", m_videoWindow->getCurrentTime(), m_videoWindow->getDuration());

        ImGui::SliderFloat("Intervalo (s)", &jumpStep, 0.1f, 5.0f, "%.1f s");

    } else {
        ImGui::Text("Carregue um vídeo na janela 'Vídeo' para habilitar os controles.");
    }

    ImGui::End();
}