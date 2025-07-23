#include "ui/windows/w_Pedal.hpp"

// Variáveis que simulam a pressão dos pedais (valores de 0 a 1)
static float throttleValue = 0.0f;
static float brakeValue    = 0.0f;

Window::Pedal::Pedal(bool* isOpen) : IWindow(isOpen) {
    this->title             = "Pedal";
    this->flags             = ImGuiWindowFlags_NoScrollbar;
    this->redPedalTexture   = (ImTextureID)AssetManager::getInstance().getTexture("assets/pedalvermelho.png");
    this->greenPedalTexture = (ImTextureID)AssetManager::getInstance().getTexture("assets/pedalverde.png");
}

void Window::Pedal::render() {
    if (!this->isOpen || !*this->isOpen) {
        return;
    }

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // --- Lógica de avanço automático do tempo ---
    if (m_isPlaying && isLoaded()) {
        m_currentTime += ImGui::GetIO().DeltaTime * m_playbackSpeed; 

        if (m_currentTime > getDuration()) {
            m_currentTime = getDuration();
            m_isPlaying = false; // Pausa ao chegar no final
        }
    }

    // --- Seção para gerenciar as colunas de dados ---
    if (ImGui::CollapsingHeader("Fontes de Dados dos Pedais")) {
        if (m_dataList.empty()) {
            ImGui::TextDisabled("Arraste colunas de acelerador e freio aqui.");
        } else {
            for (int i = 0; i < m_dataList.size(); ++i) {
                ImGui::PushID(i);
                if (ImGui::Button("X")) {
                    removeColumn(i);
                    ImGui::PopID();
                    break;
                }
                ImGui::SameLine();
                ImGui::Text("%s: %s", m_dataList[i].archive.c_str(), m_dataList[i].column.c_str());
                
                if (i == m_throttleIndex) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "(Acelerador)");
                }
                if (i == m_brakeIndex) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "(Freio)");
                }
                ImGui::PopID();
            }
        }
    }
    
    // --- Slider de controle de velocidade ---
    ImGui::Separator();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f); 
    ImGui::SliderFloat("Velocidade", &m_playbackSpeed, 1.0f, 240.0f, "%.1f dados/s");
    ImGui::PopItemWidth();
    ImGui::Separator();
    
    // --- Lógica de cálculo dos valores dos pedais ---
    float throttleValue = 0.0f;
    float brakeValue = 0.0f;

    if (isLoaded()) {
    size_t time_idx = static_cast<size_t>(m_currentTime);

    // --- NORMALIZAÇÃO DINÂMICA ---
    if (time_idx < m_dataList[m_throttleIndex].data->size()) {
        double rawThrottle = (*m_dataList[m_throttleIndex].data)[time_idx];
        double maxThrottle = m_dataList[m_throttleIndex].maxValue; // Pega o máximo específico do acelerador
        throttleValue = static_cast<float>(rawThrottle / maxThrottle);
    }
    if (time_idx < m_dataList[m_brakeIndex].data->size()) {
        double rawBrake = (*m_dataList[m_brakeIndex].data)[time_idx];
        double maxBrake = m_dataList[m_brakeIndex].maxValue; // Pega o máximo específico do freio
        brakeValue = static_cast<float>(rawBrake / maxBrake);
    }
        
        // Garante que os valores fiquem entre 0 e 1 para a visualização
        throttleValue = std::clamp(throttleValue, 0.0f, 1.0f);
        brakeValue = std::clamp(brakeValue, 0.0f, 1.0f);

    } else {
        // MODO TEMPO REAL (FALLBACK)
        if (ImGui::IsKeyDown(ImGuiKey_W)) { throttleValue = 1.0f; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { brakeValue = 1.0f; }
    }

    // --- Área de Desenho ---
    ImGui::BeginChild("PedalDrawingArea", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_NoScrollbar);
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float pedalWidth = avail.x * 0.35f;
    float pedalHeight = avail.y * 0.7f;
    ImVec2 pedalSize(pedalWidth, pedalHeight);
    float spacing = avail.x * 0.05f;

    ImVec2 startPos = ImGui::GetCursorScreenPos();

    // Desenha o pedal do Freio
    ImGui::BeginGroup();
    ImGui::Text("Freio");
    if (this->redPedalTexture) {
        ImGui::ImageButton("##Brake", this->redPedalTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           ImVec4(1.0f, 1.0f, 1.0f, brakeValue));
    }
    ImGui::EndGroup();

    ImGui::SameLine(0, spacing);

    // Desenha o pedal do Acelerador
    ImGui::BeginGroup();
    ImGui::Text("Acelerador");
    if (this->greenPedalTexture) {
        ImGui::ImageButton("##Throttle", this->greenPedalTexture, pedalSize, ImVec2(0, 0), ImVec2(1, 1),
                           ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           ImVec4(1.0f, 1.0f, 1.0f, throttleValue));
    }
    ImGui::EndGroup();

    // Configura e posiciona as barras de progresso
    float barWidth = avail.x * 0.05f;
    float barHeight = pedalHeight;
    ImVec2 barSize(barWidth, barHeight);
    ImVec2 brakeBarPos = ImVec2(startPos.x + avail.x * 0.8f + 10, startPos.y);
    ImVec2 accelBarPos = ImVec2(brakeBarPos.x + barWidth + spacing + 2, startPos.y);

    // Desenha a barra do Freio
    ImGui::GetWindowDrawList()->AddRectFilled(
        brakeBarPos, ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y), IM_COL32(50, 50, 50, 255));
    float fillHeightBrake = barSize.y * brakeValue;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y - fillHeightBrake),
                                              ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y),
                                              IM_COL32(255, 0, 0, 255));

    // Desenha a barra do Acelerador
    ImGui::GetWindowDrawList()->AddRectFilled(
        accelBarPos, ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y), IM_COL32(50, 50, 50, 255));
    float fillHeightThrottle = barSize.y * throttleValue;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y - fillHeightThrottle),
                                              ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y),
                                              IM_COL32(0, 200, 0, 255));

    // Exibe os valores percentuais abaixo das barras
    ImGui::SetCursorScreenPos(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y + 5));
    ImGui::Text("%.0f%%", brakeValue * 100);

    ImGui::SetCursorScreenPos(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y + 5));
    ImGui::Text("%.0f%%", throttleValue * 100);
    
    ImGui::EndChild();

    processColumnDragDrop();

    ImGui::End();
}

// --- Implementação da Interface IPlayable para w_Pedal ---

void Window::Pedal::play() {
    m_isPlaying = true;
}

void Window::Pedal::pause() {
    m_isPlaying = false;
}

void Window::Pedal::seek(double position) {
    if (isLoaded()) {
        m_currentTime = std::max(0.0, std::min(position, getDuration()));
    }
}

bool Window::Pedal::isPlaying() const {
    return m_isPlaying;
}

bool Window::Pedal::isLoaded() const {
    return m_throttleIndex != -1 && m_brakeIndex != -1;
}

double Window::Pedal::getCurrentTime() const {
    return m_currentTime;
}

double Window::Pedal::getDuration() const {
    if (m_throttleIndex != -1) {
        return static_cast<double>(m_dataList[m_throttleIndex].data->size() - 1);
    }
    return 0.0;
}

const char* Window::Pedal::getTitle() const {
    return this->title.c_str();
}

float Window::Pedal::getStepSize() const {
    return m_stepSize;
}

void Window::Pedal::setStepSize(float size) {
    m_stepSize = size;
}

// --- Implementação do Drag-and-Drop para w_Pedal ---

void Window::Pedal::processColumnDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);

            this->addColumn(columnPayload->fileType, columnPayload->fileName, columnPayload->columnName);
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Pedal::addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName) {
    // Verifica se a coluna já não foi adicionada
    for (const auto& data : m_dataList) {
        if (data.archive == fileName && data.column == columnName) {
            LOG("WARN", "A coluna " + columnName + " do arquivo " + fileName + " já foi adicionada ao Pedal.");
            return;
        }
    }

    PedalData pd;
    pd.archive = fileName;
    pd.column = columnName;
    
    // Lógica para carregar dados de diferentes fontes
    if (fileType == "CSV") {
        pd.data = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        pd.data = &DB::getInstance().getTelemetryData(fileName, columnName);
    }
    
    if (pd.data->empty()) {
        LOG("ERROR", "Não foi possível carregar os dados para a coluna " + columnName);
        return;
    }

    auto maxIt = std::max_element(pd.data->begin(), pd.data->end());
    if (maxIt != pd.data->end()) {
        pd.maxValue = *maxIt;
    }
    if (pd.maxValue == 0) {
        pd.maxValue = 1.0;
    }

    m_dataList.push_back(pd);

    int newIndex = m_dataList.size() - 1;
    std::string lowerColumn = columnName;
    std::transform(lowerColumn.begin(), lowerColumn.end(), lowerColumn.begin(), ::tolower);

    if (lowerColumn.find("acelerador") != std::string::npos || lowerColumn.find("throttle") != std::string::npos) {
        m_throttleIndex = newIndex;
        LOG("INFO", "Coluna de Acelerador (" + fileType + ") adicionada à janela de Pedal.");
    } else if (lowerColumn.find("freio") != std::string::npos || lowerColumn.find("brake") != std::string::npos) {
        m_brakeIndex = newIndex;
        LOG("INFO", "Coluna de Freio (" + fileType + ") adicionada à janela de Pedal.");
    }
}

void Window::Pedal::removeColumn(int index) {
    if (index >= m_dataList.size()) return;

    if (index == m_throttleIndex) m_throttleIndex = -1;
    if (index == m_brakeIndex) m_brakeIndex = -1;

    m_dataList.erase(m_dataList.begin() + index);

    // Reajustar índices
    if (m_throttleIndex > index) m_throttleIndex--;
    if (m_brakeIndex > index) m_brakeIndex--;
}
