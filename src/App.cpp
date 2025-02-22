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
            Log::getInstance().message("TRACE", "Evento de fechar janela detectado.");
            return true;
        }
        ImGui_ImplSDL2_ProcessEvent(&SDLWrapper::events);
        SDLWrapper::handleWindowEvents(SDLWrapper::events);
    }
    return false;
}

void App::loop() {
    Log::getInstance().message("TRACE", "Entrou no loop principal.");
    while (true) {
        // Fecha o programa caso cliquem em fechar.
        if (App::handleEvent()){
            break;
        }

        // Não renderiza se a janela estiver minimizada, PODE BUGAR!
        if (SDLWrapper::getWindowIsMinimized() == false) {
            ImGuiWrapper::prepareForNewFrame();
            SDLWrapper::clearScreen();

            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::ShowDemoWindow(nullptr);
            ImPlot::ShowDemoWindow(nullptr);
            
            MenuBar::render();
            Pages::render();

            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
