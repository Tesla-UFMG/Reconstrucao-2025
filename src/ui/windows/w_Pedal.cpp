#include "ui/windows/w_Pedal.hpp"

// Variáveis que simulam a pressão dos pedais (valores de 0 a 1)
static float throttleValue = 0.0f;
static float brakeValue    = 0.0f;

Window::Pedal::Pedal(bool* isOpen) : IWindow(isOpen) {
    this->title             = "Pedal";
    this->flags             = ImGuiWindowFlags_NoScrollbar;
    this->redPedalTexture   = (ImTextureID)AssetManager::getInstance().getTexture("assets/pedalvermelho.png");
    this->greenPedalTexture = (ImTextureID)AssetManager::getInstance().getTexture("assets/pedalverde.png");
}

void Window::Pedal::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        // Obtém o tamanho disponível na janela
        ImVec2 avail = ImGui::GetContentRegionAvail();

        // Dimensões fixas para os pedais
        float  pedalWidth  = avail.x * 0.35f;
        float  pedalHeight = avail.y * 0.7f;
        ImVec2 pedalSize(pedalWidth, pedalHeight);

        // Espaçamento fixo entre os elementos
        float spacing = avail.x * 0.05f;

        // Atualiza os valores dos pedais conforme os inputs do teclado
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            throttleValue = std::min(throttleValue + 0.02f, 1.0f);
        } else {
            throttleValue = std::max(throttleValue - 0.01f, 0.0f);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            brakeValue = std::min(brakeValue + 0.02f, 1.0f);
        } else {
            brakeValue = std::max(brakeValue - 0.01f, 0.0f);
        }

        // Obtém a posição inicial antes de desenhar
        ImVec2 startPos = ImGui::GetCursorScreenPos();

        // --- Desenha os pedais ---
        ImGui::BeginGroup();
        ImGui::Text("Freio");
        if (this->redPedalTexture) {
            ImGui::ImageButton("##Brake", this->redPedalTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                               ImVec4(brakeValue * 0.5f, brakeValue * 0.5f, brakeValue * 0.5f, 1.0f));
        }
        ImGui::EndGroup();

        ImGui::SameLine(0, spacing);

        ImGui::BeginGroup();
        ImGui::Text("Acelerador");
        if (this->greenPedalTexture) {
            ImGui::ImageButton("##Throttle", this->greenPedalTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                               ImVec4(throttleValue * 0.5f, throttleValue * 0.5f, throttleValue * 0.5f, 1.0f));
        }
        ImGui::EndGroup();

        // Configura as dimensões das barras
        float  barWidth  = avail.x * 0.05f;
        float  barHeight = pedalHeight; // Mantém a mesma altura dos pedais
        ImVec2 barSize(barWidth, barHeight);

        // Define as posições fixas das barras
        ImVec2 brakeBarPos = ImVec2(startPos.x + avail.x * 0.8f + 10, startPos.y);
        ImVec2 accelBarPos = ImVec2(brakeBarPos.x + barWidth + spacing + 2, startPos.y);

        // --- Barra do Freio ---
        ImGui::GetWindowDrawList()->AddRectFilled(
            brakeBarPos, ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y), IM_COL32(100, 100, 100, 255));

        float fillHeight = barSize.y * brakeValue;
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y - fillHeight),
                                                  ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y),
                                                  IM_COL32(255, 0, 0, 255));

        // --- Barra do Acelerador ---
        ImGui::GetWindowDrawList()->AddRectFilled(
            accelBarPos, ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y), IM_COL32(100, 100, 100, 255));

        fillHeight = barSize.y * throttleValue;
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y - fillHeight),
                                                  ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y),
                                                  IM_COL32(0, 200, 0, 255));

        // Exibe os valores das barras (fixos abaixo das barras)
        ImGui::SetCursorScreenPos(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y + 5));

        ImGui::Text("%.0f%%", brakeValue * 100);

        ImGui::SetCursorScreenPos(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y + 5));
        ImGui::Text("%.0f%%", throttleValue * 100);

        ImGui::End();
    }
}
