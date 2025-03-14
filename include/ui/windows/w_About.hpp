#ifndef ABOUT_WINDOW_HPP
#define ABOUT_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <string>
#include <vector>

namespace Window {
    class About : public IWindow {
        public:
            explicit About(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            std::vector<std::string> developers;
    };

} // namespace Window

#endif