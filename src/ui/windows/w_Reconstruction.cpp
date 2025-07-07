#include "ui/windows/w_Reconstruction.hpp"
#include "ImGuiWrapper.hpp" // Assuming wrapper includes ImGui
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::vector<TrackPoint> g_track;

inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

static inline float ImLength(const ImVec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }

// variaveis para aba "coordenadas"
static int g_latIndex = -1;
static int g_lonIndex = -1;

// array para corridas
const int NUM_RACE_SLOTS = 10;
RaceData  g_savedRaces[NUM_RACE_SLOTS];

float g_cartHeight         = 300.0f;
float g_cartZoom           = 1.0f;
float g_speedMultiplier    = 1.0f;
float g_HighSpeedThreshold = 50.0f;
float g_LowSpeedThreshold  = 10.0f;

// Variáveis de simulação do kart
static float g_kartSpeed    = 30.0f;
static float g_kartPosition = 0.0f;
static bool  g_autoSpeed    = false; // Se true, força a velocidade a ser a referência

// Vetores para registrar os marcadores (verde e vermelho) com informações do instante do registro
// CORRETO: guarda MarkerInfo
std::vector<MarkerInfo>  g_markedPositionsGreen;
std::vector<MarkerInfo>  g_markedPositionsRed;
std::vector<CommentInfo> g_comments;

std::vector<std::vector<size_t>> g_highSpeedSegments;
std::vector<size_t>              g_currentHighSpeed;
static bool                      prevHighSpeed = false;

std::vector<std::vector<size_t>> g_lowSpeedSegments;
std::vector<size_t>              g_currentLowSpeed;
static bool                      prevLowSpeed = false;

// Novas variáveis: contabiliza voltas e aceleração atual (em km/h por segundo)
static int   g_lapCount            = 0;
static float g_currentAcceleration = 0.0f;

// Velocidade padrão para retorno (km/h)
static const float DEFAULT_SPEED = 20.0f;

// Constantes para aceleração (km/h por segundo)
static const float ACCELERATION = 20.0f;
static const float DECELERATION = 20.0f;

// Constante para deslocar a pista para cima
static const float Y_OFFSET = 100.0f; // Ajuste conforme necessário

// Estrutura para manter cada janela de informação dos marcadores abertos
struct MarkerWindow {
        MarkerInfo  marker;
        std::string type;
        bool        open;
        bool        minimized;
        bool        reposition;
};

static std::vector<MarkerWindow> g_markerWindows;

// Estrutura para armazenar o estado de uma corrida
struct RaceState {
        float               kartSpeed;
        float               kartPosition;
        bool                autoSpeed;
        std::vector<size_t> markedPositionsGreen;
        std::vector<size_t> markedPositionsRed;
        int                 lapCount;
        float               currentAcceleration;
};

// Nome do arquivo onde as corridas serão salvas
static const std::string SAVE_FILE = "corridas_salvas.txt";

// Estrutura para as janelas de comentários
struct CommentWindow {
        ImVec2      pos;
        ImVec2      windowPos;
        std::string comment;
        bool        open;
        bool        minimized;
        bool        reposition;
};
static std::vector<CommentWindow> g_commentWindows;

void Window::Reconstruction::ConvertLatLonToXY(std::vector<float>& outX, std::vector<float>& outY) {
    // Converte lat/lon em coordenadas planas (equiretangular projection)
    size_t      n         = outX.size();
    float       originLat = outY.empty() ? 0.0f : outY[0];
    float       originLon = outX.empty() ? 0.0f : outX[0];
    const float R         = 6371000.0f; // raio da Terra em metros
    for (size_t i = 0; i < n; ++i) {
        float dLat = (outY[i] - originLat) * M_PI / 180.0f;
        float dLon = (outX[i] - originLon) * M_PI / 180.0f;
        outX[i]    = R * dLon * std::cos(originLat * M_PI / 180.0f);
        outY[i]    = R * dLat;
    }
}

// Called after columns for latitude and longitude have been added
void Window::Reconstruction::BuildTrackFromLatLon(size_t latIndex, size_t lonIndex) {
    if (latIndex >= coordDataList.size() || lonIndex >= coordDataList.size())
        return;

    // Extract raw lat/lon
    const auto& latData = coordDataList[latIndex].data;
    const auto& lonData = coordDataList[lonIndex].data;
    size_t      n       = std::min(latData.size(), lonData.size());

    // Convert to planar X/Y
    std::vector<float> xs(n), ys(n);
    for (size_t i = 0; i < n; ++i) {
        xs[i] = static_cast<float>(lonData[i]);
        ys[i] = static_cast<float>(latData[i]);
    }
    ConvertLatLonToXY(xs, ys);

    // Rebuild g_track
    g_track.clear();
    g_track.reserve(n + 1);
    for (size_t i = 0; i < n; ++i) {
        TrackPoint pt;
        pt.x              = xs[i];
        pt.y              = ys[i];
        pt.referenceSpeed = DEFAULT_SPEED; // or compute per segment
        g_track.push_back(pt);
    }
    // close loop if desired
    if (n > 1)
        g_track.push_back(g_track.front());
}

