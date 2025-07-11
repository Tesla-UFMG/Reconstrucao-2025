#include "ui/windows/w_Reconstruction.hpp"
#include "ImGuiWrapper.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

void Window::Reconstruction::play() {
    m_isSimulating = true;
}

void Window::Reconstruction::pause() {
    m_isSimulating = false;
}

void Window::Reconstruction::seek(double position) {
    if (!m_track.empty()) {
        LOG("DEBUG", "[Reconstruction::seek] Função chamada com a posição: " + std::to_string(position));
        LOG("DEBUG", "[Reconstruction::seek] m_kartPosition ANTES: " + std::to_string(m_kartPosition));

        m_kartPosition = static_cast<float>(std::max(0.0, std::min(position, (double)m_track.size() - 1.0)));
        m_seekJustOccurred = true;

        LOG("DEBUG", "[Reconstruction::seek] m_kartPosition DEPOIS: " + std::to_string(m_kartPosition));
        LOG("DEBUG", "[Reconstruction::seek] Flag m_seekJustOccurred definida como TRUE.");
    } else {
        LOG("WARN", "[Reconstruction::seek] Chamada ignorada pois a pista (m_track) está vazia.");
    }
}

bool Window::Reconstruction::isPlaying() const {
    return m_isSimulating;
}

bool Window::Reconstruction::isLoaded() const {
    return m_latIndex != -1 && m_lonIndex != -1;
}

double Window::Reconstruction::getCurrentTime() const {
    return static_cast<double>(m_kartPosition);
}

double Window::Reconstruction::getDuration() const {
    return m_track.empty() ? 0.0 : static_cast<double>(m_track.size() - 1.0);
}

static std::vector<TrackPoint> m_track;

inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

static inline float ImLength(const ImVec2& v) {
    return std::sqrt(v.x*v.x + v.y*v.y);
}

//variaveis para aba "coordenadas"
int m_latIndex = -1;
int m_lonIndex = -1;

// array para corridas
const int NUM_RACE_SLOTS = 10;
RaceData m_savedRaces[NUM_RACE_SLOTS];

float m_cartHeight        = 300.0f;
float m_cartZoom          = 1.0f;
float m_speedMultiplier   = 1.0f;
float m_HighSpeedThreshold = 50.0f;
float m_LowSpeedThreshold  = 10.0f;

// Variáveis de simulação do kart
float m_kartSpeed    = 30.0f;
float m_kartPosition = 0.0f;
bool  m_isSimulating    = false; // Se true, força a velocidade a ser a referência

// Vetores para registrar os marcadores (verde e vermelho) com informações do instante do registro
// CORRETO: guarda MarkerInfo
std::vector<MarkerInfo> m_markedPositionsGreen;
std::vector<MarkerInfo> m_markedPositionsRed;
std::vector<CommentInfo> m_comments;

std::vector<std::vector<size_t>> m_highSpeedSegments;
std::vector<size_t>              m_currentHighSpeed;
bool m_prevHighSpeed = false;

std::vector<std::vector<size_t>> m_lowSpeedSegments;
std::vector<size_t>              m_currentLowSpeed;
bool m_prevLowSpeed = false;

// Novas variáveis: contabiliza voltas e aceleração atual (em km/h por segundo)
int   m_lapCount            = 0;
float m_currentAcceleration = 0.0f;

// Velocidade padrão para retorno (km/h)
const float DEFAULT_SPEED = 20.0f;

// Constantes para aceleração (km/h por segundo)
const float ACCELERATION = 20.0f;
const float DECELERATION = 20.0f;

// Constante para deslocar a pista para cima
const float Y_OFFSET = 100.0f; // Ajuste conforme necessário

// Estrutura para manter cada janela de informação dos marcadores abertos
struct MarkerWindow {
        MarkerInfo  marker;
        std::string type;
        bool        open;
        bool        minimized;
        bool        reposition;
};

std::vector<MarkerWindow> m_markerWindows;

