#include "ui/windows/w_Reconstruction.hpp"
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include <algorithm>
#include <cmath>

static std::string stripMapExtension(const std::string& name) {
    std::string result = name;
    for (const std::string ext : {".mbtiles", ".mptiles"}) {
        if (result.size() >= ext.size() && result.compare(result.size() - ext.size(), ext.size(), ext) == 0) {
            result = result.substr(0, result.size() - ext.size());
        }
    }
    return result;
}

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) {
    this->title = "Reconstrução de Pista";

    m_zoomScale = 0.5f;

    // Escanear mapas na pasta maps
    scanAvailableMaps();

    if (m_availableMaps.empty()) {
        m_statusMessage = "Nenhum arquivo .mbtiles encontrado na pasta 'maps'!";
        LOG("ERROR", "MBTiles: " + m_statusMessage);
        return;
    }

    // Tentar carregar pampulha.mbtiles como padrão, senão o primeiro da lista
    m_currentMapName = m_availableMaps[0];
    for (const auto& mapName : m_availableMaps) {
        if (mapName == "pampulha.mbtiles") {
            m_currentMapName = mapName;
            break;
        }
    }

    std::string path = "maps/" + m_currentMapName;
    int         rc   = sqlite3_open(path.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        m_statusMessage = "Falha ao abrir banco: " + path + " (Erro: " + std::to_string(rc) + ")";
        m_db            = nullptr;
        LOG("ERROR", "MBTiles: " + m_statusMessage);
        return;
    }

    findFirstAvailableTile();
}

Window::Reconstruction::~Reconstruction() {
    clearCache();
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool Window::Reconstruction::isLoaded() const { return m_loaded; }

void Window::Reconstruction::scanAvailableMaps() {
    m_availableMaps.clear();
    const std::string mapsFolder = "maps";

    try {
        if (std::filesystem::exists(mapsFolder) && std::filesystem::is_directory(mapsFolder)) {
            for (const auto& entry : std::filesystem::directory_iterator(mapsFolder)) {
                if (entry.is_regular_file() &&
                    (entry.path().extension() == ".mbtiles" || entry.path().extension() == ".mptiles")) {
                    m_availableMaps.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        LOG("ERROR", "Erro ao escanear pasta maps: " + std::string(e.what()));
    }

    std::sort(m_availableMaps.begin(), m_availableMaps.end());
}

void Window::Reconstruction::findFirstAvailableTile() {
    if (!m_db)
        return;

    // Busca o centro geográfico do mapa no nível de zoom mais próximo de 18
    const char* sql =
        "SELECT zoom_level, MIN(tile_column), MAX(tile_column), MIN(tile_row), MAX(tile_row) "
        "FROM tiles WHERE zoom_level = (SELECT zoom_level FROM tiles ORDER BY abs(zoom_level - 18) ASC LIMIT 1);";
    sqlite3_stmt* stmt = nullptr;
    int           rc   = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            m_testZ  = sqlite3_column_int(stmt, 0);
            int minX = sqlite3_column_int(stmt, 1);
            int maxX = sqlite3_column_int(stmt, 2);
            int minY = sqlite3_column_int(stmt, 3);
            int maxY = sqlite3_column_int(stmt, 4);

            m_testX = minX + (maxX - minX) / 2;
            m_testY = minY + (maxY - minY) / 2;

            // Inicializar a câmera georreferenciada contínua no centro exato do mapa
            double cx     = m_testX + 0.5;
            double cy_tms = m_testY + 0.5;
            double cy_osm = (1 << m_testZ) - 1.0 - cy_tms;
            double pi     = 3.14159265358979323846;

            m_centerLon = cx * 360.0 / (1 << m_testZ) - 180.0;
            double v    = pi * (1.0 - 2.0 * cy_osm / (1 << m_testZ));
            m_centerLat = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

            m_loaded        = true;
            m_statusMessage = "Mapa " + m_currentMapName + " carregado em Z=" + std::to_string(m_testZ);
            LOG("INFO", "MBTiles: " + m_statusMessage);
        } else {
            m_statusMessage = "Nenhum bloco encontrado na tabela 'tiles'.";
            LOG("WARN", "MBTiles: " + m_statusMessage);
        }
        sqlite3_finalize(stmt);
    } else {
        m_statusMessage = "Erro ao preparar query de inicialização: " + std::string(sqlite3_errmsg(m_db));
        LOG("ERROR", "MBTiles: " + m_statusMessage);
    }
}

SDL_Texture* Window::Reconstruction::getTileTexture(int z, int x, int y) {
    if (!m_db)
        return nullptr;

    // 1. Buscar no cache de texturas
    for (const auto& cached : m_tileCache) {
        if (cached.z == z && cached.x == x && cached.y == y) {
            return cached.texture;
        }
    }

    // 2. Não encontrado no cache, buscar no SQLite
    const char*   sql  = "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;";
    sqlite3_stmt* stmt = nullptr;
    int           rc   = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }

    sqlite3_bind_int(stmt, 1, z);
    sqlite3_bind_int(stmt, 2, x);
    sqlite3_bind_int(stmt, 3, y);

    SDL_Texture* texture = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int         size = sqlite3_column_bytes(stmt, 0);

        if (blob && size > 0) {
            SDL_RWops* rw = SDL_RWFromMem(const_cast<void*>(blob), size);
            if (rw) {
                SDL_Surface* surface = IMG_Load_RW(rw, 1);
                if (surface) {
                    texture = SDL_CreateTextureFromSurface(SDLWrapper::renderer, surface);
                    SDL_FreeSurface(surface);
                }
            }
        }
    }
    sqlite3_finalize(stmt);

    // Salva no cache de texturas
    CachedTile cached;
    cached.z       = z;
    cached.x       = x;
    cached.y       = y;
    cached.texture = texture;
    m_tileCache.push_back(cached);

    return texture;
}

