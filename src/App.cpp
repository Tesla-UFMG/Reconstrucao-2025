#include "App.hpp"

App::App() {}

App::~App() {}

void App::init(const std::string& windowTitle, int windowWidth, int windowHeight) {
    this->windowTitle = windowTitle;
    SDLWrapper::initSubsystem();
    SDLWrapper::createWindowAndRenderer(this->windowTitle, windowWidth, windowHeight);
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
                    Window::saveWindowVisibility("./cache/layouts/.visibility_" + F_number + ".bin");
                    ImGuiWrapper::saveLayout("./cache/layouts/.layout_" + F_number + ".ini");
                } else {
                    Window::loadWindowVisibility("./cache/layouts/.visibility_" + F_number + ".bin");
                    ImGuiWrapper::loadLayout("./cache/layouts/.layout_" + F_number + ".ini");
                }
            }

            // Se CTRL estiver pressionado...
            if (SDLWrapper::events.key.keysym.mod & KMOD_CTRL) {

                // L - Limpar o log
                if (SDLWrapper::events.key.keysym.sym == SDLK_l) {
                    Log::getInstance().clearLog();
                }

                // S - Salvar o projeto
                if (SDLWrapper::events.key.keysym.sym == SDLK_s) {
                    DB::getInstance().saveProjectDialog();
                }

                // N - Carregar o projeto
                if (SDLWrapper::events.key.keysym.sym == SDLK_n) {
                    DB::getInstance().loadProjectDialog();
                }

                // C - Cria o projeto
                if (SDLWrapper::events.key.keysym.sym == SDLK_c) {
                    DB::getInstance().createProjectDialog();
                }
            }
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

            if (DB::getInstance().getCurrentProject().empty()) {
                Window::Initial();
            } else {
                Window::render();
            }

            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
