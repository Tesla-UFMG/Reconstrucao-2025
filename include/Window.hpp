#ifndef WINDOW_HPP
#define WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

#include "ui/AboutWindow.hpp"
#include "ui/DatapickerWindow.hpp"
#include "ui/DemoWindow.hpp"
#include "ui/InitialWindow.hpp"
#include "ui/LogWindow.hpp"
#include "ui/PlaybackWindow.hpp"
#include "ui/PlotWindow.hpp"
#include "ui/ReconstructionWindow.hpp"
#include "ui/VideoWindow.hpp"

// C++
#include <filesystem>
#include <fstream>
#include <string>

namespace Window {

    struct VisibilityFlags {
            bool showPlayback       = true;
            bool showDataPicker     = true;
            bool showReconstruction = true;
            bool showVideo          = true;
            bool showPlot           = true;
            bool showLog            = true;
            bool showAbout          = false;
            bool showImPlotDemo     = false;
            bool showImGuiDemo      = false;
    };

    extern VisibilityFlags visibility;

    void changeWindowVisibility(const std::string& windowName, bool* windowVisibility);

    void saveWindowVisibility(const std::filesystem::path& filepath);
    void loadWindowVisibility(const std::filesystem::path& filepath);

    void render();

} // namespace Window

#endif // WINDOW_HPP