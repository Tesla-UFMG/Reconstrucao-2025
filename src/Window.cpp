#include "Window.hpp"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <fstream>
#include <filesystem>

Window::VisibilityFlags Window::visibility;

void Window::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && std::filesystem::create_directories(parentPath)) {
        LOG("INFO", "Criada pasta '" + parentPath.string() + "'.");
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível salvar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.write(reinterpret_cast<const char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' salvo com sucesso.");
}

void Window::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' carregada com sucesso.");
}

void Window::changeWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    *windowVisibility = !(*windowVisibility);
    std::string message =
        *windowVisibility ? "Foi aberta a janela '" + windowName + "'." : "Foi fechada a janela '" + windowName + "'.";
    LOG("TRACE", message);
}

SDL_Texture* volanteTexture = nullptr;

void carregarImagemVolante() {
    SDL_Surface* surface = IMG_Load("assets/volantetesla.png");
    if (!surface) {
        printf("Erro ao carregar imagem do volante: %s\n", IMG_GetError());
        return;
    }

    volanteTexture = SDL_CreateTextureFromSurface(SDLWrapper::renderer, surface);
    SDL_FreeSurface(surface);

    if (!volanteTexture) {
        printf("Erro ao criar textura do volante: %s\n", SDL_GetError());
    } else {
        // Definir o modo de blending e ajustar brilho
        SDL_SetTextureBlendMode(volanteTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(volanteTexture, 255, 255, 255);
    }
}

// Função auxiliar para desenhar a imagem rotacionada
void DrawRotatedImage(ImTextureID texture, const ImVec2& pos, float size, float angleDeg) {
    // pos: posição no canto superior esquerdo do quadrado
    float halfSize = size * 0.5f;
    // Calcula o centro da imagem
    ImVec2 center = ImVec2(pos.x + halfSize, pos.y + halfSize);
    
    // Converte o ângulo de graus para radianos
    float angleRad = angleDeg * (3.14159265f / 180.0f);
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);
    
    // Pontos relativos (não rotacionados) em torno do centro:
    // Top-left, top-right, bottom-right e bottom-left
    ImVec2 topLeft     = ImVec2(-halfSize, -halfSize);
    ImVec2 topRight    = ImVec2( halfSize, -halfSize);
    ImVec2 bottomRight = ImVec2( halfSize,  halfSize);
    ImVec2 bottomLeft  = ImVec2(-halfSize,  halfSize);
    
    // Aplica a rotação em cada vértice
    ImVec2 p1 = ImVec2(center.x + topLeft.x * cosA - topLeft.y * sinA,
                       center.y + topLeft.x * sinA + topLeft.y * cosA);
    ImVec2 p2 = ImVec2(center.x + topRight.x * cosA - topRight.y * sinA,
                       center.y + topRight.x * sinA + topRight.y * cosA);
    ImVec2 p3 = ImVec2(center.x + bottomRight.x * cosA - bottomRight.y * sinA,
                       center.y + bottomRight.x * sinA + bottomRight.y * cosA);
    ImVec2 p4 = ImVec2(center.x + bottomLeft.x * cosA - bottomLeft.y * sinA,
                       center.y + bottomLeft.x * sinA + bottomLeft.y * cosA);
    
    // Define as coordenadas de UV para a textura (imagem completa)
    ImVec2 uv0 = ImVec2(0.0f, 0.0f);
    ImVec2 uv1 = ImVec2(1.0f, 0.0f);
    ImVec2 uv2 = ImVec2(1.0f, 1.0f);
    ImVec2 uv3 = ImVec2(0.0f, 1.0f);
    
    // Desenha a imagem rotacionada
    ImGui::GetWindowDrawList()->AddImageQuad(texture, p1, p2, p3, p4, uv0, uv1, uv2, uv3, IM_COL32_WHITE);
}

void Window::WheelControl(bool* show) {
    static float anguloVolante = 0.0f;  // Guarda a rotação do volante

    if (*show) {
        if (ImGui::Begin("Controle do Volante", show)) {
            ImGui::Text("Configuração do Volante");

            static float sensibilidade = 1.0f;
            ImGui::SliderFloat("Sensibilidade", &sensibilidade, 0.1f, 5.0f, "%.1f");

            static int anguloMaximo = 900;
            ImGui::SliderInt("Ângulo Máximo", &anguloMaximo, 90, 1080);

            static bool forceFeedback = true;
            ImGui::Checkbox("Force Feedback", &forceFeedback);

            // Atualiza o ângulo do volante com base na entrada do teclado
            const Uint8* keystates = SDL_GetKeyboardState(NULL);
            if (keystates[SDL_SCANCODE_LEFT]) {
                anguloVolante -= 5.0f * sensibilidade;
            }
            if (keystates[SDL_SCANCODE_RIGHT]) {
                anguloVolante += 5.0f * sensibilidade;
            }

            // Limita o ângulo ao máximo configurado
            if (anguloVolante > anguloMaximo / 2.0f) anguloVolante = anguloMaximo / 2.0f;
            if (anguloVolante < -anguloMaximo / 2.0f) anguloVolante = -anguloMaximo / 2.0f;

            ImGui::Text("Ângulo Atual: %.1f°", anguloVolante);

            // Carrega a imagem do volante (idealmente, isso deveria ocorrer apenas uma vez)
            carregarImagemVolante();

            // Se a textura estiver disponível, desenha o volante rotacionado
            if (volanteTexture) {
                // Obtém a área disponível dentro da janela do ImGui para o conteúdo
                ImVec2 avail = ImGui::GetContentRegionAvail();
                // Calcula o tamanho do volante como o mínimo entre a largura e a altura disponíveis,
                // garantindo que a imagem não seja cortada ao redimensionar a janela
                float size = (avail.x < avail.y) ? avail.x : avail.y;
                
                // Obtém a posição atual do cursor na tela
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                // Ajusta a posição para centralizar horizontalmente o volante
                cursorPos.x += (avail.x - size) / 2.0f;

                DrawRotatedImage((ImTextureID)volanteTexture, cursorPos, size, anguloVolante);

                // Reserva espaço equivalente ao tamanho da imagem para evitar sobreposição dos elementos
                ImGui::Dummy(ImVec2(avail.x, size));
            }

            if (ImGui::Button("Fechar")) {
                *show = false;
            }
        }
        ImGui::End();
    }
}

void Window::render() {
    Window::About(&Window::visibility.showAbout);
    Window::Playback(&Window::visibility.showPlayback);
    Window::Datapicker(&Window::visibility.showDataPicker);
    Window::Reconstruction(&Window::visibility.showReconstruction);
    Window::Video(&Window::visibility.showVideo);
    Window::Plot(&Window::visibility.showPlot);
    Window::Log(&Window::visibility.showLog);
    Window::ImGuiDemo(&Window::visibility.showImGuiDemo);
    Window::ImPlotDemo(&Window::visibility.showImPlotDemo);
    Window::WheelControl(&Window::visibility.showWheelControl);
    Window::Pedal(&Window::visibility.showPedal);
}
