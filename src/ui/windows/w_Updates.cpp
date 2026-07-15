#include "ui/windows/w_Updates.hpp"
#include "Log.hpp"
#include <fstream>

namespace Window {
    Updates::Updates(bool* isOpen) : IWindow(isOpen) {
        this->title = "Novidades da Versão";
        this->flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

        m_dontShowAgain = checkHideFileExists();
    }

    std::filesystem::path Updates::getHideFilePath() const {
        return std::filesystem::current_path() / ".tesla_updates_seen";
    }

    bool Updates::checkHideFileExists() const { return std::filesystem::exists(getHideFilePath()); }

    void Updates::writeHideFile() {
        std::ofstream file(getHideFilePath());
        if (file) {
            file << "seen";
            file.close();
            LOG("INFO", "Arquivo oculto de atualizações visto criado em: " + getHideFilePath().string());
        }
    }

    void Updates::removeHideFile() {
        std::error_code ec;
        if (std::filesystem::remove(getHideFilePath(), ec)) {
            LOG("INFO", "Arquivo oculto de atualizações visto removido.");
        }
    }

    void Updates::render() {
        if (this->isOpen && *this->isOpen) {
            if (!m_popupOpen) {
                ImGui::OpenPopup("Novidades da Versão");
                m_popupOpen     = true;
                m_currentPage   = 0;
                m_dontShowAgain = checkHideFileExists();
            }

            // Centralizar a janela modal
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            // Aumentei um pouco a altura para acomodar melhor o texto e mudei para ImGuiCond_Appearing
            ImGui::SetNextWindowSize(ImVec2(540, 440), ImGuiCond_Appearing);

            // Estilos premium para o modal
            // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
            // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);

            bool keepOpen = true;
            if (ImGui::BeginPopupModal("Novidades da Versão", &keepOpen, this->flags)) {

                // --- ÁREA DE ROLAGEM (Evita que o texto vaze da tela) ---
                // O Y = -90.0f garante que a área de texto vai ocupar todo o espaço,
                // exceto os últimos 90 pixels que ficam reservados para os botões do rodapé.
                ImGui::BeginChild("##ConteudoTexto", ImVec2(0, -90.0f));

                // Definindo o verde padrão
                ImVec4 greenColor = ImVec4(0.15f, 0.8f, 0.15f, 1.0f);

                // --- Conteúdo da Página (com quebra de linha em marcadores) ---
                switch (m_currentPage) {
                    case 0: {
                        ImGui::TextColored(greenColor, "Página 1: Vídeo e Playback Interativo");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Janela de Vídeo (Novidade): Agora tem suporte a vídeo! Dá pra abrir "
                            "as gravações on-board das corridas e ver exatamente o que rolou na pista.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Playback Interativo: O aplicativo agora tem uma barra de tempo de verdade! "
                                           "Arrastou a barrinha? Os gráficos, as matrizes e o mapa viajam no tempo junto com o vídeo, "
                                           "tudo sincronizado.");
                        ImGui::Spacing();

                        break;
                    }
                    case 1: {
                        ImGui::TextColored(greenColor, "Página 2: Novidades da Reconstrução GNSS");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Câmera Magnética: Uma nova opção 'Seguir Final' na Reconstrução de Pista. "
                            "Se você ligar, a câmera prende no carro e não solta mais, acompanhando o movimento.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Modo GPS Dinâmico: A cereja do bolo! Ativou 'Girar com o Veículo'? O mapa inteiro gira "
                            "nas curvas pra frente do carro ficar sempre apontada pra cima, igualzinho ao Waze e Google Maps.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Limitar Rastro: Coloquei uma caixinha pra você decidir quantos pontos quer ver do trajeto. "
                            "Perfeito pra quem odeia a tela poluída com aquela cauda infinita de pontos.");
                        break;
                    }
                    case 2: {
                        ImGui::TextColored(greenColor, "Página 3: Estabilidade e Correções");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Salvamento dos Layouts: Os saves agora guardam e lembram certinho de todas essas opções "
                                           "malucas novas de mapa, e não perdem mais as configurações.");
                        ImGui::Spacing();
                        
                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Tudo mais leve: O motor de renderização do mapa e da rotação foi feito pra rodar liso. "
                                           "Mesmo com dados pesados de telemetria, o programa continua voando.");
                        break;
                    }
                    default:
                        break;
                }

                ImGui::EndChild(); // Fim da área de rolagem

                // --- Espaçador ---
                // Como o BeginChild já separou o espaço corretamente, o SetCursorPosY manual não é mais necessário.
                ImGui::Separator();
                ImGui::Spacing();

                // --- Cálculo de Tamanhos Dinâmicos dos Botões ---
                float btnHeight = 28.0f;
                float padding   = ImGui::GetStyle().FramePadding.x * 2.0f;

                ImVec2 prevBtnSize  = ImGui::CalcTextSize("< Anterior");
                prevBtnSize.x      += padding;
                prevBtnSize.y       = btnHeight;

                ImVec2 nextBtnSize  = ImGui::CalcTextSize("Próximo >");
                nextBtnSize.x      += padding;
                nextBtnSize.y       = btnHeight;

                // --- Rodapé: Paginação e Controles ---

                // Botão Anterior
                if (m_currentPage > 0) {
                    if (ImGui::Button("< Anterior", prevBtnSize)) {
                        m_currentPage--;
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::Button("< Anterior", prevBtnSize);
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                // Paginação (bolinhas) no centro
                int   totalPages = 3;
                float availWidth = ImGui::GetContentRegionAvail().x;
                float dotsWidth  = totalPages * 10.0f + (totalPages - 1) * ImGui::GetStyle().ItemSpacing.x;
                float startDotsX = ImGui::GetCursorPosX() + (availWidth - nextBtnSize.x - dotsWidth) / 2.0f;
                ImGui::SetCursorPosX(startDotsX);

                for (int i = 0; i < totalPages; i++) {
                    ImGui::PushID(i);
                    bool isActive = (m_currentPage == i);

                    // Cor premium para a bolinha ativa atualizada para o Verde Tecnológico
                    ImVec4 dotColor = isActive ? ImVec4(0.15f, 0.8f, 0.15f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
                    ImGui::PushStyleColor(ImGuiCol_Button, dotColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                          isActive ? dotColor : ImVec4(0.2f, 0.9f, 0.2f, 0.8f)); // Hover mais claro
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, dotColor);

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f); // Bolinha

                    if (ImGui::Button(" ", ImVec2(10, 10))) {
                        m_currentPage = i;
                    }

                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(3);
                    ImGui::PopID();

                    if (i < totalPages - 1) {
                        ImGui::SameLine();
                    }
                }

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 24.0f - nextBtnSize.x);

                // Botão Próximo
                if (m_currentPage < totalPages - 1) {
                    if (ImGui::Button("Próximo >", nextBtnSize)) {
                        m_currentPage++;
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::Button("Próximo >", nextBtnSize);
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();
                ImGui::Spacing();

                // --- Linha Inferior com Checkbox ---
                if (ImGui::Checkbox("Não mostrar novamente", &m_dontShowAgain)) {
                    if (m_dontShowAgain) {
                        writeHideFile();
                    } else {
                        removeHideFile();
                    }
                }

                // Se o usuário clicou no "X" da própria janela do ImGui
                if (!keepOpen) {
                    *this->isOpen = false;
                    m_popupOpen   = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            } else {
                *this->isOpen = false;
                m_popupOpen   = false;
            }

            // ImGui::PopStyleVar(2);
        } else {
            m_popupOpen = false;
        }
    }
} // namespace Window