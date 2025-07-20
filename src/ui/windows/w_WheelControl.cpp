#include "ui/windows/w_WheelControl.hpp"

// Função auxiliar para desenhar a imagem rotacionada
void DrawRotatedImage(ImTextureID texture, const ImVec2& pos, float size, float angleDeg) {
    float  halfSize = size * 0.5f;
    ImVec2 center   = ImVec2(pos.x + halfSize, pos.y + halfSize);

    float angleRad = angleDeg * (3.14159265f / 180.0f);
    float cosA     = cosf(angleRad);
    float sinA     = sinf(angleRad);

    ImVec2 topLeft     = ImVec2(-halfSize, -halfSize);
    ImVec2 topRight    = ImVec2(halfSize, -halfSize);
    ImVec2 bottomRight = ImVec2(halfSize, halfSize);
    ImVec2 bottomLeft  = ImVec2(-halfSize, halfSize);

    ImVec2 p1 = ImVec2(center.x + topLeft.x * cosA - topLeft.y * sinA, center.y + topLeft.x * sinA + topLeft.y * cosA);
    ImVec2 p2 =
        ImVec2(center.x + topRight.x * cosA - topRight.y * sinA, center.y + topRight.x * sinA + topRight.y * cosA);
    ImVec2 p3 = ImVec2(center.x + bottomRight.x * cosA - bottomRight.y * sinA,
                       center.y + bottomRight.x * sinA + bottomRight.y * cosA);
    ImVec2 p4 = ImVec2(center.x + bottomLeft.x * cosA - bottomLeft.y * sinA,
                       center.y + bottomLeft.x * sinA + bottomLeft.y * cosA);

    ImVec2 uv0 = ImVec2(0.0f, 0.0f);
    ImVec2 uv1 = ImVec2(1.0f, 0.0f);
    ImVec2 uv2 = ImVec2(1.0f, 1.0f);
    ImVec2 uv3 = ImVec2(0.0f, 1.0f);

    ImGui::GetWindowDrawList()->AddImageQuad(texture, p1, p2, p3, p4, uv0, uv1, uv2, uv3, IM_COL32_WHITE);
}

