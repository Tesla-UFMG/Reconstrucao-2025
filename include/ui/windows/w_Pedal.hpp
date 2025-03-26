#ifndef PEDAL_WINDOW_HPP
#define PEDAL_WINDOW_HPP

// C++
#include <algorithm>

// Project
#include "AssetManager.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Pedal : public IWindow {
        public:
            explicit Pedal(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            ImTextureID redPedalTexture;
            ImTextureID greenPedalTexture;
            ImFont*     smallFont;
    };

} // namespace Window

#endif