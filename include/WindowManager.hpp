#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

#include "ui/windows/w_About.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "ui/windows/w_Demo.hpp"
#include "ui/windows/w_HomePage.hpp"
#include "ui/windows/w_Log.hpp"
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Plot.hpp"
#include "ui/windows/w_Reconstruction.hpp"
#include "ui/windows/w_Video.hpp"
#include "ui/windows/w_WheelControl.hpp"

#include "ui/menubar/m_Help.hpp"
#include "ui/menubar/m_Tesla.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/menubar/m_Windows.hpp"

// C++
#include <filesystem>
#include <fstream>
#include <string>

struct VisibilityFlags {
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
};

class WindowManager {
    private:
        explicit WindowManager();

    public:
        static VisibilityFlags visibility;
        WindowManager(WindowManager&&)            = delete;
        WindowManager& operator=(WindowManager&&) = delete;
        ~WindowManager();
        static WindowManager& getInstance();

        static void saveWindowVisibility(const std::filesystem::path& filepath);
        static void loadWindowVisibility(const std::filesystem::path& filepath);

        static void MenuBar();
        static void render();
};

#endif // WINDOW_HPP
