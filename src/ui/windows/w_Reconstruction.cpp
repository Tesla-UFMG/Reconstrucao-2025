#include "ui/windows/w_Reconstruction.hpp"
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
    this->title = "Reconstrução de Pista (Grade MBTiles)";
    
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
    int rc = sqlite3_open(path.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        m_statusMessage = "Falha ao abrir banco: " + path + " (Erro: " + std::to_string(rc) + ")";
        m_db = nullptr;
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

bool Window::Reconstruction::isLoaded() const {
    return m_loaded;
}

void Window::Reconstruction::scanAvailableMaps() {
    m_availableMaps.clear();
    const std::string mapsFolder = "maps";
    
    try {
        if (std::filesystem::exists(mapsFolder) && std::filesystem::is_directory(mapsFolder)) {
            for (const auto& entry : std::filesystem::directory_iterator(mapsFolder)) {
                if (entry.is_regular_file() && (entry.path().extension() == ".mbtiles" || entry.path().extension() == ".mptiles")) {
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
    if (!m_db) return;

    // Busca o centro geográfico do mapa no nível de zoom mais próximo de 18
    const char* sql = "SELECT zoom_level, MIN(tile_column), MAX(tile_column), MIN(tile_row), MAX(tile_row) "
                      "FROM tiles WHERE zoom_level = (SELECT zoom_level FROM tiles ORDER BY abs(zoom_level - 18) ASC LIMIT 1);";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            m_testZ = sqlite3_column_int(stmt, 0);
            int minX = sqlite3_column_int(stmt, 1);
            int maxX = sqlite3_column_int(stmt, 2);
            int minY = sqlite3_column_int(stmt, 3);
            int maxY = sqlite3_column_int(stmt, 4);

            m_testX = minX + (maxX - minX) / 2;
            m_testY = minY + (maxY - minY) / 2;
            m_loaded = true;
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
    if (!m_db) return nullptr;

    // 1. Buscar no cache de texturas
    for (const auto& cached : m_tileCache) {
        if (cached.z == z && cached.x == x && cached.y == y) {
            return cached.texture;
        }
    }

    // 2. Não encontrado no cache, buscar no SQLite
    const char* sql = "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }

    sqlite3_bind_int(stmt, 1, z);
    sqlite3_bind_int(stmt, 2, x);
    sqlite3_bind_int(stmt, 3, y);

    SDL_Texture* texture = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);

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
    cached.z = z;
    cached.x = x;
    cached.y = y;
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

void Window::Reconstruction::render() {
    if (!isOpen || !*isOpen) return;

    // Define margem interna zero para um canvas geográfico contínuo premium
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(this->title.c_str(), this->isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (m_loaded) {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 1. Adicionar o MenuBar com todas as informações e seletores de Mapa
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Opções")) {
                // 1. Botão de reset
                if (ImGui::MenuItem("Resetar Posição")) {
                    m_panX = 0.0f;
                    m_panY = 0.0f;
                    m_zoomScale = 0.5f;
                    findFirstAvailableTile();
                }

                ImGui::Separator();

                // 2. Seletor de Mapa/Circuito
                ImGui::Text("Mapa:");
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::BeginCombo("##MapaSelector", stripMapExtension(m_currentMapName).c_str())) {
                    for (const auto& mapName : m_availableMaps) {
                        bool isSelected = (m_currentMapName == mapName);
                        std::string cleanName = stripMapExtension(mapName);
                        if (ImGui::Selectable(cleanName.c_str(), isSelected)) {
                            m_currentMapName = mapName;
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
                                m_panX = 0.0f;
                                m_panY = 0.0f;
                                findFirstAvailableTile();
                            } else {
                                m_statusMessage = "Falha ao abrir banco: " + fullPath + " (Erro: " + std::to_string(rc) + ")";
                                m_db = nullptr;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                // 3. Seletor de Zoom
                ImGui::Text("Zoom (Z):");
                ImGui::SetNextItemWidth(80.0f);
                std::string currentZoomStr = std::to_string(m_testZ);
                if (ImGui::BeginCombo("##ZoomSelector", currentZoomStr.c_str())) {
                    for (int z = 12; z <= 18; ++z) {
                        bool isSelected = (m_testZ == z);
                        std::string zStr = std::to_string(z);
                        if (ImGui::Selectable(zStr.c_str(), isSelected)) {
                            if (z != m_testZ) {
                                float ratio = std::pow(2.0f, static_cast<float>(z - m_testZ));
                                m_testX = static_cast<int>(std::round(m_testX * ratio));
                                m_testY = static_cast<int>(std::round(m_testY * ratio));
                                m_panX *= ratio;
                                m_panY *= ratio;
                                m_testZ = z;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Separator();

                // 4. Slider de tamanho de blocos
                ImGui::Text("Tamanho Blocos:");
                ImGui::SetNextItemWidth(160.0f);
                ImGui::SliderFloat("##BlockScale", &m_zoomScale, 0.5f, 2.0f, "%.1fx");

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // 2. Capturar cliques e arraste com o mouse em qualquer parte da janela
        ImGui::InvisibleButton("MapCanvas", windowSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            m_panX += ImGui::GetIO().MouseDelta.x;
            m_panY += ImGui::GetIO().MouseDelta.y;
        }

        // 3. Capturar zoom com o Scroll do Mouse quando pairado sobre a janela (Limitado a Z de 12 a 18)
        if (ImGui::IsItemHovered()) {
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll > 0.0f && m_testZ < 18) {
                m_testZ++;
                m_testX *= 2;
                m_testY *= 2;
                m_panX *= 2.0f;
                m_panY *= 2.0f; // Mantém a visualização centrada no zoom!
                m_statusMessage = "Zoom In (Z=" + std::to_string(m_testZ) + ")";
            } else if (scroll < 0.0f && m_testZ > 12) {
                m_testZ--;
                m_testX /= 2;
                m_testY /= 2;
                m_panX /= 2.0f;
                m_panY /= 2.0f; // Mantém a visualização centrada no zoom!
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
            if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
                m_panY += arrowPanSpeed; // Move o mapa para baixo (Câmera vai para o Norte)
            }
            if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
                m_panY -= arrowPanSpeed; // Move o mapa para cima (Câmera vai para o Sul)
            }
            if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
                m_panX += arrowPanSpeed; // Move o mapa para a direita (Câmera vai para o Oeste)
            }
            if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
                m_panX -= arrowPanSpeed; // Move o mapa para a esquerda (Câmera vai para o Leste)
            }
        }

        // 5. Atualizar índices geográficos com base no arraste infinito (Corrigido o Y georreferenciado)
        float tileSize = 256.0f * m_zoomScale;
        while (m_panX > tileSize) {
            m_testX -= 1;
            m_panX -= tileSize;
        }
        while (m_panX < -tileSize) {
            m_testX += 1;
            m_panX += tileSize;
        }
        while (m_panY > tileSize) {
            m_testY += 1; // Corrigido: arrastar para baixo (m_panY aumenta) expõe blocos ao Norte (Y maior)
            m_panY -= tileSize;
        }
        while (m_panY < -tileSize) {
            m_testY -= 1; // Corrigido: arrastar para cima (m_panY diminui) expõe blocos ao Sul (Y menor)
            m_panY += tileSize;
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
                float tileY = centerTileTopLeft.y - dy * tileSize; // Y geográfico aumenta para cima, tela aumenta para baixo

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

        // 7. Barra de Status no Rodapé (Bottom Bar) para Coordenadas
        float bottomBarHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPos(ImVec2(0.0f, windowSize.y - bottomBarHeight));
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
        
        if (ImGui::BeginChild("##BottomBar", ImVec2(windowSize.x, bottomBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::Text("X: %d | Y: %d", m_testX, m_testY);
        }
        ImGui::EndChild();
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    } else {
        ImGui::TextDisabled("Nenhum mapa disponível.");
    }

    ImGui::End();
}