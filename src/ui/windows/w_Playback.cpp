#include "ui/windows/w_Playback.hpp"
#include "WindowManager.hpp"
#include "ui/windows/w_Warning.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>

Window::Playback::Playback(bool* isOpen) : IWindow(isOpen) {
    this->title = "Playback";
    this->flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    this->isPlaying        = false;
    this->playbackSpeed    = 1.0f;
    this->currentTimestamp = 0.0;
    this->startTimestamp   = 0.0;
    this->endTimestamp     = 0.0;
    this->currentIndex     = 0;
    this->maxIndex         = 0;
    this->lastUpdatedIndex = -1;
    this->lastFrameTime    = std::chrono::steady_clock::now();
    this->lastSeekTime     = std::chrono::steady_clock::now();
}

std::string Window::Playback::formatTime(double timestamp) {
    double timeInSeconds = timestamp;
    bool   isEpoch       = false;

    if (this->selectedTimeUnit == TimeUnit::Seconds) {
        timeInSeconds = timestamp;
    } else if (this->selectedTimeUnit == TimeUnit::Milliseconds) {
        timeInSeconds = timestamp / 1000.0;
    } else if (this->selectedTimeUnit == TimeUnit::Microseconds) {
        timeInSeconds = timestamp / 1000000.0;
    } else {
        if (timestamp > 1e9) {
            if (timestamp > 1e11) {
                timeInSeconds = timestamp / 1000.0;
            }
        } else {
            // Usually small timestamp means milliseconds from video
            timeInSeconds = timestamp / 1000.0;
        }
    }

    if (this->selectedTimeUnit == TimeUnit::Auto) {
        isEpoch = (timestamp > 1e9);
    } else {
        isEpoch = (timeInSeconds > 1e8);
    }

    if (isEpoch) {
        time_t     t            = static_cast<time_t>(timeInSeconds);
        struct tm* timeinfo     = std::localtime(&t);
        int        milliseconds = static_cast<int>((timeInSeconds - std::floor(timeInSeconds)) * 1000);
        char       buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), ".%03d", milliseconds);
        return std::string(buffer);
    } else {
        int totalSeconds = static_cast<int>(timeInSeconds);
        int hours        = totalSeconds / 3600;
        int displayHours = hours % 24;
        int minutes      = (totalSeconds % 3600) / 60;
        int seconds      = totalSeconds % 60;
        int milliseconds = static_cast<int>((timeInSeconds - totalSeconds) * 1000);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", displayHours, minutes, seconds, milliseconds);
        return std::string(buffer);
    }
}