Window::WheelControl::WheelControl(bool* isOpen) : IWindow(isOpen) {
    this->title = "Controle do Volante";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::WheelControl::render() {
    if (!this->isOpen || !*this->isOpen) return;

    // Variáveis de estado da UI e do volante
    static float anguloVolante = 0.0f;
    static float sensibilidade = 1.0f;
    static int   anguloMaximo  = 900;
    static bool  forceFeedback = true;
    static bool  showSettings  = false;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // --- Lógica de avanço do tempo (só funciona se os dados estiverem carregados) ---
    if (m_isPlaying && isLoaded()) {
        m_currentTime += ImGui::GetIO().DeltaTime * m_playbackSpeed;
        if (m_currentTime > getDuration()) {
            m_currentTime = getDuration();
            m_isPlaying = false;
        }
    }

    // --- Seção de Drag-and-Drop e Dados Carregados ---
    ImGui::BeginChild("DataArea", ImVec2(0, 50), false);
    if (ImGui::CollapsingHeader("Fonte de Dados do Volante")) {
        if (!isLoaded()) {
            ImGui::TextDisabled("Arraste uma coluna de volante aqui.");
        } else {
            ImGui::PushID(0);
            if (ImGui::Button("X")) {
                removeColumn(m_steerIndex);
            } else {
                ImGui::SameLine();
                ImGui::Text("%s: %s", m_dataList[m_steerIndex].archive.c_str(), m_dataList[m_steerIndex].column.c_str());
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    processColumnDragDrop(); // Permite dropar na área acima

    // --- Lógica de cálculo do ângulo do volante ---
    if (isLoaded()) {
    // MODO PLAYBACK: Usa os dados do arquivo
    size_t time_idx = static_cast<size_t>(m_currentTime);
    const auto& wheelData = m_dataList[m_steerIndex];
    
    if (time_idx < wheelData.data.size()) {
        double rawValue = wheelData.data[time_idx];

        // --- LÓGICA CONDICIONAL ---
        if (m_dataIsDegrees) {
            // Se os dados já estão em graus, usamos o valor diretamente.
            anguloVolante = static_cast<float>(rawValue);
        } else {
            // Se forem dados brutos, usamos a normalização 
            double range = wheelData.maxValue - wheelData.minValue;
            double normalizedValue = 0.0;
            if (range > 0) {
                normalizedValue = 2.0 * ((rawValue - wheelData.minValue) / range) - 1.0;
            }
            anguloVolante = static_cast<float>(normalizedValue * (anguloMaximo / 2.0));
        }
    }
} else {
    // MODO TEMPO REAL: Usa o teclado 
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    if (keystates[SDL_SCANCODE_LEFT]) anguloVolante -= 5.0f * sensibilidade;
    if (keystates[SDL_SCANCODE_RIGHT]) anguloVolante += 5.0f * sensibilidade;
}

    // Limita o ângulo do volante
    anguloVolante = std::clamp(anguloVolante, -anguloMaximo / 2.0f, anguloMaximo / 2.0f);

    // --- Desenho do Volante (código original) ---
    SDL_Texture* volanteTexture = AssetManager::getInstance().getTexture(VOLANTE_PATH);
    if (volanteTexture) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float size = std::min(avail.x, avail.y) * 0.9f;
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        cursorPos.x += (avail.x - size) * 0.5f;
        cursorPos.y += (avail.y - size) * 0.5f;

        DrawRotatedImage((ImTextureID)volanteTexture, cursorPos, size, anguloVolante);
        ImGui::Dummy(ImVec2(avail.x, size)); 
    }

    // Botão e janela de configurações
    if (ImGui::Button("Configurações")) showSettings = !showSettings;
    if (showSettings) {
        ImGui::Begin("Configurações do Volante", &showSettings, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SliderFloat("Sensibilidade (Teclado)", &sensibilidade, 0.1f, 5.0f, "%.1f");
        ImGui::SliderInt("Ângulo Máximo", &anguloMaximo, 90, 1080);
        ImGui::SliderFloat("Velocidade Playback", &m_playbackSpeed, 1.0f, 240.0f, "%.1f dados/s");

        ImGui::Separator();
        ImGui::Checkbox("Dados da coluna já estão em Graus", &m_dataIsDegrees);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Marque esta opção se o arquivo CSV já contém o ãngulo do volant em graus (ex: -450 a 450). \nDesmarque se forem dados brutos de um sensor (ex: 0 a 1023).");
        }
        ImGui::Separator();
        ImGui::Text("Ângulo Atual: %.1f°", anguloVolante);
        ImGui::End();
    }

    ImGui::End();
}

// --- Implementação da Interface IPlayable para w_WheelControl ---
void Window::WheelControl::play() { m_isPlaying = true; }
void Window::WheelControl::pause() { m_isPlaying = false; }

void Window::WheelControl::seek(double position) {
    if (isLoaded()) {
        m_currentTime = std::max(0.0, std::min(position, getDuration()));
    }
}

bool Window::WheelControl::isPlaying() const { return m_isPlaying; }
bool Window::WheelControl::isLoaded() const { return m_steerIndex != -1; }
double Window::WheelControl::getCurrentTime() const { return m_currentTime; }

double Window::WheelControl::getDuration() const {
    if (isLoaded()) {
        return static_cast<double>(m_dataList[m_steerIndex].data.size() - 1);
    }
    return 0.0;
}

const char* Window::WheelControl::getTitle() const { return this->title.c_str(); }
float Window::WheelControl::getStepSize() const { return m_stepSize; }
void Window::WheelControl::setStepSize(float size) { m_stepSize = size; }

void Window::WheelControl::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);

            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::WheelControl::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    // Verifica se a coluna já não foi adicionada
    for (const auto& data : m_dataList) {
        if (data.archive == fileName && data.column == columnName) {
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já foi adicionada ao Volante.");
            return;
        }
    }
    
    WheelData wd;
    wd.archive = fileName;
    wd.column = columnName;

    // Lógica para carregar dados de diferentes fontes
    if (fileType == "CSV") {
        wd.data = DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        wd.data = DB::getInstance().getTelemetryData(fileName, columnName);
    }

    if (wd.data.empty()) {
        LOG("ERROR", "Não foi possível carregar os dados para a coluna " + columnName);
        return;
    }

    auto minmax = std::minmax_element(wd.data.begin(), wd.data.end());
    if (minmax.first != wd.data.end()) {
        wd.minValue = *minmax.first;
        wd.maxValue = *minmax.second;
    }

    m_dataList.push_back(wd);
    int newIndex = m_dataList.size() - 1;

    std::string lowerColumn = columnName;
    std::transform(lowerColumn.begin(), lowerColumn.end(), lowerColumn.begin(), ::tolower);

    if (lowerColumn.find("steer") != std::string::npos || 
        lowerColumn.find("steering") != std::string::npos ||
        lowerColumn.find("volante") != std::string::npos) {
        m_steerIndex = newIndex;
        LOG("INFO", "Coluna de Volante (" + fileType + ") adicionada.");
    }
}

void Window::WheelControl::removeColumn(int index) {
    if (index >= m_dataList.size()) return;
    if (index == m_steerIndex) m_steerIndex = -1;
    m_dataList.erase(m_dataList.begin() + index);
    if (m_steerIndex > index) m_steerIndex--;
}