void Window::Reconstruction::clearCache() {
    for (auto& cached : m_tileCache) {
        if (cached.texture) {
            SDL_DestroyTexture(cached.texture);
        }
    }
    m_tileCache.clear();
}

static ImVec2 getScreenPosFromLatLon(double lat, double lon, double centerLat, double centerLon, int z, float tileSize,
                                     ImVec2 windowPos, ImVec2 windowSize) {
    double pi    = 3.14159265358979323846;
    double x_osm = (lon + 180.0) / 360.0 * (1 << z);

    double latRad = lat * pi / 180.0;
    double y_osm  = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / pi) / 2.0 * (1 << z);
    double y_tms  = (1 << z) - 1.0 - y_osm;

    double cx_osm  = (centerLon + 180.0) / 360.0 * (1 << z);
    double clatRad = centerLat * pi / 180.0;
    double cy_osm  = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << z);
    double cy_tms  = (1 << z) - 1.0 - cy_osm;

    ImVec2 centerScreen(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

    float screenX = centerScreen.x + (static_cast<float>(x_osm - cx_osm)) * tileSize;
    float screenY = centerScreen.y - (static_cast<float>(y_tms - cy_tms)) * tileSize;

    return ImVec2(screenX, screenY);
}

void Window::Reconstruction::centerOnTrack() {
    if (m_selectedFileName.empty() || m_selectedLatCol.empty() || m_selectedLonCol.empty())
        return;

    try {
        const std::vector<double>* latDataPtr = nullptr;
        const std::vector<double>* lonDataPtr = nullptr;

        if (m_selectedFileType == "CSV") {
            latDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLatCol);
            lonDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLonCol);
        } else if (m_selectedFileType == "Telemetry") {
            latDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLatCol);
            lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLonCol);
        }

        if (!latDataPtr || !lonDataPtr)
            return;

        const auto& latData = *latDataPtr;
        const auto& lonData = *lonDataPtr;

        size_t count = std::min(latData.size(), lonData.size());
        if (count == 0)
            return;

        double avgLat = 0.0;
        double avgLon = 0.0;
        for (size_t i = 0; i < count; ++i) {
            avgLat += latData[i];
            avgLon += lonData[i];
        }
        avgLat /= count;
        avgLon /= count;

        m_centerLat = avgLat;
        m_centerLon = avgLon;

        LOG("INFO",
            "MBTiles: Centralizado no traçado GPS (" + std::to_string(avgLat) + ", " + std::to_string(avgLon) + ")");
    } catch (...) {
    }
}