// --- Implementação das funções de drag & drop e manipulação de coordenadas ---

void Window::Reconstruction::processColumnDragDrop() {

    if (ImGui::BeginDragDropTarget()) {
        // Aceita o payload
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       filename, columnName;

            // Pega o nome do arquivo e a coluna
            if (std::getline(ss, filename, ':') && std::getline(ss, columnName, ':')) {
                this->addColumnToMap(filename, columnName);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

// Modificação em addColumnToMap para registrar índices de latitude/longitude
void Window::Reconstruction::addColumnToMap(const std::string& filename, const std::string& columnName) {

    // Verifica se a coluna do arquivo já foi adicionada
    for (COORDData& coordData : coordDataList) {
        if (coordData.archive == filename && coordData.column == columnName) {
            LOG("WARN", "Reconstrução: A coluna " + columnName + " do arquivo " + filename + " já existe.");
            return;
        }
    }

    // Adiciona os eixos
    std::vector<double> data = DB::getInstance().getCSVData(filename, columnName);
    if (data.empty()) {
        LOG("ERROR", "Não foi possível adicionar a coluna " + columnName + " do arquivo " + filename + " ao gráfico.");
        return;
    }

    // Cria e adiciona ao vetor
    COORDData coordData;
    coordData.data       = data;
    coordData.column     = columnName;
    coordData.archive    = filename;
    coordData.multiplier = 1.0;
    coordDataList.push_back(coordData);

    // Identifica se é latitude ou longitude e armazena o índice
    int         newIndex = static_cast<int>(coordDataList.size()) - 1;
    std::string lower    = columnName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("lat") != std::string::npos) {
        g_latIndex = newIndex;
        LOG("DEBUG", "Latitude cadastrada em coordDataList[" + std::to_string(newIndex) + "]");
    } else if (lower.find("lon") != std::string::npos || lower.find("lng") != std::string::npos) {
        g_lonIndex = newIndex;
        LOG("DEBUG", "Longitude cadastrada em coordDataList[" + std::to_string(newIndex) + "]");
    }

    LOG("DEBUG", "Coluna " + columnName + " adicionada à reconstrução.");
}

void Window::Reconstruction::generateSimulatedData(int numPoints, size_t coordIndex, float* x, float* y) {
    // Gera dados de uma curva simples (círculo ou senóide) como fallback
    (void)coordIndex; // Para ignorar o warning

    if (x == nullptr || y == nullptr) {
        LOG("ERROR", "generateSimulatedData: Ponteiros x ou y são nulos.");
        return;
    }
    float radius = 100.0f;
    for (int i = 0; i < numPoints; ++i) {
        float t = (float)i / (numPoints - 1) * 2.0f * M_PI;
        x[i]    = radius * std::cos(t);
        y[i]    = radius * std::sin(t) + Y_OFFSET;
    }
}

//---------------------------------------------------------
// Função auxiliar para desenhar uma seta entre dois pontos
//---------------------------------------------------------

static void DrawArrow(ImDrawList* draw_list, const ImVec2& p_from, const ImVec2& p_to, ImU32 col,
                      float thickness = 2.0f) {
    draw_list->AddLine(p_from, p_to, col, thickness);
    ImVec2 dir = {p_from.x - p_to.x, p_from.y - p_to.y};
    float  len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len <= 0.0f)
        return;
    dir.x                     /= len;
    dir.y                     /= len;
    float       arrowHeadSize  = 10.0f;
    const float arrowAngle     = 0.5f; // ~30 graus
    ImVec2      left  = {p_to.x + arrowHeadSize * (dir.x * std::cos(arrowAngle) - dir.y * std::sin(arrowAngle)),
                         p_to.y + arrowHeadSize * (dir.x * std::sin(arrowAngle) + dir.y * std::cos(arrowAngle))};
    ImVec2      right = {p_to.x + arrowHeadSize * (dir.x * std::cos(-arrowAngle) - dir.y * std::sin(-arrowAngle)),
                         p_to.y + arrowHeadSize * (dir.x * std::sin(-arrowAngle) + dir.y * std::cos(-arrowAngle))};
    draw_list->AddTriangleFilled(p_to, left, right, col);
}

// Função para SALVAR o estado atual da simulação em um slot
void SalvarCorrida(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS)
        return;

    RaceData& race = g_savedRaces[slotIndex];

    // Copia as configurações da simulação
    race.cartHeight         = g_cartHeight;
    race.cartZoom           = g_cartZoom;
    race.speedMultiplier    = g_speedMultiplier;
    race.HighSpeedThreshold = g_HighSpeedThreshold;
    race.LowSpeedThreshold  = g_LowSpeedThreshold;

    // Copia os dados resultantes da simulação
    race.markedPositionsGreen = g_markedPositionsGreen;
    race.markedPositionsRed   = g_markedPositionsRed;
    race.highSpeedSegments    = g_highSpeedSegments;
    race.lowSpeedSegments     = g_lowSpeedSegments;
    race.comments             = g_comments;

    // Salva os índices dos dados de pista
    race.latIndex = g_latIndex;
    race.lonIndex = g_lonIndex;

    // Marca o slot como salvo
    race.isSaved = true;
}

