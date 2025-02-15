#include "SDLWrapper.hpp"

bool SDLWrapper::isSubsystemInited = false;
bool SDLWrapper::isMinimized = false;
bool SDLWrapper::isFullscreen = false;
SDL_Renderer* SDLWrapper::renderer = nullptr;
SDL_Window* SDLWrapper::window = nullptr;
SDL_Event SDLWrapper::events;

void SDLWrapper::initSubsystem(){
    if (SDLWrapper::isSubsystemInited) return;

    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        SDLWrapper::closeSubystem();
        throw std::runtime_error(std::string("SDL_Init Error: ") + SDL_GetError());
    }
        
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        std::cout << "Warning: Linear texture filtering not enabled!\n";
    
    SDLWrapper::isSubsystemInited = true;
}

void SDLWrapper::createWindowAndRenderer(const std::string& windowTitle, int windowWidth, int windowHeight){
    int windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    SDLWrapper::window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, windowFlags);
    if (SDLWrapper::window == nullptr){
        SDLWrapper::closeSubystem();
        throw std::runtime_error(std::string("Could not create the window. SDL Error: ") + std::string(SDL_GetError()));
    }

    int rendererFlags =  SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC;
    SDLWrapper::renderer = SDL_CreateRenderer(SDLWrapper::window, -1, rendererFlags);
    if (SDLWrapper::renderer == nullptr){
        SDLWrapper::closeSubystem();
        throw std::runtime_error(std::string("Could not create a renderer. SDL_Error: ") + std::string(SDL_GetError()));
    }
}
void SDLWrapper::handleWindowEvents(SDL_Event& events){
    if(events.type == SDL_WINDOWEVENT){
        switch(events.window.event){
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_RenderPresent(SDLWrapper::renderer);
            break;

            case SDL_WINDOWEVENT_EXPOSED:
            SDL_RenderPresent(SDLWrapper::renderer);
            break;

            case SDL_WINDOWEVENT_MINIMIZED:
            SDLWrapper::isMinimized = true;
            break;

            case SDL_WINDOWEVENT_MAXIMIZED:
            SDLWrapper::isMinimized = false;
            break;
            
            case SDL_WINDOWEVENT_RESTORED:
            SDLWrapper::isMinimized = false;
            break;

            case SDL_WINDOWEVENT_CLOSE:
            SDL_HideWindow(SDLWrapper::window);
            break;
        }
    }

    else if(events.type == SDL_KEYDOWN && events.key.keysym.sym == SDLK_F11){
        if(SDLWrapper::isFullscreen){
            SDL_SetWindowFullscreen(SDLWrapper::window, 0);
            SDLWrapper::isFullscreen = false;
        } else {
            SDL_SetWindowFullscreen(SDLWrapper::window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            SDLWrapper::isFullscreen = true;
        }
    }
}

void SDLWrapper::clearScreen(){
    SDL_SetRenderDrawColor(SDLWrapper::renderer, 255, 255, 255, 255);
    SDL_RenderClear(SDLWrapper::renderer);
}

void SDLWrapper::render(){
    SDL_RenderPresent(SDLWrapper::renderer);
}

bool SDLWrapper::getWindowIsMinimized(){
    return SDLWrapper::isMinimized;
}

void SDLWrapper::closeSubystem(){
    if (SDLWrapper::renderer) {
        SDL_DestroyRenderer(SDLWrapper::renderer);
        SDLWrapper::renderer = nullptr;
    }
    if (SDLWrapper::window) {
        SDL_DestroyWindow(SDLWrapper::window);
        SDLWrapper::window = nullptr;
    }
    SDL_Quit();
    SDLWrapper::isSubsystemInited = false;
}
