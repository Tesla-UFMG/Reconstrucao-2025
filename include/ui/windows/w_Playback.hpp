#ifndef PLAYBACK_WINDOW_HPP
#define PLAYBACK_WINDOW_HPP

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <chrono>
#include <string>
#include <vector>

namespace Window {
    enum class TimeUnit { Auto, Seconds, Milliseconds, Microseconds };

    class Playback : public IWindow {
        public:
            explicit Playback(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            void        processDragDrop();
            void        updatePlaybackData();
            std::string formatTime(double timestamp);

            std::string selectedFileType;
            std::string selectedFileName;
            std::string selectedTimestampCol;

            bool   isPlaying;
            float  playbackSpeed;
            double currentTimestamp;
            double startTimestamp;
            double endTimestamp;

            int currentIndex;
            int maxIndex;
            int lastUpdatedIndex;

            std::vector<double>                     timestampData;
            std::vector<std::string>                cachedColNames;
            std::vector<const std::vector<double>*> cachedColumns;

            TimeUnit selectedTimeUnit = TimeUnit::Auto;

            // Timer
            std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
    };
} // namespace Window

#endif
