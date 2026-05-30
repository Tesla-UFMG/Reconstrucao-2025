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
                        ImGui::TextColored(greenColor, "Página 1: Novas Janelas Dinâmicas e Análise");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Várias Janelas (Novidade): Agora dá pra abrir várias janelas de Gráficos, Tabelas e "
                            "outras (novidades abaixo) ao mesmo tempo. Não vai ficar mais travado usando apenas "
                            "uma de cada tipo.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Janela Numérica: Janela rápida para ver valores. Ela muda de cor "
                                           "dependendo da faixa de "
                                           "valor, aceita fórmulas na hora, converte o valor pra texto e mostra de "
                                           "onde o dado veio "
                                           "quando você passa o mouse por cima. Muito daora mesmo, dá uma olhada.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Matriz de Dados: Funciona como a Janela Numérica, porém em formato de grade. Funciona "
                            "perfeitamente para ver as dezenas de tensões das stacks que o Estevão fica pedindo.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Janela de Barras: Muito útil para ver como o dado ta se comportando "
                                           "com base em um limite que você impôs. Da pra colocar limite numérico, "
                                           "colocar um gradiente de cor...");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Gráficos Avançados: É possível colocar textos no gráfico agora, só arrastar as "
                            "colunas de texto que estão no selecionador de dados para ele! Também "
                            "resolvi alguns bugs de plotagem XY e de dados com IDs diferentes.");
                        ImGui::Spacing();

                        break;
                    }
                    case 1: {
                        ImGui::TextColored(greenColor, "Página 2: Telemetria e Registro de Avisos");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Controle de Telemetria: Ficou bem mais fácil registrar os testes (como mudar o "
                            "Piloto) e tem um atalho rápido pra limpar a tela da UART.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Tempo e Marcações: Agora você consegue ver o tempo "
                            "total de gravação rolando e salvar anotações rápidas de texto junto com os dados. "
                            "Você nunca mais precisará gravar áudios que nem besta, Raphael.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped(
                            "Avisos e Logs: Dá pra definir os limites das variáveis. Por exemplo, se o valor sair "
                            "do limite, "
                            "o sistema avisa na tela, salva no selecionador de dados e ainda exporta tudo pra CSV. "
                            "Vai da pra jogar os textos nos gráficos e na Reconstrução também...");
                        break;
                    }
                    case 2: {
                        ImGui::TextColored(greenColor, "Página 3: Cockpit, GNSS e Salvamento de Layouts");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Cockpit Virtual: O volante e os pedais foram atualizado. A janela tá "
                                           "respondendo melhor aos dados em tempo real.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Mapa GNSS: AGORA TEM MAPINHA! Ta muito doido.");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Salvamento de Layouts: Agora vai salvar tudo, absolutamente tudo.");
                        break;
                    }
                    case 3: {
                        ImGui::TextColored(greenColor, "Página 4: Janela de Atualização");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Agora tem uma janela de atualização (que ninguém vai atualizar depois que "
                                           "eu sair dessa equipe)");
                        break;
                    }
                    case 4: {
                        ImGui::TextColored(greenColor, "Página 5: Desabafo");
                        ImGui::Spacing();

                        ImGui::Bullet();
                        ImGui::SameLine(0.0f, 6.0f);
                        ImGui::TextWrapped("Quero aposentar gente pelo amor de Deus");
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
                int   totalPages = 5;
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