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
#include "ui/windows/w_Statistics.hpp"
#include "ui/windows/w_Telemetry.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_WheelControl.hpp"
#include "ui/windows/w_Reconstruction.hpp" 
#include "ui/windows/w_Numeric.hpp"
#include "ui/windows/w_Graph.hpp"
#include "ui/windows/w_Bar.hpp"
#include "ui/windows/w_Warning.hpp"
#include "ui/windows/w_Matrix.hpp"
#include "ui/windows/w_Updates.hpp"
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Video.hpp"

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
        bool showDataPicker     = false;
        bool showReconstruction = false;
        bool showLog            = false;
        bool showWheelControl   = false;
        bool showPedal          = false;
        bool showAbout          = false;
        bool showImPlotDemo     = false;
        bool showImPlot3dDemo   = false;
        bool showImGuiDemo      = false;
        bool showTelemetry      = false;
        bool showWarnings       = false;
        bool showPlayback       = false;
        bool showVideo          = false;
};

namespace Window {
    class Reconstruction;
}

class WindowManager {
    private:
        explicit WindowManager();
        void                                  setup();
        std::vector<std::unique_ptr<IWindow>> windows;
        std::unique_ptr<IWindow>              home;
        SDL_Renderer*                         m_renderer = nullptr;

        // Estes ponteiros servem apenas para facilitar a comunicação entre janelas.
        Window::Reconstruction* m_reconstructionWindow = nullptr;
        Window::About* m_aboutWindow = nullptr;
        Window::Updates* m_updatesWindow = nullptr;
        Window::Playback* m_playbackWindow = nullptr;
        Window::Video* m_videoWindow = nullptr;
        void saveWindowCustomStates(const std::string& filepath);
        void loadWindowCustomStates(const std::string& filepath);

public:
    WindowManager(WindowManager&&)            = delete;
    WindowManager& operator=(WindowManager&&) = delete;
    ~WindowManager();
    static WindowManager& getInstance();

        void init(SDL_Renderer* renderer);
        void cleanup();

        VisibilityFlags visibility;
        bool            showUpdates = false;
        void            saveWindowVisibility(const std::filesystem::path& filepath);
        void            loadWindowVisibility(const std::filesystem::path& filepath);

        void menuBar();
        void homePage();
        void mainPage();
        void createNumericWindow();
        void createGraphWindow();
        void createBarWindow();
        void createMatrixWindow();
        void createTabelaWindow();

        Window::Video* getVideoWindow() { return m_videoWindow; }
        Window::Playback* getPlaybackWindow() { return m_playbackWindow; }
};

#endif // WINDOW_HPP