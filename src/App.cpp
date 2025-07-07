extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}
#include "App.hpp"

App::App() {}

App::~App() {}

void App::init(const std::string& windowTitle, int windowWidth, int windowHeight) {
    this->windowTitle = windowTitle;

    avformat_network_init();
    av_log_set_level(AV_LOG_DEBUG);

    SDLWrapper::initSubsystem();
    SDLWrapper::createWindowAndRenderer(this->windowTitle, windowWidth, windowHeight);
    WindowManager::getInstance().init(SDLWrapper::renderer);
    ImGuiWrapper::initSubsystem();
}

void App::close() {
    ImGuiWrapper::closeSubystem();
    SDLWrapper::closeSubystem();

    avformat_network_deinit();
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
                    WindowManager::getInstance().saveWindowVisibility("./cache/layouts/.visibility_" + F_number +
                                                                      ".bin");
                    ImGuiWrapper::saveLayout("./cache/layouts/.layout_" + F_number + ".ini");
                } else {
                    WindowManager::getInstance().loadWindowVisibility("./cache/layouts/.visibility_" + F_number +
                                                                      ".bin");
                    ImGuiWrapper::loadLayout("./cache/layouts/.layout_" + F_number + ".ini");
                }
            }

            if (SDLWrapper::events.key.keysym.mod & KMOD_CTRL) {
                if (SDLWrapper::events.key.keysym.sym == SDLK_l) { // L - Limpar o log
                    Log::getInstance().clearLog();
                }
                if (SDLWrapper::events.key.keysym.sym == SDLK_s) { // S - Salvar o projeto
                    DB::getInstance().saveProjectDialog();
                }
                if (SDLWrapper::events.key.keysym.sym == SDLK_n) { // N - Carregar o projeto
                    DB::getInstance().loadProjectDialog();
                }
                if (SDLWrapper::events.key.keysym.sym == SDLK_c) { // C - Cria o projeto
                    DB::getInstance().createProjectDialog();
                }
            }
        }
    }
    return false;
}

void App::loop() {
    LOG("AUDIT", "Entrou no loop principal.");

    WindowManager& wm = WindowManager::getInstance();

    wm.loadWindowVisibility("./cache/layouts/.visibility_1.bin");
    ImGuiWrapper::loadLayout("./cache/layouts/.layout_1.ini");

    while (true) {
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

            // Se não tiver um projeto carregado
            // MELHORAR ESSA PORCARIA - de lucas para lucas
            if (DB::getInstance().getProject().currentProjectName.empty()) {
                wm.homePage();
            } else {
                wm.mainPage();
            }

            ImGuiWrapper::render();
            SDLWrapper::render();
        }
    }
}
