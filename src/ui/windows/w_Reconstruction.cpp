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


// Estrutura que armazena as informações do marcador
struct MarkerInfo {
        ImVec2 pos;
        float  speed;
        float  acceleration;
        float  position;
        int    lap;
};

// Define cada ponto da pista com posição (x,y) e velocidade de referência
struct TrackPoint {
        float x, y;
        float referenceSpeed;
};

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

void Window::Reconstruction::addColumnToMap(const std::string& archiveName, const std::string& columnName) {
    // Verifica se a coluna do arquivo já foi adicionada
    for (COORDData& coordData : coordDataList) {
        if (coordData.archive == archiveName && coordData.column == columnName) {
            LOG("WARN", "Reconstrução: A coluna " + columnName + " do arquivo " + archiveName + " já existe.");
            return;
        }
    }

    // Adiciona os eixos
    std::vector<double> data = DB::getInstance().getCSVData(archiveName, columnName);
    if (data.size() == 0) {
        LOG("ERROR",
            "Não foi possível adicionar a coluna " + columnName + " do arquivo " + archiveName + " ao gráfico.");
        return;
    }

    COORDData coordData;
    coordData.data       = data;
    coordData.column     = columnName;
    coordData.archive    = archiveName;
    coordData.multiplier = 1.0;
    coordDataList.push_back(coordData);

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

        // Conteúdo dependendo da área ativa selecionada no menu
        if (activeTab == 0) {
            ImGui::BeginChild("DragAndDropArea");
            {
                // --- Área de Simulação ---
                static Uint32 lastTime    = SDL_GetTicks();
                Uint32        currentTime = SDL_GetTicks();
                float         deltaTime   = (currentTime - lastTime) / 1000.0f;
                lastTime                  = currentTime;
                UpdateKartSimulation(deltaTime);

                // Área para desenho da pista
                ImVec2 region     = ImGui::GetContentRegionAvail();
                ImVec2 drawOrigin = ImGui::GetCursorScreenPos();

                // Cálculos para centralização e escala da pista
                float  trackWidth    = g_track[g_track.size() - 1].x - g_track[0].x;
                float  trackHeight   = g_track[2].y - g_track[1].y;
                float  scaleX        = region.x / trackWidth;
                float  scaleY        = region.y / trackHeight;
                float  scale         = std::min(scaleX, scaleY) * 0.8f;
                float  trackCenterX  = (g_track[0].x + g_track[2].x) / 2;
                float  trackCenterY  = (g_track[0].y + g_track[1].y) / 2;
                float  windowCenterX = region.x / 2;
                float  windowCenterY = region.y / 2;
                ImVec2 offset  = ImVec2(windowCenterX - trackCenterX * scale, windowCenterY - trackCenterY * scale);
                drawOrigin.x  += offset.x;
                drawOrigin.y  += offset.y;

                // Salva valores para reposicionamento das janelas de marcador e comentário
                ImVec2 s_drawOrigin = drawOrigin;
                float  s_scale      = scale;

                // In render, after processing drag & drop, detect when two columns present and build track
                if (coordDataList.size() >= 2) {
                    // assume first is lon, second lat (or match by column names)
                    BuildTrackFromLatLon(1, 0);
                }

                // Desenha a pista, marcadores e o kart
                DrawTrackAndKartAt(drawOrigin, scale);

                // Criação de triângulo para comentário (ao clicar com o botão direito)
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    ImVec2 worldPos;
                    worldPos.x = (mousePos.x - drawOrigin.x) / scale;
                    worldPos.y = (mousePos.y - drawOrigin.y) / scale + Y_OFFSET;
                    ImVec2 initialWindowPos =
                        ImVec2(drawOrigin.x + worldPos.x * scale, drawOrigin.y + (worldPos.y - Y_OFFSET) * scale - 40);
                    g_commentWindows.push_back({worldPos, initialWindowPos, "", true, false, true});
                }

                // Desenho dos Marker Windows
                ImDrawList* overlayDrawList = ImGui::GetForegroundDrawList();
                for (size_t i = 0; i < g_markerWindows.size(); i++) {
                    MarkerWindow& mw         = g_markerWindows[i];
                    std::string   windowName = "Marker Info (" + mw.type + ")##" + std::to_string(i);

                    // Se estiver minimizada, podemos desenhar um indicador extra (ou nada) e não abrir a janela
                    if (mw.minimized) {
                        // Exemplo: desenha um pequeno ponto extra para indicar que há uma janela minimizada
                        ImDrawList* overlayDrawList = ImGui::GetForegroundDrawList();
                        ImVec2      markerScreenPos = ImVec2(s_drawOrigin.x + mw.marker.pos.x * s_scale,
                                                             s_drawOrigin.y + (mw.marker.pos.y - Y_OFFSET) * s_scale);
                        overlayDrawList->AddCircle(markerScreenPos, 6.0f, IM_COL32(255, 255, 255, 255));
                        // Não chama ImGui::Begin, mantendo a janela "escondida"
                        continue;
                    }

                    // Se não estiver minimizada, posiciona e desenha a janela
                    if (mw.reposition) {
                        ImGui::SetNextWindowPos(ImVec2(s_drawOrigin.x + mw.marker.pos.x * s_scale,
                                                       s_drawOrigin.y + (mw.marker.pos.y - Y_OFFSET) * s_scale - 40),
                                                ImGuiCond_Always);
                        mw.reposition = false;
                    } else {
                        ImGui::SetNextWindowPos(ImVec2(s_drawOrigin.x + mw.marker.pos.x * s_scale,
                                                       s_drawOrigin.y + (mw.marker.pos.y - Y_OFFSET) * s_scale - 40),
                                                ImGuiCond_FirstUseEver);
                    }

                    // Usamos um booleano temporário para controlar o fechamento via "x"
                    bool windowOpen = true;
                    if (ImGui::Begin(windowName.c_str(), &windowOpen)) {
                        ImGui::Text("Tipo: %s", mw.type.c_str());
                        ImGui::Text("Velocidade: %.1f km/h", mw.marker.speed);
                        ImGui::Text("Aceleração: %.1f km/h/s", mw.marker.acceleration);
                        ImGui::Text("Posição: %.2f", mw.marker.position);
                        ImGui::Text("Volta: %d", mw.marker.lap);
                        // Botão para minimizar manualmente a janela
                        if (ImGui::Button("Minimizar")) {
                            mw.minimized = true;
                        }
                        ImVec2 winPos          = ImGui::GetWindowPos();
                        ImVec2 winSize         = ImGui::GetWindowSize();
                        ImVec2 arrowTarget     = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y);
                        ImVec2 markerScreenPos = ImVec2(s_drawOrigin.x + mw.marker.pos.x * s_scale,
                                                        s_drawOrigin.y + (mw.marker.pos.y - Y_OFFSET) * s_scale);
                        DrawArrow(ImGui::GetForegroundDrawList(), markerScreenPos, arrowTarget,
                                  IM_COL32(255, 255, 255, 255));
                    }
                    ImGui::End();

                    // Se o usuário fechou a janela via "x", em vez de remover, apenas minimiza
                    if (!windowOpen) {
                        mw.minimized = true;
                    }
                }

                // Desenho dos Comment Windows
                for (size_t i = 0; i < g_commentWindows.size();) {
                    CommentWindow& cw = g_commentWindows[i];
                    ImVec2         commentScreenPos =
                        ImVec2(s_drawOrigin.x + cw.pos.x * s_scale, s_drawOrigin.y + (cw.pos.y - Y_OFFSET) * s_scale);
                    float  markerSize    = 8.0f;
                    ImU32  triangleColor = cw.minimized ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 0, 255);
                    ImVec2 v1            = ImVec2(commentScreenPos.x, commentScreenPos.y - markerSize);
                    ImVec2 v2            = ImVec2(commentScreenPos.x - markerSize, commentScreenPos.y + markerSize);
                    ImVec2 v3            = ImVec2(commentScreenPos.x + markerSize, commentScreenPos.y + markerSize);
                    overlayDrawList->AddTriangleFilled(v1, v2, v3, triangleColor);

                    ImVec2 triMin(commentScreenPos.x - markerSize, commentScreenPos.y - markerSize);
                    ImVec2 triMax(commentScreenPos.x + markerSize, commentScreenPos.y + markerSize);
                    if (ImGui::IsMouseHoveringRect(triMin, triMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        cw.minimized = !cw.minimized;
                        if (!cw.minimized)
                            cw.reposition = true;
                    }

                    if (!cw.minimized) {
                        std::string windowName = "Comentário##" + std::to_string(i);
                        if (cw.reposition) {
                            ImGui::SetNextWindowPos(cw.windowPos, ImGuiCond_Always);
                            cw.reposition = false;
                        }

                        if (ImGui::Begin(windowName.c_str(), &cw.open)) {
                            ImGui::Text("Comentário:");
                            char buf[256];
                            std::strncpy(buf, cw.comment.c_str(), 255);
                            buf[255] = '\0';
                            if (ImGui::InputTextMultiline("##comentario", buf, 256, ImVec2(200, 100))) {
                                cw.comment = std::string(buf);
                            }
                            ImVec2 winPos      = ImGui::GetWindowPos();
                            ImVec2 winSize     = ImGui::GetWindowSize();
                            ImVec2 arrowTarget = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y);
                            DrawArrow(overlayDrawList, commentScreenPos, arrowTarget, IM_COL32(255, 255, 0, 255));
                        }
                        cw.windowPos = ImGui::GetWindowPos();
                        ImGui::End();
                    }

                    if (!cw.open) {
                        g_commentWindows.erase(g_commentWindows.begin() + i);
                    } else {
                        i++;
                    }
                }

                if (showTrackInfo) {
                    ImGui::Separator();
                    ImGui::Text("Informações da Pista:");
                    ImGui::Text("Velocidade: %.1f km/h", g_kartSpeed);
                    ImGui::Text("Posição: %.2f", g_kartPosition);
                    ImGui::Text("Voltas: %d", g_lapCount);
                    ImGui::Text("Aceleração: %.1f km/h/s", g_currentAcceleration);
                    ImGui::Text("Auto Speed: %s", g_autoSpeed ? "ON" : "OFF");
                }
            }
            ImGui::EndChild();
            processColumnDragDrop();
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
void Window::Reconstruction::DrawTrackAndKartAt(const ImVec2& origin, float scale) {
    ImDrawList* draw_list  = ImGui::GetWindowDrawList();
    ImU32       trackColor = GetColorForSpeed(g_kartSpeed);
    for (size_t i = 0; i + 1 < g_track.size(); i++) {
        ImVec2 p1(origin.x + g_track[i].x * scale, origin.y + (g_track[i].y - Y_OFFSET) * scale);
        ImVec2 p2(origin.x + g_track[i + 1].x * scale, origin.y + (g_track[i + 1].y - Y_OFFSET) * scale);
        draw_list->AddLine(p1, p2, trackColor, 4.0f);
    }

    // Lambda para detectar clique sobre o retângulo do marcador
    auto checkClick = [&](const MarkerInfo& marker, const std::string& type) {
        ImVec2 rectCenter(origin.x + marker.pos.x * scale, origin.y + (marker.pos.y - Y_OFFSET) * scale);
        float  halfSize = 5.0f;
        ImVec2 rectMin(rectCenter.x - halfSize, rectCenter.y - halfSize);
        ImVec2 rectMax(rectCenter.x + halfSize, rectCenter.y + halfSize);
        if (ImGui::IsMouseHoveringRect(rectMin, rectMax) && ImGui::IsMouseClicked(0)) {
            const float epsilon     = 0.001f;
            bool        windowFound = false;
            for (auto& mw : g_markerWindows) {
                if (mw.type == type && std::abs(mw.marker.pos.x - marker.pos.x) < epsilon &&
                    std::abs(mw.marker.pos.y - marker.pos.y) < epsilon) {
                    // Se existir, alterna o estado minimizado
                    mw.minimized = !mw.minimized;
                    if (!mw.minimized)
                        mw.reposition = true;
                    windowFound = true;
                    break;
                }
            }
            if (!windowFound) {
                // Cria a janela com estado minimizado = false (visível)
                g_markerWindows.push_back({marker, type, true, false, false});
            }
        }
        ImU32 color = (type == "Verde") ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
        draw_list->AddRectFilled(rectMin, rectMax, color);
    };

    // Desenha os marcadores e verifica clique
    for (const auto& marker : g_markedPositionsGreen)
        checkClick(marker, "Verde");
    for (const auto& marker : g_markedPositionsRed)
        checkClick(marker, "Vermelho");

    // Desenha o kart (círculo amarelo)
    int   idx  = static_cast<int>(g_kartPosition);
    float frac = g_kartPosition - idx;
    if (idx >= static_cast<int>(g_track.size()) - 1) {
        idx  = static_cast<int>(g_track.size()) - 2;
        frac = 1.0f;
    }
    float  xPos = g_track[idx].x + (g_track[idx + 1].x - g_track[idx].x) * frac;
    float  yPos = g_track[idx].y + (g_track[idx + 1].y - g_track[idx].y) * frac;
    ImVec2 kartPos(origin.x + xPos * scale, origin.y + (yPos - Y_OFFSET) * scale);
    draw_list->AddCircleFilled(kartPos, 8.0f, IM_COL32(255, 255, 0, 255));
}

