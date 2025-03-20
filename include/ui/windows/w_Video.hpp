#ifndef VIDEO_WINDOW_HPP
#define VIDEO_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Video : public IWindow {
        public:
            explicit Video(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif