#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"

#define MIN_COORD_SIZE 50.0
#define MAX_COORD_SIZE 1600.0

struct COORDData {
        std::string         column;
        std::string         archive;
        std::vector<double> data;
        double              multiplier;
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
            void processColumnDragDrop();
            void addColumnToMap(const std::string& archiveName, const std::string& columnName);
            void removeColumnFromMap(size_t index);

            void generateSimulatedData(int numPoints, size_t coordIndex, float* x, float* y);

            std::vector<COORDData> coordDataList;
            static constexpr float Y_OFFSET = 100.0f;

            // Funções de renderização das coordenadas
            void ConvertLatLonToXY(std::vector<float>& outX, std::vector<float>& outY);
            void BuildTrackFromLatLon(size_t latIndex, size_t lonIndex);
    };
} // namespace Window

#endif
