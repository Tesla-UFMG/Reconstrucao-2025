#include "SDLWrapper.hpp"

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

bool SDLWrapper::isSubsystemInited = false;
bool SDLWrapper::isMinimized = false;
bool SDLWrapper::isFullscreen = false;

SDL_Renderer* SDLWrapper::renderer = nullptr;
SDL_Window* SDLWrapper::window = nullptr;
SDL_Event SDLWrapper::events;
std::string SDLWrapper::windowTitle;

namespace {

int findRendererDriver(const char* requestedName) {
    const int count = SDL_GetNumRenderDrivers();

    for (int index = 0; index < count; ++index) {
        SDL_RendererInfo info{};

        if (SDL_GetRenderDriverInfo(index, &info) != 0) {
            LOG(
                "WARN",
                std::string("Falha ao consultar SDL renderer [") +
                    std::to_string(index) +
                    "]: " +
                    SDL_GetError());
            continue;
        }

        std::ostringstream message;
        message << "SDL renderer [" << index << "]: "
                << (info.name ? info.name : "unknown")
                << ", flags=0x"
                << std::hex
                << info.flags;

        LOG("TRACE", message.str());

        if (info.name && std::strcmp(info.name, requestedName) == 0) {
            return index;
        }
    }

    return -1;
}

void setRequiredOpenGLAttributes() {
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0) != 0) {
        throw std::runtime_error(
            std::string("SDL_GL_CONTEXT_PROFILE_MASK: ") + SDL_GetError());
    }

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0) {
        throw std::runtime_error(
            std::string("SDL_GL_CONTEXT_MAJOR_VERSION: ") + SDL_GetError());
    }

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0) {
        throw std::runtime_error(
            std::string("SDL_GL_CONTEXT_MINOR_VERSION: ") + SDL_GetError());
    }

    if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
        throw std::runtime_error(
            std::string("SDL_GL_DOUBLEBUFFER: ") + SDL_GetError());
    }
}

} // namespace

void SDLWrapper::initSubsystem() {
    if (SDLWrapper::isSubsystemInited) {
        LOG("WARN", "SDL já está iniciado. Não é possível iniciá-lo novamente.");
        return;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        const std::string error =
            std::string("Erro ao iniciar SDL: ") + SDL_GetError();

        LOG("FATAL", error);
        std::cerr << error << '\n';

        SDLWrapper::closeSubystem();
        throw std::runtime_error(error);
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        LOG("WARN", "Não foi possível habilitar a filtragem linear.");
    }

    SDLWrapper::isSubsystemInited = true;

    const char* videoDriver = SDL_GetCurrentVideoDriver();

    LOG(
        "TRACE",
        std::string("SDL foi iniciado com sucesso. Driver de vídeo: ") +
            (videoDriver ? videoDriver : "desconhecido"));
}

void SDLWrapper::createWindowAndRenderer(
    const std::string& windowTitle,
    int windowWidth,
    int windowHeight) {
    try {
        SDLWrapper::windowTitle = windowTitle;

        setRequiredOpenGLAttributes();

        const int rendererDriver = findRendererDriver("opengl");

        if (rendererDriver < 0) {
            throw std::runtime_error(
                "SDL foi compilado sem o renderer OpenGL.");
        }

        const Uint32 windowFlags =
            SDL_WINDOW_SHOWN |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_ALLOW_HIGHDPI |
            SDL_WINDOW_OPENGL;

        SDLWrapper::window = SDL_CreateWindow(
            windowTitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windowWidth,
            windowHeight,
            windowFlags);

        if (!SDLWrapper::window) {
            throw std::runtime_error(
                std::string("SDL_CreateWindow: ") + SDL_GetError());
        }

        const Uint32 rendererFlags =
            SDL_RENDERER_ACCELERATED |
            SDL_RENDERER_PRESENTVSYNC;

        SDL_ClearError();

        SDLWrapper::renderer = SDL_CreateRenderer(
            SDLWrapper::window,
            rendererDriver,
            rendererFlags);

        if (!SDLWrapper::renderer) {
            throw std::runtime_error(
                std::string("SDL_CreateRenderer: ") + SDL_GetError());
        }

        SDL_RendererInfo rendererInfo{};

        if (SDL_GetRendererInfo(SDLWrapper::renderer, &rendererInfo) != 0) {
            throw std::runtime_error(
                std::string("SDL_GetRendererInfo: ") + SDL_GetError());
        }

        const char* videoDriver = SDL_GetCurrentVideoDriver();

        LOG(
            "TRACE",
            std::string("SDL Window e Renderer criados. Video: ") +
                (videoDriver ? videoDriver : "unknown") +
                ", renderer: " +
                (rendererInfo.name ? rendererInfo.name : "unknown"));
    } catch (const std::exception& e) {
        const std::string error =
            std::string("Erro ao criar Window/Renderer SDL: ") + e.what();

        LOG("FATAL", error);
        std::cerr << error << '\n';

        SDLWrapper::closeSubystem();
        throw;
    }
}

