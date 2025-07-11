#include "WindowManager.hpp"
#include "Log.hpp"

#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_About.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "ui/windows/w_Demo.hpp"
#include "ui/windows/w_HomePage.hpp"
#include "ui/windows/w_Pedal.hpp"
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Plot.hpp"
#include "ui/windows/w_Statistics.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_Video.hpp"
#include "ui/windows/w_WheelControl.hpp"
#include "ui/windows/w_Reconstruction.hpp"
#include "ui/windows/IPlayable.hpp" 

WindowManager& WindowManager::getInstance() {
    static WindowManager instance;
    return instance;
}

WindowManager::WindowManager() {
    LOG("TRACE", "Window Manager iniciado com sucesso.");
}

WindowManager::~WindowManager() { LOG("TRACE", "Window Manager encerrado."); }

void WindowManager::init(SDL_Renderer* renderer) {
    this->m_renderer = renderer;
    this->setup();
}

void WindowManager::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::filesystem::create_directories(parentPath);
        LOG("INFO", "Criada pasta '" + parentPath.string() + "'.");
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível salvar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.write(reinterpret_cast<const char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' salvo com sucesso.");
}

void WindowManager::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' carregada com sucesso.");
}

void WindowManager::setup() {
    auto temp_video_ptr = std::make_unique<Window::Video>(m_renderer, &visibility.showVideo);

    m_videoWindow = temp_video_ptr.get();

    auto temp_reconstruction_ptr = std::make_unique<Window::Reconstruction>(&visibility.showReconstruction);

    m_reconstructionWindow = temp_reconstruction_ptr.get();

    std::vector<IPlayable*> playables;
    playables.push_back(m_videoWindow);
    playables.push_back(m_reconstructionWindow);

    auto temp_playback_ptr = std::make_unique<Window::Playback>(&visibility.showPlayback, playables);
    m_playbackWindow = temp_playback_ptr.get();

    windows.emplace_back(std::move(temp_video_ptr));
    windows.emplace_back(std::move(temp_reconstruction_ptr));
    windows.emplace_back(std::move(temp_playback_ptr));

    home = std::make_unique<Window::HomePage>();
    windows.emplace_back(std::make_unique<Window::About>(&visibility.showAbout));
    windows.emplace_back(std::make_unique<Window::DataPicker>(&visibility.showDataPicker));
    windows.emplace_back(std::make_unique<Window::Plot>(&visibility.showPlot));
    windows.emplace_back(std::make_unique<Window::Terminal>(&visibility.showLog));
    windows.emplace_back(std::make_unique<Window::Pedal>(&visibility.showPedal));
    windows.emplace_back(std::make_unique<Window::ImGuiDemo>(&visibility.showImGuiDemo));
    windows.emplace_back(std::make_unique<Window::ImPlotDemo>(&visibility.showImPlotDemo));
    windows.emplace_back(std::make_unique<Window::ImPlot3dDemo>(&visibility.showImPlot3dDemo));
    windows.emplace_back(std::make_unique<Window::WheelControl>(&visibility.showWheelControl));
    windows.emplace_back(std::make_unique<Window::Statistics>(&visibility.showStatistics));
}

void WindowManager::homePage() {
    MenuBar::render();
    windows[0]->render(); // About
    home->render();       // Home page
}

void WindowManager::mainPage() {
    MenuBar::render();
    for (auto& window : windows) {
        window->render();
    }
}