// Função para CARREGAR o estado de um slot para a simulação ativa
void CarregarCorrida(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS || !g_savedRaces[slotIndex].isSaved)
        return;

    const RaceData& race = g_savedRaces[slotIndex];

    // Restaura as configurações da simulação
    g_cartHeight         = race.cartHeight;
    g_cartZoom           = race.cartZoom;
    g_speedMultiplier    = race.speedMultiplier;
    g_HighSpeedThreshold = race.HighSpeedThreshold;
    g_LowSpeedThreshold  = race.LowSpeedThreshold;

    // Restaura os dados da simulação
    g_markedPositionsGreen = race.markedPositionsGreen;
    g_markedPositionsRed   = race.markedPositionsRed;
    g_highSpeedSegments    = race.highSpeedSegments;
    g_lowSpeedSegments     = race.lowSpeedSegments;
    g_comments             = race.comments;

    // Restaura os índices dos dados de pista
    g_latIndex = race.latIndex;
    g_lonIndex = race.lonIndex;

    // Opcional: Resetar a posição do kart para o início
    g_kartPosition = 0.0f;
}

// Função para LIMPAR um slot de corrida
void LimparCorrida(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS)
        return;

    // Reseta o slot para o estado inicial, criando um novo objeto RaceData vazio
    g_savedRaces[slotIndex] = RaceData();
}

//---------------------------------------------------------
// Implementação da janela de reconstrução
//---------------------------------------------------------

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) { this->title = "Reconstrução de Pista"; }

void Window::Reconstruction::removeColumnFromMap(size_t index) {
    if (index >= coordDataList.size()) {
        LOG("ERROR", "Não foi possível remover o gráfico, índice inválido.");
        return;
    }

    std::string columnName = coordDataList[index].column;
    coordDataList.erase(coordDataList.begin() + index);
    LOG("DEBUG", "Coluna '" + columnName + "' removida da gráfico.");
}

