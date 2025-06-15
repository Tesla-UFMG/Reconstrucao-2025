#ifndef STATISTICS_WINDOW_HPP
#define STATISTICS_WINDOW_HPP

// Project
#include "ImGuiWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"
#include "DB.hpp"

namespace Window {
    class Statistics : public IWindow {
        private:
            void processColumnDragDrop();
            void renderMenuBar();
            void renderTable();

        public:
            explicit Statistics(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif