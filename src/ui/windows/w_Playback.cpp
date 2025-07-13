// Em src/ui/windows/w_Playback.cpp
#include "ui/windows/w_Playback.hpp"
#include "Log.hpp"
#include <cstring>
#include <string> // Necessário para std::to_string

Window::Playback::Playback(bool* isOpen, const std::vector<IPlayable*>& playables)
    : IWindow(isOpen), m_playables(playables) {
    this->title = "Playback";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::Playback::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    if (!m_isCreatingGroup) {
        if (ImGui::Button("Criar Novo Grupo de Sincronização")) {
            m_isCreatingGroup = true;
            m_groupCreationSelection.assign(m_playables.size(), false);
            memset(m_newGroupNameBuffer, 0, sizeof(m_newGroupNameBuffer));
        }
    }

    if (m_isCreatingGroup) {
        RenderGroupCreationUI();
    }
    
    ImGui::Separator();

    RenderGroupControls();

    ImGui::Separator();

    RenderIndividualControls();

    ImGui::End();
}

void Window::Playback::RenderGroupCreationUI() {
    ImGui::BeginGroup();
    ImGui::Text("Criar Novo Grupo");
    ImGui::Separator();
    
    ImGui::InputText("Nome do Grupo", m_newGroupNameBuffer, sizeof(m_newGroupNameBuffer));
    
    ImGui::Text("Selecione os itens para agrupar:");
    for (size_t i = 0; i < m_playables.size(); ++i) {
        if (m_playables[i] && m_playables[i]->isLoaded()) {
            std::string checkbox_label = std::string(m_playables[i]->getTitle()) + "##group_checkbox";
            
            bool selection = m_groupCreationSelection[i];
            if (ImGui::Checkbox(checkbox_label.c_str(), &selection)) {
                m_groupCreationSelection[i] = selection;
            }
        }
    }

    if (ImGui::Button("Confirmar")) {
        std::string groupName = m_newGroupNameBuffer;
        if (!groupName.empty()) {
            std::set<IPlayable*> newGroup;
            for (size_t i = 0; i < m_playables.size(); ++i) {
                if (m_groupCreationSelection[i]) {
                    newGroup.insert(m_playables[i]);
                }
            }
            if (!newGroup.empty()) {
                m_playbackGroups[groupName] = newGroup;
                LOG("INFO", "Grupo de Playback '" + groupName + "' criado.");
            }
        }
        m_isCreatingGroup = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancelar")) {
        m_isCreatingGroup = false;
    }
    ImGui::EndGroup();
}

void Window::Playback::RenderGroupControls() {
    if (m_playbackGroups.empty()) {
        ImGui::TextDisabled("Nenhum grupo de sincronização criado.");
        return;
    }

    ImGui::Text("Controles Sincronizados");

    for (auto it = m_playbackGroups.begin(); it != m_playbackGroups.end(); ) {
        const std::string& name = it->first;
        const std::set<IPlayable*>& group = it->second;

        if (group.empty()) {
            ++it;
            continue;
        }

        ImGui::PushID(name.c_str());
        
        if (ImGui::Button("X")) {
            it = m_playbackGroups.erase(it);
            ImGui::PopID();
            continue;
        }
        ImGui::SameLine();

        bool isAnyPlaying = false;
        for (IPlayable* playable : group) {
            if (playable->isPlaying()) {
                isAnyPlaying = true;
                break;
            }
        }

        if (isAnyPlaying) {
            if (ImGui::Button("Pausar Grupo")) {
                for (IPlayable* playable : group) {
                    playable->pause();
                }
            }
        } else {
            if (ImGui::Button("Iniciar Grupo")) {
                for (IPlayable* playable : group) {
                    playable->play();
                }
            }
        }
        ImGui::SameLine();
        ImGui::Text(name.c_str());

        ImGui::PopID();
        ++it;
    }
}

// --- CONTROLES INDIVIDUAIS COM DEBUG ADICIONADO ---
void Window::Playback::RenderIndividualControls() {
    bool anyLoaded = false;
    for (IPlayable* playable : m_playables) {
        if (playable && playable->isLoaded()) {
            anyLoaded = true;
            if (ImGui::CollapsingHeader(playable->getTitle())) {
                
                ImGui::PushID(playable);

                float step_size = playable->getStepSize();
                
                ImGui::PushItemWidth(80.0f);
                if (ImGui::InputFloat("Passo", &step_size, 1.0f, 10.0f, "%.1f")) {
                    playable->setStepSize(step_size);
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();

                // Usar double para consistência com a interface IPlayable
                double currentTime = playable->getCurrentTime();
                const double duration = playable->getDuration();
                bool isPlaying = playable->isPlaying();

                if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
                    // --- DEBUG ---
                    double newPosition = currentTime - step_size;
                    LOG("DEBUG", "[Playback] Seta Esquerda: tempo_atual=" + std::to_string(currentTime) + ", passo=" + std::to_string(step_size) + ", nova_posicao=" + std::to_string(newPosition));
                    playable->seek(newPosition);
                }
                ImGui::SameLine();

                if (isPlaying) {
                    if (ImGui::Button("Pausar")) { playable->pause(); }
                } else {
                    if (ImGui::Button("Iniciar")) { playable->play(); }
                }
                ImGui::SameLine();

                if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
                    // --- DEBUG ---
                    double newPosition = currentTime + step_size;
                    LOG("DEBUG", "[Playback] Seta Direita: tempo_atual=" + std::to_string(currentTime) + ", passo=" + std::to_string(step_size) + ", nova_posicao=" + std::to_string(newPosition));
                    playable->seek(newPosition);
                }
                ImGui::SameLine();
                
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120);
                // O slider precisa de um float, então fazemos a conversão aqui
                float sliderCurrentTime = static_cast<float>(currentTime);
                if (ImGui::SliderFloat("##Tempo", &sliderCurrentTime, 0.0f, static_cast<float>(duration), "%.2f")) {
                    playable->seek(sliderCurrentTime);
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();

                ImGui::Text("%.2f / %.2f", playable->getCurrentTime(), duration);

                ImGui::PopID();
            }
        }
    }
    if (!anyLoaded && m_playbackGroups.empty()) {
        ImGui::Text("Carregue dados em uma janela (Vídeo, Reconstrução) para habilitar os controles.");
    }
}