// Estrutura para armazenar o estado de uma corrida
struct RaceState {
        float                   kartSpeed;
        float                   kartPosition;
        bool                    autoSpeed;
        std::vector<size_t> markedPositionsGreen;
        std::vector<size_t> markedPositionsRed;
        int                     lapCount;
        float                   currentAcceleration;
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
static std::vector<CommentWindow> m_commentWindows;

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
void Window::Reconstruction::BuildTrackFromLatLon() {
    if (m_latIndex >= m_coordDataList.size() || m_lonIndex >= m_coordDataList.size()) return;

    // Extract raw lat/lon
    const auto& latData = m_coordDataList[m_latIndex].data;
    const auto& lonData = m_coordDataList[m_lonIndex].data;
    size_t n = std::min(latData.size(), lonData.size());
    
    // Convert to planar X/Y
    std::vector<float> xs(n), ys(n);
    for (size_t i = 0; i < n; ++i) {
        xs[i] = static_cast<float>(lonData[i]);
        ys[i] = static_cast<float>(latData[i]);
    }
    ConvertLatLonToXY(xs, ys);

    // Rebuild m_track
    m_track.clear();
    m_track.reserve(n+1);
    for (size_t i = 0; i < n; ++i) {
        TrackPoint pt;
        pt.x = xs[i];
        pt.y = ys[i];
        pt.referenceSpeed = DEFAULT_SPEED; // or compute per segment
        m_track.push_back(pt);
    }
    // close loop if desired
    if (n > 1) m_track.push_back(m_track.front());
}

// --- Implementação das funções de drag & drop e manipulação de coordenadas ---

void Window::Reconstruction::processColumnDragDrop() {

    if (ImGui::BeginDragDropTarget()) {
        // Aceita o payload
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            std::stringstream ss(static_cast<const char*>(payload->Data));
            std::string       archiveName, columnName;

            // Pega o nome do arquivo e a coluna
            if (std::getline(ss, archiveName, ':') && std::getline(ss, columnName, ':')) {
                this->addColumnToMap(archiveName, columnName);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

// Modificação em addColumnToMap para registrar índices de latitude/longitude
void Window::Reconstruction::addColumnToMap(const std::string& archiveName,
                                            const std::string& columnName) {

                                                
    // Verifica se a coluna do arquivo já foi adicionada
    for (COORDData& coordData : m_coordDataList) {
        if (coordData.archive == archiveName && coordData.column == columnName) {
            LOG("WARN", "Reconstrução: A coluna " + columnName + " do arquivo " + archiveName + " já existe.");
            return;
        }
    }

    // Adiciona os eixos
    std::vector<double> data = DB::getInstance().getCSVData(archiveName, columnName);
    if (data.empty()) {
        LOG("ERROR",
            "Não foi possível adicionar a coluna " + columnName + " do arquivo " + archiveName + " ao gráfico.");
        return;
    }

    // Cria e adiciona ao vetor
    COORDData coordData;
    coordData.data       = data;
    coordData.column     = columnName;
    coordData.archive    = archiveName;
    coordData.multiplier = 1.0;
    m_coordDataList.push_back(coordData);

    // Identifica se é latitude ou longitude e armazena o índice
    int newIndex = static_cast<int>(m_coordDataList.size()) - 1;
    std::string lower = columnName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("lat") != std::string::npos) {
        m_latIndex = newIndex;
        LOG("DEBUG", "Latitude cadastrada em m_coordDataList[" + std::to_string(newIndex) + "]");
    } else if (lower.find("lon") != std::string::npos || lower.find("lng") != std::string::npos) {
        m_lonIndex = newIndex;
        LOG("DEBUG", "Longitude cadastrada em m_coordDataList[" + std::to_string(newIndex) + "]");
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
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS) return;

    RaceData& race = m_savedRaces[slotIndex];

    // Copia as configurações da simulação
    race.cartHeight        = m_cartHeight;
    race.cartZoom          = m_cartZoom;
    race.speedMultiplier   = m_speedMultiplier;
    race.HighSpeedThreshold = m_HighSpeedThreshold;
    race.LowSpeedThreshold  = m_LowSpeedThreshold;

    // Copia os dados resultantes da simulação
    race.markedPositionsGreen = m_markedPositionsGreen;
    race.markedPositionsRed   = m_markedPositionsRed;
    race.highSpeedSegments    = m_highSpeedSegments;
    race.lowSpeedSegments     = m_lowSpeedSegments;
    race.comments             = m_comments;
    
    // Salva os índices dos dados de pista
    race.latIndex = m_latIndex;
    race.lonIndex = m_lonIndex;

    // Marca o slot como salvo
    race.isSaved = true;
}

// Função para CARREGAR o estado de um slot para a simulação ativa
void CarregarCorrida(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS || !m_savedRaces[slotIndex].isSaved) return;

    const RaceData& race = m_savedRaces[slotIndex];

    // Restaura as configurações da simulação
    m_cartHeight        = race.cartHeight;
    m_cartZoom          = race.cartZoom;
    m_speedMultiplier   = race.speedMultiplier;
    m_HighSpeedThreshold = race.HighSpeedThreshold;
    m_LowSpeedThreshold  = race.LowSpeedThreshold;

    // Restaura os dados da simulação
    m_markedPositionsGreen = race.markedPositionsGreen;
    m_markedPositionsRed   = race.markedPositionsRed;
    m_highSpeedSegments    = race.highSpeedSegments;
    m_lowSpeedSegments     = race.lowSpeedSegments;
    m_comments             = race.comments;

    // Restaura os índices dos dados de pista
    m_latIndex = race.latIndex;
    m_lonIndex = race.lonIndex;

    // Opcional: Resetar a posição do kart para o início
    m_kartPosition = 0.0f;
}

// Função para LIMPAR um slot de corrida
void LimparCorrida(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= NUM_RACE_SLOTS) return;

    // Reseta o slot para o estado inicial, criando um novo objeto RaceData vazio
    m_savedRaces[slotIndex] = RaceData(); 
}

//---------------------------------------------------------
// Implementação da janela de reconstrução
//---------------------------------------------------------

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) {
    this->title = "Reconstrução de Pista";

}

