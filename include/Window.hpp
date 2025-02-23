#ifndef WINDOW_HPP
#define WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

namespace Window {

    struct VisibilityFlags {
        bool showAbout = true;
        bool showPlayback = true;
        bool showDataPicker = true;
        bool showReconstruction = true;
        bool showVideo = true;
        bool showPlot = true;
        bool showLog = true;
        bool showImPlotDemo = false;
        bool showImGuiDemo = false;
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

} // namespace WINDOW

#endif // WINDOW_HPP