#ifndef IWINDOW_HPP
#define IWINDOW_HPP

// Project
#include "DataFiles.hpp"
#include "Dialogs.hpp"
#include "ImGuiWrapper.hpp"

// C++
#include <string>

class IWindow {
    protected:
        ImGuiWindowFlags flags  = 0;
        bool*            isOpen = nullptr;
        std::string      title  = "";

    public:
        explicit IWindow(bool* isOpen = nullptr) { this->setupVisibility(isOpen); }
        virtual ~IWindow() = default;

        virtual void setupVisibility(bool* isOpen) { this->isOpen = isOpen; }
        virtual void render() = 0;
        virtual bool isDynamic() const { return false; }
        bool getIsOpen() const { return isOpen ? *isOpen : false; }
};

#endif