#ifndef APP_HPP
#define APP_HPP

// C++
#include <iostream>
#include <stdexcept>
#include <string>

// Project
#include "ImGuiWrapper.hpp"
#include "MenuBar.hpp"
#include "SDLWrapper.hpp"
#include "Window.hpp"

class App {
    private:
        bool handleEvent();

    public:
        App();
        ~App();
        void init(const std::string& windowTitle, int windowWidth,
                  int windowHeight); // Inicia os subsistemas e cria uma janela
        void loop();                 // Loop principal da janela
        void close();                // Fecha a janela
};

#endif // APP_HPP