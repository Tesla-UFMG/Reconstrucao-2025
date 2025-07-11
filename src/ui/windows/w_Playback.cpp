// Em src/ui/windows/w_Playback.cpp
#include "ui/windows/w_Playback.hpp"
#include "Log.hpp"
#include <cstring>

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
            
            // --- CORREÇÃO DO CONFLITO DE ID ---
            // Criamos um rótulo único para o checkbox adicionando "##group_checkbox"
            // Isso o diferencia de outros widgets que usam o mesmo título.
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

    // Usamos um iterador para poder remover itens do mapa de forma segura enquanto iteramos
    for (auto it = m_playbackGroups.begin(); it != m_playbackGroups.end(); ) {
        const std::string& name = it->first;
        const std::set<IPlayable*>& group = it->second;

        if (group.empty()) {
            ++it; // Pula para o próximo item
            continue;
        }

        ImGui::PushID(name.c_str());
        
        // --- BOTÃO DE REMOÇÃO ADICIONADO ---
        if (ImGui::Button("X")) {
            // Se o botão 'X' for clicado, marca o iterador para ser apagado e quebra o loop interno
            // para evitar acessar dados que serão deletados.
            it = m_playbackGroups.erase(it);
            ImGui::PopID();
            continue; // Continua para a próxima iteração do loop for
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
        ++it; // Avança o iterador para o próximo grupo
    }
}

// --- CONTROLES INDIVIDUAIS REATIVADOS ---
void Window::Playback::RenderIndividualControls() {
    bool anyLoaded = false;
    for (IPlayable* playable : m_playables) {
        if (playable && playable->isLoaded()) {
            anyLoaded = true;
            if (ImGui::CollapsingHeader(playable->getTitle())) {
                
                // Todo o nosso código de controle individual está de volta aqui.
                ImGui::PushID(playable);

                float step_size = playable->getStepSize();
                
                ImGui::PushItemWidth(80.0f);
                if (ImGui::InputFloat("Passo", &step_size, 1.0f, 10.0f, "%.1f")) {
                    playable->setStepSize(step_size);
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();

                float currentTime = static_cast<float>(playable->getCurrentTime());
                const float duration = static_cast<float>(playable->getDuration());
                bool isPlaying = playable->isPlaying();

                if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
                    // A lógica das setas pode ser revisitada aqui quando você quiser
                    playable->seek(currentTime - step_size);
                }
                ImGui::SameLine();

                if (isPlaying) {
                    if (ImGui::Button("Pausar")) { playable->pause(); }
                } else {
                    if (ImGui::Button("Iniciar")) { playable->play(); }
                }
                ImGui::SameLine();

                if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
                    playable->seek(currentTime + step_size);
                }
                ImGui::SameLine();
                
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120);
                if (ImGui::SliderFloat("##Tempo", &currentTime, 0.0f, duration, "%.2f")) {
                    playable->seek(currentTime);
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