#include "ui/windows/w_Reconstruction.hpp"

// Define cada ponto da pista com posição (x,y) e velocidade de referência (não usada aqui, mas pode ser útil)
struct TrackPoint {
        float x, y;
        float referenceSpeed;
};

// Pista retangular: definindo os vértices (fechamos o loop para formar o retângulo)
static std::vector<TrackPoint> g_track = {
    {50.f,  50.f,  80.f},
    {350.f, 50.f,  80.f},
    {350.f, 250.f, 80.f},
    {50.f,  250.f, 80.f},
    {50.f,  50.f,  80.f}  // Fecha a pista
};

// Variáveis de simulação do kart
static float g_kartSpeed    = 50.0f; // Velocidade atual em km/h
static float g_kartPosition = 0.0f;  // Progresso ao longo da pista (índice fracionário)
static bool  g_autoSpeed    = false; // (Opcional) se true, força a velocidade a ser a referência

// Velocidade padrão para retorno (km/h)
static const float DEFAULT_SPEED = 50.0f;

// Constantes para aceleração (km/h por segundo)
static const float ACCELERATION = 20.0f;
static const float DECELERATION = 20.0f;

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) { this->title = "Reconstrução de Pista"; }

void Window::Reconstruction::render() {
    if (this->isOpen && *this->isOpen) {

        static Uint32 lastTime    = SDL_GetTicks();
        Uint32        currentTime = SDL_GetTicks();
        float         deltaTime   = (currentTime - lastTime) / 1000.0f;
        lastTime                  = currentTime;
        UpdateKartSimulation(deltaTime);
        ImGuiWindowFlags flags = 0;

        ImGui::Begin(this->title.c_str(), this->isOpen, flags);

        // Controles de simulação
        ImGui::Text("Simulação do Kart:");
        ImGui::Text("Velocidade: %.1f km/h", g_kartSpeed);
        ImGui::Checkbox("Auto Speed", &g_autoSpeed);

        // Reserva uma área para desenhar a pista (todo o espaço disponível)
        ImVec2 region = ImGui::GetContentRegionAvail();
        // Salva a posição atual do cursor como origem para o desenho da pista
        ImVec2 drawOrigin = ImGui::GetCursorScreenPos();
        // Reserva o espaço sem reposicionar o cursor (Dummy só reserva espaço visualmente)
        ImGui::Dummy(region);

        // Desenha a pista e o kart a partir do ponto de origem salvo
        DrawTrackAndKartAt(drawOrigin);

        ImGui::End();
    }
}

// Função para mapear a velocidade a uma cor:
// Velocidades mais baixas resultarão em cores escuras,
// velocidades mais altas resultarão em cores claras.
ImU32 Window::Reconstruction::GetColorForSpeed(float speed) {
    const float minSpeed = 20.0f;
    const float maxSpeed = 120.0f;
    float       t        = (speed - minSpeed) / (maxSpeed - minSpeed);
    t                    = std::clamp(t, 0.0f, 1.0f);
    int c                = static_cast<int>(50 + t * (220 - 50)); // interpolação entre 50 (escuro) e 220 (claro)
    return IM_COL32(c, c, c, 255);
}

// Desenha o traçado da pista e o kart a partir da origem dada
void Window::Reconstruction::DrawTrackAndKartAt(const ImVec2& origin) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Desenha cada segmento da pista com a cor baseada na velocidade atual
    ImU32 trackColor = GetColorForSpeed(g_kartSpeed);
    for (size_t i = 0; i + 1 < g_track.size(); i++) {
        ImVec2 p1(origin.x + g_track[i].x, origin.y + g_track[i].y);
        ImVec2 p2(origin.x + g_track[i + 1].x, origin.y + g_track[i + 1].y);
        draw_list->AddLine(p1, p2, trackColor, 4.0f);
    }

    // Desenha o kart (um círculo amarelo) na posição atual
    int   idx  = static_cast<int>(g_kartPosition);
    float frac = g_kartPosition - idx;
    if (idx >= static_cast<int>(g_track.size()) - 1) {
        idx  = static_cast<int>(g_track.size()) - 2;
        frac = 1.0f;
    }
    float  xPos = g_track[idx].x + (g_track[idx + 1].x - g_track[idx].x) * frac;
    float  yPos = g_track[idx].y + (g_track[idx + 1].y - g_track[idx].y) * frac;
    ImVec2 kartPos(origin.x + xPos, origin.y + yPos);
    draw_list->AddCircleFilled(kartPos, 8.0f, IM_COL32(255, 255, 0, 255));
}

void Window::Reconstruction::UpdateKartSimulation(float deltaTime) {
    const Uint8* keystates    = SDL_GetKeyboardState(NULL);
    bool         accelerating = keystates[SDL_SCANCODE_W];
    bool         braking      = keystates[SDL_SCANCODE_S];

    if (accelerating) {
        g_kartSpeed += ACCELERATION * deltaTime;
    }
    if (braking) {
        g_kartSpeed -= DECELERATION * deltaTime;
    }
    // Se nem acelerar nem frear, retorna gradualmente para a velocidade padrão
    if (!accelerating && !braking) {
        if (g_kartSpeed < DEFAULT_SPEED) {
            g_kartSpeed += ACCELERATION * deltaTime;
            if (g_kartSpeed > DEFAULT_SPEED)
                g_kartSpeed = DEFAULT_SPEED;
        } else if (g_kartSpeed > DEFAULT_SPEED) {
            g_kartSpeed -= DECELERATION * deltaTime;
            if (g_kartSpeed < DEFAULT_SPEED)
                g_kartSpeed = DEFAULT_SPEED;
        }
    }
    g_kartSpeed = std::max(0.0f, g_kartSpeed);

    if (g_autoSpeed) {
        int idx = static_cast<int>(g_kartPosition);
        if (idx >= 0 && idx < static_cast<int>(g_track.size()))
            g_kartSpeed = g_track[idx].referenceSpeed;
    }

    float mps       = g_kartSpeed / 3.6f;
    g_kartPosition += mps * deltaTime * 0.5f;

    if (g_kartPosition >= g_track.size() - 1) {
        g_kartPosition = 0.0f;
    }
}
