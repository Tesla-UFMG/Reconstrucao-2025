#ifndef CIRCUIT_RECONSTRUCTION_WINDOW_HPP
#define CIRCUIT_RECONSTRUCTION_WINDOW_HPP

// C++
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "XYAlignment.hpp"
#include "ui/windows/iWindow.hpp"

// Third Party
#include "sqlite3/sqlite3.h"

struct CachedTile {
        int          z;
        int          x;
        int          y;
        SDL_Texture* texture;
};

struct TrackTextAnnotation {
        std::string archiveName;
        std::string columnName;
};

class WindowManager;

namespace Window {
    class Reconstruction : public IWindow {
            friend class ::WindowManager;

        public:
            explicit Reconstruction(bool* isOpen = nullptr);
            virtual ~Reconstruction();
            virtual void render() override;
            void         drawMenuBar();
            bool         m_followTheEnd    = false;
            bool         m_rotateMap       = false; // New toggle for map rotation
            bool         m_limitPoints     = false;
            int          m_numPointsToShow = 1000;

            bool isLoaded() const;

        private:
            SDL_Texture* getTileTexture(int z, int x, int y);
            void         findFirstAvailableTile();
            void         clearCache();
            void         scanAvailableMaps();
            void         setMap(const std::string& mapName);

            sqlite3*                m_db = nullptr;
            std::vector<CachedTile> m_tileCache;
            int                     m_testZ         = 0;
            int                     m_testX         = 0;
            int                     m_testY         = 0;
            bool                    m_loaded        = false;
            bool                    m_wasOpen       = false;
            std::string             m_statusMessage = "Iniciando...";

            // Mouse panning and zooming variables
            float m_panX      = 0.0f;
            float m_panY      = 0.0f;
            float m_zoomScale = 0.5f;

            // Map files variables
            std::vector<std::string> m_availableMaps;
            std::string              m_currentMapName;

            // Track reconstruction variables
            std::string                   m_selectedLatFileType = "CSV"; // "CSV" or "Telemetry"
            std::string                   m_selectedLatFileName;         // CSV file name or Telemetry packetId
            std::string                   m_selectedLatCol;
            std::string                   m_selectedLonFileType = "CSV"; // "CSV" or "Telemetry"
            std::string                   m_selectedLonFileName;         // CSV file name or Telemetry packetId
            std::string                   m_selectedLonCol;
            XYAlignmentMode               m_alignmentMode      = ALIGN_LINEAR_INTERPOLATION;
            XYAlignmentMode               m_colorAlignmentMode = ALIGN_LINEAR_INTERPOLATION;
            std::string                   m_selectedColorCol;
            std::map<std::string, ImVec2> m_textOffsets;

            // Database limits para normalização das cores
            std::string m_selectedColorFileName;
            std::string m_selectedColorFileType;
            double      m_gradMinVal      = 0.0;
            double      m_gradMaxVal      = 100.0;
            bool        m_autoFitGradient = true;
            float       m_gradMinColor[4] = {0.0f, 0.4f, 1.0f, 1.0f}; // Blue
            float       m_gradMaxColor[4] = {1.0f, 0.1f, 0.1f, 1.0f}; // Red
            int         m_colorMode       = 1;                        // 1 = ImPlot, 2 = Manual
            int         m_colormap        = 0;
            bool        m_reverseColormap = false;

            // Georeferenced camera center variables (continuous camera target)
            double m_centerLat = 0.0;
            double m_centerLon = 0.0;

            // Track calibration offset variables
            bool   m_moveTrackMode  = false;
            double m_trackOffsetLat = 0.0;
            double m_trackOffsetLon = 0.0;

            // Track style colors (RGBA) - Coordinated Premium Green Scale Palette
            float m_colorLine[4]      = {0.0f, 0.7f, 0.2f, 0.0f}; // Transparent
            float m_colorPoint[4]     = {0.2f, 0.9f, 0.4f, 1.0f}; // Mint green
            float m_colorLastPoint[4] = {0.7f, 1.0f, 0.0f, 1.0f}; // Neon lime green

            double m_currentHeading = 0.0; // Smoothed map rotation heading

            void                             centerOnTrack();
            void                             autoFitColorLimits();
            std::vector<TrackTextAnnotation> m_textAnnotations;
    };
} // namespace Window

#endif