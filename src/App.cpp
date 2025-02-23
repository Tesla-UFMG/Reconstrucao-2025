#include "App.hpp"

App::App() {}

App::~App() {}

void App::init(const std::string& windowTitle, int windowWidth, int windowHeight) {
    SDLWrapper::initSubsystem();
    SDLWrapper::createWindowAndRenderer(windowTitle, windowWidth, windowHeight);
    ImGuiWrapper::initSubsystem();
}

void App::close() {
    ImGuiWrapper::closeSubystem();
    SDLWrapper::closeSubystem();
}

bool App::handleEvent() {
    while (SDL_PollEvent(&SDLWrapper::events)) {
        if (SDLWrapper::events.type == SDL_QUIT) {
            LOG("TRACE", "Evento de fechar janela detectado.");
            return true;
        }

        ImGuiWrapper::handleEvent(SDLWrapper::events);
        SDLWrapper::handleEvent(SDLWrapper::events);
    }
    return false;
}

void App::loop() {
    LOG("TRACE", "Entrou no loop principal.");
    while (true) {
        // Fecha o programa caso cliquem em fechar.
        if (App::handleEvent()) {
            break;
        }

        // Não renderiza se a janela estiver minimizada, PODE BUGAR!
        if (SDLWrapper::getWindowIsMinimized() == false) {
            ImGuiWrapper::prepareForNewFrame();
            SDLWrapper::clearScreen();

            ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
            ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(),
                                         ImGuiDockNodeFlags_PassthruCentralNode);

            MenuBar::render();
            Window::render();

            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
