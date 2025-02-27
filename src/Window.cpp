#include "Window.hpp"

Window::VisibilityFlags Window::visibility;

void Window::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && std::filesystem::create_directories(parentPath)) {
        LOG("INFO", "Criada pasta '" + parentPath.string() + "'.");
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível salvar a visiblidade '" + filepath.string() + "'.");
        return;
    }

    file.write(reinterpret_cast<const char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' salvo com sucesso.");
}

void Window::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' carregado com sucesso.")
}

void Window::changeWindowVisibility(const std::string& windowName, bool* windowVisibility) {
    *windowVisibility = !(*windowVisibility);
    std::string message =
        *windowVisibility ? "Foi aberta a janela '" + windowName + "'." : "Foi fechada a janela '" + windowName + "'.";
    LOG("TRACE", message);
}

void Window::render() {
    Window::About(&Window::visibility.showAbout);
    Window::Playback(&Window::visibility.showPlayback);
    Window::Datapicker(&Window::visibility.showDataPicker);
    Window::Reconstruction(&Window::visibility.showReconstruction);
    Window::Video(&Window::visibility.showVideo);
    Window::Plot(&Window::visibility.showPlot);
    Window::Log(&Window::visibility.showLog);
    Window::ImGuiDemo(&Window::visibility.showImGuiDemo);
    Window::ImPlotDemo(&Window::visibility.showImPlotDemo);
}
