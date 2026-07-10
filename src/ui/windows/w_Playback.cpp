#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Warning.hpp"
#include <cmath>
#include <ctime>

Window::Playback::Playback(bool* isOpen) : IWindow(isOpen) {
    this->title = "Playback";
    this->flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    this->isPlaying = false;
    this->playbackSpeed = 1.0f;
    this->currentTimestamp = 0.0;
    this->startTimestamp = 0.0;
    this->endTimestamp = 0.0;
    this->currentIndex = 0;
    this->maxIndex = 0;
    this->lastUpdatedIndex = -1;
    this->lastFrameTime = std::chrono::steady_clock::now();
}

std::string Window::Playback::formatTime(double timestamp) {
    if (timestamp > 1e9) { 
        double timeInSeconds = timestamp;
        if (timestamp > 1e11) {
            timeInSeconds = timestamp / 1000.0;
        }
        time_t t = static_cast<time_t>(timeInSeconds);
        struct tm* timeinfo = std::localtime(&t);
        int milliseconds = static_cast<int>((timeInSeconds - std::floor(timeInSeconds)) * 1000);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), ".%03d", milliseconds);
        return std::string(buffer);
    } else {
        double timeInSeconds = timestamp;
        
        // Se for um número grande (ex: ms desde meia noite), converte para segundos
        if (timestamp > 86400.0) { 
            timeInSeconds = timestamp / 1000.0;
        }

        int totalSeconds = static_cast<int>(timeInSeconds);
        int hours = totalSeconds / 3600;
        int displayHours = hours % 24;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        int milliseconds = static_cast<int>((timeInSeconds - totalSeconds) * 1000);
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", displayHours, minutes, seconds, milliseconds);
        return std::string(buffer);
    }
}

