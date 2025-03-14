#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

#include "ui/w_About.hpp"
#include "ui/w_DataPicker.hpp"
#include "ui/w_Demo.hpp"
#include "ui/w_HomePage.hpp"
#include "ui/w_Log.hpp"
#include "ui/w_Playback.hpp"
#include "ui/w_Plot.hpp"
#include "ui/w_Reconstruction.hpp"
#include "ui/w_Video.hpp"
#include "ui/w_WheelControl.hpp"

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

        void render();
};

#endif // WINDOW_HPP
