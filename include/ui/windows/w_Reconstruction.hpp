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
    };
} // namespace Window

#endif