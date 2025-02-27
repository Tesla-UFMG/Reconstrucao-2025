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
        static std::string   windowTitle;
        static SDL_Renderer* renderer;
        static SDL_Window*   window;
        static SDL_Event     events;

        static void initSubsystem(); // Inicia todos os subsistemas do SDL
        static void createWindowAndRenderer(const std::string& windowTitle, int windowWidth,
                                            int windowHeight);         // Cria uma janela e um renderizador
        static void clearScreen();                                     // Limpa a tela de fundo
        static void handleEvent(SDL_Event& event);                     // Trata os Eventos
        static bool getWindowIsMinimized();                            // Pega se a janela está minimizada
        static void closeSubystem();                                   // Fecha todos os subsistemas do SDL
        static void changeFullscreen();                                // Tira/Coloca em tela cheia
        static bool getIsFullscreen();                                 // Checa se a tela esta cheia
        static void changeWindowTitle(const std::string& windowTitle); // Muda o nome da janela
        static void raiseExitEvent();                                  // Cria evento de saida
        static void render();                                          // Renderiza a tela
};

#endif