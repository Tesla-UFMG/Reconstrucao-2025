#define SDL_MAIN_HANDLED

// Project
#include "Window.hpp"
#include "Log.hpp"
#include "DB.hpp"

// C++
#include <iostream>

int main() {
    const std::string WINDOW_TITLE  = "Resconstrução de Pista";
    const int         WINDOW_WIDTH  = 1280;
    const int         WINDOW_HEIGHT = 720;

    Window window;
    window.init(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);
    window.loop();
    window.close();

    DB& db = DB::getInstance();
        
    return 0;
}
