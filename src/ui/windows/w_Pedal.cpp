#include "ui/windows/w_Pedal.hpp"

Window::Pedal::Pedal(bool* isOpen) : IWindow(isOpen) {
    this->title             = "Pedal";
    this->flags             = ImGuiWindowFlags_NoScrollbar;
}

void Window::Pedal::render() {
    if (!this->isOpen || !*this->isOpen) {
        return;
    }

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // --- Definir a área inteira da janela como Drag and Drop Target ---
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::Dummy(avail);
    processColumnDragDrop();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    // --- Menu de contexto no clique com o botão direito ---
    if (ImGui::BeginPopupContextWindow()) {
        ImGui::Text("Dados Selecionados:");
        ImGui::Separator();
        
        bool anyData = false;
        if (m_throttleIndex != -1) {
            anyData = true;
            ImGui::PushID(100);
            if (ImGui::Button("Remover Acelerador (X)")) {
                removeColumn(m_throttleIndex);
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::SameLine();
                std::string label = "Acelerador: " + m_dataList[m_throttleIndex].archive + " (" + m_dataList[m_throttleIndex].column + ")";
                ImGui::TextUnformatted(label.c_str());
            }
            ImGui::PopID();
        }
        if (m_brakeIndex != -1) {
            anyData = true;
            ImGui::PushID(200);
            if (ImGui::Button("Remover Freio (X)")) {
                removeColumn(m_brakeIndex);
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::SameLine();
                std::string label = "Freio: " + m_dataList[m_brakeIndex].archive + " (" + m_dataList[m_brakeIndex].column + ")";
                ImGui::TextUnformatted(label.c_str());
            }
            ImGui::PopID();
        }
        
        if (!anyData) {
            ImGui::TextDisabled("(Nenhum dado carregado)");
        }

        ImGui::Separator();
        ImGui::Checkbox("Mostrar Imagens dos Pedais", &m_showPedalImages);

        ImGui::EndPopup();
    }

    // --- Lógica de cálculo dos valores dos pedais com base no último elemento do vetor ---
    float throttleValue = 0.0f;
    float brakeValue = 0.0f;

    if (m_throttleIndex != -1) {
        const auto& pedalData = m_dataList[m_throttleIndex];
        if (!pedalData.data->empty()) {
            double rawThrottle = pedalData.data->back();
            double maxThrottle = pedalData.maxValue;
            throttleValue = static_cast<float>(rawThrottle / maxThrottle);
        }
    }
    if (m_brakeIndex != -1) {
        const auto& pedalData = m_dataList[m_brakeIndex];
        if (!pedalData.data->empty()) {
            double rawBrake = pedalData.data->back();
            double maxBrake = pedalData.maxValue;
            brakeValue = static_cast<float>(rawBrake / maxBrake);
        }
    }
    throttleValue = std::clamp(throttleValue, 0.0f, 1.0f);
    brakeValue = std::clamp(brakeValue, 0.0f, 1.0f);

    // --- Desenho dos Pedais e Barras Lado a Lado ---
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    float textHeight = ImGui::GetTextLineHeightWithSpacing();

    // Proporções dinâmicas dependendo se as imagens estão ativas ou não
    float pedalWidth = m_showPedalImages ? (avail.x * 0.3f) : 0.0f;
    float pedalHeight = avail.y * 0.65f;
    float barWidth = m_showPedalImages ? (avail.x * 0.06f) : (avail.x * 0.15f);
    float spacing = m_showPedalImages ? (avail.x * 0.1f) : (avail.x * 0.2f);
    float innerSpacing = m_showPedalImages ? (avail.x * 0.03f) : 0.0f;

    ImVec2 pedalSize(pedalWidth, pedalHeight);
    ImVec2 barSize(barWidth, pedalHeight);

    SDL_Texture* redPedalTex   = AssetManager::getInstance().getTexture("assets/pedalvermelho.png");
    SDL_Texture* greenPedalTex = AssetManager::getInstance().getTexture("assets/pedalverde.png");

    float totalBlockWidth = m_showPedalImages ? (pedalWidth + innerSpacing + barWidth) : barWidth;
    float totalLayoutWidth = (totalBlockWidth * 2.0f) + spacing;
    ImVec2 initialCursorPos = ImGui::GetCursorPos();
    float startOffsetX = std::max(0.0f, (avail.x - totalLayoutWidth) * 0.5f);

    // 1. Bloco do Freio (Pedal + Barra de Pressão lado a lado)
    ImVec2 brakeStartPos = ImVec2(initialCursorPos.x + startOffsetX, initialCursorPos.y);
    ImGui::SetCursorPos(brakeStartPos);
    ImGui::BeginGroup();
    
    // Centraliza o texto "Freio" sobre a largura total do bloco
    float brakeTextWidth = ImGui::CalcTextSize("Freio").x;
    ImGui::SetCursorPos(ImVec2(brakeStartPos.x + (totalBlockWidth - brakeTextWidth) * 0.5f, brakeStartPos.y));
    ImGui::TextUnformatted("Freio");

    // Posiciona abaixo do texto
    ImGui::SetCursorPos(ImVec2(brakeStartPos.x, brakeStartPos.y + textHeight));

    // Renderiza a imagem se ativa
    if (m_showPedalImages && redPedalTex) {
        if (brakeValue > 0.0f) {
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)redPedalTex, screenPos, ImVec2(screenPos.x + pedalWidth, screenPos.y + pedalHeight), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);
        }
        ImGui::Dummy(pedalSize);
        ImGui::SameLine(0, innerSpacing);
    }

    // Desenha a barra do Freio
    ImVec2 brakeBarPos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        brakeBarPos, ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y), IM_COL32(50, 50, 50, 255));
    float fillHeightBrake = barSize.y * brakeValue;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(brakeBarPos.x, brakeBarPos.y + barSize.y - fillHeightBrake),
                                              ImVec2(brakeBarPos.x + barSize.x, brakeBarPos.y + barSize.y),
                                              IM_COL32(255, 0, 0, 255));
    ImGui::Dummy(barSize);

    // Centraliza a porcentagem abaixo da barra
    char brakePercentText[16];
    snprintf(brakePercentText, sizeof(brakePercentText), "%.0f%%", brakeValue * 100);
    float brakePercentWidth = ImGui::CalcTextSize(brakePercentText).x;
    float brakeBarStartXLocal = brakeStartPos.x + (m_showPedalImages ? (pedalWidth + innerSpacing) : 0.0f);
    ImGui::SetCursorPos(ImVec2(brakeBarStartXLocal + (barWidth - brakePercentWidth) * 0.5f, brakeStartPos.y + textHeight + pedalHeight + 5.0f));
    ImGui::TextUnformatted(brakePercentText);

    ImGui::EndGroup();

    ImGui::SameLine(0, spacing);

    // 2. Bloco do Acelerador (Pedal + Barra de Pressão lado a lado)
    ImVec2 accelStartPos = ImGui::GetCursorPos();
    ImGui::BeginGroup();

    // Centraliza o texto "Acelerador" sobre a largura total do bloco
    float accelTextWidth = ImGui::CalcTextSize("Acelerador").x;
    ImGui::SetCursorPos(ImVec2(accelStartPos.x + (totalBlockWidth - accelTextWidth) * 0.5f, accelStartPos.y));
    ImGui::TextUnformatted("Acelerador");

    // Posiciona abaixo do texto
    ImGui::SetCursorPos(ImVec2(accelStartPos.x, accelStartPos.y + textHeight));

    // Renderiza a imagem se ativa
    if (m_showPedalImages && greenPedalTex) {
        if (throttleValue > 0.0f) {
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)greenPedalTex, screenPos, ImVec2(screenPos.x + pedalWidth, screenPos.y + pedalHeight), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);
        }
        ImGui::Dummy(pedalSize);
        ImGui::SameLine(0, innerSpacing);
    }

    // Desenha a barra do Acelerador
    ImVec2 accelBarPos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        accelBarPos, ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y), IM_COL32(50, 50, 50, 255));
    float fillHeightThrottle = barSize.y * throttleValue;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(accelBarPos.x, accelBarPos.y + barSize.y - fillHeightThrottle),
                                              ImVec2(accelBarPos.x + barSize.x, accelBarPos.y + barSize.y),
                                              IM_COL32(0, 200, 0, 255));
    ImGui::Dummy(barSize);

    // Centraliza a porcentagem abaixo da barra
    char accelPercentText[16];
    snprintf(accelPercentText, sizeof(accelPercentText), "%.0f%%", throttleValue * 100);
    float accelPercentWidth = ImGui::CalcTextSize(accelPercentText).x;
    float accelBarStartXLocal = accelStartPos.x + (m_showPedalImages ? (pedalWidth + innerSpacing) : 0.0f);
    ImGui::SetCursorPos(ImVec2(accelBarStartXLocal + (barWidth - accelPercentWidth) * 0.5f, accelStartPos.y + textHeight + pedalHeight + 5.0f));
    ImGui::TextUnformatted(accelPercentText);

    ImGui::EndGroup();

    // Resetar o cursor da janela para o final da janela
    ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + avail.y));

    ImGui::End();
}