void Window::Playback::processDragDrop() {
    if (ImGui::BeginDragDropTarget()) {
        bool        accepted = false;
        std::string fileType, fileName, colName;

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COLUMN_NAME")) {
            const ColumnPayload* columnPayload = reinterpret_cast<const ColumnPayload*>(payload->Data);
            fileType                           = columnPayload->fileType;
            fileName                           = columnPayload->fileName;
            colName                            = columnPayload->columnName;
            if (fileType == "CSV" || fileType == "Telemetry") {
                accepted = true;
            }
        } else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVE_NAME")) {
            const ArchivePayload* archivePayload = reinterpret_cast<const ArchivePayload*>(payload->Data);
            fileType                             = archivePayload->fileType;
            fileName                             = archivePayload->fileName;
            colName                              = "timestamp";
            if (fileType == "Video") {
                this->loadedVideoName = fileName;

                const auto& vFiles = DB::getInstance().getProject().getVideoFiles();
                for (const auto& vf : vFiles) {
                    if (vf.getPath().filename().string() == fileName) {
                        if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
                            wVideo->setLoadedVideo(vf.getPath().string());
                            this->videoLengthMs   = wVideo->getPlayer()->getLength();
                            this->videoBlockStart = 0;
                            this->globalTime      = 0.0;

                            if (this->selectedFileName.empty()) {
                                this->csvBlockStart = 0.0;
                                this->csvBlockEnd   = this->videoLengthMs > 0 ? this->videoLengthMs : 10000.0;
                            } else {
                                this->csvBlockStart = 0.0;
                                this->csvBlockEnd   = this->videoLengthMs > 0 ? this->videoLengthMs : 10000.0;
                            }
                        }
                        break;
                    }
                }
            } else if (DB::getInstance().columnExists(fileType, fileName, colName)) {
                accepted = true;
            }
        }

        if (accepted) {
            this->selectedFileType     = fileType;
            this->selectedFileName     = fileName;
            this->selectedTimestampCol = colName;

            this->timestampData.clear();
            if (this->selectedFileType == "CSV") {
                this->timestampData = DB::getInstance().getCSVData(this->selectedFileName, this->selectedTimestampCol);
            } else if (this->selectedFileType == "Telemetry") {
                this->timestampData =
                    DB::getInstance().getTelemetryData(this->selectedFileName, this->selectedTimestampCol);
            }

            if (!this->timestampData.empty()) {
                this->maxIndex         = static_cast<int>(this->timestampData.size() - 1);
                this->startTimestamp   = this->timestampData.front();
                this->endTimestamp     = this->timestampData.back();
                this->currentTimestamp = this->startTimestamp;
                this->currentIndex     = 0;
            } else {
                this->maxIndex         = 0;
                this->startTimestamp   = 0.0;
                this->endTimestamp     = 0.0;
                this->currentTimestamp = 0.0;
                this->currentIndex     = 0;
            }

            this->lastUpdatedIndex = -1;
            this->isPlaying        = false;

            if (this->loadedVideoName.empty()) {
                this->videoBlockStart = 0.0;
                this->csvBlockStart   = 0.0;
                this->csvBlockEnd     = 10000.0;
            } else {
                this->csvBlockStart = 0.0;
                this->csvBlockEnd   = this->videoLengthMs > 0 ? this->videoLengthMs : 10000.0;
            }
            this->globalTime = 0.0;

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

            ProjectData& pd    = DB::getInstance().getProject();
            bool         found = false;
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

void Window::Playback::refreshData() {
    if (this->selectedFileName.empty() || this->selectedTimestampCol.empty())
        return;

    this->timestampData.clear();
    if (this->selectedFileType == "CSV") {
        this->timestampData = DB::getInstance().getCSVData(this->selectedFileName, this->selectedTimestampCol);
    } else if (this->selectedFileType == "Telemetry") {
        this->timestampData = DB::getInstance().getTelemetryData(this->selectedFileName, this->selectedTimestampCol);
    }

    if (!this->timestampData.empty()) {
        this->maxIndex       = static_cast<int>(this->timestampData.size() - 1);
        this->startTimestamp = this->timestampData.front();
        this->endTimestamp   = this->timestampData.back();
        // currentTimestamp is not reset here to preserve layout state
    } else {
        this->maxIndex       = 0;
        this->startTimestamp = 0.0;
        this->endTimestamp   = 0.0;
        this->currentIndex   = 0;
    }

    this->lastUpdatedIndex = -1;
    this->cachedColNames.clear();
    this->cachedColumns.clear();
}

void Window::Playback::updatePlaybackData() {
    if (this->currentIndex < this->lastUpdatedIndex) {
        if (Window::Warning::getInstance()) {
            Window::Warning::getInstance()->removeLogsAfter(this->currentTimestamp);
        }
    }

    if (this->selectedFileName.empty() || this->maxIndex <= 0)
        return;

    if (this->cachedColNames.empty()) {
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
    }

    if (this->cachedColNames.empty())
        return;

    bool           found        = false;
    ProjectData&   pd           = DB::getInstance().getProject();
    TelemetryFile* targetPacket = nullptr;

    for (auto& tel : pd.telemetryFiles) {
        if (tel.getPacketId() == "PLAYBACK") {
            found        = true;
            targetPacket = &tel;
            break;
        }
    }

    if (!found) {
        pd.loadPacket("Reprodução", "PLAYBACK", this->cachedColNames);
        for (auto& tel : pd.telemetryFiles) {
            if (tel.getPacketId() == "PLAYBACK") {
                tel.clearData();
                targetPacket = &tel;
                break;
            }
        }
        if (!targetPacket)
            return;
    }

    if (this->currentIndex < this->lastUpdatedIndex) {
        targetPacket->clearData();
        this->lastUpdatedIndex = -1;
    }

    if (this->currentIndex > this->lastUpdatedIndex) {
        targetPacket->shrinkTo(
            this->lastUpdatedIndex +
            1); // Garante que a linha interpolada anterior seja removida antes de adicionar o bloco real
        targetPacket->insertDataSlice(this->cachedColumns, this->timestampData, this->currentIndex);
        this->lastUpdatedIndex = this->currentIndex;
    }

    // Interpolação do último valor para fluidez
    if (this->currentIndex < this->maxIndex && this->currentIndex >= 0) {
        int    i0 = this->currentIndex;
        int    i1 = i0 + 1;
        double t0 = this->timestampData[i0];
        double t1 = this->timestampData[i1];

        double ratio = 0.0;
        if (t1 > t0) {
            ratio = (this->currentTimestamp - t0) / (t1 - t0);
            if (ratio < 0.0)
                ratio = 0.0;
            if (ratio > 1.0)
                ratio = 1.0;
        }

        targetPacket->shrinkTo(i0 + 1); // Volta o tamanho para remover o frame interpolado anterior
        targetPacket->setInterpolatedRow(this->cachedColumns, ratio, i0, i1);
    }
}

void Window::Playback::render() {
    if (!this->isOpen || !*this->isOpen)
        return;

    // Apply #dbdbdb menu bar color in light mode
    ImVec4 winBgCheck  = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    float  lumCheck    = 0.299f * winBgCheck.x + 0.587f * winBgCheck.y + 0.114f * winBgCheck.z;
    bool   isLightMode = lumCheck >= 0.5f;
    if (isLightMode) {
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(219.0f / 255.0f, 219.0f / 255.0f, 219.0f / 255.0f, 1.0f));
    }

    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    // Sync with Video Window status
    if (!this->loadedVideoName.empty()) {
        if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
            std::string currentVidPath = wVideo->getLoadedVideo();
            if (currentVidPath.empty() || currentVidPath.find(this->loadedVideoName) == std::string::npos) {
                this->loadedVideoName.clear();
                this->videoLengthMs = 0.0;
                this->isPlaying     = false;
            }
        }
    }

    if (this->selectedFileName.empty() && this->loadedVideoName.empty()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y < 50.0f)
            avail.y = 50.0f;
        ImGui::Dummy(avail);
        this->processDragDrop();

        ImVec2 text_size   = ImGui::CalcTextSize("Arraste uma base para cá (CSV, Telemetria ou Vídeo)");
        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 text_pos((window_size.x - text_size.x) * 0.5f, (window_size.y - text_size.y) * 0.5f);
        ImGui::SetCursorPos(text_pos);
        ImGui::TextDisabled("Arraste uma base para cá (CSV, Telemetria ou Vídeo)");
        if (isLightMode)
            ImGui::PopStyleColor(); // MenuBarBg
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Arquivo")) {
            if (ImGui::MenuItem("Limpar / Trocar Base")) {
                this->selectedFileName.clear();
                this->timestampData.clear();
                this->cachedColNames.clear();
                this->cachedColumns.clear();
                this->loadedVideoName.clear();
                this->isPlaying        = false;
                this->lastUpdatedIndex = -1;

                if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
                    wVideo->setLoadedVideo("");
                }

                // Remove the packet entirely so it disappears from the data picker
                DB::getInstance().getProject().removePacket("PLAYBACK");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Configurações de Tempo")) {
            int         currentUnit = static_cast<int>(this->selectedTimeUnit);
            const char* units[]     = {"Automático", "Segundos", "Milissegundos", "Microssegundos"};
            if (ImGui::Combo("Unidade de Tempo", &currentUnit, units, IM_ARRAYSIZE(units))) {
                this->selectedTimeUnit = static_cast<TimeUnit>(currentUnit);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (this->tracksLocked) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
        }
        if (ImGui::MenuItem("Cadeado")) {
            this->tracksLocked = !this->tracksLocked;
        }
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::TextDisabled("CSV: %s | Vídeo: %s",
                            this->selectedFileName.empty() ? "Nenhum" : this->selectedFileName.c_str(),
                            this->loadedVideoName.empty() ? "Nenhum" : this->loadedVideoName.c_str());
        ImGui::EndMenuBar();
    }

    ImVec2 cursorBefore = ImGui::GetCursorPos();
    ImGui::Dummy(ImGui::GetContentRegionAvail());
    this->processDragDrop();
    ImGui::SetCursorPos(cursorBefore);

    auto   now          = std::chrono::steady_clock::now();
    double dt           = std::chrono::duration<double>(now - this->lastFrameTime).count();
    this->lastFrameTime = now;

    if (!this->loadedVideoName.empty()) {
        if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
            int64_t len = wVideo->getPlayer()->getLength();
            if (len > 0) {
                this->videoLengthMs = static_cast<double>(len);
            }
        }
    }

    if (this->isPlaying) {
        bool videoIsMaster = (!this->loadedVideoName.empty() && this->globalTime >= this->videoBlockStart &&
                              this->globalTime <= this->videoBlockStart + this->videoLengthMs);
        if (!videoIsMaster) {
            this->globalTime += dt * 1000.0 * this->playbackSpeed;
        }
    }

    double timelineStart = std::min(this->videoBlockStart, this->csvBlockStart);
    double timelineEnd   = std::max(this->videoBlockStart + this->videoLengthMs, this->csvBlockEnd);
    if (timelineEnd <= timelineStart + 1.0)
        timelineEnd = timelineStart + 10000.0;

    if (this->globalTime >= timelineEnd) {
        this->globalTime = timelineEnd;
        this->isPlaying  = false;
    }
    if (this->globalTime < timelineStart) {
        this->globalTime = timelineStart;
    }

    bool   shouldVideoPlay = false;
    double videoTargetPos  = this->globalTime - this->videoBlockStart;
    if (this->globalTime >= this->videoBlockStart && this->globalTime <= this->videoBlockStart + this->videoLengthMs) {
        shouldVideoPlay = true;
    }

    if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
        if (!wVideo->getLoadedVideo().empty()) {
            auto*  player          = wVideo->getPlayer();
            auto   now             = std::chrono::steady_clock::now();
            double msSinceLastSeek = std::chrono::duration<double, std::milli>(now - this->lastSeekTime).count();

            // Only try to control video if it has been parsed and has a length
            if (player->getLength() > 0) {
                if (this->isScrubbing) {
                    // User is dragging the timeline cursor: seek video to the target position
                    // instead of reading position from it (which would overwrite globalTime).
                    if (this->videoPlayCommandSent) {
                        player->pause();
                        this->videoPlayCommandSent = false;
                    }

                    if (shouldVideoPlay) {
                        int64_t currentVidTime = player->getTime();
                        bool    canSeek        = false;

                        // Allow seeking if 300ms have passed (timeout) OR
                        // if VLC has successfully reached near the PREVIOUS seek target (meaning it's ready for another)
                        if (msSinceLastSeek > 300.0) {
                            canSeek = true;
                        } else if (msSinceLastSeek > 30.0) {
                            if (this->lastSeekTarget < 0 || std::abs(currentVidTime - this->lastSeekTarget) < 300.0) {
                                canSeek = true;
                            }
                        }

                        if (canSeek && std::abs(currentVidTime - videoTargetPos) > 16.0) {
                            int64_t safeTarget = static_cast<int64_t>(videoTargetPos);
                            if (safeTarget >= this->videoLengthMs - 150 && this->videoLengthMs > 150)
                                safeTarget = static_cast<int64_t>(this->videoLengthMs - 150);
                            player->setTime(safeTarget);
                            this->lastSeekTime   = now;
                            this->lastSeekTarget = safeTarget;
                        }
                    }
                } else if (this->isPlaying && shouldVideoPlay) {
                    if (!this->videoPlayCommandSent) {
                        // BUG FIX: always seek to the correct position BEFORE calling play().
                        // Without this, VLC resumes from wherever it last stopped instead of
                        // starting at videoTargetPos (typically 0 at the block start).
                        if (msSinceLastSeek > 80.0) {
                            int64_t safeTarget = static_cast<int64_t>(videoTargetPos);
                            if (safeTarget >= this->videoLengthMs - 150 && this->videoLengthMs > 150)
                                safeTarget = static_cast<int64_t>(this->videoLengthMs - 150);
                            player->setTime(safeTarget);
                            this->lastSeekTime   = now;
                            this->lastSeekTarget = safeTarget;
                        }
                        player->play();
                        this->videoPlayCommandSent = true;
                    }

                    int64_t currentVidTime = player->getTime();
                    if (currentVidTime >= this->videoLengthMs - 150 && this->videoLengthMs > 150) {
                        if (player->isPlaying()) {
                            player->pause();
                            this->videoPlayCommandSent = false;
                        }
                        this->globalTime += dt * 1000.0 * this->playbackSpeed;
                    } else {
                        // Smoothly advance globalTime using dt for 60fps CSV rendering
                        this->globalTime += dt * 1000.0 * this->playbackSpeed;

                        // Sync with VLC only if drift is large (e.g., buffering or big jumps)
                        // This prevents the CSV from updating in "chunks" since player->getTime()
                        // is not updated at 60fps internally by libvlc.
                        double expectedVidTime = this->globalTime - this->videoBlockStart;
                        if (std::abs(expectedVidTime - currentVidTime) > 500.0) {
                            this->globalTime = this->videoBlockStart + currentVidTime;
                        }
                    }
                } else {
                    // Pause if: we sent a play command, OR the video is playing due to
                    // an external trigger (e.g. setLoadedVideo auto-play on drag & drop).
                    if (this->videoPlayCommandSent || player->isPlaying()) {
                        player->pause();
                        this->videoPlayCommandSent = false;
                    }
                    if (shouldVideoPlay) {
                        // Cursor is inside the video block but paused: keep video seeked to cursor.
                        int64_t currentVidTime = player->getTime();
                        bool    canSeek        = false;

                        if (msSinceLastSeek > 300.0) {
                            canSeek = true;
                        } else if (msSinceLastSeek > 30.0) {
                            if (this->lastSeekTarget < 0 || std::abs(currentVidTime - this->lastSeekTarget) < 300.0) {
                                canSeek = true;
                            }
                        }

                        if (canSeek && std::abs(currentVidTime - videoTargetPos) > 16.0) {
                            int64_t safeTarget = static_cast<int64_t>(videoTargetPos);
                            if (safeTarget >= this->videoLengthMs - 150 && this->videoLengthMs > 150)
                                safeTarget = static_cast<int64_t>(this->videoLengthMs - 150);
                            player->setTime(safeTarget);
                            this->lastSeekTime   = now;
                            this->lastSeekTarget = safeTarget;
                        }
                    } else if (videoTargetPos < 0.0) {
                        // BUG FIX: cursor is BEFORE the video block — reset video to position 0
                        // so the next play() always starts from the beginning.
                        if (player->getTime() > 150 && msSinceLastSeek > 500.0) {
                            player->setTime(0);
                            this->lastSeekTime   = now;
                            this->lastSeekTarget = 0;
                        }
                    }
                }
            }
        }
    }

    if (!this->selectedFileName.empty() && this->maxIndex > 0) {
        double csvRatio = 0.0;
        if (this->csvBlockEnd > this->csvBlockStart) {
            csvRatio = (this->globalTime - this->csvBlockStart) / (this->csvBlockEnd - this->csvBlockStart);
        }

        if (csvRatio < 0.0)
            csvRatio = 0.0;
        if (csvRatio > 1.0)
            csvRatio = 1.0;

        this->currentTimestamp = this->startTimestamp + csvRatio * (this->endTimestamp - this->startTimestamp);

        // Find the last index whose timestamp <= currentTimestamp.
        // This means currentIndex is the row we've "arrived at", and
        // currentIndex+1 is the next row — perfect for interpolation.
        this->currentIndex = 0;
        while (this->currentIndex + 1 < this->maxIndex &&
               this->timestampData[this->currentIndex + 1] <= this->currentTimestamp) {
            this->currentIndex++;
        }
    }

    this->updatePlaybackData();

    if (!this->loadedVideoName.empty() && !this->selectedFileName.empty()) {
        ImGui::Spacing();

        ImVec2      p            = ImGui::GetCursorScreenPos();
        float       canvasWidth  = ImGui::GetContentRegionAvail().x;
        float       canvasHeight = 140.0f; // A bit taller for ruler
        ImDrawList* drawList     = ImGui::GetWindowDrawList();

        // Background
        // --- Adaptive color palette: explicit values per theme for good contrast ---
        ImVec4 winBg  = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        float  lum    = 0.299f * winBg.x + 0.587f * winBg.y + 0.114f * winBg.z;
        bool   isDark = lum < 0.5f;

        ImU32 colBg, colBgBorder, colRulerBg, colRulerLine, colTrackBg, colTick, colTickText;
        if (isDark) {
            // Dark theme — original deep-dark palette
            colBg        = IM_COL32(30, 30, 30, 255);    // main canvas bg
            colBgBorder  = IM_COL32(60, 60, 60, 255);    // canvas border
            colRulerBg   = IM_COL32(45, 45, 45, 255);    // ruler strip
            colRulerLine = IM_COL32(20, 20, 20, 255);    // ruler bottom line
            colTrackBg   = IM_COL32(40, 40, 40, 255);    // empty track lane
            colTick      = IM_COL32(150, 150, 150, 255); // ruler tick marks
            colTickText  = IM_COL32(200, 200, 200, 255); // ruler tick labels
        } else {
            // Light theme:
            //  - colRulerBg  = #dbdbdb (219,219,219): the strip BEHIND the tick texts
            //  - colBg / colTrackBg: lighter variants of #dbdbdb
            //  - colBgBorder, colRulerLine, colTick, colTickText: all the same near-black
            colBg        = IM_COL32(235, 235, 235, 255); // main canvas bg     (lighter variant)
            colBgBorder  = IM_COL32(30, 30, 30, 255);    // canvas border      (same as tick text)
            colRulerBg   = IM_COL32(219, 219, 219, 255); // ruler strip bg     (#dbdbdb — behind ticks)
            colRulerLine = IM_COL32(30, 30, 30, 255);    // ruler separator    (same as tick text)
            colTrackBg   = IM_COL32(228, 228, 228, 255); // empty track lane   (lighter variant)
            colTick      = IM_COL32(30, 30, 30, 255);    // ruler tick marks   (same as tick text)
            colTickText  = IM_COL32(30, 30, 30, 255);    // ruler tick labels  (near-black)
        }

        drawList->AddRectFilled(p, ImVec2(p.x + canvasWidth, p.y + canvasHeight), colBg);
        // Border is drawn AFTER PopClipRect so it always sits on top of track backgrounds

        // Scale factors with zoom? For now just fit everything with some margin
        double visibleStart    = timelineStart - (timelineEnd - timelineStart) * 0.02;
        double visibleEnd      = timelineEnd + (timelineEnd - timelineStart) * 0.02;
        double visibleDuration = visibleEnd - visibleStart;
        if (visibleDuration < 1.0)
            visibleDuration = 1.0;

        auto timeToX = [&](double t) -> float {
            return p.x + (float)((t - visibleStart) / visibleDuration * canvasWidth);
        };

        auto xToTime = [&](float x) -> double { return visibleStart + ((x - p.x) / canvasWidth) * visibleDuration; };

        // Snap threshold: 10 pixels converted to time units
        double snapThreshMs = 10.0 / canvasWidth * visibleDuration;

        // Returns the value snapped to the nearest snap point if within threshold.
        // Also sets outSnapX to the screen X of the snap point (for guide line), or -1 if not snapping.
        double m_lastSnapX = -1.0; // screen X of active snap guide line (or -1)
        auto   trySnap     = [&](double value, std::initializer_list<double> snapPoints, double& outSnapX) -> double {
            outSnapX = -1.0;
            for (double sp : snapPoints) {
                if (std::abs(value - sp) < snapThreshMs) {
                    outSnapX = (double)timeToX(sp);
                    return sp;
                }
            }
            return value;
        };
        (void)m_lastSnapX;

        drawList->PushClipRect(p, ImVec2(p.x + canvasWidth, p.y + canvasHeight), true);

        // --- DRAW RULER ---
        float rulerHeight = 25.0f;
        drawList->AddRectFilled(p, ImVec2(p.x + canvasWidth, p.y + rulerHeight), colRulerBg);
        drawList->AddLine(ImVec2(p.x, p.y + rulerHeight), ImVec2(p.x + canvasWidth, p.y + rulerHeight), colRulerLine);

        // Draw ruler ticks
        double tickStep  = visibleDuration / 10.0;
        double firstTick = std::floor(visibleStart / tickStep) * tickStep;
        for (double t = firstTick; t < visibleEnd; t += tickStep) {
            float tx = timeToX(t);
            if (tx >= p.x && tx <= p.x + canvasWidth) {
                drawList->AddLine(ImVec2(tx, p.y + rulerHeight - 5), ImVec2(tx, p.y + rulerHeight), colTick);
                std::string tStr = this->formatTime(t);
                drawList->AddText(ImVec2(tx + 2, p.y + 2), colTickText, tStr.c_str());
            }
        }

        // Tracks Layout
        float track1Y = p.y + rulerHeight + 10.0f;
        float trackH  = 35.0f;

        // --- VIDEO TRACK ---
        float vStartX = timeToX(this->videoBlockStart);
        float vEndX   = timeToX(this->videoBlockStart + this->videoLengthMs);

        // Draw track background (the empty lane)
        drawList->AddRectFilled(ImVec2(p.x, track1Y), ImVec2(p.x + canvasWidth, track1Y + trackH), colTrackBg);

        // Draw the Video Block
        drawList->AddRectFilled(ImVec2(vStartX, track1Y), ImVec2(vEndX, track1Y + trackH), IM_COL32(20, 90, 60, 255),
                                3.0f);
        drawList->AddRect(ImVec2(vStartX, track1Y), ImVec2(vEndX, track1Y + trackH), IM_COL32(40, 160, 100, 255), 3.0f);
        drawList->AddText(ImVec2(vStartX + 8, track1Y + 10), IM_COL32(180, 255, 200, 255),
                          (std::string("Vídeo: ") + this->loadedVideoName).c_str());

        // Drag Video
        float vW = vEndX - vStartX;
        if (vW < 1.0f)
            vW = 1.0f; // PREVENT CRASH
        ImGui::SetCursorScreenPos(ImVec2(vStartX, track1Y));
        if (!this->tracksLocked) {
            ImGui::InvisibleButton("##VideoTrackDrag", ImVec2(vW, trackH));
        } else {
            ImGui::Dummy(ImVec2(vW, trackH));
        }
        double videoSnapGuideX = -1.0;
        if (ImGui::IsItemActive()) {
            if (!this->isDraggingVideo) {
                this->isDraggingVideo = true;
                this->dragOffset      = ImGui::GetMousePos().x - vStartX;
            }
            float  newStartX     = ImGui::GetMousePos().x - this->dragOffset;
            double rawVideoStart = xToTime(newStartX);
            double rawVideoEnd   = rawVideoStart + this->videoLengthMs;

            // Try snapping: video start or video end to CSV edges and Playhead Cursor
            double snapX = -1.0;
            double snappedStart =
                trySnap(rawVideoStart, {this->csvBlockStart, this->csvBlockEnd, this->globalTime}, snapX);
            if (snapX >= 0.0) {
                rawVideoStart   = snappedStart;
                videoSnapGuideX = snapX;
            } else {
                double snappedEnd =
                    trySnap(rawVideoEnd, {this->csvBlockStart, this->csvBlockEnd, this->globalTime}, snapX);
                if (snapX >= 0.0) {
                    rawVideoStart   = snappedEnd - this->videoLengthMs;
                    videoSnapGuideX = snapX;
                }
            }

            this->videoBlockStart = rawVideoStart;
            if (this->videoBlockStart < 0.0)
                this->videoBlockStart = 0.0;
        } else {
            this->isDraggingVideo = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // --- CSV TRACK ---
        float track2Y = track1Y + trackH + 5.0f;

        float cStartX = timeToX(this->csvBlockStart);
        float cEndX   = timeToX(this->csvBlockEnd);

        // Draw track background
        drawList->AddRectFilled(ImVec2(p.x, track2Y), ImVec2(p.x + canvasWidth, track2Y + trackH), colTrackBg);

        // Draw the CSV Block
        drawList->AddRectFilled(ImVec2(cStartX, track2Y), ImVec2(cEndX, track2Y + trackH), IM_COL32(15, 110, 55, 255),
                                3.0f);
        drawList->AddRect(ImVec2(cStartX, track2Y), ImVec2(cEndX, track2Y + trackH), IM_COL32(50, 200, 110, 255), 3.0f);
        drawList->AddText(ImVec2(cStartX + 8, track2Y + 10), IM_COL32(180, 255, 200, 255),
                          (std::string("Dados: ") + this->selectedFileName).c_str());

        float handleW = 8.0f;

        // Left handle
        ImGui::SetCursorScreenPos(ImVec2(cStartX - handleW, track2Y));
        if (!this->tracksLocked) {
            ImGui::InvisibleButton("##CsvLeft", ImVec2(handleW * 2, trackH));
        } else {
            ImGui::Dummy(ImVec2(handleW * 2, trackH));
        }
        double csvLeftSnapGuideX = -1.0;
        if (ImGui::IsItemActive()) {
            double rawVal = xToTime(ImGui::GetMousePos().x);
            double snapX  = -1.0;
            rawVal        = trySnap(
                rawVal, {this->videoBlockStart, this->videoBlockStart + this->videoLengthMs, this->globalTime}, snapX);
            csvLeftSnapGuideX = snapX;
            this->globalTime  = rawVal;
            if (this->globalTime < 0.0)
                this->globalTime = 0.0;
            this->csvBlockStart = rawVal;
            // Minimum duration 1 ms to prevent crash
            if (this->csvBlockStart > this->csvBlockEnd - 1.0)
                this->csvBlockStart = this->csvBlockEnd - 1.0;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        // Center drag
        ImGui::SetCursorScreenPos(ImVec2(cStartX + handleW, track2Y));
        float centerW = (cEndX - cStartX) - handleW * 2;
        if (centerW < 1.0f)
            centerW = 1.0f; // PREVENT CRASH
            
        if (!this->tracksLocked) {
            ImGui::InvisibleButton("##CsvCenter", ImVec2(centerW, trackH));
        } else {
            ImGui::Dummy(ImVec2(centerW, trackH));
        }
        double csvCenterSnapGuideX = -1.0;
        if (ImGui::IsItemActive()) {
            if (!this->isDraggingCsv) {
                this->isDraggingCsv = true;
                this->dragOffset    = ImGui::GetMousePos().x - cStartX;
            }
            float  newStartX      = ImGui::GetMousePos().x - this->dragOffset;
            double rawNewCsvStart = xToTime(newStartX);
            double csvDuration    = this->csvBlockEnd - this->csvBlockStart;
            double rawNewCsvEnd   = rawNewCsvStart + csvDuration;

            // Try snapping: CSV start or CSV end to Video edges and Playhead Cursor
            double snapX = -1.0;
            double snappedStart =
                trySnap(rawNewCsvStart,
                        {this->videoBlockStart, this->videoBlockStart + this->videoLengthMs, this->globalTime}, snapX);
            if (snapX >= 0.0) {
                rawNewCsvStart      = snappedStart;
                csvCenterSnapGuideX = snapX;
            } else {
                double snappedEnd = trySnap(
                    rawNewCsvEnd,
                    {this->videoBlockStart, this->videoBlockStart + this->videoLengthMs, this->globalTime}, snapX);
                if (snapX >= 0.0) {
                    rawNewCsvStart      = snappedEnd - csvDuration;
                    csvCenterSnapGuideX = snapX;
                }
            }

            double dtMove = rawNewCsvStart - this->csvBlockStart;
            // Limit left side to 0
            if (this->csvBlockStart + dtMove < 0.0) {
                dtMove = -this->csvBlockStart;
            }
            this->csvBlockStart += dtMove;
            this->csvBlockEnd   += dtMove;
        } else {
            this->isDraggingCsv = false;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // Right handle
        ImGui::SetCursorScreenPos(ImVec2(cEndX - handleW, track2Y));
        if (!this->tracksLocked) {
            ImGui::InvisibleButton("##CsvRight", ImVec2(handleW * 2, trackH));
        } else {
            ImGui::Dummy(ImVec2(handleW * 2, trackH));
        }
        double csvRightSnapGuideX = -1.0;
        if (ImGui::IsItemActive()) {
            double rawVal = xToTime(ImGui::GetMousePos().x);
            double snapX  = -1.0;
            rawVal        = trySnap(
                rawVal, {this->videoBlockStart, this->videoBlockStart + this->videoLengthMs, this->globalTime}, snapX);
            csvRightSnapGuideX = snapX;
            this->csvBlockEnd  = rawVal;
            // Minimum duration 1 ms to prevent crash
            if (this->csvBlockEnd < this->csvBlockStart + 1.0)
                this->csvBlockEnd = this->csvBlockStart + 1.0;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        // --- SNAP GUIDE LINES (drawn after all drag logic, before playhead) ---
        // Show a bright yellow vertical line at the active snap point
        {
            auto drawSnapGuide = [&](double guideX) {
                if (guideX >= 0.0) {
                    float gx = (float)guideX;
                    drawList->AddLine(ImVec2(gx, p.y + rulerHeight), ImVec2(gx, p.y + canvasHeight),
                                      IM_COL32(255, 230, 50, 200), 1.0f);
                    // Small diamond at the top of the guide
                    drawList->AddTriangleFilled(ImVec2(gx - 4, p.y + rulerHeight), ImVec2(gx + 4, p.y + rulerHeight),
                                                ImVec2(gx, p.y + rulerHeight + 7), IM_COL32(255, 230, 50, 230));
                }
            };
            drawSnapGuide(videoSnapGuideX);
            drawSnapGuide(csvLeftSnapGuideX);
            drawSnapGuide(csvCenterSnapGuideX);
            drawSnapGuide(csvRightSnapGuideX);
        }

        // Draw Global Playhead Cursor (After Effects Style)
        float cursorX = timeToX(this->globalTime);

        // Cursor Line
        drawList->AddLine(ImVec2(cursorX, p.y), ImVec2(cursorX, p.y + canvasHeight), IM_COL32(50, 220, 120, 255), 1.5f);

        // Cursor Head (Polygon)
        ImVec2 head[5] = {ImVec2(cursorX - 6, p.y), ImVec2(cursorX + 6, p.y), ImVec2(cursorX + 6, p.y + 12),
                          ImVec2(cursorX, p.y + 18), ImVec2(cursorX - 6, p.y + 12)};
        drawList->AddConvexPolyFilled(head, 5, IM_COL32(50, 220, 120, 255));
        drawList->AddPolyline(head, 5, IM_COL32(200, 255, 220, 200), ImDrawFlags_Closed, 1.0f);

        drawList->PopClipRect();

        // Draw border on top of all content so track backgrounds can't overdraw it
        drawList->AddRect(p, ImVec2(p.x + canvasWidth, p.y + canvasHeight), colBgBorder);

        // Drag Global Cursor (Clicking anywhere on ruler or background)
        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton("##TimelineBg", ImVec2(canvasWidth, canvasHeight));
        bool timelineBgActive = ImGui::IsItemActive() && !this->isDraggingVideo && !this->isDraggingCsv;
        if (timelineBgActive) {
            this->globalTime = xToTime(ImGui::GetMousePos().x);
            if (this->globalTime < 0.0)
                this->globalTime = 0.0;
        }
        // Update scrubbing state for next frame's video control logic
        this->isScrubbing = timelineBgActive;

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + canvasHeight + 10.0f));

    } else {
        ImGui::Spacing();

        std::string startTimeStr   = this->formatTime(timelineStart);
        std::string endTimeStr     = this->formatTime(timelineEnd);
        std::string currentTimeStr = this->formatTime(this->globalTime);

        float startW   = ImGui::CalcTextSize(startTimeStr.c_str()).x;
        float endW     = ImGui::CalcTextSize(endTimeStr.c_str()).x;
        float sliderW  = ImGui::GetWindowWidth() * 0.75f;
        float itemSpc  = ImGui::GetStyle().ItemSpacing.x;
        float rowWidth = startW + sliderW + endW + (itemSpc * 2);

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - rowWidth) * 0.5f);

        ImGui::Text("%s", startTimeStr.c_str());
        ImGui::SameLine();

        ImGui::SetNextItemWidth(sliderW);
        float sliderVal = static_cast<float>(this->globalTime);
        if (ImGui::SliderFloat("##TimeSlider", &sliderVal, static_cast<float>(timelineStart),
                               static_cast<float>(timelineEnd), currentTimeStr.c_str())) {
            this->globalTime = static_cast<double>(sliderVal);
        }
        ImGui::SameLine();
        ImGui::Text("%s", endTimeStr.c_str());

        ImGui::Spacing();
        ImGui::Spacing();
    }

    float pad           = ImGui::GetStyle().FramePadding.x * 2.0f;
    float playBtnW      = ImGui::CalcTextSize("Pause").x + pad + 10.0f;
    float shortBtnW     = ImGui::CalcTextSize("<<").x + pad + 10.0f;
    float medBtnW       = ImGui::CalcTextSize("|<<").x + pad + 10.0f;
    float comboW        = 100.0f;
    float spaceX        = 8.0f;
    float totalBtnWidth = (medBtnW * 2) + (shortBtnW * 2) + playBtnW + comboW + (5 * spaceX);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalBtnWidth) * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spaceX, 4.0f));

    if (ImGui::Button("|<<", ImVec2(medBtnW, 0))) {
        this->globalTime = timelineStart;
    }
    ImGui::SameLine();
    if (ImGui::Button("<<", ImVec2(shortBtnW, 0))) {
        this->globalTime -= 5000.0 * this->playbackSpeed;
        if (this->globalTime < timelineStart)
            this->globalTime = timelineStart;
    }
    ImGui::SameLine();
    if (ImGui::Button(this->isPlaying ? "Pause" : "Play", ImVec2(playBtnW, 0))) {
        this->isPlaying = !this->isPlaying;
        if (this->isPlaying && this->globalTime >= timelineEnd) {
            this->globalTime = timelineStart;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(">>", ImVec2(shortBtnW, 0))) {
        this->globalTime += 5000.0 * this->playbackSpeed;
        if (this->globalTime > timelineEnd)
            this->globalTime = timelineEnd;
    }
    ImGui::SameLine();
    if (ImGui::Button(">>|", ImVec2(medBtnW, 0))) {
        this->globalTime = timelineEnd;
    }

    ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(comboW);
    const char* speeds[]        = {"0.25x", "0.5x", "1.0x", "1.5x", "2.0x", "5.0x"};
    float       speedValues[]   = {0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 5.0f};
    int         currentSpeedIdx = 2;
    for (int i = 0; i < 6; i++) {
        if (this->playbackSpeed == speedValues[i])
            currentSpeedIdx = i;
    }
    if (ImGui::Combo("##Velocidade", &currentSpeedIdx, speeds, IM_ARRAYSIZE(speeds))) {
        this->playbackSpeed = speedValues[currentSpeedIdx];
        if (auto* wVideo = WindowManager::getInstance().getVideoWindow()) {
            wVideo->getPlayer()->setRate(this->playbackSpeed);
        }
    }

    if (isLightMode)
        ImGui::PopStyleColor(); // MenuBarBg
    ImGui::End();
}