void Window::Reconstruction::render() {
    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen);

        static int    activeTab   = 0;
        static int    prevTab     = -1;
        static Uint32 simLastTime = SDL_GetTicks();

        // Se mudou de aba, reseta o relógio
        if (activeTab != prevTab) {
            simLastTime = SDL_GetTicks();
            prevTab     = activeTab;
        }

        // --- Funcionalidade de minimização ---
        if (ImGui::IsWindowCollapsed()) {
            if (ImGui::Button("Restaurar Reconstrução"))
                ImGui::SetWindowCollapsed(false);
            ImGui::End();
            return;
        }
        // --- Fim da minimização ---

        // Variáveis estáticas para controle do menu
        // activeTab: 0 para Simulação; 1 para Gerenciar Corridas
        static bool showTrackInfo = false;

        if (ImGui::CollapsingHeader("Janelas de Reconstrução")) {
            // Menu horizontal com os 4 botões
            if (ImGui::Button("Simulação"))
                activeTab = 0;
            ImGui::SameLine();
            if (ImGui::Button("Gerenciar Corridas"))
                activeTab = 1;
            ImGui::SameLine();
            if (ImGui::Button("Coordenadas"))
                activeTab = 2;
            if (ImGui::Button(showTrackInfo ? "Ocultar Informações" : "Mostrar Informações"))
                showTrackInfo = !showTrackInfo;
            ImGui::SameLine();
            if (ImGui::Button("Limpar Corrida")) {
                g_markedPositionsGreen.clear();
                g_markedPositionsRed.clear();
                g_markerWindows.clear();
                g_comments.clear();
                g_highSpeedSegments.clear();
                g_lowSpeedSegments.clear();
            }
        }

        if (ImGui::BeginTable("TabelaColunas", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Coluna", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Multiplicador", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < coordDataList.size(); i++) {
                COORDData coordData = coordDataList[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                std::string btnLabel = "X##" + std::to_string(i);
                if (ImGui::Button(btnLabel.c_str())) {
                    this->removeColumnFromMap(i);
                    break;
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(coordData.archive.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(coordData.column.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::PushItemWidth(120.0f);
                ImGui::InputDouble(("##mult" + std::to_string(i)).c_str(), &coordData.multiplier, 0.001, 100.0,
                                   "%.15gx");

                ImGui::PopItemWidth();
            }
            ImGui::EndTable();
        }
        ImGui::Separator();

        ImGui::BeginChild("DragAndDropArea");
        // Conteúdo dependendo da área ativa selecionada no menu

        // --- Aba Simulação ---
        if (activeTab == 0) {
            // 1) Calcula dt uma única vez por frame
            Uint32 now  = SDL_GetTicks();
            float  dt   = (now - simLastTime) * 0.001f;
            simLastTime = now;

            if (ImGui::CollapsingHeader("Configurações da Simulação")) {
                ImGui::SliderFloat("Altura do Gráfico", &g_cartHeight, 100.0f, 800.0f, "%.0f px");
                ImGui::SliderFloat("Zoom (escala)", &g_cartZoom, 0.1f, 5.0f, "%.2fx");
                ImGui::SliderFloat("Fator Velocidade", &g_speedMultiplier, 0.1f, 100.0f, "%.1fx");

                ImGui::Separator();
                ImGui::Text("Ajuste de Traçado");
                ImGui::SliderFloat("Traçado Verde", &g_HighSpeedThreshold, 0.0f, 200.0f, "%.0f km/h");
                ImGui::SliderFloat("Traçado Vermelho", &g_LowSpeedThreshold, 0.0f, 200.0f, "%.0f km/h");
            }
            ImGui::Separator();

            // 3) Pan offset
            static ImVec2 panOffset = ImVec2(0, 0);

            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 childSize(avail.x, g_cartHeight);

            // 4) Canvas dedicado
            if (childSize.x > 0.0f && childSize.y > 0.0f) {
                ImGui::BeginChild("Sim_Cartesiano", childSize, true);
                ImDrawList* draw   = ImGui::GetWindowDrawList();
                ImVec2      origin = ImGui::GetCursorScreenPos();
                ImVec2      size   = ImGui::GetContentRegionAvail();
                ImVec2      maxPt(origin.x + size.x, origin.y + size.y);

                // fundo + eixos
                draw->AddRectFilled(origin, maxPt, IM_COL32(20, 20, 20, 255));
                ImVec2 mid((origin.x + maxPt.x) * 0.5f, (origin.y + maxPt.y) * 0.5f);
                draw->AddLine({origin.x, mid.y}, {maxPt.x, mid.y}, IM_COL32(100, 100, 100, 255));
                draw->AddLine({mid.x, origin.y}, {mid.x, maxPt.y}, IM_COL32(100, 100, 100, 255));

                // captura drag
                ImGui::InvisibleButton("canvas_drag", size);
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    panOffset.x += ImGui::GetIO().MouseDelta.x;
                    panOffset.y += ImGui::GetIO().MouseDelta.y;
                }

                // 5) Geração de screenPts
                std::vector<ImVec2> screenPts;
                if (g_latIndex >= 0 && g_lonIndex >= 0) {
                    const auto& lat = coordDataList[g_latIndex].data;
                    const auto& lon = coordDataList[g_lonIndex].data;
                    size_t      n   = std::min(lat.size(), lon.size());
                    if (n > 1) {
                        double minX = lon[0], maxX = lon[0], minY = lat[0], maxY = lat[0];
                        for (size_t i = 1; i < n; ++i) {
                            minX = std::min(minX, lon[i]);
                            maxX = std::max(maxX, lon[i]);
                            minY = std::min(minY, lat[i]);
                            maxY = std::max(maxY, lat[i]);
                        }
                        ImVec2 inner(size.x * g_cartZoom, size.y * g_cartZoom);
                        screenPts.reserve(n);
                        for (size_t i = 0; i < n; ++i) {
                            float nx = (maxX > minX) ? (lon[i] - minX) / float(maxX - minX) : 0.5f;
                            float ny = (maxY > minY) ? (lat[i] - minY) / float(maxY - minY) : 0.5f;
                            screenPts.push_back({origin.x + panOffset.x + nx * inner.x,
                                                 origin.y + panOffset.y + (1.0f - ny) * inner.y});
                        }
                    }
                }

                // 6) Desenha a pista
                if (!screenPts.empty()) {
                    draw->AddPolyline(screenPts.data(), screenPts.size(), IM_COL32(255, 255, 255, 255), false, 2.0f);
                }

                // 7) Entrada e simulação do kart
                const Uint8* keys         = SDL_GetKeyboardState(NULL);
                static float prevSpeed    = 30.0f;
                static float currentAccel = 0.0f;
                const float  accelRate    = 30.0f; // km/h por segundo
                const float  recoverRate  = 5.0f;  // taxa de retorno ao padrão
                const float  defaultSpeed = 30.0f; // km/h

                if (keys[SDL_SCANCODE_W]) {
                    currentAccel = +accelRate;
                } else if (keys[SDL_SCANCODE_S]) {
                    currentAccel = -accelRate;
                } else {
                    // retorna gradativamente ao padrão
                    currentAccel = (defaultSpeed - g_kartSpeed) * recoverRate;
                }

                // atualiza velocidade
                g_kartSpeed += currentAccel * dt;
                g_kartSpeed  = ImMax(0.0f, ImMin(g_kartSpeed, 200.0f));

                // detecta crossings e armazena MarkerInfo
                if (!screenPts.empty()) {
                    size_t idx = (size_t)(g_kartPosition + 0.5f);
                    if (idx >= screenPts.size())
                        idx = screenPts.size() - 1;
                    ImVec2 kartPosOnTrack = screenPts[idx];
                    if (prevSpeed < g_HighSpeedThreshold && g_kartSpeed >= g_HighSpeedThreshold) {
                        g_markedPositionsGreen.push_back(
                            {idx, kartPosOnTrack, g_kartSpeed, currentAccel, g_kartPosition, g_lapCount});
                    }
                    if (prevSpeed > g_LowSpeedThreshold && g_kartSpeed <= g_LowSpeedThreshold) {
                        g_markedPositionsRed.push_back(
                            {idx, kartPosOnTrack, g_kartSpeed, currentAccel, g_kartPosition, g_lapCount});
                    }
                }
                prevSpeed = g_kartSpeed;

                // 8) Move o kart e desenha o kart
                if (!screenPts.empty()) {
                    float speed_mps  = g_kartSpeed / 3.6f;
                    g_kartPosition  += speed_mps * dt * g_speedMultiplier;
                    size_t N         = screenPts.size();
                    float  maxP      = float(N) - 1e-3f;
                    if (g_kartPosition > maxP)
                        g_kartPosition = fmodf(g_kartPosition, maxP);

                    DrawTrackAndKartAt(screenPts, origin, g_cartZoom);
                }

                // 8) Lógica de high-speed trace
                if (!screenPts.empty()) {
                    size_t idx = (size_t)(g_kartPosition + 0.5f);
                    if (idx >= screenPts.size())
                        idx = screenPts.size() - 1;
                    ImVec2 kartPos = screenPts[idx];
                    bool   nowHigh = (g_kartSpeed > g_HighSpeedThreshold);
                    if (nowHigh) {
                        g_currentHighSpeed.push_back(idx);
                    }
                    if (prevHighSpeed && !nowHigh) {
                        // finaliza trecho
                        if (!g_currentHighSpeed.empty()) {
                            g_highSpeedSegments.push_back(g_currentHighSpeed);
                            g_currentHighSpeed.clear();
                        }
                    }
                    prevHighSpeed = nowHigh;

                    // baixa velocidade
                    bool nowLow = (g_kartSpeed < g_LowSpeedThreshold);
                    if (nowLow) {
                        g_currentLowSpeed.push_back(idx);
                    }
                    if (prevLowSpeed && !nowLow) {
                        if (!g_currentLowSpeed.empty()) {
                            g_lowSpeedSegments.push_back(g_currentLowSpeed);
                            g_currentLowSpeed.clear();
                        }
                    }
                    prevLowSpeed = nowLow;
                }

                // Desenha trace fixo adaptado a zoom/pan (screenPts atualizado toda frame)
                // Desenha trechos de alta velocidade (verde)
                // HIGH-SPEED (verde)
                for (auto& segIdx : g_highSpeedSegments) {
                    if (segIdx.size() > 1) {
                        std::vector<ImVec2> pts;
                        pts.reserve(segIdx.size());
                        for (size_t i : segIdx) {
                            if (i < screenPts.size())
                                pts.push_back(screenPts[i]);
                        }
                        if (pts.size() > 1)
                            draw->AddPolyline(pts.data(), pts.size(), IM_COL32(0, 255, 0, 255), false, 3.0f);
                    }
                }
                if (g_currentHighSpeed.size() > 1) {
                    std::vector<ImVec2> pts;
                    pts.reserve(g_currentHighSpeed.size());
                    for (size_t i : g_currentHighSpeed) {
                        if (i < screenPts.size())
                            pts.push_back(screenPts[i]);
                    }
                    if (pts.size() > 1)
                        draw->AddPolyline(pts.data(), pts.size(), IM_COL32(0, 255, 0, 255), false, 3.0f);
                }

                // LOW-SPEED (vermelho)
                for (auto& segIdx : g_lowSpeedSegments) {
                    if (segIdx.size() > 1) {
                        std::vector<ImVec2> pts;
                        pts.reserve(segIdx.size());
                        for (size_t i : segIdx) {
                            if (i < screenPts.size())
                                pts.push_back(screenPts[i]);
                        }
                        if (pts.size() > 1)
                            draw->AddPolyline(pts.data(), pts.size(), IM_COL32(255, 0, 0, 255), false, 3.0f);
                    }
                }
                if (g_currentLowSpeed.size() > 1) {
                    std::vector<ImVec2> pts;
                    pts.reserve(g_currentLowSpeed.size());
                    for (size_t i : g_currentLowSpeed) {
                        if (i < screenPts.size())
                            pts.push_back(screenPts[i]);
                    }
                    if (pts.size() > 1)
                        draw->AddPolyline(pts.data(), pts.size(), IM_COL32(255, 0, 0, 255), false, 3.0f);
                }

                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImVec2 mouse    = ImGui::GetIO().MousePos;
                    size_t bestIdx  = 0;
                    float  bestDist = FLT_MAX;
                    for (size_t i = 0; i < screenPts.size(); ++i) {
                        float dx = mouse.x - screenPts[i].x;
                        float dy = mouse.y - screenPts[i].y;
                        float d2 = dx * dx + dy * dy;
                        if (d2 < bestDist) {
                            bestDist = d2;
                            bestIdx  = i;
                        }
                    }
                    CommentInfo c;
                    c.idx       = bestIdx;
                    c.triOffset = ImVec2(0, -10);
                    c.visible   = true;
                    c.text[0]   = '\0';
                    g_comments.push_back(c);
                }

                // Desenha triângulos e janelas de comentário
                for (size_t i = 0; i < g_comments.size(); ++i) {
                    auto& cm = g_comments[i];
                    if (cm.idx >= screenPts.size())
                        continue;
                    ImVec2 pt   = screenPts[cm.idx];
                    ImVec2 base = ImVec2(pt.x + cm.triOffset.x, pt.y + cm.triOffset.y);
                    float  s    = 8.0f * g_cartZoom;
                    ImVec2 p1{base.x, base.y - s};
                    ImVec2 p2{base.x - s, base.y + s};
                    ImVec2 p3{base.x + s, base.y + s};
                    draw->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 165, 0, 255));

                    // Define retângulo de interação cobrindo todo o triângulo
                    ImVec2 triMin{base.x - s, base.y - s};
                    ImVec2 triMax{base.x + s, base.y + s};

                    // clique esquerdo no triângulo alterna visibilidade
                    if (ImGui::IsMouseHoveringRect(triMin, triMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        cm.visible = !cm.visible;
                    }

                    if (cm.visible) {
                        // força janela sobre o triângulo toda vez que reaparecer
                        ImGui::SetNextWindowPos(ImVec2(base.x + 10, base.y - 10), ImGuiCond_Always);
                        char title[32];
                        sprintf(title, "Comentário %zu", i);
                        ImGui::Begin(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
                        ImGui::SetWindowFocus(); // garante foco e topo

                        // área de texto com ID único
                        char inputId[32];
                        sprintf(inputId, "##comment_input_%zu", i);
                        ImGui::InputTextMultiline(inputId, cm.text, sizeof(cm.text), ImVec2(200, 100));

                        // botão para remover comentário
                        if (ImGui::Button("Remover")) {
                            g_comments.erase(g_comments.begin() + i);
                            ImGui::End();
                            break;
                        }
                        ImGui::End();
                    }
                }

                // 9) Interação com marcadores (clicar para info/remover)
                // assegura vetores de visibilidade
                static std::vector<bool> visG, visR;
                if (visG.size() != g_markedPositionsGreen.size())
                    visG.assign(g_markedPositionsGreen.size(), false);
                if (visR.size() != g_markedPositionsRed.size())
                    visR.assign(g_markedPositionsRed.size(), false);

                // loop verde
                for (size_t i = 0; i < g_markedPositionsGreen.size(); ++i) {
                    auto& m = g_markedPositionsGreen[i];
                    if (m.idx >= screenPts.size())
                        continue;
                    ImVec2 pos = screenPts[m.idx];
                    float  s   = 6.0f * g_cartZoom; // tamanho escala com zoom
                    ImVec2 a{pos.x - s, pos.y - s};
                    ImVec2 b{pos.x + s, pos.y + s};
                    draw->AddRectFilled(a, b, IM_COL32(0, 255, 0, 255));
                    if (ImGui::IsMouseHoveringRect(a, b) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        visG[i] = !visG[i];

                    if (visG[i]) {
                        ImGui::SetNextWindowPos({pos.x + 10, pos.y - 10}, ImGuiCond_Always);
                        ImGui::Begin("Info Verde", nullptr,
                                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
                        ImGui::Text("Índice: %zu", m.idx);
                        ImGui::Text("Vel.: %.1f km/h", m.speed);
                        ImGui::Text("Acel.: %.1f km/h²", m.acceleration);
                        ImGui::SameLine();
                        if (ImGui::Button("X")) {
                            g_markedPositionsGreen.erase(g_markedPositionsGreen.begin() + i);
                            visG.erase(visG.begin() + i);
                            ImGui::End();
                            break;
                        }
                        ImGui::End();
                    }
                }

                // loop vermelho (mesma lógica, cor e vector diferente)
                for (size_t i = 0; i < g_markedPositionsRed.size(); ++i) {
                    auto& m = g_markedPositionsRed[i];
                    if (m.idx >= screenPts.size())
                        continue;
                    ImVec2 pos = screenPts[m.idx];
                    float  s   = 6.0f * g_cartZoom;
                    ImVec2 a{pos.x - s, pos.y - s};
                    ImVec2 b{pos.x + s, pos.y + s};
                    draw->AddRectFilled(a, b, IM_COL32(255, 0, 0, 255));
                    if (ImGui::IsMouseHoveringRect(a, b) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        visR[i] = !visR[i];

                    if (visR[i]) {
                        ImGui::SetNextWindowPos({pos.x + 10, pos.y - 10}, ImGuiCond_Always);
                        ImGui::Begin("Info Vermelho", nullptr,
                                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
                        ImGui::Text("Índice: %zu", m.idx);
                        ImGui::Text("Vel.: %.1f km/h", m.speed);
                        ImGui::Text("Acel.: %.1f km/h²", m.acceleration);
                        ImGui::SameLine();
                        if (ImGui::Button("X")) {
                            g_markedPositionsRed.erase(g_markedPositionsRed.begin() + i);
                            visR.erase(visR.begin() + i);
                            ImGui::End();
                            break;
                        }
                        ImGui::End();
                    }
                }

                // 10) Info extra
                if (showTrackInfo) {
                    ImGui::SetNextWindowPos(ImVec2(origin.x + size.x - 10, origin.y + 10), ImGuiCond_Always,
                                            ImVec2(1.0f, 0.0f));
                    ImGui::Begin("Info Pista", nullptr,
                                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoMove);
                    ImGui::Text("Velocidade: %.1f km/h", g_kartSpeed);
                    ImGui::Text("Aceleração: %.1f km/h²", currentAccel);
                    ImGui::Text("Multiplicador: %.1fx", g_speedMultiplier);
                    ImGui::Text("Posição (índice): %.1f", g_kartPosition);
                    ImGui::Text("Markers G: %zu", g_markedPositionsGreen.size());
                    ImGui::Text("Markers R: %zu", g_markedPositionsRed.size());
                    ImGui::End();
                }

                ImGui::EndChild();
            }

        }

        // --- PASSO 4: CÓDIGO PARA A ABA "GERENCIAR CORRIDAS" ---
        else if (activeTab == 1) {
            ImGui::Text("Gerencie até %d corridas salvas.", NUM_RACE_SLOTS);
            ImGui::Text("Salve o estado atual da simulação ou carregue um estado anterior.");
            ImGui::Separator();

            // Cria uma seção para cada slot de corrida
            for (int i = 0; i < NUM_RACE_SLOTS; ++i) {
                // PushID é essencial para que o ImGui saiba diferenciar botões com o mesmo nome em um loop
                ImGui::PushID(i);

                // Usa um CollapsingHeader para manter a UI organizada
                char headerName[32];
                sprintf(headerName, "Slot de Corrida %d", i + 1);
                if (ImGui::CollapsingHeader(headerName)) {

                    // Campo para nomear a corrida
                    ImGui::InputText("Nome", g_savedRaces[i].name, sizeof(g_savedRaces[i].name));

                    // Exibe o status do slot
                    const char* status = g_savedRaces[i].isSaved ? "Salvo" : "Vazio";
                    ImGui::Text("Status: %s", status);

                    // Botão para Salvar
                    if (ImGui::Button("Salvar Estado Atual Neste Slot")) {
                        SalvarCorrida(i);
                    }

                    ImGui::SameLine(); // Coloca o próximo item na mesma linha

                    // Desabilita o botão de carregar se o slot estiver vazio
                    if (!g_savedRaces[i].isSaved) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Carregar Este Slot")) {
                        CarregarCorrida(i);
                        ImGui::SetWindowFocus(NULL); // Opcional: Tira o foco da janela para evitar cliques duplos
                        activeTab = 0;               // Opcional: Muda para a aba de simulação após carregar
                    }
                    if (!g_savedRaces[i].isSaved) {
                        ImGui::EndDisabled();
                    }

                    ImGui::SameLine();

                    // Botão para Limpar
                    if (ImGui::Button("Limpar Slot")) {
                        LimparCorrida(i);
                    }
                }
                ImGui::PopID(); // Libera o ID
            }
        }

        else if (activeTab == 2) {

            ImGui::Separator();
            ImGui::Text("Coordenadas carregadas:");
            if (g_latIndex < 0 || g_lonIndex < 0) {
                ImGui::TextDisabled("Arraste colunas de latitude e longitude primeiro.");
            } else {
                const auto& latData = coordDataList[g_latIndex].data;
                const auto& lonData = coordDataList[g_lonIndex].data;
                size_t      n       = std::min(latData.size(), lonData.size());

                if (ImGui::BeginTable("TabelaCoordenadas", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();
                    for (size_t i = 0; i < n; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%.6f", latData[i]);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.6f", lonData[i]);
                    }
                    ImGui::EndTable();
                }
            }
        }

        ImGui::EndChild();
        processColumnDragDrop();
        ImGui::End();
    }
}

//---------------------------------------------------------
// Função para mapear a velocidade a uma cor
//---------------------------------------------------------
ImU32 Window::Reconstruction::GetColorForSpeed(float speed) {
    const float minSpeed = 20.0f;
    const float maxSpeed = 120.0f;
    float       t        = (speed - minSpeed) / (maxSpeed - minSpeed);
    t                    = std::clamp(t, 0.0f, 1.0f);
    int c                = static_cast<int>(50 + t * (220 - 50));
    return IM_COL32(c, c, c, 255);
}

//---------------------------------------------------------
// Desenha a pista, marcadores e o kart
//---------------------------------------------------------
void Window::Reconstruction::DrawTrackAndKartAt(const std::vector<ImVec2>& screenPts, const ImVec2& origin,
                                                float scale) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Desenha a linha da pista
    if (!screenPts.empty()) {
        ImU32 trackColor = GetColorForSpeed(g_kartSpeed);
        draw_list->AddPolyline(screenPts.data(), (int)screenPts.size(), trackColor, false, 4.0f * scale);
    }

    // Desenha marcadores (verde/vermelho) na posição interpolada em screenPts
    auto drawMarkers = [&](const std::vector<MarkerInfo>& markers, ImU32 col) {
        if (screenPts.size() < 2)
            return;
        for (const auto& m : markers) {
            int    idx = std::clamp(m.trackIndex, 0, (int)screenPts.size() - 2);
            float  f   = std::clamp(m.trackFrac, 0.0f, 1.0f);
            ImVec2 p   = ImLerp(screenPts[idx], screenPts[idx + 1], f);
            draw_list->AddRectFilled({p.x - 5, p.y - 5}, {p.x + 5, p.y + 5}, col);
        }
    };

    // Desenha o kart amarelo seguindo exatamente screenPts
    if (screenPts.size() >= 2) {
        int idx      = (int)std::floor(g_kartPosition);
        idx          = std::clamp(idx, 0, (int)screenPts.size() - 2);
        float frac   = g_kartPosition - (float)idx;
        frac         = std::clamp(frac, 0.0f, 1.0f);
        ImVec2 kartP = ImLerp(screenPts[idx], screenPts[idx + 1], frac);
        draw_list->AddCircleFilled(kartP, 8.0f * scale, IM_COL32(255, 255, 0, 255));
    }
}
