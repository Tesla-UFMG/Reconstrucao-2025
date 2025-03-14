#include "WindowManager.hpp"
#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <filesystem>
#include <fstream>

WindowManager::VisibilityFlags WindowManager::visibility;

void WindowManager::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && std::filesystem::create_directories(parentPath)) {
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

void WindowManager::changeWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    *windowVisibility = !(*windowVisibility);
    std::string message =
        *windowVisibility ? "Foi aberta a janela '" + windowName + "'." : "Foi fechada a janela '" + windowName + "'.";
    LOG("TRACE", message);
}

void WindowManager::render() {
    Window::About(&WindowManager::visibility.showAbout);
    Window::Playback(&WindowManager::visibility.showPlayback);
    Window::Datapicker(&WindowManager::visibility.showDataPicker);
    Window::Reconstruction(&WindowManager::visibility.showReconstruction);
    Window::Video(&WindowManager::visibility.showVideo);
    Window::Plot(&WindowManager::visibility.showPlot);
    Window::Log(&WindowManager::visibility.showLog);
    Window::ImGuiDemo(&WindowManager::visibility.showImGuiDemo);
    Window::ImPlotDemo(&WindowManager::visibility.showImPlotDemo);
    Window::WheelControl(&WindowManager::visibility.showWheelControl);
}
