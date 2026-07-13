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

static std::vector<double> alignThirdVector(size_t targetSize, const std::vector<double>& srcData,
                                            XYAlignmentMode mode) {
    if (srcData.empty())
        return std::vector<double>(targetSize, 0.0);
    std::vector<double> result(targetSize);
    size_t              Ns = srcData.size();
    if (Ns == 1) {
        std::fill(result.begin(), result.end(), srcData[0]);
        return result;
    }
    if (targetSize <= 1) {
        if (targetSize == 1)
            result[0] = srcData[0];
        return result;
    }

    if (mode == ALIGN_MIN_SIZE) {
        size_t safeSize = std::min(targetSize, Ns);
        for (size_t i = 0; i < targetSize; ++i) {
            if (i < safeSize) {
                result[i] = srcData[i];
            } else {
                result[i] = srcData.back();
            }
        }
        return result;
    }

    for (size_t i = 0; i < targetSize; ++i) {
        double t   = static_cast<double>(i) / (targetSize - 1);
        double f_j = t * (Ns - 1);
        if (mode == ALIGN_TIME_PROXIMITY) {
            size_t j = static_cast<size_t>(std::round(f_j));
            if (j >= Ns)
                j = Ns - 1;
            result[i] = srcData[j];
        } else {
            // Linear interpolation
            size_t j_low  = static_cast<size_t>(std::floor(f_j));
            size_t j_high = static_cast<size_t>(std::ceil(f_j));
            if (j_low >= Ns)
                j_low = Ns - 1;
            if (j_high >= Ns)
                j_high = Ns - 1;
            if (j_low == j_high) {
                result[i] = srcData[j_low];
            } else {
                double weight = f_j - j_low;
                result[i]     = srcData[j_low] * (1.0 - weight) + srcData[j_high] * weight;
            }
        }
    }
    return result;
}

Window::Reconstruction::Reconstruction(bool* isOpen) : IWindow(isOpen) {
    this->title = "Reconstrução de Pista";
    this->flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;

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
            double cy_osm = (1 << m_testZ) - cy_tms;
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
    double y_tms  = (1 << z) - y_osm;

    double cx_osm  = (centerLon + 180.0) / 360.0 * (1 << z);
    double clatRad = centerLat * pi / 180.0;
    double cy_osm  = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << z);
    double cy_tms  = (1 << z) - cy_osm;

    ImVec2 centerScreen(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

    float screenX = centerScreen.x + (static_cast<float>(x_osm - cx_osm)) * tileSize;
    float screenY = centerScreen.y - (static_cast<float>(y_tms - cy_tms)) * tileSize;

    return ImVec2(screenX, screenY);
}

