#ifndef IMGUIWRAPPER_HPP
#define IMGUIWRAPPER_HPP

// C++
#include <iostream>
#include <stdexcept>
#include <string>

// Project
#include "Log.hpp"
#include "SDLWrapper.hpp"

// Third party
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

class ImGuiWrapper {
    private:
        static bool     isSubsystemInited;
        static ImGuiIO* io;

    public:
        static void initSubsystem();      // Inicia todos os subsistemas do ImGui
        static void prepareForNewFrame(); // Prepara o ImGui para um novo frame
        static void render();             // Renderiza as janelas do ImGui
        static void closeSubystem();      // Fecha todos os subsistemas do ImGui
};

#endif