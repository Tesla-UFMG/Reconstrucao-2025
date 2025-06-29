#include "WindowManager.hpp"
#include "Log.hpp"

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
    auto video_window = std::make_unique<Window::Video>(m_renderer, &visibility.showVideo);
    Window::Video* video_ptr = video_window.get();

    home = std::make_unique<Window::HomePage>();

    windows.emplace_back(std::make_unique<Window::About>(&visibility.showAbout));
    windows.emplace_back(std::make_unique<Window::Playback>(&visibility.showPlayback, video_ptr));
    windows.emplace_back(std::make_unique<Window::DataPicker>(&visibility.showDataPicker));
    windows.emplace_back(std::make_unique<Window::Reconstruction>(&visibility.showReconstruction));
    windows.emplace_back(std::move(video_window)); 
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