bool Window::Pedal::isLoaded() const {
    return m_throttleIndex != -1 && m_brakeIndex != -1;
}

double Window::Pedal::getDuration() const {
    if (m_throttleIndex != -1) {
        return static_cast<double>(m_dataList[m_throttleIndex].data->size() - 1);
    }
    return 0.0;
}

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
    if (fileType == "Text") {
        return;
    }

    PedalData pd;
    pd.archive = fileName;
    pd.column = columnName;
    pd.fileType = fileType;
    
    // Carregar dados dependendo da fonte (CSV ou Telemetria)
    if (fileType == "CSV") {
        pd.data = &DB::getInstance().getCSVData(fileName, columnName);
    } else if (fileType == "Telemetry") {
        pd.data = &DB::getInstance().getTelemetryData(fileName, columnName);
    }
    
    if (!pd.data) {
        LOG("ERROR", "[Pedal] Falha ao carregar dados da coluna '" + columnName + "' do arquivo '" + fileName + "'.");
        return;
    }

    auto maxIt = std::max_element(pd.data->begin(), pd.data->end());
    if (maxIt != pd.data->end()) {
        pd.maxValue = *maxIt;
    }
    if (pd.maxValue == 0) {
        pd.maxValue = 1.0;
    }

    // Aceita qualquer coluna sem nenhum tipo de filtro de nome!
    // Se o freio estiver vazio, coloca no freio.
    // Caso contrário, coloca no acelerador.
    // Se ambos estiverem cheios, substitui o acelerador.
    if (m_brakeIndex == -1) {
        m_dataList.push_back(pd);
        m_brakeIndex = static_cast<int>(m_dataList.size() - 1);
        LOG("INFO", "[Pedal] Freio carregado com a coluna: '" + columnName + "' (" + fileType + ") de '" + fileName + "'. Total de registros: " + std::to_string(pd.data->size()));
    } else if (m_throttleIndex == -1) {
        m_dataList.push_back(pd);
        m_throttleIndex = static_cast<int>(m_dataList.size() - 1);
        LOG("INFO", "[Pedal] Acelerador carregado com a coluna: '" + columnName + "' (" + fileType + ") de '" + fileName + "'. Total de registros: " + std::to_string(pd.data->size()));
    } else {
        removeColumn(m_throttleIndex);
        m_dataList.push_back(pd);
        m_throttleIndex = static_cast<int>(m_dataList.size() - 1);
        LOG("INFO", "[Pedal] Acelerador (substituído) carregado com a coluna: '" + columnName + "' (" + fileType + ") de '" + fileName + "'. Total de registros: " + std::to_string(pd.data->size()));
    }
}

void Window::Pedal::removeColumn(int index) {
    if (index < 0 || static_cast<size_t>(index) >= m_dataList.size()) return;

    if (index == m_throttleIndex) m_throttleIndex = -1;
    if (index == m_brakeIndex) m_brakeIndex = -1;

    m_dataList.erase(m_dataList.begin() + index);

    // Reajustar índices
    if (m_throttleIndex > index) m_throttleIndex--;
    if (m_brakeIndex > index) m_brakeIndex--;
}


