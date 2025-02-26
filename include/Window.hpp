#ifndef WINDOW_HPP
#define WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

#include "ui/aboutWindow.hpp"
#include "ui/circuitReconstructionWindow.hpp"
#include "ui/dataPickerWindow.hpp"
#include "ui/demoWindow.hpp"
#include "ui/logWindow.hpp"
#include "ui/playbackWindow.hpp"
#include "ui/plotWindow.hpp"
#include "ui/videoWindow.hpp"

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