void Window::Reconstruction::removeColumnFromMap(size_t index) {
    if (index >= m_coordDataList.size()) {
        LOG("ERROR", "Não foi possível remover o gráfico, índice inválido.");
        return;
    }

    std::string columnName = m_coordDataList[index].column;
    m_coordDataList.erase(m_coordDataList.begin() + index);
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
            m_markedPositionsGreen.clear();
            m_markedPositionsRed.clear();
            m_markerWindows.clear();
            m_comments.clear();
            m_highSpeedSegments.clear();
            m_lowSpeedSegments.clear();
            
        }
    }

        if (ImGui::BeginTable("TabelaColunas", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Coluna", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Multiplicador", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < m_coordDataList.size(); i++) {
                COORDData coordData = m_coordDataList[i];
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
    Uint32 now      = SDL_GetTicks();
    float  dt       = (now - simLastTime) * 0.001f;
    simLastTime     = now;

    if (ImGui::CollapsingHeader("Configurações da Simulação")) {
        ImGui::SliderFloat("Altura do Gráfico", &m_cartHeight,      100.0f, 800.0f,  "%.0f px");
        ImGui::SliderFloat("Zoom (escala)",     &m_cartZoom,        0.1f,   5.0f,    "%.2fx");
        ImGui::SliderFloat("Fator Velocidade",  &m_speedMultiplier, 0.1f,   100.0f,  "%.1fx");

        ImGui::Separator();
        ImGui::Text ("Ajuste de Traçado");
        ImGui::SliderFloat("Traçado Verde", &m_HighSpeedThreshold, 0.0f, 200.0f, "%.0f km/h");
        ImGui::SliderFloat("Traçado Vermelho", &m_LowSpeedThreshold, 0.0f, 200.0f, "%.0f km/h");
    }
            ImGui::Separator();

    // 3) Pan offset
    static ImVec2 panOffset = ImVec2(0, 0);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float childWidth = std::max(1.0f, avail.x);
    ImVec2 childSize(childWidth, m_cartHeight);

    // 4) Canvas dedicado
       if (childSize.y > 0.0f) {
    ImGui::BeginChild("Sim_Cartesiano", childSize, true);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin    = ImGui::GetCursorScreenPos();
        ImVec2 size      = ImGui::GetContentRegionAvail();
        ImVec2 maxPt(origin.x + size.x, origin.y + size.y);

        // fundo + eixos
        draw->AddRectFilled(origin, maxPt, IM_COL32(20,20,20,255));
        ImVec2 mid((origin.x + maxPt.x)*0.5f, (origin.y + maxPt.y)*0.5f);
        draw->AddLine({origin.x, mid.y}, {maxPt.x, mid.y}, IM_COL32(100,100,100,255));
        draw->AddLine({mid.x, origin.y}, {mid.x, maxPt.y}, IM_COL32(100,100,100,255));

        // captura drag
        ImGui::InvisibleButton("canvas_drag", size);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            panOffset.x += ImGui::GetIO().MouseDelta.x;
            panOffset.y += ImGui::GetIO().MouseDelta.y;
        }

        // 5) Geração de screenPts
        std::vector<ImVec2> screenPts;
        if (m_latIndex >= 0 && m_lonIndex >= 0) {
            const auto& lat = m_coordDataList[m_latIndex].data;
            const auto& lon = m_coordDataList[m_lonIndex].data;
            size_t n = std::min(lat.size(), lon.size());
            if (n > 1) {
                double minX = lon[0], maxX = lon[0],
                       minY = lat[0], maxY = lat[0];
                for (size_t i = 1; i < n; ++i) {
                    minX = std::min(minX, lon[i]);
                    maxX = std::max(maxX, lon[i]);
                    minY = std::min(minY, lat[i]);
                    maxY = std::max(maxY, lat[i]);
                }
                ImVec2 inner(size.x * m_cartZoom, size.y * m_cartZoom);
                screenPts.reserve(n);
                for (size_t i = 0; i < n; ++i) {
                    float nx = (maxX > minX) ? (lon[i] - minX) / float(maxX - minX) : 0.5f;
                    float ny = (maxY > minY) ? (lat[i] - minY) / float(maxY - minY) : 0.5f;
                    screenPts.push_back({
                        origin.x + panOffset.x + nx * inner.x,
                        origin.y + panOffset.y + (1.0f - ny) * inner.y
                    });
                }
            }
        }

        // 6) Desenha a pista
        if (!screenPts.empty()) {
            draw->AddPolyline(screenPts.data(), screenPts.size(),
                              IM_COL32(255,255,255,255), false, 2.0f);
        }

        // 7) Entrada e simulação do kart
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        static float prevSpeed    = 30.0f;
        static float currentAccel = 0.0f;
        const float accelRate     = 30.0f;    // km/h por segundo
        const float recoverRate   = 5.0f;     // taxa de retorno ao padrão
        const float defaultSpeed  = 30.0f;    // km/h

        if (keys[SDL_SCANCODE_W]) {
            currentAccel = +accelRate;
        } else if (keys[SDL_SCANCODE_S]) {
            currentAccel = -accelRate;
        } else {
            // retorna gradativamente ao padrão
            currentAccel = (defaultSpeed - m_kartSpeed) * recoverRate;
        }

        // atualiza velocidade
        m_kartSpeed += currentAccel * dt;
        m_kartSpeed = ImMax(0.0f, ImMin(m_kartSpeed, 200.0f));

        // detecta crossings e armazena MarkerInfo
        if (!screenPts.empty()) {
            size_t idx = (size_t)(m_kartPosition + 0.5f);
            if (idx >= screenPts.size()) idx = screenPts.size() - 1;
            ImVec2 kartPosOnTrack = screenPts[idx];
            if (prevSpeed < m_HighSpeedThreshold && m_kartSpeed >= m_HighSpeedThreshold) {
                m_markedPositionsGreen.push_back({idx, kartPosOnTrack,
                                                   m_kartSpeed,
                                                   currentAccel,
                                                   m_kartPosition,
                                                   m_lapCount });
            }
            if (prevSpeed > m_LowSpeedThreshold && m_kartSpeed <= m_LowSpeedThreshold) {
                m_markedPositionsRed.push_back({ idx, kartPosOnTrack,
                                                 m_kartSpeed,
                                                 currentAccel,
                                                 m_kartPosition,
                                                 m_lapCount });
            }
        }
        prevSpeed = m_kartSpeed;

        // 8) Lógica de movimento do kart
        LOG("DEBUG", "[Render] Início do frame. Valor de m_seekJustOccurred: " + std::to_string(m_seekJustOccurred));
        if (m_seekJustOccurred) {
            LOG("DEBUG", "[Render] CONDIÇÃO 1: 'seek' ocorreu. Ignorando simulação e resetando a flag.");
            // Um seek manual acabou de acontecer.
            // Ignoramos a atualização da simulação neste frame para que o pulo seja visível.
            m_seekJustOccurred = false; // Resetamos a flag para o próximo frame.
        } 
        else if (m_isSimulating && !screenPts.empty()) {
            LOG("DEBUG", "[Render] CONDIÇÃO 2: 'play' ativo. Atualizando posição do kart.");
            // Se nenhum seek ocorreu e estamos em "play", atualiza a posição normalmente.
            float speed_mps = m_kartSpeed / 3.6f;
            m_kartPosition += speed_mps * dt * m_speedMultiplier;
            size_t N    = screenPts.size();
            float  maxP = float(N) - 1e-3f;
            if (m_kartPosition >= maxP) {
                m_kartPosition = fmodf(m_kartPosition, maxP);
                m_lapCount++;
            }
        } 

        if (!screenPts.empty()) {
            DrawTrackAndKartAt(screenPts, origin, m_cartZoom);
        }

        // 8) Lógica de high-speed trace
    if (!screenPts.empty()) {
        size_t idx = (size_t)(m_kartPosition + 0.5f);
        if (idx >= screenPts.size()) idx = screenPts.size() - 1;
        ImVec2 kartPos = screenPts[idx];
        bool nowHigh = (m_kartSpeed > m_HighSpeedThreshold);
        if (nowHigh) {
            m_currentHighSpeed.push_back(idx);
        }
        if (m_prevHighSpeed && !nowHigh) {
            // finaliza trecho
            if (!m_currentHighSpeed.empty()) {
                m_highSpeedSegments.push_back(m_currentHighSpeed);
                m_currentHighSpeed.clear();
            }
        }
        m_prevHighSpeed = nowHigh;

        // baixa velocidade
        bool nowLow = (m_kartSpeed < m_LowSpeedThreshold);
        if (nowLow) {
            m_currentLowSpeed.push_back(idx);
        }
        if (m_prevLowSpeed && !nowLow) {
            if (!m_currentLowSpeed.empty()) {
                m_lowSpeedSegments.push_back(m_currentLowSpeed);
                m_currentLowSpeed.clear();
            }
        }
        m_prevLowSpeed = nowLow;
    }

// Desenha trace fixo adaptado a zoom/pan (screenPts atualizado toda frame)
// Desenha trechos de alta velocidade (verde)
// HIGH-SPEED (verde)
for (auto &segIdx : m_highSpeedSegments) {
    if (segIdx.size() > 1) {
        std::vector<ImVec2> pts;
        pts.reserve(segIdx.size());
        for (size_t i : segIdx) {
            if (i < screenPts.size())
                pts.push_back(screenPts[i]);
        }
        if (pts.size() > 1)
            draw->AddPolyline(pts.data(), pts.size(), IM_COL32(0,255,0,255), false, 3.0f);
    }
}
if (m_currentHighSpeed.size() > 1) {
    std::vector<ImVec2> pts;
    pts.reserve(m_currentHighSpeed.size());
    for (size_t i : m_currentHighSpeed) {
        if (i < screenPts.size())
            pts.push_back(screenPts[i]);
    }
    if (pts.size() > 1)
        draw->AddPolyline(pts.data(), pts.size(), IM_COL32(0,255,0,255), false, 3.0f);
}

// LOW-SPEED (vermelho)
for (auto &segIdx : m_lowSpeedSegments) {
    if (segIdx.size() > 1) {
        std::vector<ImVec2> pts;
        pts.reserve(segIdx.size());
        for (size_t i : segIdx) {
            if (i < screenPts.size())
                pts.push_back(screenPts[i]);
        }
        if (pts.size() > 1)
            draw->AddPolyline(pts.data(), pts.size(), IM_COL32(255,0,0,255), false, 3.0f);
    }
}
if (m_currentLowSpeed.size() > 1) {
    std::vector<ImVec2> pts;
    pts.reserve(m_currentLowSpeed.size());
    for (size_t i : m_currentLowSpeed) {
        if (i < screenPts.size())
            pts.push_back(screenPts[i]);
    }
    if (pts.size() > 1)
        draw->AddPolyline(pts.data(), pts.size(), IM_COL32(255,0,0,255), false, 3.0f);
}


 if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    size_t bestIdx = 0;
    float  bestDist = FLT_MAX;
    for (size_t i = 0; i < screenPts.size(); ++i) {
        float dx = mouse.x - screenPts[i].x;
        float dy = mouse.y - screenPts[i].y;
        float d2 = dx*dx + dy*dy;
        if (d2 < bestDist) { bestDist = d2; bestIdx = i; }
    }
    CommentInfo c;
    c.idx       = bestIdx;
    c.triOffset = ImVec2(0, -10);
    c.visible   = true;
    c.text[0]   = '\0';
    m_comments.push_back(c);
}

// Desenha triângulos e janelas de comentário
for (size_t i = 0; i < m_comments.size(); ++i) {
    auto& cm = m_comments[i];
    if (cm.idx >= screenPts.size()) continue;
    ImVec2 pt   = screenPts[cm.idx];
    ImVec2 base = ImVec2(pt.x + cm.triOffset.x, pt.y + cm.triOffset.y);
    float  s    = 8.0f * m_cartZoom;
    ImVec2 p1{ base.x,       base.y - s };
    ImVec2 p2{ base.x - s,   base.y + s };
    ImVec2 p3{ base.x + s,   base.y + s };
    draw->AddTriangleFilled(p1, p2, p3, IM_COL32(255,165,0,255));

    // Define retângulo de interação cobrindo todo o triângulo
    ImVec2 triMin{ base.x - s, base.y - s };
    ImVec2 triMax{ base.x + s, base.y + s };

    // clique esquerdo no triângulo alterna visibilidade
    if (ImGui::IsMouseHoveringRect(triMin, triMax) \
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        cm.visible = !cm.visible;
    }

    if (cm.visible) {
        // força janela sobre o triângulo toda vez que reaparecer
        ImGui::SetNextWindowPos(ImVec2(base.x + 10, base.y - 10), ImGuiCond_Always);
        char title[32]; sprintf(title, "Comentário %zu", i);
        ImGui::Begin(title, nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowFocus();  // garante foco e topo

        // área de texto com ID único
        char inputId[32]; sprintf(inputId, "##comment_input_%zu", i);
        ImGui::InputTextMultiline(inputId, cm.text, sizeof(cm.text), ImVec2(200,100));

        // botão para remover comentário
        if (ImGui::Button("Remover")) {
            m_comments.erase(m_comments.begin() + i);
            ImGui::End();
            break;
        }
        ImGui::End();
    }
}


        // 9) Interação com marcadores (clicar para info/remover)
// assegura vetores de visibilidade
static std::vector<bool> visG, visR;
if (visG.size() != m_markedPositionsGreen.size())
    visG.assign(m_markedPositionsGreen.size(), false);
if (visR.size() != m_markedPositionsRed.size())
    visR.assign(m_markedPositionsRed.size(), false);

// loop verde
for (size_t i = 0; i < m_markedPositionsGreen.size(); ++i) {
    auto& m = m_markedPositionsGreen[i];
    if (m.idx >= screenPts.size()) continue;
    ImVec2 pos = screenPts[m.idx];
    float  s   = 6.0f * m_cartZoom;            // tamanho escala com zoom
    ImVec2 a{ pos.x - s, pos.y - s };
    ImVec2 b{ pos.x + s, pos.y + s };
    draw->AddRectFilled(a, b, IM_COL32(0,255,0,255));
    if (ImGui::IsMouseHoveringRect(a, b)
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        visG[i] = !visG[i];

    if (visG[i]) {
        ImGui::SetNextWindowPos({ pos.x + 10, pos.y - 10 }, ImGuiCond_Always);
        ImGui::Begin("Info Verde", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Índice: %zu",      m.idx);
        ImGui::Text("Vel.: %.1f km/h", m.speed);
        ImGui::Text("Acel.: %.1f km/h²", m.acceleration);
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            m_markedPositionsGreen.erase(
                m_markedPositionsGreen.begin() + i
            );
            visG.erase(visG.begin() + i);
            ImGui::End();
            break;
        }
        ImGui::End();
    }
}

// loop vermelho (mesma lógica, cor e vector diferente)
for (size_t i = 0; i < m_markedPositionsRed.size(); ++i) {
    auto& m = m_markedPositionsRed[i];
    if (m.idx >= screenPts.size()) continue;
    ImVec2 pos = screenPts[m.idx];
    float  s   = 6.0f * m_cartZoom;
    ImVec2 a{ pos.x - s, pos.y - s };
    ImVec2 b{ pos.x + s, pos.y + s };
    draw->AddRectFilled(a, b, IM_COL32(255,0,0,255));
    if (ImGui::IsMouseHoveringRect(a, b)
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        visR[i] = !visR[i];

    if (visR[i]) {
        ImGui::SetNextWindowPos({ pos.x + 10, pos.y - 10 }, ImGuiCond_Always);
        ImGui::Begin("Info Vermelho", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Índice: %zu",      m.idx);
        ImGui::Text("Vel.: %.1f km/h", m.speed);
        ImGui::Text("Acel.: %.1f km/h²", m.acceleration);
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            m_markedPositionsRed.erase(
                m_markedPositionsRed.begin() + i
            );
            visR.erase(visR.begin() + i);
            ImGui::End();
            break;
        }
        ImGui::End();
    }
}

        // 10) Info extra
        if (showTrackInfo) {
            ImGui::SetNextWindowPos(
                ImVec2(origin.x + size.x - 10, origin.y + 10),
                ImGuiCond_Always,
                ImVec2(1.0f, 0.0f)
            );
            ImGui::Begin("Info Pista", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove);
            ImGui::Text("Velocidade: %.1f km/h",    m_kartSpeed);
            ImGui::Text("Aceleração: %.1f km/h²",   currentAccel);
            ImGui::Text("Multiplicador: %.1fx",     m_speedMultiplier);
            ImGui::Text("Posição (índice): %.1f",   m_kartPosition);
            ImGui::Text("Markers G: %zu",           m_markedPositionsGreen.size());
            ImGui::Text("Markers R: %zu",           m_markedPositionsRed.size());
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
            ImGui::InputText("Nome", m_savedRaces[i].name, sizeof(m_savedRaces[i].name));

            // Exibe o status do slot
            const char* status = m_savedRaces[i].isSaved ? "Salvo" : "Vazio";
            ImGui::Text("Status: %s", status);

            // Botão para Salvar
            if (ImGui::Button("Salvar Estado Atual Neste Slot")) {
                SalvarCorrida(i);
            }

            ImGui::SameLine(); // Coloca o próximo item na mesma linha

            // Desabilita o botão de carregar se o slot estiver vazio
            if (!m_savedRaces[i].isSaved) {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Carregar Este Slot")) {
                CarregarCorrida(i);
                ImGui::SetWindowFocus(NULL); // Opcional: Tira o foco da janela para evitar cliques duplos
                activeTab = 0; // Opcional: Muda para a aba de simulação após carregar
            }
            if (!m_savedRaces[i].isSaved) {
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
    if (m_latIndex < 0 || m_lonIndex < 0) {
        ImGui::TextDisabled("Arraste colunas de latitude e longitude primeiro.");
    } else {
        const auto& latData = m_coordDataList[m_latIndex].data;
        const auto& lonData = m_coordDataList[m_lonIndex].data;
        size_t n = std::min(latData.size(), lonData.size());

        if (ImGui::BeginTable("TabelaCoordenadas", 2,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Latitude",  ImGuiTableColumnFlags_WidthStretch);
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
void Window::Reconstruction::DrawTrackAndKartAt(const std::vector<ImVec2>& screenPts,
                                                const ImVec2& origin,
                                                float scale)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Desenha a linha da pista
    if (!screenPts.empty()) {
        ImU32 trackColor = GetColorForSpeed(m_kartSpeed);
        draw_list->AddPolyline(screenPts.data(),
                               (int)screenPts.size(),
                               trackColor,
                               false,
                               4.0f * scale);
    }

    // Desenha marcadores (verde/vermelho) na posição interpolada em screenPts
    auto drawMarkers = [&](const std::vector<MarkerInfo>& markers, ImU32 col) {
        if (screenPts.size() < 2) return;
        for (const auto& m : markers) {
            int idx = std::clamp(m.trackIndex, 0, (int)screenPts.size() - 2);
            float f = std::clamp(m.trackFrac, 0.0f, 1.0f);
            ImVec2 p = ImLerp(screenPts[idx], screenPts[idx + 1], f);
            draw_list->AddRectFilled({p.x - 5, p.y - 5}, {p.x + 5, p.y + 5}, col);
        }
    };

    // Desenha o kart amarelo seguindo exatamente screenPts
    if (screenPts.size() >= 2) {
        int idx = (int)std::floor(m_kartPosition);
        idx = std::clamp(idx, 0, (int)screenPts.size() - 2);
        float frac = m_kartPosition - (float)idx;
        frac = std::clamp(frac, 0.0f, 1.0f);
        ImVec2 kartP = ImLerp(screenPts[idx], screenPts[idx + 1], frac);
        draw_list->AddCircleFilled(kartP, 8.0f * scale, IM_COL32(255,255,0,255));
    }
}

float Window::Reconstruction::getStepSize() const {
    return m_stepSize;
}

void Window::Reconstruction::setStepSize(float size) {
    m_stepSize = size;
}