#include "ui/windows/w_Reconstruction.hpp"
#include "ImGuiWrapper.hpp" // Assuming wrapper includes ImGui
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

static std::vector<TrackPoint> g_track;

inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}


//variaveis para aba "coordenadas"
static int g_latIndex = -1;
static int g_lonIndex = -1;

// Variáveis de simulação do kart
static float g_kartSpeed    = 50.0f;
static float g_kartPosition = 0.0f;
static bool  g_autoSpeed    = false; // Se true, força a velocidade a ser a referência

// Vetores para registrar os marcadores (verde e vermelho) com informações do instante do registro
static std::vector<MarkerInfo> g_markedPositionsGreen;
static std::vector<MarkerInfo> g_markedPositionsRed;

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
        float                   kartSpeed;
        float                   kartPosition;
        bool                    autoSpeed;
        std::vector<MarkerInfo> markedPositionsGreen;
        std::vector<MarkerInfo> markedPositionsRed;
        int                     lapCount;
        float                   currentAcceleration;
};
// Armazena até 10 corridas e indica se o slot está preenchido
static RaceState g_savedRaces[10];
static bool      g_savedRaceExists[10] = {false, false, false, false, false, false, false, false, false, false};

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
    if (latIndex >= coordDataList.size() || lonIndex >= coordDataList.size()) return;

    // Extract raw lat/lon
    const auto& latData = coordDataList[latIndex].data;
    const auto& lonData = coordDataList[lonIndex].data;
    size_t n = std::min(latData.size(), lonData.size());
    
    // Convert to planar X/Y
    std::vector<float> xs(n), ys(n);
    for (size_t i = 0; i < n; ++i) {
        xs[i] = static_cast<float>(lonData[i]);
        ys[i] = static_cast<float>(latData[i]);
    }
    ConvertLatLonToXY(xs, ys);

    // Rebuild g_track
    g_track.clear();
    g_track.reserve(n+1);
    for (size_t i = 0; i < n; ++i) {
        TrackPoint pt;
        pt.x = xs[i];
        pt.y = ys[i];
        pt.referenceSpeed = DEFAULT_SPEED; // or compute per segment
        g_track.push_back(pt);
    }
    // close loop if desired
    if (n > 1) g_track.push_back(g_track.front());
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
    for (COORDData& coordData : coordDataList) {
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
    coordDataList.push_back(coordData);

    // Identifica se é latitude ou longitude e armazena o índice
    int newIndex = static_cast<int>(coordDataList.size()) - 1;
    std::string lower = columnName;
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
// Funções de salvamento e carregamento de corridas
//---------------------------------------------------------

static void SaveRacesToDisk() {
    std::ofstream ofs(SAVE_FILE);
    if (!ofs.is_open())
        return;
    for (int i = 0; i < 10; i++) {
        ofs << (g_savedRaceExists[i] ? 1 : 0) << "\n";
        if (g_savedRaceExists[i]) {
            RaceState& rs = g_savedRaces[i];
            ofs << rs.kartSpeed << " " << rs.kartPosition << " " << rs.autoSpeed << " " << rs.lapCount << " "
                << rs.currentAcceleration << "\n";
            ofs << rs.markedPositionsGreen.size() << "\n";
            for (auto& m : rs.markedPositionsGreen) {
                ofs << m.pos.x << " " << m.pos.y << " " << m.speed << " " << m.acceleration << " " << m.position << " "
                    << m.lap << "\n";
            }
            ofs << rs.markedPositionsRed.size() << "\n";
            for (auto& m : rs.markedPositionsRed) {
                ofs << m.pos.x << " " << m.pos.y << " " << m.speed << " " << m.acceleration << " " << m.position << " "
                    << m.lap << "\n";
            }
        }
    }
    ofs.close();
}

static void LoadRacesFromDisk() {
    std::ifstream ifs(SAVE_FILE);
    if (!ifs.is_open())
        return;
    for (int i = 0; i < 10; i++) {
        int exists;
        ifs >> exists;
        g_savedRaceExists[i] = (exists == 1);
        if (g_savedRaceExists[i]) {
            RaceState& rs = g_savedRaces[i];
            ifs >> rs.kartSpeed >> rs.kartPosition >> rs.autoSpeed >> rs.lapCount >> rs.currentAcceleration;
            size_t greenSize;
            ifs >> greenSize;
            rs.markedPositionsGreen.clear();
            for (size_t j = 0; j < greenSize; j++) {
                MarkerInfo mi;
                ifs >> mi.pos.x >> mi.pos.y >> mi.speed >> mi.acceleration >> mi.position >> mi.lap;
                rs.markedPositionsGreen.push_back(mi);
            }
            size_t redSize;
            ifs >> redSize;
            rs.markedPositionsRed.clear();
            for (size_t j = 0; j < redSize; j++) {
                MarkerInfo mi;
                ifs >> mi.pos.x >> mi.pos.y >> mi.speed >> mi.acceleration >> mi.position >> mi.lap;
                rs.markedPositionsRed.push_back(mi);
            }
        }
    }
    ifs.close();
}

static void SaveRace(int index) {
    g_savedRaces[index].kartSpeed            = g_kartSpeed;
    g_savedRaces[index].kartPosition         = g_kartPosition;
    g_savedRaces[index].autoSpeed            = g_autoSpeed;
    g_savedRaces[index].markedPositionsGreen = g_markedPositionsGreen;
    g_savedRaces[index].markedPositionsRed   = g_markedPositionsRed;
    g_savedRaces[index].lapCount             = g_lapCount;
    g_savedRaces[index].currentAcceleration  = g_currentAcceleration;
    g_savedRaceExists[index]                 = true;
    SaveRacesToDisk();
}

static void LoadRace(int index) {
    if (!g_savedRaceExists[index])
        return;
    g_kartSpeed            = g_savedRaces[index].kartSpeed;
    g_kartPosition         = g_savedRaces[index].kartPosition;
    g_autoSpeed            = g_savedRaces[index].autoSpeed;
    g_markedPositionsGreen = g_savedRaces[index].markedPositionsGreen;
    g_markedPositionsRed   = g_savedRaces[index].markedPositionsRed;
    g_lapCount             = g_savedRaces[index].lapCount;
    g_currentAcceleration  = g_savedRaces[index].currentAcceleration;
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

//---------------------------------------------------------
// Implementação da janela de reconstrução
//---------------------------------------------------------

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) {
    this->title = "Reconstrução de Pista";
    LoadRacesFromDisk();
}

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
        static int  activeTab     = 0;
        static bool showTrackInfo = false;

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
            g_commentWindows.clear();
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

 if (activeTab == 0) {
    // 1) Sliders para altura e zoom do gráfico
    static float cartHeight = 300.0f;
    static float cartZoom   = 1.0f;
    ImGui::SliderFloat("Altura do Gráfico", &cartHeight, 100.0f, 800.0f, "%.0f px");
    ImGui::SliderFloat("Zoom (escala)",     &cartZoom,   0.1f, 5.0f,   "%.2fx");
    ImGui::Separator();

    // 2) Pan offset
    static ImVec2 panOffset = ImVec2(0, 0);

    // 3) Canvas dedicado
    ImGui::BeginChild("Sim_Cartesiano", ImVec2(0, cartHeight), true);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin    = ImGui::GetCursorScreenPos();
        ImVec2 size      = ImGui::GetContentRegionAvail();
        ImVec2 maxPt(origin.x + size.x, origin.y + size.y);

        // fundo + eixos
        draw->AddRectFilled(origin, maxPt, IM_COL32(20,20,20,255));
        ImVec2 mid((origin.x+maxPt.x)*0.5f, (origin.y+maxPt.y)*0.5f);
        draw->AddLine({origin.x, mid.y}, {maxPt.x, mid.y}, IM_COL32(100,100,100,255));
        draw->AddLine({mid.x, origin.y}, {mid.x, maxPt.y}, IM_COL32(100,100,100,255));

        // captura drag
        ImGui::InvisibleButton("canvas_drag", size);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            panOffset.x += ImGui::GetIO().MouseDelta.x;
            panOffset.y += ImGui::GetIO().MouseDelta.y;
        }

        // coleta de screenPts
        std::vector<ImVec2> screenPts;
        if (g_latIndex >= 0 && g_lonIndex >= 0) {
            const auto& lat = coordDataList[g_latIndex].data;
            const auto& lon = coordDataList[g_lonIndex].data;
            size_t n = std::min(lat.size(), lon.size());
            if (n) {
                // bounds
                double minX=lon[0], maxX=lon[0], minY=lat[0], maxY=lat[0];
                for (size_t i=1;i<n;++i) {
                    minX = std::min(minX, lon[i]); maxX = std::max(maxX, lon[i]);
                    minY = std::min(minY, lat[i]); maxY = std::max(maxY, lat[i]);
                }
                double rX = maxX-minX, rY = maxY-minY;
                ImVec2 inner(size.x*cartZoom, size.y*cartZoom);
                for (size_t i=0;i<n;++i) {
                    float nx = rX>0?(lon[i]-minX)/rX:0.5f;
                    float ny = rY>0?(lat[i]-minY)/rY:0.5f;
                    ImVec2 p = {
                        origin.x + panOffset.x + nx*inner.x,
                        origin.y + panOffset.y + (1.0f-ny)*inner.y
                    };
                    screenPts.push_back(p);
                    draw->AddCircleFilled(p, 5.0f, IM_COL32(255,255,255,255));
                }
                // linha grossa
                draw->AddPolyline(screenPts.data(), (int)screenPts.size(),
                                IM_COL32(255,255,255,255), false, 10.0f);
            }
        }

        // simulação do kart sobre screenPts
        static Uint32 last = SDL_GetTicks();
        Uint32 now = SDL_GetTicks();
        float dt = (now-last)/1000.0f; last=now;
        UpdateKartSimulation(dt, screenPts);
        if (!screenPts.empty()) {
            int idx = (int)floor(g_kartPosition) % screenPts.size();
            int idx2 = (idx+1)%screenPts.size();
            float f = g_kartPosition - floor(g_kartPosition);
            ImVec2 kp = {
                screenPts[idx].x + (screenPts[idx2].x-screenPts[idx].x)*f,
                screenPts[idx].y + (screenPts[idx2].y-screenPts[idx].y)*f
            };
            draw->AddCircleFilled(kp, 8.0f, IM_COL32(255,255,0,255));
        }
        if (showTrackInfo) {
            ImGui::SetNextWindowPos(ImVec2(origin.x + size.x - 10, origin.y + 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;
            ImGui::Begin("Info Pista", nullptr, flags);
            ImGui::Text("Velocidade: %.1f km/h", g_kartSpeed);
            ImGui::Text("Voltas: %d", g_lapCount);
            ImGui::Text("Posição: %.1f", g_kartPosition);
            ImGui::Text("Markers G: %zu", g_markedPositionsGreen.size());
            ImGui::Text("Markers R: %zu", g_markedPositionsRed.size());
            ImGui::End();
        }

        // ——— Desenhar marcadores nas posições reais (após linha branca) ———
        if (!screenPts.empty() && g_track.size() >= 2) {
            const auto& lat = coordDataList[g_latIndex].data;
            const auto& lon = coordDataList[g_lonIndex].data;
            size_t n = std::min(lat.size(), lon.size());
            double minX = lon[0], maxX = lon[0], minY = lat[0], maxY = lat[0];
            for (size_t i = 1; i < n; ++i) {
                minX = std::min(minX, lon[i]); maxX = std::max(maxX, lon[i]);
                minY = std::min(minY, lat[i]); maxY = std::max(maxY, lat[i]);
            }
            double rX = maxX - minX, rY = maxY - minY;
            ImVec2 inner = ImVec2(size.x * cartZoom, size.y * cartZoom);

            auto drawMarker = [&](const MarkerInfo& m, ImU32 col) {
                float nx = rX > 0 ? (m.pos.x - minX) / rX : 0.5f;
                float ny = rY > 0 ? (m.pos.y - minY) / rY : 0.5f;
                ImVec2 p = {
                    origin.x + panOffset.x + nx * inner.x,
                    origin.y + panOffset.y + (1.0f - ny) * inner.y
                };
                draw->AddCircleFilled(p, 6.0f, col); // desenha no mesmo draw!
            };

            for (auto& m : g_markedPositionsGreen)
                drawMarker(m, IM_COL32(0,255,0,255));
            for (auto& m : g_markedPositionsRed)
                drawMarker(m, IM_COL32(255,0,0,255));
        }

    ImGui::EndChild();
}
    

        else if (activeTab == 1) {
            // --- Área de Gerenciamento de Corridas ---
            ImGui::Text("Salvar Corrida:");
            for (int i = 0; i < 10; i++) {
                std::string saveLabel = "Salvar Corrida " + std::to_string(i + 1);
                if (ImGui::Button(saveLabel.c_str()))
                    SaveRace(i);
                ImGui::SameLine();
                if (g_savedRaceExists[i]) {
                    std::string loadLabel = "Carregar Corrida " + std::to_string(i + 1);
                    if (ImGui::Button(loadLabel.c_str()))
                        LoadRace(i);
                } else {
                    ImGui::Text("Slot vazio");
                }
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
    ImU32 trackColor = GetColorForSpeed(g_kartSpeed);

    // 1) Desenha a linha do track
    if (!screenPts.empty()) {
        draw_list->AddPolyline(screenPts.data(),
                               (int)screenPts.size(),
                               trackColor,
                               false,
                               4.0f * scale);
    }

    // 2) Desenha marcadores verdes/vermelhos
    auto drawMarkers = [&](const std::vector<MarkerInfo>& markers, ImU32 col){
        for (auto& m : markers) {
            // encontra posição interpolada em screenPts
            // aqui assumimos m.trackIndex e m.trackFrac definidos na simulação
            int idx = m.trackIndex;
            float f = m.trackFrac;
            idx = std::clamp(idx, 0, (int)screenPts.size()-2);
            ImVec2 p = {
                screenPts[idx].x + (screenPts[idx+1].x - screenPts[idx].x)*f,
                screenPts[idx].y + (screenPts[idx+1].y - screenPts[idx].y)*f
            };
            draw_list->AddRectFilled(
                ImVec2(p.x-5, p.y-5),
                ImVec2(p.x+5, p.y+5),
                col
            );
        }
    };
    drawMarkers(g_markedPositionsGreen, IM_COL32(0,255,0,255));
    drawMarkers(g_markedPositionsRed,   IM_COL32(255,0,0,255));

    // 3) Desenha o kart
    if (!screenPts.empty()) {
        int idx = (int)std::floor(g_kartPosition);
        float frac = g_kartPosition - idx;
        idx = std::clamp(idx, 0, (int)screenPts.size()-2);
        ImVec2 kartP = {
            screenPts[idx].x + (screenPts[idx+1].x - screenPts[idx].x) * frac,
            screenPts[idx].y + (screenPts[idx+1].y - screenPts[idx].y) * frac
        };
        draw_list->AddCircleFilled(kartP, 8.0f * scale, IM_COL32(255,255,0,255));
    }
}

// Atualização da função UpdateKartSimulation para usar pontos de pista em screenPts
void Window::Reconstruction::UpdateKartSimulation(float deltaTime,
                                                  const std::vector<ImVec2>& screenPts)
{
    // Captura estado do teclado (W/S) para aceleração e frenagem
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    bool accelerating = keystates[SDL_SCANCODE_W];
    bool braking      = keystates[SDL_SCANCODE_S];

    // --- CONTROLE DE VELOCIDADE ---
    if (accelerating)
        g_kartSpeed += ACCELERATION * deltaTime;
    if (braking)
        g_kartSpeed -= DECELERATION * deltaTime;
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

    // --- PISTA INSUFICIENTE: AVANÇO LINEAR ---
    if (screenPts.size() < 2) {
        float mps = g_kartSpeed / 3.6f;
        g_kartPosition += mps * deltaTime * 0.5f;
        return;
    }

    // --- CÁLCULO DE COMPRIMENTOS DE SEGMENTOS ---
    std::vector<float> segLengths(screenPts.size() - 1);
    for (size_t i = 0; i + 1 < screenPts.size(); ++i) {
        float dx = screenPts[i + 1].x - screenPts[i].x;
        float dy = screenPts[i + 1].y - screenPts[i].y;
        segLengths[i] = sqrtf(dx * dx + dy * dy);
    }

    // --- MARCADORES ---
    // Verde: ultrapassa 50 km/h
    static bool wasBelow50 = true;
    if (wasBelow50 && g_kartSpeed > 50.0f) {
        int idx = static_cast<int>(std::floor(g_kartPosition));
        float frac = g_kartPosition - idx;
        idx = std::clamp(idx, 0, (int)screenPts.size() - 2);
        ImVec2 p1 = screenPts[idx];
        ImVec2 p2 = screenPts[idx + 1];
        ImVec2 pos = ImVec2(
            p1.x + (p2.x - p1.x) * frac,
            p1.y + (p2.y - p1.y) * frac
        );
        g_markedPositionsGreen.push_back({ pos,
                                           g_kartSpeed,
                                           g_currentAcceleration,
                                           g_kartPosition,
                                           g_lapCount,
                                           idx,
                                           frac });
        wasBelow50 = false;
    }
    if (g_kartSpeed <= 50.0f)
        wasBelow50 = true;

    // Vermelho: freia abaixo de 10 km/h
    static bool wasAbove10 = true;
    if (wasAbove10 && g_kartSpeed <= 10.0f && braking) {
        int idx = static_cast<int>(std::floor(g_kartPosition));
        float frac = g_kartPosition - idx;
        idx = std::clamp(idx, 0, (int)screenPts.size() - 2);
        ImVec2 p1 = screenPts[idx];
        ImVec2 p2 = screenPts[idx + 1];
        ImVec2 pos = ImVec2(
            p1.x + (p2.x - p1.x) * frac,
            p1.y + (p2.y - p1.y) * frac
        );
        g_markedPositionsRed.push_back({ pos,
                                         g_kartSpeed,
                                         g_currentAcceleration,
                                         g_kartPosition,
                                         g_lapCount,
                                         idx,
                                         frac });
        wasAbove10 = false;
    }
    if (g_kartSpeed > 10.0f)
        wasAbove10 = true;

    // --- ATUALIZAÇÃO DE POSIÇÃO ---
    // Converte km/h para pixels/s
    float speedPxPerS = (g_kartSpeed / 3.6f) * 1.0f;
    float distToMove = speedPxPerS * deltaTime * 0.5f;

    float remaining = distToMove;
    int idx = static_cast<int>(std::floor(g_kartPosition));
    float frac = g_kartPosition - idx;

    // Consome resto do segmento atual
    if (idx < (int)segLengths.size()) {
        float segRemain = segLengths[idx] * (1.0f - frac);
        if (remaining < segRemain) {
            g_kartPosition += remaining / segLengths[idx];
            remaining = 0;
        } else {
            remaining -= segRemain;
            ++idx;
            frac = 0.0f;
        }
    }
    // Consome segmentos seguintes
    while (remaining > 0 && idx < (int)segLengths.size()) {
        if (remaining < segLengths[idx]) {
            frac = remaining / segLengths[idx];
            remaining = 0;
        } else {
            remaining -= segLengths[idx];
            ++idx;
            frac = 0.0f;
        }
    }
    g_kartPosition = idx + frac;

    // Reinicia volta
    if (g_kartPosition >= screenPts.size() - 1) {
        g_kartPosition = 0.0f;
        g_lapCount++;
    }

    // Atualiza aceleração
    static float prevSpeed = g_kartSpeed;
    if (deltaTime > 0.0f)
        g_currentAcceleration = (g_kartSpeed - prevSpeed) / deltaTime;
    prevSpeed = g_kartSpeed;
}
