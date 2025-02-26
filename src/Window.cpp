#include "Window.hpp"

Window::VisibilityFlags Window::visibility;

void Window::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(&visibility), sizeof(VisibilityFlags));
        LOG("INFO", "Visibilidade '" + filepath.string() + "' salvo com sucesso.");
    } else {
        LOG("WARN", "Não foi possível salvar a visiblidade '" + filepath.string() + "'.");
    }
}

void Window::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (file) {
        file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
        LOG("INFO", "Visibilidade '" + filepath.string() + "' carregado com sucesso.")
    } else {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
    }
}

void Window::changeWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    *windowVisibility = !(*windowVisibility);
    std::string message =
        *windowVisibility ? "Foi aberta a janela '" + windowName + "'." : "Foi fechada a janela '" + windowName + "'.";
    LOG("TRACE", message);
}

void Window::render() {
    Window::about(&Window::visibility.showAbout);
    Window::playback(&Window::visibility.showPlayback);
    Window::dataPicker(&Window::visibility.showDataPicker);
    Window::circuitReconstruction(&Window::visibility.showReconstruction);
    Window::video(&Window::visibility.showVideo);
    Window::plot(&Window::visibility.showPlot);
    Window::log(&Window::visibility.showLog);
    Window::ImGuiDemo(&Window::visibility.showImGuiDemo);
    Window::ImPlotDemo(&Window::visibility.showImPlotDemo);
}