void Window::Playback::processDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            
            if (std::string(columnPayload->fileType) != "CSV") {
                ImGui::EndDragDropTarget();
                return;
            }

            this->selectedFileType = columnPayload->fileType;
            this->selectedFileName = columnPayload->fileName;
            this->selectedTimestampCol = columnPayload->columnName;
            
            this->timestampData.clear();
            if (this->selectedFileType == "CSV") {
                this->timestampData = DB::getInstance().getCSVData(this->selectedFileName, this->selectedTimestampCol);
            } else if (this->selectedFileType == "Telemetry") {
                this->timestampData = DB::getInstance().getTelemetryData(this->selectedFileName, this->selectedTimestampCol);
            }

            if (!this->timestampData.empty()) {
                this->maxIndex = static_cast<int>(this->timestampData.size() - 1);
                this->startTimestamp = this->timestampData.front();
                this->endTimestamp = this->timestampData.back();
                this->currentTimestamp = this->startTimestamp;
                this->currentIndex = 0;
            } else {
                this->maxIndex = 0;
                this->startTimestamp = 0.0;
                this->endTimestamp = 0.0;
                this->currentTimestamp = 0.0;
                this->currentIndex = 0;
            }

            this->lastUpdatedIndex = -1;
            this->isPlaying = false;

            // Cache columns
            this->cachedColNames.clear();
            this->cachedColumns.clear();

            if (this->selectedFileType == "CSV") {
                for (const auto& csv : DB::getInstance().getProject().getCSVFiles()) {
                    if (csv.getName() == this->selectedFileName) {
                        this->cachedColNames = csv.getColumnNames();
                        break;
                    }
                }
            } else if (this->selectedFileType == "Telemetry") {
                for (const auto& tel : DB::getInstance().getProject().getTelemetryFiles()) {
                    if (tel.getPacketId() == this->selectedFileName) {
                        this->cachedColNames = tel.getColumnNames();
                        break;
                    }
                }
            }

            for (const auto& colName : this->cachedColNames) {
                if (this->selectedFileType == "CSV") {
                    this->cachedColumns.push_back(&DB::getInstance().getCSVData(this->selectedFileName, colName));
                } else {
                    this->cachedColumns.push_back(&DB::getInstance().getTelemetryData(this->selectedFileName, colName));
                }
            }
            
            // Força reinicialização do pacote de telemetria baseada no drag-and-drop
            ProjectData& pd = DB::getInstance().getProject();
            bool found = false;
            for (auto& tel : pd.telemetryFiles) {
                if (tel.getPacketId() == "PLAYBACK") {
                    found = true;
                    break;
                }
            }

            if (!found) {
                pd.loadPacket("Reprodução", "PLAYBACK", this->cachedColNames);
            } else {
                pd.updatePacket("PLAYBACK", "PLAYBACK", "Reprodução", this->cachedColNames);
            }
            
            for (auto& tel : pd.telemetryFiles) {
                if (tel.getPacketId() == "PLAYBACK") {
                    tel.clearData();
                    break;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Window::Playback::updatePlaybackData() {
    if (this->currentIndex < this->lastUpdatedIndex) {
        if (Window::Warning::getInstance()) {
            Window::Warning::getInstance()->removeLogsAfter(this->currentTimestamp);
        }
    }

    if (this->selectedFileName.empty() || this->maxIndex <= 0 || this->cachedColNames.empty()) return;

    bool found = false;
    ProjectData& pd = DB::getInstance().getProject();
    TelemetryFile* targetPacket = nullptr;

    for (auto& tel : pd.telemetryFiles) {
        if (tel.getPacketId() == "PLAYBACK") {
            found = true;
            targetPacket = &tel;
            break;
        }
    }

    if (!found) return;

    if (this->currentIndex < this->lastUpdatedIndex) {
        targetPacket->clearData();
        this->lastUpdatedIndex = -1;
    }

    if (this->currentIndex > this->lastUpdatedIndex) {
        // Obter pointers mais recentes das colunas para prevenir falha caso o CSV tenha sido atualizado
        size_t numCols = this->cachedColNames.size();
        std::vector<const std::vector<double>*> currentColumns(numCols, nullptr);
        for (size_t c = 0; c < numCols; ++c) {
            if (this->selectedFileType == "CSV") {
                currentColumns[c] = &DB::getInstance().getCSVData(this->selectedFileName, this->cachedColNames[c]);
            } else {
                currentColumns[c] = &DB::getInstance().getTelemetryData(this->selectedFileName, this->cachedColNames[c]);
            }
        }
        
        targetPacket->insertDataSlice(currentColumns, this->timestampData, this->currentIndex);
        this->lastUpdatedIndex = this->currentIndex;
    }
}

void Window::Playback::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Drop Target Background
    if (this->selectedFileName.empty()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y < 50.0f) avail.y = 50.0f;
        ImGui::Dummy(avail);
        this->processDragDrop();
        
        ImVec2 text_size = ImGui::CalcTextSize("Arraste uma base para cá (CSV ou Telemetria)");
        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 text_pos((window_size.x - text_size.x) * 0.5f, (window_size.y - text_size.y) * 0.5f);
        ImGui::SetCursorPos(text_pos);
        ImGui::TextDisabled("Arraste uma base para cá (CSV ou Telemetria)");
        ImGui::End();
        return;
    }

    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Arquivo")) {
            if (ImGui::MenuItem("Limpar / Trocar Base")) {
                this->selectedFileName.clear();
                this->timestampData.clear();
                this->isPlaying = false;
                
                // Clear PLAYBACK
                for (auto& tel : DB::getInstance().getProject().telemetryFiles) {
                    if (tel.getPacketId() == "PLAYBACK") {
                        tel.clearData();
                        break;
                    }
                }
            }
            ImGui::EndMenu();
        }
        
        ImGui::Separator();
        ImGui::TextDisabled("Base Ativa: %s  |  Coluna Tempo: %s", this->selectedFileName.c_str(), this->selectedTimestampCol.c_str());
        ImGui::EndMenuBar();
    }

    // Criar Drop Target invisível no fundo para permitir a substituição da base atual
    ImVec2 cursorBefore = ImGui::GetCursorPos();
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    this->processDragDrop();
    ImGui::SetCursorPos(cursorBefore);

    // Logic update
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - this->lastFrameTime).count();
    this->lastFrameTime = now;

    double timeMultiplier = 1.0;
    // Se o timestamp final for > 86400 (24h em s), está em milissegundos!
    if (this->endTimestamp > 86400.0) {
        timeMultiplier = 1000.0;
    }

    if (this->isPlaying) {
        this->currentTimestamp += dt * timeMultiplier * this->playbackSpeed;
        
        if (this->currentTimestamp > this->endTimestamp) {
            this->currentTimestamp = this->endTimestamp;
            this->isPlaying = false;
        }

        // Find the correct index
        while (this->currentIndex < this->maxIndex && this->timestampData[this->currentIndex] < this->currentTimestamp) {
            this->currentIndex++;
        }
    }

    // Centralização Vertical Opcional
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float totalHeight = 80.0f; // Estimativa da altura dos botões + slider
    if (avail.y > totalHeight) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - totalHeight) * 0.4f);
    }

    // --- 1. SLIDER CENTRALIZADO ---
    ImGui::Spacing();
    
    std::string startTimeStr = this->formatTime(this->startTimestamp);
    std::string endTimeStr = this->formatTime(this->endTimestamp);
    std::string currentTimeStr = this->formatTime(this->currentTimestamp);

    float startW = ImGui::CalcTextSize(startTimeStr.c_str()).x;
    float endW = ImGui::CalcTextSize(endTimeStr.c_str()).x;
    float sliderW = ImGui::GetWindowWidth() * 0.75f;
    float itemSpc = ImGui::GetStyle().ItemSpacing.x;
    float rowWidth = startW + sliderW + endW + (itemSpc * 2);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - rowWidth) * 0.5f);

    ImGui::Text("%s", startTimeStr.c_str());
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(sliderW);
    float sliderVal = static_cast<float>(this->currentTimestamp);
    if (ImGui::SliderFloat("##TimeSlider", &sliderVal, static_cast<float>(this->startTimestamp), static_cast<float>(this->endTimestamp), currentTimeStr.c_str())) {
        this->currentTimestamp = sliderVal;
        
        // Find index matching slider
        this->currentIndex = 0;
        while (this->currentIndex < this->maxIndex && this->timestampData[this->currentIndex] < this->currentTimestamp) {
            this->currentIndex++;
        }
    }
    ImGui::SameLine();
    ImGui::Text("%s", endTimeStr.c_str());

    ImGui::Spacing();
    ImGui::Spacing();

    // --- 2. CONTROLES DE MÍDIA E VELOCIDADE ---
    // Calcular larguras para centralização horizontal dos botões
    float pad = ImGui::GetStyle().FramePadding.x * 2.0f;
    float playBtnW = ImGui::CalcTextSize("Pause").x + pad + 10.0f;
    float shortBtnW = ImGui::CalcTextSize("<<").x + pad + 10.0f;
    float medBtnW = ImGui::CalcTextSize("|<<").x + pad + 10.0f;
    float comboW = 100.0f;
    float spaceX = 8.0f;
    float totalBtnWidth = (medBtnW * 2) + (shortBtnW * 2) + playBtnW + comboW + (5 * spaceX);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalBtnWidth) * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spaceX, 4.0f));
    
    if (ImGui::Button("|<<", ImVec2(medBtnW, 0))) {
        this->currentTimestamp = this->startTimestamp;
        this->currentIndex = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("<<", ImVec2(shortBtnW, 0))) {
        this->currentTimestamp -= 5.0 * timeMultiplier * this->playbackSpeed;
        if (this->currentTimestamp < this->startTimestamp) this->currentTimestamp = this->startTimestamp;
        while (this->currentIndex > 0 && this->timestampData[this->currentIndex] > this->currentTimestamp) {
            this->currentIndex--;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(this->isPlaying ? "Pause" : "Play", ImVec2(playBtnW, 0))) {
        this->isPlaying = !this->isPlaying;
        if (this->isPlaying && this->currentIndex >= this->maxIndex) {
            this->currentIndex = 0;
            this->currentTimestamp = this->startTimestamp;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(">>", ImVec2(shortBtnW, 0))) {
        this->currentTimestamp += 5.0 * timeMultiplier * this->playbackSpeed;
        if (this->currentTimestamp > this->endTimestamp) this->currentTimestamp = this->endTimestamp;
        while (this->currentIndex < this->maxIndex && this->timestampData[this->currentIndex] < this->currentTimestamp) {
            this->currentIndex++;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(">>|", ImVec2(medBtnW, 0))) {
        this->currentTimestamp = this->endTimestamp;
        this->currentIndex = this->maxIndex;
    }
    
    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(comboW);
    const char* speeds[] = { "0.25x", "0.5x", "1.0x", "1.5x", "2.0x", "5.0x" };
    float speedValues[] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 5.0f };
    int currentSpeedIdx = 2; // Default 1.0x
    for (int i = 0; i < 6; i++) {
        if (this->playbackSpeed == speedValues[i]) currentSpeedIdx = i;
    }
    if (ImGui::Combo("##Velocidade", &currentSpeedIdx, speeds, IM_ARRAYSIZE(speeds))) {
        this->playbackSpeed = speedValues[currentSpeedIdx];
    }

    this->updatePlaybackData();

    ImGui::End();
}