void Window::Reconstruction::UpdateKartSimulation(float deltaTime) {
    const Uint8* keystates    = SDL_GetKeyboardState(NULL);
    bool         accelerating = keystates[SDL_SCANCODE_W];
    bool         braking      = keystates[SDL_SCANCODE_S];

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

    if (g_autoSpeed) {
        int idx = static_cast<int>(g_kartPosition);
        if (idx >= 0 && idx < static_cast<int>(g_track.size()))
            g_kartSpeed = g_track[idx].referenceSpeed;
    }

    static bool wasBelow50 = true;
    if (wasBelow50 && g_kartSpeed > 50.0f) {
        int   idx  = static_cast<int>(g_kartPosition);
        float frac = g_kartPosition - idx;
        if (idx >= static_cast<int>(g_track.size()) - 1) {
            idx  = static_cast<int>(g_track.size()) - 2;
            frac = 1.0f;
        }
        float xPos = g_track[idx].x + (g_track[idx + 1].x - g_track[idx].x) * frac;
        float yPos = g_track[idx].y + (g_track[idx + 1].y - g_track[idx].y) * frac;
        g_markedPositionsGreen.push_back(
            {ImVec2(xPos, yPos), g_kartSpeed, g_currentAcceleration, g_kartPosition, g_lapCount});
        wasBelow50 = false;
    }
    if (g_kartSpeed <= 50.0f)
        wasBelow50 = true;

    static bool wasAbove10 = true;
    if (wasAbove10 && g_kartSpeed <= 10.0f && braking) {
        int   idx  = static_cast<int>(g_kartPosition);
        float frac = g_kartPosition - idx;
        if (idx >= static_cast<int>(g_track.size()) - 1) {
            idx  = static_cast<int>(g_track.size()) - 2;
            frac = 1.0f;
        }
        float xPos = g_track[idx].x + (g_track[idx + 1].x - g_track[idx].x) * frac;
        float yPos = g_track[idx].y + (g_track[idx + 1].y - g_track[idx].y) * frac;
        g_markedPositionsRed.push_back(
            {ImVec2(xPos, yPos), g_kartSpeed, g_currentAcceleration, g_kartPosition, g_lapCount});
        wasAbove10 = false;
    }
    if (g_kartSpeed > 10.0f)
        wasAbove10 = true;

    float mps       = g_kartSpeed / 3.6f;
    g_kartPosition += mps * deltaTime * 0.5f;
    if (g_kartPosition >= g_track.size() - 1) {
        g_kartPosition = 0.0f;
        g_lapCount++;
    }

    static float prevSpeed = g_kartSpeed;
    if (deltaTime > 0.0f)
        g_currentAcceleration = (g_kartSpeed - prevSpeed) / deltaTime;
    prevSpeed = g_kartSpeed;
}