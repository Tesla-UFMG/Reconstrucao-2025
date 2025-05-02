#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++ 
#include <algorithm>
#include <cmath>
#include <vector>
#include <sstream>
#include <string>

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"

#define MIN_COORD_SIZE  50.0
#define MAX_COORD_SIZE  1600.0

enum COORD { COORD_X, COORD_Y };

struct COORDData {
        std::vector<std::string> columns;
        std::vector<std::string> archives;

        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;
        std::vector<double>              multiplier;
};

namespace Window {
    class Reconstruction : public IWindow {
        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            ImU32 GetColorForSpeed(float speed);
            void  DrawTrackAndKartAt(const ImVec2& origin, float scale);
            void  UpdateKartSimulation(float deltaTime);

           // --- Drag & Drop de CSV para reconstrução ---
            void processColumnDragDrop(COORDData& coordData);
            void generateSimulatedData(int numPoints, size_t coordIndex, float* x, float* y);

            void addCoord(std::vector<COORDData>& coord);
            void removeCoord(std::vector<COORDData>& coord, size_t coordIndex);
            std::vector<COORDData> coordDataList;
            COORDData coordData;
            static constexpr float Y_OFFSET = 100.0f;

            // Funções de renderização das coordenadas
            void renderGraph(size_t coordIndex); 
            void renderResizeButton(size_t coordIndex);
            void ConvertLatLonToXY(std::vector<float>& outX, std::vector<float>& outY);
    };
} // namespace Window

#endif
