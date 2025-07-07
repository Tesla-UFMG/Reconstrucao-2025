#ifndef PLAYBACK_WINDOW_HPP
#define PLAYBACK_WINDOW_HPP

#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Video;

    class Playback : public IWindow {
        public:
            explicit Playback(bool* isOpen, Video* video_window);
            void render() override;

        private:
            Video* m_videoWindow;
            float  m_sliderTime = 0.0f;
    };

} // namespace Window

#endif