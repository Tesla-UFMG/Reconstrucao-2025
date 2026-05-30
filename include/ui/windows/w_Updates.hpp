#ifndef UPDATES_WINDOW_HPP
#define UPDATES_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <string>
#include <vector>
#include <filesystem>

namespace Window {
    class Updates : public IWindow {
        public:
            explicit Updates(bool* isOpen = nullptr);
            virtual void render() override;

        private:
            int m_currentPage = 0;
            bool m_dontShowAgain = false;
            bool m_popupOpen = false;

            std::filesystem::path getHideFilePath() const;
            bool checkHideFileExists() const;
            void writeHideFile();
            void removeHideFile();
    };
} // namespace Window

#endif // UPDATES_WINDOW_HPP
