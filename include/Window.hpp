#ifndef WINDOW_HPP
#define WINDOW_HPP

// C++
#include <iostream>
#include <stdexcept>
#include <string>

// Project
#include "ImGuiWrapper.hpp"
#include "SDLWrapper.hpp"
#include "Pages.hpp"

class Window {
    private:
        bool handleEvent();

    public:
        Window();
        ~Window();
        void init(const std::string& windowTitle, int windowWidth,
                  int windowHeight); // Inicia os subsistemas e cria uma janela
        void loop();                 // Loop principal da janela
        void close();                // Fecha a janela
};

#endif