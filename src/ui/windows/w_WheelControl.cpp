#include "ui/windows/w_WheelControl.hpp"

// Função auxiliar para desenhar a imagem rotacionada
void DrawRotatedImage(ImTextureID texture, const ImVec2& pos, float size, float angleDeg) {
    float halfSize = size * 0.5f;
    ImVec2 center = ImVec2(pos.x + halfSize, pos.y + halfSize);

    float angleRad = angleDeg * (3.14159265f / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);

    ImVec2 topLeft = ImVec2(-halfSize, -halfSize);
    ImVec2 topRight = ImVec2(halfSize, -halfSize);
    ImVec2 bottomRight = ImVec2(halfSize, halfSize);
    ImVec2 bottomLeft = ImVec2(-halfSize, halfSize);

    ImVec2 p1 = ImVec2(center.x + topLeft.x * cosA - topLeft.y * sinA, center.y + topLeft.x * sinA + topLeft.y * cosA);
    ImVec2 p2 = ImVec2(center.x + topRight.x * cosA - topRight.y * sinA, center.y + topRight.x * sinA + topRight.y * cosA);
    ImVec2 p3 = ImVec2(center.x + bottomRight.x * cosA - bottomRight.y * sinA, center.y + bottomRight.x * sinA + bottomRight.y * cosA);
    ImVec2 p4 = ImVec2(center.x + bottomLeft.x * cosA - bottomLeft.y * sinA, center.y + bottomLeft.x * sinA + bottomLeft.y * cosA);

    ImVec2 uv0 = ImVec2(0.0f, 0.0f);
    ImVec2 uv1 = ImVec2(1.0f, 0.0f);
    ImVec2 uv2 = ImVec2(1.0f, 1.0f);
    ImVec2 uv3 = ImVec2(0.0f, 1.0f);

    ImGui::GetWindowDrawList()->AddImageQuad(texture, p1, p2, p3, p4, uv0, uv1, uv2, uv3, IM_COL32_WHITE);
}

Window::WheelControl::WheelControl(bool* isOpen) : IWindow(isOpen) {
    this->title = "Controle do Volante";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::WheelControl::render() {
    static float anguloVolante = 0.0f;
    static float sensibilidade = 1.0f;
    static int anguloMaximo = 900;
    static bool forceFeedback = true;
    static bool showSettings = false;

    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        if (ImGui::Button("Configurações")) {
            showSettings = !showSettings;
        }

        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_LEFT]) {
            anguloVolante -= 5.0f * sensibilidade;
        }
        if (keystates[SDL_SCANCODE_RIGHT]) {
            anguloVolante += 5.0f * sensibilidade;
        }

        if (anguloVolante > anguloMaximo / 2.0f) anguloVolante = anguloMaximo / 2.0f;
        if (anguloVolante < -anguloMaximo / 2.0f) anguloVolante = -anguloMaximo / 2.0f;

        SDL_Texture* volanteTexture = AssetManager::getInstance().getTexture(VOLANTE_PATH);
        if (volanteTexture) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float size = (avail.x < avail.y) ? avail.x : avail.y;
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            cursorPos.x += (avail.x - size) / 2.0f;

            DrawRotatedImage((ImTextureID)volanteTexture, cursorPos, size, anguloVolante);
            ImGui::Dummy(ImVec2(avail.x, size));
        }

        if (showSettings) {
            ImGui::Begin("Configurações do Volante", &showSettings, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("Sensibilidade", &sensibilidade, 0.1f, 5.0f, "%.1f");
            ImGui::SliderInt("Ângulo Máximo", &anguloMaximo, 90, 1080);
            ImGui::Checkbox("Force Feedback", &forceFeedback);
            ImGui::Text("Ângulo Atual: %.1f°", anguloVolante);
            ImGui::End();
        }

        ImGui::End();
    }
}
