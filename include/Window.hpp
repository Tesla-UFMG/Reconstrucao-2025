#ifndef WINDOW_HPP
#define WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

// C++
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

    void about();
    void playback();
    void dataPicker();
    void video();
    void plot();
    void log();
    void ImGuiDemo();
    void ImPlotDemo();

    void circuitReconstruction();

    void changeWindowVisibility(const std::string& windowName, bool* windowVisibility);
    void render();

} // namespace Window

#endif // WINDOW_HPP