void SDLWrapper::handleEvent(SDL_Event& events) {
    if (events.type == SDL_WINDOWEVENT) {
        switch (events.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_EXPOSED:
                if (SDLWrapper::renderer) {
                    SDL_RenderPresent(SDLWrapper::renderer);
                }
                break;

            case SDL_WINDOWEVENT_MINIMIZED:
                SDLWrapper::isMinimized = true;
                break;

            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                SDLWrapper::isMinimized = false;
                break;

            case SDL_WINDOWEVENT_CLOSE:
                if (SDLWrapper::window) {
                    SDL_HideWindow(SDLWrapper::window);
                }
                break;

            default:
                break;
        }
    } else if (
        events.type == SDL_KEYDOWN &&
        events.key.keysym.sym == SDLK_F11) {
        SDLWrapper::changeFullscreen();
    }
}

void SDLWrapper::changeFullscreen() {
    if (!SDLWrapper::window) {
        LOG(
            "WARN",
            "Não foi possível alterar o modo de tela: janela SDL inexistente.");
        return;
    }

    const bool nextFullscreenState = !SDLWrapper::isFullscreen;
    const Uint32 fullscreenFlag =
        nextFullscreenState ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

    if (SDL_SetWindowFullscreen(SDLWrapper::window, fullscreenFlag) != 0) {
        LOG(
            "ERROR",
            std::string("Não foi possível alterar o modo de tela: ") +
                SDL_GetError());
        return;
    }

    SDLWrapper::isFullscreen = nextFullscreenState;

    LOG(
        "TRACE",
        SDLWrapper::isFullscreen
            ? "Mudou a tela para modo fullscreen."
            : "Mudou a tela para modo janela.");
}

bool SDLWrapper::getIsFullscreen() {
    return SDLWrapper::isFullscreen;
}

void SDLWrapper::clearScreen() {
    if (!SDLWrapper::renderer) {
        return;
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (ImGuiWrapper::currentTheme == DARK) {
        r = 35;
        g = 35;
        b = 35;
    } else {
        const ImVec4 backgroundColour =
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg];

        r = static_cast<uint8_t>(backgroundColour.x * 255.0F);
        g = static_cast<uint8_t>(backgroundColour.y * 255.0F);
        b = static_cast<uint8_t>(backgroundColour.z * 255.0F);
    }

    if (SDL_SetRenderDrawColor(SDLWrapper::renderer, r, g, b, 255) != 0) {
        LOG(
            "ERROR",
            std::string("SDL_SetRenderDrawColor falhou: ") + SDL_GetError());
        return;
    }

    if (SDL_RenderClear(SDLWrapper::renderer) != 0) {
        LOG(
            "ERROR",
            std::string("SDL_RenderClear falhou: ") + SDL_GetError());
    }
}

void SDLWrapper::render() {
    if (SDLWrapper::renderer) {
        SDL_RenderPresent(SDLWrapper::renderer);
    }
}

bool SDLWrapper::getWindowIsMinimized() {
    return SDLWrapper::isMinimized;
}

void SDLWrapper::changeWindowTitle(const std::string& windowTitle) {
    SDLWrapper::windowTitle = windowTitle;

    if (SDLWrapper::window) {
        SDL_SetWindowTitle(SDLWrapper::window, windowTitle.c_str());
    }
}

void SDLWrapper::raiseExitEvent() {
    SDL_Event event{};
    event.type = SDL_QUIT;

    if (SDL_PushEvent(&event) < 0) {
        LOG(
            "ERROR",
            std::string("Não foi possível enviar SDL_QUIT: ") + SDL_GetError());
    }
}

void SDLWrapper::closeSubystem() {
    if (SDLWrapper::renderer) {
        SDL_DestroyRenderer(SDLWrapper::renderer);
        SDLWrapper::renderer = nullptr;
        LOG("TRACE", "SDL Renderer destruído.");
    }

    if (SDLWrapper::window) {
        SDL_DestroyWindow(SDLWrapper::window);
        SDLWrapper::window = nullptr;
        LOG("TRACE", "SDL Window destruída.");
    }

    if (SDLWrapper::isSubsystemInited) {
        SDL_Quit();
        SDLWrapper::isSubsystemInited = false;
        LOG("TRACE", "SDL encerrado.");
    }

    SDLWrapper::isMinimized = false;
    SDLWrapper::isFullscreen = false;
}
