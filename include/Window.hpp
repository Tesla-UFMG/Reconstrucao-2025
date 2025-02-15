#ifndef WINDOW_HPP
#define WINDOW_HPP

//C++
#include <iostream>
#include <string>
#include <stdexcept>

// Classes
#include "SDLWrapper.hpp"
#include "ImGuiWrapper.hpp"

class Window{
    private:
        bool handleEvent();

    public:
        Window();
        ~Window();
        void init(const std::string& windowTitle, int windowWidth, int windowHeight); // Inicia os subsistemas e cria uma janela
        void loop(); // Loop principal da janela
        void close(); // Fecha a janela
};

#endif