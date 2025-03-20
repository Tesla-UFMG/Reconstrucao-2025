#include "ui/Pedal.hpp"
#include "imgui.h"
#include "SDL.h"
#include "SDL_image.h"
#include <iostream>
#include <algorithm>
#include "SDLWrapper.hpp"  // Para acessar SDLWrapper::renderer

// Variáveis para as texturas dos pedais
static ImTextureID g_throttleTexture = 0;
static ImTextureID g_brakeTexture = 0;

// Fonte pequena para exibir as porcentagens
static ImFont* smallFont = nullptr;

// Função para carregar uma textura usando SDL_image
ImTextureID LoadTexture(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        std::cerr << "Erro ao carregar imagem: " << filename << " - " << IMG_GetError() << std::endl;
        return 0;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(SDLWrapper::renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Erro ao criar textura: " << SDL_GetError() << std::endl;
        return 0;
    }
    return (ImTextureID)texture;
}

// Carrega as texturas dos pedais
void LoadPedalTextures() {
    if (g_throttleTexture == 0)
        g_throttleTexture = LoadTexture("assets/pedalverde.png");
    if (g_brakeTexture == 0)
        g_brakeTexture = LoadTexture("assets/pedalvermelho.png");
}

// Libera as texturas ao fechar o programa
void UnloadPedalTextures() {
    if (g_throttleTexture)
        SDL_DestroyTexture((SDL_Texture*)g_throttleTexture);
    if (g_brakeTexture)
        SDL_DestroyTexture((SDL_Texture*)g_brakeTexture);
    g_throttleTexture = 0;
    g_brakeTexture = 0;
}

// Variáveis que simulam a pressão dos pedais (valores de 0 a 1)
static float throttleValue = 0.0f;
static float brakeValue = 0.0f;

void Window::Pedal(bool* isOpen) {
    if (*isOpen) {
        ImGui::Begin("Pedal", isOpen, ImGuiWindowFlags_NoCollapse);

        // Inicializa a fonte pequena, se necessário
        ImGuiIO& io = ImGui::GetIO();
        if (!smallFont) {
            if (!io.Fonts->Locked) {
                smallFont = io.Fonts->AddFontFromFileTTF("assets/YUMINL.TTF", 9.0f);
                io.Fonts->Build();
            }
            if (!smallFont)
                smallFont = ImGui::GetFont();
        }

        // Obtém o tamanho disponível na janela
        ImVec2 avail = ImGui::GetContentRegionAvail();

        // Dimensões fixas para os pedais
        float pedalWidth  = avail.x * 0.35f;
        float pedalHeight = avail.y * 0.7f;
        ImVec2 pedalSize(pedalWidth, pedalHeight);

        // Espaçamento fixo entre os elementos
        float spacing = avail.x * 0.05f;

        // Carrega as texturas dos pedais se necessário
        LoadPedalTextures();

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
        if (g_brakeTexture) {
            ImGui::ImageButton("##Brake", g_brakeTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                               ImVec4(brakeValue * 0.5f, brakeValue * 0.5f, brakeValue * 0.5f, 1.0f));
        }
        ImGui::EndGroup();

        ImGui::SameLine(0, spacing);

        ImGui::BeginGroup();
        ImGui::Text("Acelerador");
        if (g_throttleTexture) {
            ImGui::ImageButton("##Throttle", g_throttleTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                               ImVec4(throttleValue * 0.5f, throttleValue * 0.5f, throttleValue * 0.5f, 1.0f));
        }
        ImGui::EndGroup();

        // Configura as dimensões das barras
        float barWidth = avail.x * 0.05f;
        float barHeight = pedalHeight; // Mantém a mesma altura dos pedais
        ImVec2 barSize(barWidth, barHeight);

        // Define as posições fixas das barras
        ImVec2 brakeBarPos = ImVec2(startPos.x + avail.x * 0.8f + 10, startPos.y);
        ImVec2 accelBarPos = ImVec2(brakeBarPos.x + barWidth + spacing + 2, startPos.y);

        // --- Barra do Freio ---
        ImGui::GetWindowDrawList()->AddRectFilled(
            brakeBarPos,
            ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y),
            IM_COL32(100, 100, 100, 255)
        );

        float fillHeight = barSize.y * brakeValue;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y - fillHeight),
            ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y),
            IM_COL32(255, 0, 0, 255)
        );

        // --- Barra do Acelerador ---
        ImGui::GetWindowDrawList()->AddRectFilled(
            accelBarPos,
            ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y),
            IM_COL32(100, 100, 100, 255)
        );

        fillHeight = barSize.y * throttleValue;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(accelBarPos.x, accelBarPos.y + barSize.y - fillHeight),
            ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y),
            IM_COL32(0, 200, 0, 255)
        );

// Exibe os valores das barras (fixos abaixo das barras)
ImGui::SetCursorScreenPos(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y + 5));

// Usa a fonte pequena (já configurada anteriormente com smallFont)
ImGui::PushFont(smallFont);  // Usa a fonte pequena

ImGui::Text("%.0f%%", brakeValue * 100);

ImGui::SetCursorScreenPos(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y + 5));
ImGui::Text("%.0f%%", throttleValue * 100);

ImGui::PopFont();  // Restaura a fonte original


        ImGui::End();
    }
}

