#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++ 
#include <algorithm>
#include <cmath>
#include <vector>

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Reconstruction : public IWindow {
        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            ImU32 GetColorForSpeed(float speed);
            void  DrawTrackAndKartAt(const ImVec2& origin, float scale);
            void  UpdateKartSimulation(float deltaTime);
    };
} // namespace Window

#endif
