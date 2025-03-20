#ifndef PLAYBACK_WINDOW_HPP
#define PLAYBACK_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Playback : public IWindow {
        public:
            explicit Playback(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif