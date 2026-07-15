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

    // Variáveis de estado do volante
    static float anguloVolante = 0.0f;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // --- Definir a área inteira da janela como Drag and Drop Target (Estratégia idêntica à janela Statistics) ---
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    // --- Menu de contexto no clique com o botão direito ---
    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::BeginMenu("Dados & Exibição")) {
            if (!isLoaded()) {
                ImGui::TextDisabled("(Nenhum dado carregado)");
            } else {
                ImGui::Text("Colunas Carregadas:");
                ImGui::PushID(0);
                std::string label = m_dataList[m_steerIndex].archive + ": " + m_dataList[m_steerIndex].column;
                ImGui::TextUnformatted(label.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("X##removeData")) {
                    removeColumn(m_steerIndex);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
                
                ImGui::Separator();
                if (ImGui::Button("Limpar Todas as Colunas")) {
                    removeColumn(m_steerIndex);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    // --- Lógica de cálculo do ângulo do volante baseada no último valor do vetor continuamente ---
    if (isLoaded()) {
        const auto& wheelData = m_dataList[m_steerIndex];
        const std::vector<double>* data = nullptr;
        if (wheelData.fileType == "CSV") {
            data = &DB::getInstance().getCSVData(wheelData.archive, wheelData.column);
        } else if (wheelData.fileType == "Telemetry") {
            data = &DB::getInstance().getTelemetryData(wheelData.archive, wheelData.column);
        }

        if (data && !data->empty()) {
            double rawValue = data->back();
            anguloVolante = static_cast<float>(rawValue);
        }
    } else {
        anguloVolante = 0.0f;
    }

    // --- Desenho do Volante ---
    SDL_Texture* volanteTexture = AssetManager::getInstance().getTexture(VOLANTE_PATH);
    if (volanteTexture) {
        ImVec2 startCursorPos = ImGui::GetCursorPos();
        float textHeight = ImGui::GetTextLineHeightWithSpacing();
        float size = std::min(avail.x, avail.y - textHeight - 15.0f) * 0.9f;
        if (size < 0) size = 0;

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float startX = cursorPos.x + (avail.x - size) * 0.5f;
        float startY = cursorPos.y + (avail.y - textHeight - size) * 0.5f;

        DrawRotatedImage((ImTextureID)volanteTexture, ImVec2(startX, startY), size, anguloVolante);
        
        // Formatar apenas o valor numérico com o símbolo "°"
        double valorExibido = 0.0;
        if (isLoaded()) {
            const auto& wheelData = m_dataList[m_steerIndex];
            const std::vector<double>* data = nullptr;
            if (wheelData.fileType == "CSV") {
                data = &DB::getInstance().getCSVData(wheelData.archive, wheelData.column);
            } else if (wheelData.fileType == "Telemetry") {
                data = &DB::getInstance().getTelemetryData(wheelData.archive, wheelData.column);
            }
            if (data && !data->empty()) {
                valorExibido = data->back();
            }
        }
        char formattedText[32];
        snprintf(formattedText, sizeof(formattedText), "%.2f°", valorExibido);

        // Obter tamanho do texto para centralizar
        float textWidth = ImGui::CalcTextSize(formattedText).x;

        // Calcular posições locais e setar cursor localmente para desenhar o texto
        float localStartY = startCursorPos.y + (avail.y - textHeight - size) * 0.5f;
        float localTextX = startCursorPos.x + (avail.x - textWidth) * 0.5f;
        float localTextY = localStartY + size + 8.0f;

        ImGui::SetCursorPos(ImVec2(localTextX, localTextY));
        ImGui::TextUnformatted(formattedText);

        // Resetar o cursor da janela para o final da janela
        ImGui::SetCursorPos(ImVec2(startCursorPos.x, startCursorPos.y + avail.y));
    }

    ImGui::End();
}

bool Window::WheelControl::isLoaded() const { return m_steerIndex != -1; }

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
    if (fileType == "Text") {
        return;
    }

    m_dataList.clear();
    m_steerIndex = -1;

    WheelData wd;
    wd.archive = fileName;
    wd.column = columnName;
    wd.fileType = fileType;

    if (fileType == "CSV") {
        const auto& vec = DB::getInstance().getCSVData(fileName, columnName);
        if (!vec.empty()) {
            auto minmax = std::minmax_element(vec.begin(), vec.end());
            wd.minValue = *minmax.first;
            wd.maxValue = *minmax.second;
        }
    } else if (fileType == "Telemetry") {
        const auto& vec = DB::getInstance().getTelemetryData(fileName, columnName);
        if (!vec.empty()) {
            auto minmax = std::minmax_element(vec.begin(), vec.end());
            wd.minValue = *minmax.first;
            wd.maxValue = *minmax.second;
        }
    } else {
        LOG("ERROR", "[Volante] Falha ao carregar dados da coluna '" + columnName + "' do arquivo '" + fileName + "'.");
        return;
    }

    m_dataList.push_back(wd);
    m_steerIndex = 0; // Vincula imediatamente como a coluna ativa do volante

    LOG("INFO", "[Volante] Dados carregados com sucesso! Coluna: '" + columnName + "' (" + fileType + ") de '" + fileName + "'.");
}

void Window::WheelControl::removeColumn(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_dataList.size()) return;
    if (index == m_steerIndex) m_steerIndex = -1;
    m_dataList.erase(m_dataList.begin() + index);
    if (m_steerIndex > index) m_steerIndex--;
}
