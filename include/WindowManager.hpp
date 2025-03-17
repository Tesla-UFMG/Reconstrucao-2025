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
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Plot.hpp"
#include "ui/windows/w_Reconstruction.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_Video.hpp"
#include "ui/windows/w_WheelControl.hpp"
#include "ui/windows/w_Statistics.hpp"

#include "ui/menubar/MenuBar.hpp"

// C++
#include <filesystem>
#include <fstream>
#include <string>

struct VisibilityFlags {
    public:
        bool showPlayback       = true;
        bool showDataPicker     = true;
        bool showReconstruction = true;
        bool showVideo          = true;
        bool showPlot           = true;
        bool showLog            = true;
        bool showWheelControl   = true;
        bool showAbout          = false;
        bool showImPlotDemo     = false;
        bool showImGuiDemo      = false;
        bool showStatistics     = true;
    };

class WindowManager {
    private:
        explicit WindowManager();
        void                                  setup();
        std::vector<std::unique_ptr<IWindow>> windows;
        std::unique_ptr<IWindow>              home;

    public:
        WindowManager(WindowManager&&)            = delete;
        WindowManager& operator=(WindowManager&&) = delete;
        ~WindowManager();
        static WindowManager& getInstance();

        VisibilityFlags visibility;
        void            saveWindowVisibility(const std::filesystem::path& filepath);
        void            loadWindowVisibility(const std::filesystem::path& filepath);

        void menuBar();
        void homePage();
        void mainPage();
};

#endif // WINDOW_HPP
