#ifndef PAGES_HPP
#define PAGES_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"

namespace Pages {

    extern bool showAbout;
    extern bool showPlayback;
    extern bool showDataPicker;
    extern bool showReconstruction;
    extern bool showVideo;
    extern bool showPlot;

    void about();
    void playback();
    void dataPicker();
    void video();
    void plot();
    
    void circuitReconstruction();

    void render();

} // namespace Pages

#endif // PAGES_HPP