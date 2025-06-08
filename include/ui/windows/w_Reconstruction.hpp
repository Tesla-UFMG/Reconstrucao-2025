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

// Estrutura que armazena as informações do marcador
struct MarkerInfo {
    size_t idx;
    ImVec2 pos;
    float  speed;
    float  acceleration;
    float  position;
    int    lap;
    int trackIndex;
    float trackFrac;
};
struct CommentInfo {
    size_t idx;             // índice em screenPts
    ImVec2 triOffset;       // deslocamento opcional (se quiser ajustar posição)
    bool   visible;         // janela de comentário aberta?
    char   text[256];       // conteúdo do comentário
};

// Define cada ponto da pista com posição (x,y) e velocidade de referência
struct TrackPoint {
    float x, y;
    float referenceSpeed;
};

struct RaceData {
    bool isSaved = false;

    char name[64] = "Corrida sem nome"; 

    float cartHeight;
    float cartZoom;
    float speedMultiplier;
    float HighSpeedThreshold;
    float LowSpeedThreshold;

    std::vector<MarkerInfo> markedPositionsGreen;
    std::vector<MarkerInfo> markedPositionsRed;
    std::vector<std::vector<size_t>> highSpeedSegments;
    std::vector<std::vector<size_t>> lowSpeedSegments;
    std::vector<CommentInfo> comments;

    int latIndex;
    int lonIndex;
};


namespace Window {
    class Reconstruction : public IWindow {
        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            ImU32 GetColorForSpeed(float speed);
            void  DrawTrackAndKartAt(const std::vector<ImVec2>& screenPts,
                                                const ImVec2& origin,
                                                float scal);
            void  UpdateKartSimulation(float deltaTime,
                                                  const std::vector<ImVec2>& screenPts);

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
            void RenderTrackOverlay();
    };
} // namespace Window

#endif
