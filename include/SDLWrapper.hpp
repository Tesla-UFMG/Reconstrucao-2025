#ifndef SDLWRAPPER_HPP
#define SDLWRAPPER_HPP

// C++
#include <iostream>
#include <stdexcept>
#include <string>

// Project
#include "Log.hpp"

// Third Party
#include <SDL2/SDL.h>

class SDLWrapper {
    private:
        static bool isSubsystemInited;
        static bool isMinimized;
        static bool isFullscreen;

    public:
        static SDL_Renderer* renderer;
        static SDL_Window*   window;
        static SDL_Event     events;

        static void initSubsystem(); // Inicia todos os subsistemas do SDL
        static void createWindowAndRenderer(const std::string& windowTitle, int windowWidth,
                                            int windowHeight); // Cria uma janela e um renderizador
        static void clearScreen();                             // Limpa a tela de fundo
        static void handleWindowEvents(SDL_Event& event);      // Cria
        static bool getWindowIsMinimized();                    // Pega se a janela está minimizada
        static void render();                                  // Renderiza a tela
        static void closeSubystem();                           // Fecha todos os subsistemas do SDL
};

#endif