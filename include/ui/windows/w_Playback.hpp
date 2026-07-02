#ifndef PLAYBACK_WINDOW_HPP
#define PLAYBACK_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"
#include "DB.hpp"

// C++
#include <string>
#include <vector>
#include <chrono>

namespace Window {
    class Playback : public IWindow {
        public:
            explicit Playback(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            void processDragDrop();
            void updatePlaybackData();
            std::string formatTime(double timestamp);

            std::string selectedFileType;
            std::string selectedFileName;
            std::string selectedTimestampCol;

            bool isPlaying;
            float playbackSpeed;
            double currentTimestamp;
            double startTimestamp;
            double endTimestamp;

            int currentIndex;
            int maxIndex;
            int lastUpdatedIndex;

            std::vector<double> timestampData;
            std::vector<std::string> cachedColNames;
            std::vector<const std::vector<double>*> cachedColumns;

            // Timer
            std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    };
}

#endif
