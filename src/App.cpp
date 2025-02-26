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

        if (SDLWrapper::events.type == SDL_KEYDOWN) {

            // Carrega ou salva os layouts -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            int offset = 1073741881;
            if (SDLWrapper::events.key.keysym.sym > offset &&
                SDLWrapper::events.key.keysym.sym < offset + 11) { // Entre F1 e F10. Olhe no SDL_keycode
                std::string F_number = std::to_string(SDLWrapper::events.key.keysym.sym - offset);
                if ((SDLWrapper::events.key.keysym.mod & KMOD_CTRL)) { // Se CTRL estiver precionado...
                    Window::saveWindowVisibility("./data/layouts/.visibility_" + F_number);
                    ImGuiWrapper::saveLayout("./data/layouts/.layout_" + F_number);
                } else {
                    Window::loadWindowVisibility("./data/layouts/.visibility_" + F_number);
                    ImGuiWrapper::loadLayout("./data/layouts/.layout_" + F_number);
                }
            }
            // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

            // Limpa o terminal -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            if (SDLWrapper::events.key.keysym.mod & KMOD_CTRL) {
                if (SDLWrapper::events.key.keysym.sym == SDLK_l) {
                    Log::getInstance().clearLog();
                }
            }
            // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        }
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
