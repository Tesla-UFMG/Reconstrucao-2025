#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_About.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "ui/windows/w_Demo.hpp"
#include "ui/windows/w_HomePage.hpp"
#include "ui/windows/w_Pedal.hpp"
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Plot.hpp"
#include "ui/windows/w_Reconstruction.hpp"
#include "ui/windows/w_Statistics.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_Video.hpp"
#include "ui/windows/w_WheelControl.hpp"

#include "ui/menubar/MenuBar.hpp"

// C++
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

struct SDL_Renderer;

struct VisibilityFlags {
    public:
        bool showPlayback       = false;
        bool showDataPicker     = false;
        bool showReconstruction = false;
        bool showVideo          = false;
        bool showPlot           = false;
        bool showLog            = false;
        bool showWheelControl   = false;
        bool showPedal          = false;
        bool showAbout          = false;
        bool showImPlotDemo     = false;
        bool showImPlot3dDemo   = false;
        bool showImGuiDemo      = false;
        bool showStatistics     = false;
};

class WindowManager {
    private:
        explicit WindowManager();
        void                                  setup();
        std::vector<std::unique_ptr<IWindow>> windows;
        std::unique_ptr<IWindow>              home;
        SDL_Renderer*                         m_renderer = nullptr;

    public:
        WindowManager(WindowManager&&)            = delete;
        WindowManager& operator=(WindowManager&&) = delete;
        ~WindowManager();
        static WindowManager& getInstance();

        void init(SDL_Renderer* renderer);

        VisibilityFlags visibility;
        void            saveWindowVisibility(const std::filesystem::path& filepath);
        void            loadWindowVisibility(const std::filesystem::path& filepath);

        void menuBar();
        void homePage();
        void mainPage();
};

#endif // WINDOW_HPP