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
    while (true) {
        if (App::handleEvent())
            break; // Fecha o programa caso cliquem em fechar.
        if (SDLWrapper::getWindowIsMinimized() == false) {
            // Não renderiza se a janela estiver minimizada, PODE BUGAR!
            ImGuiWrapper::prepareForNewFrame();
            SDLWrapper::clearScreen();

            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);


            ImGui::ShowDemoWindow(nullptr);

            MenuBar::render();
            Pages::render();


            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
