#include "ui/windows/w_Video.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "DB.hpp"
#include <imgui.h>

namespace Window {

Video::Video(bool* showFlag, SDL_Renderer* renderer) 
    : m_showFlag(showFlag), m_player(renderer) {
}

void Video::setLoadedVideo(const std::string& path) {
    m_currentArchiveName = path;
    if (!path.empty()) {
        m_player.load(path);
        m_player.play();
        m_player.setVolume(static_cast<int>(m_volume));
    } else {
        m_player.stop();
    }
}

void Video::setVolume(float volume) {
    m_volume = volume;
    m_player.setVolume(static_cast<int>(m_volume));
}

void Video::render() {
    if (!*m_showFlag) {
        // Se a janela for fechada e o video estava rodando, podemos pausar ou parar, 
        // mas como é estática, o usuário pode querer que continue tocando. 
        // Para economizar recurso, podemos pausar. Mas manteremos simples.
        return;
    }
    
    // Atualiza textura da libvlc
    m_player.updateTexture();

    ImGui::Begin("Vídeo", m_showFlag);

    // Menu de contexto com botão direito na área da janela
    if (ImGui::BeginPopupContextWindow("ContextoVideo")) {
        if (ImGui::SliderFloat("Volume", &m_volume, 0.0f, 100.0f, "%.0f")) {
            m_player.setVolume(static_cast<int>(m_volume));
        }
        ImGui::Separator();
        if (ImGui::Button("Remover Vídeo", ImVec2(-1, 0))) {
            setLoadedVideo(""); // Remove e para o vídeo
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (m_currentArchiveName.empty()) {
        // Mostra placeholder
        ImGui::TextDisabled("Arraste um vídeo para o Playback para carregar aqui.");
        ImGui::Dummy(avail);
    } else {
        // Tenta renderizar o frame
        SDL_Texture* tex = m_player.getTexture();
        float vW = static_cast<float>(m_player.getWidth());
        float vH = static_cast<float>(m_player.getHeight());
        
        if (tex && vW > 0.0f && vH > 0.0f) {
            // Mantém a proporção (aspect ratio)
            float aspect = vW / vH;
            float displayW = avail.x;
            float displayH = avail.x / aspect;
            
            if (displayH > avail.y) {
                displayH = avail.y;
                displayW = avail.y * aspect;
            }
            
            // Centraliza o vídeo
            float curX = ImGui::GetCursorPosX();
            float curY = ImGui::GetCursorPosY();
            ImGui::SetCursorPos(ImVec2(curX + (avail.x - displayW) * 0.5f, curY + (avail.y - displayH) * 0.5f));
            
            ImGui::Image(reinterpret_cast<ImTextureID>(tex), ImVec2(displayW, displayH));
        } else {
            ImGui::TextDisabled("Carregando vídeo...");
            ImGui::Dummy(avail);
        }
    }

    ImGui::End();
}

} // namespace Window
