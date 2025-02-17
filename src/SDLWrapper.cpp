#include "SDLWrapper.hpp"

bool          SDLWrapper::isSubsystemInited = false;
bool          SDLWrapper::isMinimized       = false;
bool          SDLWrapper::isFullscreen      = false;
SDL_Renderer* SDLWrapper::renderer          = nullptr;
SDL_Window*   SDLWrapper::window            = nullptr;
SDL_Event     SDLWrapper::events;

void SDLWrapper::initSubsystem() {
    Log& logger = Log::getInstance();
    if (SDLWrapper::isSubsystemInited) {
        logger.message("WARN", "SDL já está iniciado. Não é possível iniciá-lo novamente.");
        return;
    }

    try {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao iniciar SDL: ") + e.what();
        logger.message("ERROR", error);
        std::cout << error << "\n";
        SDLWrapper::closeSubystem();
        throw;
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        std::cout << "Warning: Linear texture filtering not enabled!\n";
        logger.message("WARN", "Não foi possível habilitar a filtragem linear.");
    }

    logger.message("TRACE", "SDL foi iniciado com sucesso.");
    SDLWrapper::isSubsystemInited = true;
}

void SDLWrapper::createWindowAndRenderer(const std::string& windowTitle, int windowWidth,
                                         int windowHeight) {

    Log& logger = Log::getInstance();

    try {
        int windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
        SDLWrapper::window =
            SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             windowWidth, windowHeight, windowFlags);

        if (!SDLWrapper::window)
            throw std::runtime_error(SDL_GetError());

        int rendererFlags    = SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC;
        SDLWrapper::renderer = SDL_CreateRenderer(SDLWrapper::window, -1, rendererFlags);

        if (!SDLWrapper::renderer){
            throw std::runtime_error(SDL_GetError());
        }

        logger.message("TRACE", "SDL Window e Renderer criados.");
    } catch (const std::exception& e) {
        std::string error = std::string("Erro ao iniciar SDL: ") + e.what();
        logger.message("ERROR", error);
        std::cout << error << "\n";
        SDLWrapper::closeSubystem();
        throw;
    }
}

void SDLWrapper::handleWindowEvents(SDL_Event& events) {
    if (events.type == SDL_WINDOWEVENT) {
        switch (events.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_EXPOSED:
                SDL_RenderPresent(SDLWrapper::renderer);
                break;

            case SDL_WINDOWEVENT_MINIMIZED:
                SDLWrapper::isMinimized = true;
                break;

            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                SDLWrapper::isMinimized = false;
                break;

            case SDL_WINDOWEVENT_CLOSE:
                SDL_HideWindow(SDLWrapper::window);
                break;
        }
    }

    else if (events.type == SDL_KEYDOWN && events.key.keysym.sym == SDLK_F11) {
        SDLWrapper::isFullscreen = !SDLWrapper::isFullscreen;
        SDL_SetWindowFullscreen(SDLWrapper::window,
                                SDLWrapper::isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

void SDLWrapper::clearScreen() {
    SDL_SetRenderDrawColor(SDLWrapper::renderer, 255, 255, 255, 255);
    SDL_RenderClear(SDLWrapper::renderer);
}

void SDLWrapper::render() { SDL_RenderPresent(SDLWrapper::renderer); }

bool SDLWrapper::getWindowIsMinimized() { return SDLWrapper::isMinimized; }

void SDLWrapper::closeSubystem() {
    Log& logger = Log::getInstance();

    if (SDLWrapper::renderer) {
        SDL_DestroyRenderer(SDLWrapper::renderer);
        SDLWrapper::renderer = nullptr;
        logger.message("TRACE", "SDL Renderer destruído.");
    }

    if (SDLWrapper::window) {
        SDL_DestroyWindow(SDLWrapper::window);
        SDLWrapper::window = nullptr;
        logger.message("TRACE", "SDL Window destruída.");
    }

    if (SDLWrapper::isSubsystemInited) {
        SDL_Quit();
        logger.message("TRACE", "SDL encerrado.");
        SDLWrapper::isSubsystemInited = false;
    }
}
