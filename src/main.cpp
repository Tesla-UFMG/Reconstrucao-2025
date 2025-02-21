#define SDL_MAIN_HANDLED

// Project
#include "App.hpp"
#include "DB.hpp"
#include "Log.hpp"

// C++
#include <iostream>

int main() {
    const std::string WINDOW_TITLE  = "Resconstrução de Pista";
    const int         WINDOW_WIDTH  = 1280;
    const int         WINDOW_HEIGHT = 720;

    App app;
    app.init(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT);
    app.loop();
    app.close();

    return 0;
}