void Window::Reconstruction::centerOnTrack() {
    if (m_selectedLatFileName.empty() || m_selectedLatCol.empty() || m_selectedLonFileName.empty() ||
        m_selectedLonCol.empty())
        return;

    if (!DB::getInstance().columnExists(m_selectedLatFileType, m_selectedLatFileName, m_selectedLatCol) ||
        !DB::getInstance().columnExists(m_selectedLonFileType, m_selectedLonFileName, m_selectedLonCol))
        return;

    try {
        const std::vector<double>* latDataPtr = nullptr;
        const std::vector<double>* lonDataPtr = nullptr;

        if (m_selectedLatFileType == "CSV") {
            latDataPtr = &DB::getInstance().getCSVData(m_selectedLatFileName, m_selectedLatCol);
        } else if (m_selectedLatFileType == "Telemetry") {
            latDataPtr = &DB::getInstance().getTelemetryData(m_selectedLatFileName, m_selectedLatCol);
        }

        if (m_selectedLonFileType == "CSV") {
            lonDataPtr = &DB::getInstance().getCSVData(m_selectedLonFileName, m_selectedLonCol);
        } else if (m_selectedLonFileType == "Telemetry") {
            lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedLonFileName, m_selectedLonCol);
        }

        if (!latDataPtr || !lonDataPtr)
            return;

        auto        aligned = alignVectors(*latDataPtr, *lonDataPtr, m_alignmentMode);
        const auto& latData = aligned.first;
        const auto& lonData = aligned.second;

        if (latData.empty() || lonData.empty())
            return;

        size_t count = std::min(latData.size(), lonData.size());

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

void Window::Reconstruction::autoFitColorLimits() {
    if (m_selectedColorCol.empty())
        return;
    try {
        const std::vector<double>* colorDataPtr = nullptr;
        std::string colFile = m_selectedColorFileName.empty() ? m_selectedLatFileName : m_selectedColorFileName;
        std::string colType = m_selectedColorFileType.empty() ? m_selectedLatFileType : m_selectedColorFileType;
        if (colType == "CSV") {
            colorDataPtr = &DB::getInstance().getCSVData(colFile, m_selectedColorCol);
        } else if (colType == "Telemetry") {
            colorDataPtr = &DB::getInstance().getTelemetryData(colFile, m_selectedColorCol);
        }
        if (colorDataPtr && !colorDataPtr->empty()) {
            auto minmax  = std::minmax_element(colorDataPtr->begin(), colorDataPtr->end());
            m_gradMinVal = *minmax.first;
            m_gradMaxVal = *minmax.second;
            if (std::abs(m_gradMaxVal - m_gradMinVal) < 1e-5) {
                m_gradMaxVal = m_gradMinVal + 1.0;
            }
            LOG("INFO", "Gradiente auto-ajustado: Min=" + std::to_string(m_gradMinVal) +
                            ", Max=" + std::to_string(m_gradMaxVal));
        }
    } catch (...) {
    }
}

void Window::Reconstruction::render() {
    if (!isOpen || !*isOpen) {
        m_wasOpen = false;
        return;
    }

    // Define margem interna zero para um canvas geográfico contínuo premium
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    bool window_is_expanded =
        ImGui::Begin(this->title.c_str(), this->isOpen,
                     ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (window_is_expanded) {
        if (m_loaded) {
            if (m_autoFitGradient && !m_selectedColorCol.empty() && !m_wasOpen) {
                autoFitColorLimits();
            }
            m_wasOpen = true;

            ImVec2      windowPos  = ImGui::GetWindowPos();
            ImVec2      windowSize = ImGui::GetWindowSize();
            ImDrawList* drawList   = ImGui::GetWindowDrawList();

            // Calcular dinamicamente a posição do bloco (testX, testY) e pan da tela a partir da câmera
            // georreferenciada contínua
            double pi      = 3.14159265358979323846;
            double cx_osm  = (m_centerLon + 180.0) / 360.0 * (1 << m_testZ);
            double clatRad = m_centerLat * pi / 180.0;
            double cy_osm  = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << m_testZ);
            double cy_tms  = (1 << m_testZ) - cy_osm;

            float tileSize = 256.0f * m_zoomScale;

            m_testX = static_cast<int>(std::floor(cx_osm));
            m_testY = static_cast<int>(std::floor(cy_tms));

            m_panX = -static_cast<float>(cx_osm - (m_testX + 0.5)) * tileSize;
            m_panY = static_cast<float>(cy_tms - (m_testY + 0.5)) * tileSize;

            // 1. Adicionar o MenuBar com todas as informações e seletores de Mapa

            // 2. Capturar cliques e arraste com o mouse em qualquer parte da janela
            ImGui::InvisibleButton("MapCanvas", windowSize);
            ImGui::SetItemAllowOverlap();
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
                    double cy_tms_ref = (1 << m_testZ) - cy_osm_ref;

                    double cy_tms_new = cy_tms_ref - cy_offset;
                    double cy_osm_new = (1 << m_testZ) - cy_tms_new;
                    double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                    double newLat     = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                    m_trackOffsetLat += (newLat - m_centerLat);
                } else {
                    // Modo Normal: Deslocar o mapa
                    cx_osm -= deltaX / tileSize;
                    cy_tms += deltaY / tileSize;

                    m_centerLon       = cx_osm * 360.0 / (1 << m_testZ) - 180.0;
                    double cy_osm_new = (1 << m_testZ) - cy_tms;
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

                    if (std::string(columnPayload->fileType) == "Text") {
                        TrackTextAnnotation ann;
                        ann.archiveName = columnPayload->fileName;
                        ann.columnName  = columnPayload->columnName;

                        bool exists = false;
                        for (const auto& a : m_textAnnotations) {
                            if (a.archiveName == ann.archiveName && a.columnName == ann.columnName) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            m_textAnnotations.push_back(ann);
                            LOG("INFO", "Anotação de texto '" + ann.columnName + "' de '" + ann.archiveName +
                                            "' adicionada à reconstrução de pista.");
                        }
                    } else {
                        std::string colName = columnPayload->columnName;

                        if (m_selectedLatCol.empty()) {
                            m_selectedLatFileType = columnPayload->fileType;
                            m_selectedLatFileName = columnPayload->fileName;
                            m_selectedLatCol      = colName;
                            LOG("INFO", "MBTiles: Latitude definida por Drag & Drop para '" + colName + "' (" +
                                            m_selectedLatFileType + ")");
                            centerOnTrack();
                        } else if (m_selectedLonCol.empty()) {
                            m_selectedLonFileType = columnPayload->fileType;
                            m_selectedLonFileName = columnPayload->fileName;
                            m_selectedLonCol      = colName;
                            LOG("INFO", "MBTiles: Longitude definida por Drag & Drop para '" + colName + "' (" +
                                            m_selectedLonFileType + ")");
                            centerOnTrack();
                        } else {
                            m_selectedColorCol      = colName;
                            m_selectedColorFileName = columnPayload->fileName;
                            m_selectedColorFileType = columnPayload->fileType;
                            LOG("INFO", "MBTiles: Coluna de Gradiente definida por Drag & Drop para '" + colName +
                                            "' (" + m_selectedColorFileType + ")");
                            autoFitColorLimits();
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            /*
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
            */

            if (ImGui::IsItemHovered()) {
                float scroll = ImGui::GetIO().MouseWheel;
                if (scroll > 0.0f) {
                    m_zoomScale += 0.05f;
                } else if (scroll < 0.0f) {
                    m_zoomScale -= 0.05f;
                }
                // Ensure zoom scale stays within reasonable bounds
                if (m_zoomScale < 0.1f)
                    m_zoomScale = 0.1f;
            }

            // 4. Capturar comandos do teclado (movimentação por setas e escala por +/-)
            if (ImGui::IsWindowFocused()) {
                // Tecla "+" ou "=" (Aumenta escala dos blocos)
                if (ImGui::IsKeyDown(ImGuiKey_Equal) || ImGui::IsKeyDown(ImGuiKey_KeypadAdd)) {
                    m_zoomScale += 0.01f;
                }
                // Tecla "-" (Diminui escala dos blocos)
                if (ImGui::IsKeyDown(ImGuiKey_Minus) || ImGui::IsKeyDown(ImGuiKey_KeypadSubtract)) {
                    m_zoomScale -= 0.01f;
                }
                // Garantir que a escala não fique zero ou negativa (evita divisão por zero)
                if (m_zoomScale < 0.1f)
                    m_zoomScale = 0.1f;

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
                        double cy_tms_ref = (1 << m_testZ) - cy_osm_ref;

                        double cy_tms_new = cy_tms_ref - cy_offset;
                        double cy_osm_new = (1 << m_testZ) - cy_tms_new;
                        double v          = pi * (1.0 - 2.0 * cy_osm_new / (1 << m_testZ));
                        double newLat     = (2.0 * std::atan(std::exp(v)) - pi / 2.0) * 180.0 / pi;

                        m_trackOffsetLat += (newLat - m_centerLat);
                    } else {
                        // Modo Normal: Deslocar o mapa
                        cx_osm -= dx / tileSize;
                        cy_tms += dy / tileSize;

                        m_centerLon       = cx_osm * 360.0 / (1 << m_testZ) - 180.0;
                        double cy_osm_new = (1 << m_testZ) - cy_tms;
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

            // 5. Transição automática de nível de zoom baseada na escala dos blocos (Zoom Infinito Contínuo e
            // Responsivo)
            if (m_zoomScale > 2.0f) {
                if (m_testZ < 18) {
                    m_testZ++;
                    m_zoomScale     /= 2.0f;
                    m_statusMessage  = "Zoom Automático In (Z=" + std::to_string(m_testZ) + ")";
                } else {
                    m_zoomScale = 2.0f;
                }
            } else if (m_zoomScale < 1.0f) {
                if (m_testZ > 12) {
                    m_testZ--;
                    m_zoomScale     *= 2.0f;
                    m_statusMessage  = "Zoom Automático Out (Z=" + std::to_string(m_testZ) + ")";
                } else {
                    if (m_zoomScale < 0.5f) {
                        m_zoomScale = 0.5f;
                    }
                }
            }

            // Recalcular parâmetros georreferenciados para garantir alinhamento perfeito na mesma frame
            cx_osm   = (m_centerLon + 180.0) / 360.0 * (1 << m_testZ);
            clatRad  = m_centerLat * pi / 180.0;
            cy_osm   = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << m_testZ);
            cy_tms   = (1 << m_testZ) - cy_osm;
            tileSize = 256.0f * m_zoomScale;
            m_testX  = static_cast<int>(std::floor(cx_osm));
            m_testY  = static_cast<int>(std::floor(cy_tms));
            m_panX   = -static_cast<float>(cx_osm - (m_testX + 0.5)) * tileSize;
            m_panY   = static_cast<float>(cy_tms - (m_testY + 0.5)) * tileSize;

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
            if (!m_selectedLatFileName.empty() && !m_selectedLatCol.empty() && !m_selectedLonFileName.empty() &&
                !m_selectedLonCol.empty() &&
                DB::getInstance().columnExists(m_selectedLatFileType, m_selectedLatFileName, m_selectedLatCol) &&
                DB::getInstance().columnExists(m_selectedLonFileType, m_selectedLonFileName, m_selectedLonCol)) {
                try {
                    const std::vector<double>* latDataPtr   = nullptr;
                    const std::vector<double>* lonDataPtr   = nullptr;
                    const std::vector<double>* colorDataPtr = nullptr;

                    if (m_selectedLatFileType == "CSV") {
                        latDataPtr = &DB::getInstance().getCSVData(m_selectedLatFileName, m_selectedLatCol);
                    } else if (m_selectedLatFileType == "Telemetry") {
                        latDataPtr = &DB::getInstance().getTelemetryData(m_selectedLatFileName, m_selectedLatCol);
                    }

                    if (m_selectedLonFileType == "CSV") {
                        lonDataPtr = &DB::getInstance().getCSVData(m_selectedLonFileName, m_selectedLonCol);
                    } else if (m_selectedLonFileType == "Telemetry") {
                        lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedLonFileName, m_selectedLonCol);
                    }

                    if (!m_selectedColorCol.empty()) {
                        std::string colFile =
                            m_selectedColorFileName.empty() ? m_selectedLatFileName : m_selectedColorFileName;
                        std::string colType =
                            m_selectedColorFileType.empty() ? m_selectedLatFileType : m_selectedColorFileType;
                        if (colType == "CSV") {
                            colorDataPtr = &DB::getInstance().getCSVData(colFile, m_selectedColorCol);
                        } else if (colType == "Telemetry") {
                            colorDataPtr = &DB::getInstance().getTelemetryData(colFile, m_selectedColorCol);
                        }
                    }

                    if (latDataPtr && lonDataPtr) {
                        auto        aligned = alignVectors(*latDataPtr, *lonDataPtr, m_alignmentMode);
                        const auto& latData = aligned.first;
                        const auto& lonData = aligned.second;

                        if (!latData.empty() && !lonData.empty()) {
                            size_t numPoints = std::min(latData.size(), lonData.size());
                            size_t startIdx = 0;
                            size_t endIdx = numPoints;
                            if (m_followTheEnd) {
                                startIdx = (numPoints > (size_t)m_numPointsToShow) ? numPoints - m_numPointsToShow : 0;
                            }

                            double pi = 3.14159265358979323846;
                            double cx_osm = (m_centerLon + 180.0) / 360.0 * (1 << m_testZ);
                            double clatRad = m_centerLat * pi / 180.0;
                            double cy_osm_center = (1.0 - std::log(std::tan(clatRad) + 1.0 / std::cos(clatRad)) / pi) / 2.0 * (1 << m_testZ);
                            double cy_tms_center = (1 << m_testZ) - cy_osm_center;
                            ImVec2 centerScreen(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);

                            std::vector<std::pair<size_t, ImVec2>> linePoints;
                            std::vector<std::pair<size_t, ImVec2>> drawablePoints;
                            linePoints.reserve(endIdx - startIdx);
                            drawablePoints.reserve(endIdx - startIdx);

                            ImVec2 lastDrawnPos(-10000.0f, -10000.0f);
                            float minX = windowPos.x - 50.0f;
                            float maxX = windowPos.x + windowSize.x + 50.0f;
                            float minY = windowPos.y - 50.0f;
                            float maxY = windowPos.y + windowSize.y + 50.0f;

                            double powZoom = (1 << m_testZ);

                            for (size_t i = startIdx; i < endIdx; ++i) {
                                double lat = latData[i] + m_trackOffsetLat;
                                double lon = lonData[i] + m_trackOffsetLon;

                                double x_osm = (lon + 180.0) / 360.0 * powZoom;
                                double latRad = lat * pi / 180.0;
                                double y_osm  = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / pi) / 2.0 * powZoom;
                                double y_tms  = powZoom - y_osm;

                                float screenX = centerScreen.x + static_cast<float>(x_osm - cx_osm) * tileSize;
                                float screenY = centerScreen.y - static_cast<float>(y_tms - cy_tms_center) * tileSize;
                                ImVec2 sPos(screenX, screenY);

                                float distSq = (sPos.x - lastDrawnPos.x) * (sPos.x - lastDrawnPos.x) + (sPos.y - lastDrawnPos.y) * (sPos.y - lastDrawnPos.y);

                                if (distSq > 4.0f || i == numPoints - 1) {
                                    linePoints.push_back({i, sPos});
                                    bool isInside = (sPos.x >= minX && sPos.x <= maxX && sPos.y >= minY && sPos.y <= maxY);
                                    if (isInside || i == numPoints - 1) {
                                        drawablePoints.push_back({i, sPos});
                                    }
                                    lastDrawnPos = sPos;
                                }
                            }

                            // Align the color data to match the coordinate size and index mapping
                            std::vector<double> alignedColor;
                            bool                useGradient = (colorDataPtr != nullptr && !colorDataPtr->empty());
                            if (useGradient) {
                                alignedColor = alignThirdVector(numPoints, *colorDataPtr, m_colorAlignmentMode);
                            }

                            // Desenhar a linha conectando a pista (neon verde premium ou customizado)
                            ImU32 colorLine = ImGui::GetColorU32(
                                ImVec4(m_colorLine[0], m_colorLine[1], m_colorLine[2], m_colorLine[3]));
                            
                            ImU32 colorPoint = ImGui::GetColorU32(
                                ImVec4(m_colorPoint[0], m_colorPoint[1], m_colorPoint[2], m_colorPoint[3]));
                            ImU32 colorLastPoint = ImGui::GetColorU32(
                                ImVec4(m_colorLastPoint[0], m_colorLastPoint[1], m_colorLastPoint[2], m_colorLastPoint[3]));

                            auto getColorForIndex = [&](size_t idx) -> ImU32 {
                                if (idx == numPoints - 1) return colorLastPoint;
                                if (!useGradient) return colorPoint;
                                double computedVal = alignedColor[idx];
                                double t           = 0.0;
                                if (m_gradMaxVal > m_gradMinVal) {
                                    t = (computedVal - m_gradMinVal) / (m_gradMaxVal - m_gradMinVal);
                                    if (t < 0.0) t = 0.0;
                                    if (t > 1.0) t = 1.0;
                                }
                                if (m_colorMode == 1) {
                                    double sampleT = m_reverseColormap ? (1.0 - t) : t;
                                    ImVec4 col     = ImPlot::SampleColormap((float)sampleT, m_colormap);
                                    return ImGui::ColorConvertFloat4ToU32(col);
                                } else {
                                    float r = m_gradMinColor[0] * (1.0f - t) + m_gradMaxColor[0] * t;
                                    float g = m_gradMinColor[1] * (1.0f - t) + m_gradMaxColor[1] * t;
                                    float b = m_gradMinColor[2] * (1.0f - t) + m_gradMaxColor[2] * t;
                                    float a = m_gradMinColor[3] * (1.0f - t) + m_gradMaxColor[3] * t;
                                    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
                                }
                            };

                            if (!linePoints.empty()) {
                                if (!useGradient) {
                                    std::vector<ImVec2> rawPoints;
                                    rawPoints.reserve(linePoints.size());
                                    for (const auto& p : linePoints) rawPoints.push_back(p.second);
                                    drawList->AddPolyline(rawPoints.data(), rawPoints.size(), colorLine, 0, 3.0f);
                                } else {
                                    for (size_t k = 0; k < linePoints.size() - 1; ++k) {
                                        ImU32 segmentColor = getColorForIndex(linePoints[k].first);
                                        drawList->AddLine(linePoints[k].second, linePoints[k+1].second, segmentColor, 3.0f);
                                    }
                                }
                            }

                            // Desenhar os pontos (scatter plot) - Amarelo para os normais ou com gradiente dinâmico
                            for (const auto& dp : drawablePoints) {
                                size_t i = dp.first;
                                ImVec2 sPos = dp.second;

                                if (i == numPoints - 1) {
                                    // Último ponto: Vermelho vibrante maior para destacar com auréola branca
                                    drawList->AddCircleFilled(sPos, 6.0f, colorLastPoint);
                                    drawList->AddCircle(sPos, 8.0f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
                                } else {
                                    ImU32 ptColor = getColorForIndex(i);
                                    drawList->AddCircleFilled(sPos, 3.5f, ptColor);
                                }
                            }

                            // Renderizar anotações textuais georreferenciadas sincronizadas pelo tempo mais próximo
                            if (!m_textAnnotations.empty()) {
                                const std::vector<double>* trackDates = nullptr;
                                if (m_selectedLatFileType == "Telemetry") {
                                    for (const auto& tf : DB::getInstance().getProject().getTelemetryFiles()) {
                                        if (tf.getPacketId() == m_selectedLatFileName) {
                                            trackDates = &tf.getNumericDate();
                                            break;
                                        }
                                    }
                                }

                                if (trackDates && !trackDates->empty()) {
                                    int annotationCount = 0;
                                    for (const auto& ann : m_textAnnotations) {
                                        const TextFile* targetTF = nullptr;
                                        for (const auto& tf : DB::getInstance().getProject().getTextFiles()) {
                                            if (tf.getName() == ann.archiveName) {
                                                targetTF = &tf;
                                                break;
                                            }
                                        }

                                        if (targetTF) {
                                            const auto& dates = targetTF->getNumericDates();
                                            const auto& data  = targetTF->getData();

                                            int colIdx = -1;
                                            for (size_t c = 0; c < targetTF->getColumnNames().size(); ++c) {
                                                if (targetTF->getColumnNames()[c] == ann.columnName) {
                                                    colIdx = static_cast<int>(c);
                                                    break;
                                                }
                                            }

                                            if (colIdx != -1 && colIdx < static_cast<int>(data.size())) {
                                                const auto& colData = data[colIdx];

                                                for (size_t row = 0; row < dates.size(); ++row) {
                                                    if (row < colData.size() && !colData[row].empty()) {
                                                        std::string text = colData[row];
                                                        try {
                                                            double commentTime = dates[row];

                                                            // Achar o índice numérico mais próximo no traçado do GPS
                                                            int    closestIdx = -1;
                                                            double minDiff    = std::numeric_limits<double>::max();
                                                            for (size_t i = 0; i < trackDates->size(); ++i) {
                                                                double t    = (*trackDates)[i];
                                                                double diff = std::abs(t - commentTime);
                                                                if (diff < minDiff) {
                                                                    minDiff    = diff;
                                                                    closestIdx = static_cast<int>(i);
                                                                }
                                                            }

                                                            if (closestIdx != -1 && closestIdx < static_cast<int>(latData.size()) && closestIdx < static_cast<int>(lonData.size())) {
                                                                double lat_closest = latData[closestIdx] + m_trackOffsetLat;
                                                                double lon_closest = lonData[closestIdx] + m_trackOffsetLon;
                                                                ImVec2 p_track = getScreenPosFromLatLon(lat_closest, lon_closest, m_centerLat, m_centerLon, m_testZ, tileSize, windowPos, windowSize);

                                                                std::string key = ann.archiveName + "|" +
                                                                                  ann.columnName + "|" +
                                                                                  std::to_string(row);
                                                                ImVec2      currentOffset(
                                                                    40.0f, -30.0f - (annotationCount % 4) * 28.0f);
                                                                if (m_textOffsets.find(key) != m_textOffsets.end()) {
                                                                    currentOffset = m_textOffsets[key];
                                                                }

                                                                ImVec2 p_text(p_track.x + currentOffset.x,
                                                                              p_track.y + currentOffset.y);
                                                                annotationCount++;

                                                                ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
                                                                ImVec2 boxMin(p_text.x - 6.0f, p_text.y - 4.0f);
                                                                ImVec2 boxMax(p_text.x + textSize.x + 6.0f,
                                                                              p_text.y + textSize.y + 4.0f);

                                                                ImGui::SetCursorScreenPos(boxMin);
                                                                ImGui::InvisibleButton(
                                                                    key.c_str(),
                                                                    ImVec2(boxMax.x - boxMin.x, boxMax.y - boxMin.y));
                                                                bool isHovered = ImGui::IsItemHovered();
                                                                bool isActive  = ImGui::IsItemActive();

                                                                if (isActive) {
                                                                    currentOffset.x    += ImGui::GetIO().MouseDelta.x;
                                                                    currentOffset.y    += ImGui::GetIO().MouseDelta.y;
                                                                    m_textOffsets[key]  = currentOffset;
                                                                    p_text.x           += ImGui::GetIO().MouseDelta.x;
                                                                    p_text.y           += ImGui::GetIO().MouseDelta.y;
                                                                    boxMin = ImVec2(p_text.x - 6.0f, p_text.y - 4.0f);
                                                                    boxMax = ImVec2(p_text.x + textSize.x + 6.0f,
                                                                                    p_text.y + textSize.y + 4.0f);
                                                                }

                                                                // Linha conectora amarela
                                                                drawList->AddLine(
                                                                    p_track,
                                                                    ImVec2(boxMin.x, (boxMin.y + boxMax.y) * 0.5f),
                                                                    IM_COL32(255, 255, 0, 180), 1.5f);

                                                                // Fundo escuro premium semi-transparente
                                                                drawList->AddRectFilled(
                                                                    boxMin, boxMax,
                                                                    isActive ? IM_COL32(40, 40, 40, 240)
                                                                             : (isHovered ? IM_COL32(30, 30, 30, 230)
                                                                                          : IM_COL32(15, 15, 15, 220)),
                                                                    4.0f);
                                                                // Borda amarela premium para destacar o comentário
                                                                drawList->AddRect(boxMin, boxMax,
                                                                                  IM_COL32(255, 255, 0, 255), 4.0f, 0,
                                                                                  isActive ? 2.0f : 1.2f);
                                                                // Texto em amarelo brilhante
                                                                drawList->AddText(p_text, IM_COL32(255, 255, 0, 255),
                                                                                  text.c_str());
                                                            }
                                                        } catch (...) {
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
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
            if (!m_selectedLatFileName.empty() && !m_selectedLatCol.empty() && !m_selectedLonFileName.empty() &&
                !m_selectedLonCol.empty() &&
                DB::getInstance().columnExists(m_selectedLatFileType, m_selectedLatFileName, m_selectedLatCol) &&
                DB::getInstance().columnExists(m_selectedLonFileType, m_selectedLonFileName, m_selectedLonCol)) {
                try {
                    const std::vector<double>* latDataPtr = nullptr;
                    const std::vector<double>* lonDataPtr = nullptr;

                    if (m_selectedLatFileType == "CSV") {
                        latDataPtr = &DB::getInstance().getCSVData(m_selectedLatFileName, m_selectedLatCol);
                    } else if (m_selectedLatFileType == "Telemetry") {
                        latDataPtr = &DB::getInstance().getTelemetryData(m_selectedLatFileName, m_selectedLatCol);
                    }

                    if (m_selectedLonFileType == "CSV") {
                        lonDataPtr = &DB::getInstance().getCSVData(m_selectedLonFileName, m_selectedLonCol);
                    } else if (m_selectedLonFileType == "Telemetry") {
                        lonDataPtr = &DB::getInstance().getTelemetryData(m_selectedLonFileName, m_selectedLonCol);
                    }
                    if (latDataPtr && lonDataPtr && !latDataPtr->empty() && !lonDataPtr->empty()) {
                        auto aligned = alignVectors(*latDataPtr, *lonDataPtr, m_alignmentMode);
                        if (!aligned.first.empty()) {
                            size_t idx = aligned.first.size() - 1;
                            lastLat    = aligned.first.at(idx);
                            lastLon    = aligned.second.at(idx);
                            hasPoints  = true;
                        }
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
    }

            if (ImGui::BeginMenuBar()) {
                this->drawMenuBar();
                ImGui::EndMenuBar();
            }
    ImGui::End();
}
void Window::Reconstruction::drawMenuBar() {
    
                if (ImGui::BeginMenu("Configurações")) {
                if (ImGui::BeginMenu("Mapa")) {
                    if (ImGui::MenuItem("Resetar Posição")) {
                        m_zoomScale = 0.5f;
                        findFirstAvailableTile();
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Configurações do Mapa")) {
                        // Seletor de Mapa/Circuito
                        ImGui::Text("Mapa de Fundo:");
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
                        ImGui::Text("Nível de Zoom (Z):");
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
                        ImGui::Text("Escala Visual Blocos:");
                        ImGui::SetNextItemWidth(160.0f);
                        ImGui::SliderFloat("##BlockScale", &m_zoomScale, 0.5f, 2.0f, "%.1fx");

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Estilo")) {
                    ImGui::Text("Cores dos Símbolos:");
                    ImGui::ColorEdit4("Linha", m_colorLine);
                    ImGui::ColorEdit4("Pontos", m_colorPoint);
                    ImGui::ColorEdit4("Último Ponto", m_colorLastPoint);

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Gradiente de Cores (Pontos)")) {
                        ImGui::Text("Arraste a coluna de gradiente para o campo abaixo:");
                        ImGui::Spacing();

                        // --- COLUNA DE GRADIENTE ---
                        std::string colorLabel = m_selectedColorCol.empty() ? "(Nenhuma - Arraste aqui)##ColorButton"
                                                                            : (m_selectedColorCol + "##ColorButton");

                        ImGui::PushStyleColor(ImGuiCol_Button, m_selectedColorCol.empty()
                                                                   ? ImVec4(0.2f, 0.2f, 0.2f, 0.4f)
                                                                   : ImVec4(0.1f, 0.35f, 0.45f, 0.6f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_selectedColorCol.empty()
                                                                          ? ImVec4(0.3f, 0.3f, 0.3f, 0.5f)
                                                                          : ImVec4(0.15f, 0.45f, 0.55f, 0.7f));
                        ImGui::Button(colorLabel.c_str(), ImVec2(200.0f, 0.0f));
                        ImGui::PopStyleColor(2);

                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                                const ColumnPayload* columnPayload =
                                    reinterpret_cast<const ColumnPayload*>(payload->Data);
                                m_selectedColorCol      = columnPayload->columnName;
                                m_selectedColorFileName = columnPayload->fileName;
                                m_selectedColorFileType = columnPayload->fileType;
                                autoFitColorLimits();
                            }
                            ImGui::EndDragDropTarget();
                        }

                        if (!m_selectedColorCol.empty()) {
                            ImGui::SameLine();
                            if (ImGui::Button("X##ClearColor")) {
                                m_selectedColorCol      = "";
                                m_selectedColorFileName = "";
                                m_selectedColorFileType = "";
                            }
                        }

                        if (!m_selectedColorCol.empty()) {
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::TextDisabled("Origem do Gradiente:");
                            std::string originColorText = m_selectedColorFileName;
                            originColorText += (m_selectedColorFileType == "CSV") ? " (CSV)" : " (Telemetria)";
                            ImGui::TextWrapped("%s", originColorText.c_str());
                            ImGui::Separator();
                            ImGui::Text("Estilo do Gradiente:");
                            ImGui::RadioButton("ImPlot Colormap", &m_colorMode, 1);
                            ImGui::SameLine();
                            ImGui::RadioButton("Manual", &m_colorMode, 2);

                            ImGui::Separator();
                            ImGui::Text("Limites do Gradiente:");
                            ImGui::PushItemWidth(140.0f);
                            ImGui::InputDouble("Mín##rec", &m_gradMinVal, 0.1, 1.0, "%.2f");
                            ImGui::InputDouble("Máx##rec", &m_gradMaxVal, 0.1, 1.0, "%.2f");

                            if (m_colorMode == 1) {
                                ImGui::Separator();
                                ImGui::Text("Mapa de Cores:");
                                if (ImPlot::ColormapButton(ImPlot::GetColormapName(m_colormap), ImVec2(200, 0),
                                                           m_colormap)) {
                                    m_colormap = (m_colormap + 1) % ImPlot::GetColormapCount();
                                    ImPlot::BustItemCache();
                                }
                                ImGui::SetNextItemWidth(200.0f);
                                ImPlotColormap prev_cmap    = ImPlot::GetStyle().Colormap;
                                ImPlot::GetStyle().Colormap = m_colormap;
                                if (ImPlot::ShowColormapSelector("##colormap_rec")) {
                                    m_colormap = ImPlot::GetStyle().Colormap;
                                    ImPlot::BustItemCache();
                                }
                                ImPlot::GetStyle().Colormap = prev_cmap;
                                ImGui::Checkbox("Inverter Cores", &m_reverseColormap);
                            } else {
                                ImGui::Separator();
                                ImGui::Text("Cores Manuais:");
                                ImGui::ColorEdit4("Cor Min##gradMinColor_rec", m_gradMinColor,
                                                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                                ImGui::ColorEdit4("Cor Max##gradMaxColor_rec", m_gradMaxColor,
                                                  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            }
                            ImGui::PopItemWidth();

                            if (ImGui::Button("Auto-ajustar Limites", ImVec2(200.0f, 0.0f))) {
                                autoFitColorLimits();
                            }
                            ImGui::Checkbox("Auto Atualizar Gradiente", &m_autoFitGradient);

                            ImGui::Separator();
                            ImGui::Text("Alinhamento do Gradiente:");
                            ImGui::SetNextItemWidth(200.0f);
                            const char* colorAlignmentModes[] = {"Tamanho Mínimo", "Valor Mais Próximo",
                                                                 "Interpolação Linear (Técnico)"};
                            int         currentColorMode      = static_cast<int>(m_colorAlignmentMode);
                            if (ImGui::Combo("##ColorAlignMode", &currentColorMode, colorAlignmentModes,
                                             IM_ARRAYSIZE(colorAlignmentModes))) {
                                m_colorAlignmentMode = static_cast<XYAlignmentMode>(currentColorMode);
                            }
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
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
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Dados")) {
                if (ImGui::BeginMenu("Trajeto")) {
                    // Botão de centralizar no trajeto
                    bool hasTrack = !m_selectedLatFileName.empty() && !m_selectedLatCol.empty() &&
                                    !m_selectedLonFileName.empty() && !m_selectedLonCol.empty();
                    if (hasTrack) {
                        if (ImGui::MenuItem("Centralizar no Trajeto")) {
                            centerOnTrack();
                        }
                    } else {
                        ImGui::TextDisabled("(Arraste colunas Lat/Lon)");
                    }

                    ImGui::Separator();

                    if (ImGui::BeginMenu("Dados da Trajetória")) {
                        ImGui::Text("Arraste colunas de coordenadas para os campos abaixo:");
                        ImGui::Spacing();

                        // --- LATITUDE ---
                        ImGui::Text("Latitude:");
                        std::string latLabel = m_selectedLatCol.empty() ? "(Nenhuma - Arraste aqui)##LatButton"
                                                                        : (m_selectedLatCol + "##LatButton");

                        ImGui::PushStyleColor(ImGuiCol_Button, m_selectedLatCol.empty()
                                                                   ? ImVec4(0.2f, 0.2f, 0.2f, 0.4f)
                                                                   : ImVec4(0.1f, 0.4f, 0.2f, 0.6f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_selectedLatCol.empty()
                                                                          ? ImVec4(0.3f, 0.3f, 0.3f, 0.5f)
                                                                          : ImVec4(0.15f, 0.5f, 0.25f, 0.7f));
                        ImGui::Button(latLabel.c_str(), ImVec2(200.0f, 0.0f));
                        ImGui::PopStyleColor(2);

                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                                const ColumnPayload* columnPayload =
                                    reinterpret_cast<const ColumnPayload*>(payload->Data);
                                m_selectedLatFileType = columnPayload->fileType;
                                m_selectedLatFileName = columnPayload->fileName;
                                m_selectedLatCol      = columnPayload->columnName;
                                centerOnTrack();
                            }
                            ImGui::EndDragDropTarget();
                        }

                        if (!m_selectedLatCol.empty()) {
                            ImGui::SameLine();
                            if (ImGui::Button("X##ClearLat")) {
                                m_selectedLatCol      = "";
                                m_selectedLatFileName = "";
                            }
                        }

                        ImGui::Spacing();

                        ImGui::Separator();
                        ImGui::Checkbox("Seguir o Final", &m_followTheEnd);
                        if (m_followTheEnd) {
                            ImGui::InputInt("Pontos", &m_numPointsToShow, 1, 10);
                        }
                        ImGui::Separator();

                        // --- LONGITUDE ---
                        ImGui::Text("Longitude:");
                        std::string lonLabel = m_selectedLonCol.empty() ? "(Nenhuma - Arraste aqui)##LonButton"
                                                                        : (m_selectedLonCol + "##LonButton");

                        ImGui::PushStyleColor(ImGuiCol_Button, m_selectedLonCol.empty()
                                                                   ? ImVec4(0.2f, 0.2f, 0.2f, 0.4f)
                                                                   : ImVec4(0.1f, 0.4f, 0.2f, 0.6f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_selectedLonCol.empty()
                                                                          ? ImVec4(0.3f, 0.3f, 0.3f, 0.5f)
                                                                          : ImVec4(0.15f, 0.5f, 0.25f, 0.7f));
                        ImGui::Button(lonLabel.c_str(), ImVec2(200.0f, 0.0f));
                        ImGui::PopStyleColor(2);

                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
                                const ColumnPayload* columnPayload =
                                    reinterpret_cast<const ColumnPayload*>(payload->Data);
                                m_selectedLonFileType = columnPayload->fileType;
                                m_selectedLonFileName = columnPayload->fileName;
                                m_selectedLonCol      = columnPayload->columnName;
                                centerOnTrack();
                            }
                            ImGui::EndDragDropTarget();
                        }

                        if (!m_selectedLonCol.empty()) {
                            ImGui::SameLine();
                            if (ImGui::Button("X##ClearLon")) {
                                m_selectedLonCol      = "";
                                m_selectedLonFileName = "";
                            }
                        }

                        if (!m_selectedLatCol.empty() || !m_selectedLonCol.empty()) {
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::TextDisabled("Origem dos dados:");
                            if (!m_selectedLatCol.empty()) {
                                std::string latOriginText =
                                    "Lat: " + m_selectedLatFileName +
                                    ((m_selectedLatFileType == "CSV") ? " (CSV)" : " (Telemetria)");
                                ImGui::TextWrapped("%s", latOriginText.c_str());
                            }
                            if (!m_selectedLonCol.empty()) {
                                std::string lonOriginText =
                                    "Lon: " + m_selectedLonFileName +
                                    ((m_selectedLonFileType == "CSV") ? " (CSV)" : " (Telemetria)");
                                ImGui::TextWrapped("%s", lonOriginText.c_str());
                            }
                        }

                        ImGui::Separator();
                        ImGui::Text("Alinhamento Lat/Lon:");
                        ImGui::SetNextItemWidth(200.0f);
                        const char* alignmentModes[] = {"Tamanho Mínimo", "Proximidade Temporal",
                                                        "Interpolação Linear (Técnico)"};
                        int         currentMode      = static_cast<int>(m_alignmentMode);
                        if (ImGui::Combo("##AlignMode", &currentMode, alignmentModes, IM_ARRAYSIZE(alignmentModes))) {
                            m_alignmentMode = static_cast<XYAlignmentMode>(currentMode);
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Anotações Textuais")) {
                    if (m_textAnnotations.empty()) {
                        ImGui::TextDisabled("Nenhuma anotação (Arraste colunas de Texto)");
                    } else {
                        if (ImGui::BeginTable("TabelaTextosRec", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
                            ImGui::TableSetupColumn("Remover", ImGuiTableColumnFlags_WidthFixed);
                            ImGui::TableSetupColumn("Anotação", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();
                            for (size_t i = 0; i < m_textAnnotations.size(); ++i) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                if (ImGui::Button(("X##txtRec" + std::to_string(i)).c_str())) {
                                    m_textAnnotations.erase(m_textAnnotations.begin() + i);
                                    break;
                                }
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(m_textAnnotations[i].columnName.c_str());
                            }
                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndMenu(); // Closes Anotações Textuais
                }
                ImGui::EndMenu(); // Closes Dados
            }
}
