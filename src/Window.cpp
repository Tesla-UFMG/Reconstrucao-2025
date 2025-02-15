#include "Window.hpp"

Window::Window() {}

Window::~Window() { 
    this->close();
}

void Window::init(const std::string& windowTitle, int windowWidth, int windowHeight) {
    SDLWrapper::initSubsystem();
    SDLWrapper::createWindowAndRenderer(windowTitle, windowWidth, windowHeight);
    ImGuiWrapper::initSubsystem();
}

void Window::close() {
    ImGuiWrapper::closeSubystem();
    SDLWrapper::closeSubystem();
}

bool Window::handleEvent() {
    while (SDL_PollEvent(&SDLWrapper::events)) {
        if (SDLWrapper::events.type == SDL_QUIT)
            return true;
        ImGui_ImplSDL2_ProcessEvent(&SDLWrapper::events);
        SDLWrapper::handleWindowEvents(SDLWrapper::events);
    }
    return false;
}

void Window::loop() {
    while (true) {
        if (Window::handleEvent())
            break; // Fecha o programa caso cliquem em fechar.
        if (SDLWrapper::getWindowIsMinimized() == false) { 
            // Não renderiza se a janela estiver minimizada, PODE BUGAR!
            ImGuiWrapper::prepareForNewFrame();
            SDLWrapper::clearScreen();

            ImGui::ShowDemoWindow(nullptr);

            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
