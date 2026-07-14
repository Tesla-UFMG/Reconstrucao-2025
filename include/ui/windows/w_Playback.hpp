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

class WindowManager;

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

            friend class ::WindowManager;

        private:
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

            // Timeline NLE Variables
            std::string loadedVideoName;
            double videoLengthMs = 0.0;
            
            double globalTime = 0.0;
            
            double videoBlockStart = 0.0; 
            double csvBlockStart = 0.0;   
            double csvBlockEnd = 10000.0; 
            
            // Drag states
            bool isDraggingVideo = false;
            bool isDraggingCsv = false;
            bool isResizingCsvLeft = false;
            bool isResizingCsvRight = false;
            float dragOffset = 0.0f;

            // Timer
            std::chrono::time_point<std::chrono::steady_clock> lastFrameTime;
            std::chrono::time_point<std::chrono::steady_clock> lastSeekTime;
            bool videoPlayCommandSent = false;

            // Scrubbing: true when user is clicking/dragging the timeline cursor
            // Updated at end of frame; used by video control logic at start of next frame
            bool isScrubbing = false;
    };
} // namespace Window

#endif