void Window::Reconstruction::render() {
    if (!isOpen || !*isOpen)
        return;

    // Define margem interna zero para um canvas geográfico contínuo premium
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(this->title.c_str(), this->isOpen,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (m_loaded) {
        ImVec2      windowPos  = ImGui::GetWindowPos();
        ImVec2      windowSize = ImGui::GetWindowSize();
        ImDrawList* drawList   = ImGui::GetWindowDrawList();

        // Calcular dinamicamente a posição do bloco (testX, testY) e pan da tela a partir da câmera georreferenciada
        // contínua
        double pi      = 3.14159265358979323846;
        double cx_osm  = (m_centerLon + 180.0) / 360.0 * (1 << m_testZ);
        double clatRad = m_centerLat * pi / 180.0;
        double cy_osm  = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << m_testZ);
        double cy_tms  = (1 << m_testZ) - 1.0 - cy_osm;

        float tileSize = 256.0f * m_zoomScale;

        m_testX = static_cast<int>(std::floor(cx_osm));
        m_testY = static_cast<int>(std::floor(cy_tms));

        m_panX = -static_cast<float>(cx_osm - (m_testX + 0.5)) * tileSize;
        m_panY = static_cast<float>(cy_tms - (m_testY + 0.5)) * tileSize;

        // 1. Adicionar o MenuBar com todas as informações e seletores de Mapa
        if (ImGui::BeginMenuBar()) {
            // Menu 1: Mapa
            if (ImGui::BeginMenu("Mapa")) {
                if (ImGui::MenuItem("Resetar Posição")) {
                    m_zoomScale = 0.5f;
                    findFirstAvailableTile();
                }

                ImGui::Separator();

                // Seletor de Mapa/Circuito
                ImGui::Text("Mapa:");
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::BeginCombo("##MapaSelector", stripMapExtension(m_currentMapName).c_str())) {
                    for (const auto& mapName : m_availableMaps) {
                        bool        isSelected = (m_currentMapName == mapName);
                        std::string cleanName  = stripMapExtension(mapName);
                        if (ImGui::Selectable(cleanName.c_str(), isSelected)) {
                            m_currentMapName     = mapName;
                            std::string fullPath = "maps/" + mapName;

                            clearCache();
                            if (m_db) {
                                sqlite3_close(m_db);
                                m_db = nullptr;
                            }
                            m_loaded = false;

                            int rc = sqlite3_open(fullPath.c_str(), &m_db);
                            if (rc == SQLITE_OK) {
                                m_zoomScale = 0.5f;
                                findFirstAvailableTile();
                            } else {
                                m_statusMessage =
                                    "Falha ao abrir banco: " + fullPath + " (Erro: " + std::to_string(rc) + ")";
                                m_db = nullptr;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                // Seletor de Zoom
                ImGui::Text("Zoom (Z):");
                ImGui::SetNextItemWidth(80.0f);
                std::string currentZoomStr = std::to_string(m_testZ);
                if (ImGui::BeginCombo("##ZoomSelector", currentZoomStr.c_str())) {
                    for (int z = 12; z <= 18; ++z) {
                        bool        isSelected = (m_testZ == z);
                        std::string zStr       = std::to_string(z);
                        if (ImGui::Selectable(zStr.c_str(), isSelected)) {
                            m_testZ = z;
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                // Slider de tamanho de blocos
                ImGui::Text("Tamanho Blocos:");
                ImGui::SetNextItemWidth(160.0f);
                ImGui::SliderFloat("##BlockScale", &m_zoomScale, 0.5f, 2.0f, "%.1fx");

                ImGui::EndMenu();
            }

            // Menu 2: Trajeto
            if (ImGui::BeginMenu("Trajeto")) {
                // Botão de centralizar no trajeto
                bool hasTrack = !m_selectedFileName.empty() && !m_selectedLatCol.empty() && !m_selectedLonCol.empty();
                if (hasTrack) {
                    if (ImGui::MenuItem("Centralizar no Trajeto")) {
                        centerOnTrack();
                    }
                } else {
                    ImGui::TextDisabled("(Arraste colunas Lat/Lon)");
                }

                ImGui::Separator();

                // Seleção da Fonte de Dados
                ImGui::Text("Fonte de Dados:");
                if (ImGui::RadioButton("CSV File", m_selectedFileType == "CSV")) {
                    m_selectedFileType = "CSV";
                    m_selectedFileName = "";
                    m_selectedLatCol   = "";
                    m_selectedLonCol   = "";
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Telemetria", m_selectedFileType == "Telemetry")) {
                    m_selectedFileType = "Telemetry";
                    m_selectedFileName = "";
                    m_selectedLatCol   = "";
                    m_selectedLonCol   = "";
                }

                ImGui::Separator();

                std::vector<std::string> availableFiles;
                std::vector<std::string> availableCols;

                if (m_selectedFileType == "CSV") {
                    const auto& csvFiles = DB::getInstance().getProject().getCSVFiles();
                    for (const auto& f : csvFiles) {
                        availableFiles.push_back(f.getName());
                    }
                    if (!availableFiles.empty()) {
                        if (m_selectedFileName.empty()) {
                            m_selectedFileName = availableFiles[0];
                        }
                        // Encontrar colunas do CSV
                        for (const auto& f : csvFiles) {
                            if (f.getName() == m_selectedFileName) {
                                availableCols = f.getColumnNames();
                                break;
                            }
                        }
                    }
                } else {
                    const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
                    for (const auto& f : telemetryFiles) {
                        availableFiles.push_back(f.getPacketId());
                    }
                    if (!availableFiles.empty()) {
                        if (m_selectedFileName.empty()) {
                            m_selectedFileName = availableFiles[0];
                        }
                        // Encontrar colunas de Telemetria
                        for (const auto& f : telemetryFiles) {
                            if (f.getPacketId() == m_selectedFileName) {
                                availableCols = f.getColumnNames();
                                break;
                            }
                        }
                    }
                }

                if (availableFiles.empty()) {
                    ImGui::TextDisabled("Nenhum dado disponível.");
                } else {
                    ImGui::Text("Arquivo/Pacote:");
                    ImGui::SetNextItemWidth(160.0f);
                    if (ImGui::BeginCombo("##FileSelector", m_selectedFileName.c_str())) {
                        for (const auto& f : availableFiles) {
                            bool isSelected = (m_selectedFileName == f);
                            if (ImGui::Selectable(f.c_str(), isSelected)) {
                                m_selectedFileName = f;
                                m_selectedLatCol   = "";
                                m_selectedLonCol   = "";
                            }
                        }
                        ImGui::EndCombo();
                    }

                    // Combo de Latitude
                    ImGui::Text("Latitude:");
                    ImGui::SetNextItemWidth(160.0f);
                    if (ImGui::BeginCombo("##LatCol", m_selectedLatCol.empty() ? "(Arraste ou Selecione)"
                                                                               : m_selectedLatCol.c_str())) {
                        for (const auto& col : availableCols) {
                            bool isSelected = (m_selectedLatCol == col);
                            if (ImGui::Selectable(col.c_str(), isSelected)) {
                                m_selectedLatCol = col;
                                centerOnTrack();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
                            m_selectedFileType                 = columnPayload->fileType;
                            m_selectedFileName                 = columnPayload->fileName;
                            m_selectedLatCol                   = columnPayload->columnName;
                            centerOnTrack();
                        }
                        ImGui::EndDragDropTarget();
                    }

                    // Combo de Longitude
                    ImGui::Text("Longitude:");
                    ImGui::SetNextItemWidth(160.0f);
                    if (ImGui::BeginCombo("##LonCol", m_selectedLonCol.empty() ? "(Arraste ou Selecione)"
                                                                               : m_selectedLonCol.c_str())) {
                        for (const auto& col : availableCols) {
                            bool isSelected = (m_selectedLonCol == col);
                            if (ImGui::Selectable(col.c_str(), isSelected)) {
                                m_selectedLonCol = col;
                                centerOnTrack();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
                            m_selectedFileType                 = columnPayload->fileType;
                            m_selectedFileName                 = columnPayload->fileName;
                            m_selectedLonCol                   = columnPayload->columnName;
                            centerOnTrack();
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::EndMenu();
            }

            // Menu 3: Estilo
            if (ImGui::BeginMenu("Estilo")) {
                ImGui::Text("Cores dos Símbolos:");
                ImGui::ColorEdit4("Linha", m_colorLine);
                ImGui::ColorEdit4("Pontos", m_colorPoint);
                ImGui::ColorEdit4("Último Ponto", m_colorLastPoint);
                ImGui::EndMenu();
            }

            // Menu 4: Calibração
            if (ImGui::BeginMenu("Calibração")) {
                ImGui::Checkbox("Ajustar Pista com Mouse", &m_moveTrackMode);
                if (m_moveTrackMode) {
                    ImGui::TextDisabled("(Use o mouse ou as setas)");
                    ImGui::Text("Offset Lat: %.6f", m_trackOffsetLat);
                    ImGui::Text("Offset Lon: %.6f", m_trackOffsetLon);
                    if (ImGui::Button("Zerar Ajuste")) {
                        m_trackOffsetLat = 0.0;
                        m_trackOffsetLon = 0.0;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // 2. Capturar cliques e arraste com o mouse em qualquer parte da janela
        ImGui::InvisibleButton("MapCanvas", windowSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float deltaX = ImGui::GetIO().MouseDelta.x;
            float deltaY = ImGui::GetIO().MouseDelta.y;

            if (m_moveTrackMode) {
                // Modo Calibração: Deslocar a pista
                double cx_offset = deltaX / tileSize;
                double cy_offset = deltaY / tileSize;

                double deltaLon   = cx_offset * 360.0 / (1 << m_testZ);
                m_trackOffsetLon += deltaLon;

                double latRad = m_centerLat * pi / 180.0;
                double cy_osm_ref =
                    (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / pi) / 2.0 * (1 << m_testZ);
                double cy_tms_ref = (1 << m_testZ) - 1.0 - cy_osm_ref;

                double cy_tms_new = cy_tms_ref - cy_offset;
                double cy_osm_new = (1 << m_testZ) - 1.0 - cy_tms_new;
                double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                double newLat     = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                m_trackOffsetLat += (newLat - m_centerLat);
            } else {
                // Modo Normal: Deslocar o mapa
                cx_osm -= deltaX / tileSize;
                cy_tms += deltaY / tileSize;

                m_centerLon       = cx_osm * 360.0 / (1 << m_testZ) - 180.0;
                double cy_osm_new = (1 << m_testZ) - 1.0 - cy_tms;
                double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                m_centerLat       = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                if (m_centerLon < -180.0)
                    m_centerLon = -180.0;
                if (m_centerLon > 180.0)
                    m_centerLon = 180.0;
                if (m_centerLat < -85.05112878)
                    m_centerLat = -85.05112878;
                if (m_centerLat > 85.05112878)
                    m_centerLat = 85.05112878;
            }
        }

        // Drag & Drop na tela principal do mapa
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
                m_selectedFileType                 = columnPayload->fileType;
                m_selectedFileName                 = columnPayload->fileName;

                std::string colName  = columnPayload->columnName;
                std::string colLower = colName;
                std::transform(colLower.begin(), colLower.end(), colLower.begin(), ::tolower);

                if (colLower.find("lat") != std::string::npos) {
                    m_selectedLatCol = colName;
                    LOG("INFO", "MBTiles: Latitude definida por Drag & Drop para '" + colName + "' (" +
                                    m_selectedFileType + ")");
                } else if (colLower.find("lon") != std::string::npos || colLower.find("lng") != std::string::npos) {
                    m_selectedLonCol = colName;
                    LOG("INFO", "MBTiles: Longitude definida por Drag & Drop para '" + colName + "' (" +
                                    m_selectedFileType + ")");
                } else {
                    if (m_selectedLatCol.empty()) {
                        m_selectedLatCol = colName;
                        LOG("INFO", "MBTiles: Latitude atribuída sequencialmente para '" + colName + "' (" +
                                        m_selectedFileType + ")");
                    } else {
                        m_selectedLonCol = colName;
                        LOG("INFO", "MBTiles: Longitude atribuída sequencialmente para '" + colName + "' (" +
                                        m_selectedFileType + ")");
                    }
                }
                centerOnTrack();
            }
            ImGui::EndDragDropTarget();
        }

        // 3. Capturar zoom com o Scroll do Mouse quando pairado sobre a janela (Limitado a Z de 12 a 18)
        if (ImGui::IsItemHovered()) {
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll > 0.0f && m_testZ < 18) {
                m_testZ++;
                m_statusMessage = "Zoom In (Z=" + std::to_string(m_testZ) + ")";
            } else if (scroll < 0.0f && m_testZ > 12) {
                m_testZ--;
                m_statusMessage = "Zoom Out (Z=" + std::to_string(m_testZ) + ")";
            }
        }

        // 4. Capturar comandos do teclado (movimentação por setas e escala por +/-)
        if (ImGui::IsWindowFocused()) {
            // Tecla "+" ou "=" (Aumenta escala dos blocos)
            if (ImGui::IsKeyDown(ImGuiKey_Equal) || ImGui::IsKeyDown(ImGuiKey_KeypadAdd)) {
                m_zoomScale = std::min(m_zoomScale + 0.01f, 2.0f);
            }
            // Tecla "-" (Diminui escala dos blocos)
            if (ImGui::IsKeyDown(ImGuiKey_Minus) || ImGui::IsKeyDown(ImGuiKey_KeypadSubtract)) {
                m_zoomScale = std::max(m_zoomScale - 0.01f, 0.5f);
            }

            // Movimentação suave contínua com todas as setas do teclado (Hold keys)
            float arrowPanSpeed = 10.0f;
            float dx            = 0.0f;
            float dy            = 0.0f;
            if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
                dy += arrowPanSpeed; // Move o mapa para baixo (Câmera vai para o Norte)
            }
            if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
                dy -= arrowPanSpeed; // Move o mapa para cima (Câmera vai para o Sul)
            }
            if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
                dx += arrowPanSpeed; // Move o mapa para a direita (Câmera vai para o Oeste)
            }
            if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
                dx -= arrowPanSpeed; // Move o mapa para a esquerda (Câmera vai para o Leste)
            }

            if (dx != 0.0f || dy != 0.0f) {
                if (m_moveTrackMode) {
                    // Mover os pontos da pista com teclado
                    double cx_offset = dx / tileSize;
                    double cy_offset = dy / tileSize;

                    double deltaLon   = cx_offset * 360.0 / (1 << m_testZ);
                    m_trackOffsetLon += deltaLon;

                    double latRad = m_centerLat * pi / 180.0;
                    double cy_osm_ref =
                        (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / pi) / 2.0 * (1 << m_testZ);
                    double cy_tms_ref = (1 << m_testZ) - 1.0 - cy_osm_ref;

                    double cy_tms_new = cy_tms_ref - cy_offset;
                    double cy_osm_new = (1 << m_testZ) - 1.0 - cy_tms_new;
                    double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                    double newLat     = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                    m_trackOffsetLat += (newLat - m_centerLat);
                } else {
                    // Modo Normal: Deslocar o mapa
                    cx_osm -= dx / tileSize;
                    cy_tms += dy / tileSize;

                    m_centerLon       = cx_osm * 360.0 / (1 << m_testZ) - 180.0;
                    double cy_osm_new = (1 << m_testZ) - 1.0 - cy_tms;
                    double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                    m_centerLat       = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                    if (m_centerLon < -180.0)
                        m_centerLon = -180.0;
                    if (m_centerLon > 180.0)
                        m_centerLon = 180.0;
                    if (m_centerLat < -85.05112878)
                        m_centerLat = -85.05112878;
                    if (m_centerLat > 85.05112878)
                        m_centerLat = 85.05112878;
                }
            }
        }

        // 6. Determinar dinamicamente a grade de tiles necessária para cobrir 100% da janela
        int halfTilesX = static_cast<int>(std::ceil(windowSize.x * 0.5f / tileSize)) + 1;
        int halfTilesY = static_cast<int>(std::ceil(windowSize.y * 0.5f / tileSize)) + 1;

        ImVec2 centerScreen(windowPos.x + windowSize.x * 0.5f + m_panX, windowPos.y + windowSize.y * 0.5f + m_panY);
        ImVec2 centerTileTopLeft(centerScreen.x - tileSize * 0.5f, centerScreen.y - tileSize * 0.5f);

        // Varredura da grade dinâmica calculada para preenchimento total
        for (int dy = halfTilesY; dy >= -halfTilesY; --dy) {
            for (int dx = -halfTilesX; dx <= halfTilesX; ++dx) {
                int targetX = m_testX + dx;
                int targetY = m_testY + dy;

                SDL_Texture* tex = getTileTexture(m_testZ, targetX, targetY);

                float tileX = centerTileTopLeft.x + dx * tileSize;
                float tileY =
                    centerTileTopLeft.y - dy * tileSize; // Y geográfico aumenta para cima, tela aumenta para baixo

                ImVec2 p_min(tileX, tileY);
                ImVec2 p_max(tileX + tileSize, tileY + tileSize);

                if (tex) {
                    drawList->AddImage(reinterpret_cast<ImTextureID>(tex), p_min, p_max);
                } else {
                    // Preenchimento preto sólido para áreas fora de cobertura geocartográfica
                    drawList->AddRectFilled(p_min, p_max, IM_COL32(0, 0, 0, 255));
                }
            }
        }

        // 6.5 Renderizar os pontos da reconstrução de pista (traçado do circuito) se selecionados
        if (!m_selectedFileName.empty() && !m_selectedLatCol.empty() && !m_selectedLonCol.empty()) {
            try {
                const std::vector<double>* latDataPtr = nullptr;
                const std::vector<double>* lonDataPtr = nullptr;

                if (m_selectedFileType == "CSV") {
                    latDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLatCol);
                    lonDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLonCol);
                } else if (m_selectedFileType == "Telemetry") {
                    latDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLatCol);
                    lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLonCol);
                }

                if (latDataPtr && lonDataPtr) {
                    const auto& latData = *latDataPtr;
                    const auto& lonData = *lonDataPtr;

                    size_t numPoints = std::min(latData.size(), lonData.size());
                    if (numPoints > 0) {
                        std::vector<ImVec2> screenPoints;
                        screenPoints.reserve(numPoints);

                        for (size_t i = 0; i < numPoints; ++i) {
                            double lat = latData[i] + m_trackOffsetLat;
                            double lon = lonData[i] + m_trackOffsetLon;

                            ImVec2 sPos = getScreenPosFromLatLon(lat, lon, m_centerLat, m_centerLon, m_testZ, tileSize,
                                                                 windowPos, windowSize);
                            screenPoints.push_back(sPos);
                        }

                        // Desenhar a linha conectando a pista (neon verde premium ou customizado)
                        ImU32 colorLine =
                            ImGui::GetColorU32(ImVec4(m_colorLine[0], m_colorLine[1], m_colorLine[2], m_colorLine[3]));
                        for (size_t i = 0; i < numPoints - 1; ++i) {
                            drawList->AddLine(screenPoints[i], screenPoints[i + 1], colorLine, 3.0f);
                        }

                        // Desenhar os pontos (scatter plot) - Amarelo para os normais
                        ImU32 colorPoint = ImGui::GetColorU32(
                            ImVec4(m_colorPoint[0], m_colorPoint[1], m_colorPoint[2], m_colorPoint[3]));
                        ImU32 colorLastPoint = ImGui::GetColorU32(
                            ImVec4(m_colorLastPoint[0], m_colorLastPoint[1], m_colorLastPoint[2], m_colorLastPoint[3]));
                        for (size_t i = 0; i < numPoints; ++i) {
                            if (i == numPoints - 1) {
                                // Último ponto: Vermelho vibrante maior para destacar com auréola branca
                                drawList->AddCircleFilled(screenPoints[i], 6.0f, colorLastPoint);
                                drawList->AddCircle(screenPoints[i], 8.0f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
                            } else {
                                drawList->AddCircleFilled(screenPoints[i], 3.5f, colorPoint);
                            }
                        }
                    }
                }
            } catch (...) {
                // Silenciar erros de leitura de colunas
            }
        }

        // 7. Barra de Status no Rodapé (Bottom Bar) para Coordenadas
        double lastLat   = 0.0;
        double lastLon   = 0.0;
        bool   hasPoints = false;
        if (!m_selectedFileName.empty() && !m_selectedLatCol.empty() && !m_selectedLonCol.empty()) {
            try {
                const std::vector<double>* latDataPtr = nullptr;
                const std::vector<double>* lonDataPtr = nullptr;
                if (m_selectedFileType == "CSV") {
                    latDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLatCol);
                    lonDataPtr = &DB::getInstance().getCSVData(m_selectedFileName, m_selectedLonCol);
                } else if (m_selectedFileType == "Telemetry") {
                    latDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLatCol);
                    lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedFileName, m_selectedLonCol);
                }
                if (latDataPtr && lonDataPtr && !latDataPtr->empty() && !lonDataPtr->empty()) {
                    size_t idx = std::min(latDataPtr->size(), lonDataPtr->size()) - 1;
                    lastLat    = latDataPtr->at(idx);
                    lastLon    = lonDataPtr->at(idx);
                    hasPoints  = true;
                }
            } catch (...) {
            }
        }

        float bottomBarHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPos(ImVec2(0.0f, windowSize.y - bottomBarHeight));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));

        if (ImGui::BeginChild("##BottomBar", ImVec2(windowSize.x, bottomBarHeight), false,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (m_moveTrackMode) {
                ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f),
                                   "[AJUSTE DE PISTA ATIVO (Arraste com o mouse/setas)]");
                ImGui::SameLine();
                ImGui::TextUnformatted(" | ");
                ImGui::SameLine();
            }
            if (hasPoints) {
                ImGui::Text("Lat: %.6f | Lon: %.6f", lastLat, lastLon);
            } else {
                ImGui::TextUnformatted("Lat: --- | Lon: ---");
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    } else {
        ImGui::TextDisabled("Nenhum mapa disponível.");
    }

    ImGui::End();
}