#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <filesystem>

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// Third Party
#include "sqlite3/sqlite3.h"

struct CachedTile {
    int z;
    int x;
    int y;
    SDL_Texture* texture;
};

struct TrackTextAnnotation {
    std::string archiveName;
    std::string columnName;
};

namespace Window {
    class Reconstruction : public IWindow {
        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual ~Reconstruction();
            virtual void render() override;

            bool isLoaded() const;

        private:
            SDL_Texture* getTileTexture(int z, int x, int y);
            void findFirstAvailableTile();
            void clearCache();
            void scanAvailableMaps();

            sqlite3*                m_db = nullptr;
            std::vector<CachedTile> m_tileCache;
            int                     m_testZ = 0;
            int                     m_testX = 0;
            int                     m_testY = 0;
            bool                    m_loaded = false;
            std::string             m_statusMessage = "Iniciando...";
            
            // Mouse panning and zooming variables
            float                   m_panX = 0.0f;
            float                   m_panY = 0.0f;
            float                   m_zoomScale = 0.5f;

            // Map files variables
            std::vector<std::string> m_availableMaps;
            std::string              m_currentMapName;

            // Track reconstruction variables
            std::string              m_selectedFileType = "CSV"; // "CSV" or "Telemetry"
            std::string              m_selectedFileName;         // CSV file name or Telemetry packetId
            std::string              m_selectedLatCol;
            std::string              m_selectedLonCol;

            // Georeferenced camera center variables (continuous camera target)
            double                   m_centerLat = 0.0;
            double                   m_centerLon = 0.0;

            // Track calibration offset variables
            bool                     m_moveTrackMode = false;
            double                   m_trackOffsetLat = 0.0;
            double                   m_trackOffsetLon = 0.0;

            // Track style colors (RGBA) - Coordinated Premium Green Scale Palette
            float                   m_colorLine[4] = {0.0f, 0.7f, 0.2f, 0.8f};       // Forest green
            float                   m_colorPoint[4] = {0.2f, 0.9f, 0.4f, 1.0f};      // Mint green
            float                   m_colorLastPoint[4] = {0.7f, 1.0f, 0.0f, 1.0f};  // Neon lime green

            void centerOnTrack();
            std::vector<TrackTextAnnotation> m_textAnnotations;
    };
} // namespace Window

#endif