#ifndef STATISTICS_WINDOW_HPP
#define STATISTICS_WINDOW_HPP

// C++
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

// Project
#include "DB.hpp"
#include "DataFiles.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/menubar/m_Utils.hpp"
#include "ui/windows/iWindow.hpp"

// Defines
#define HISTORY_SIZE 200

struct Metric {
        std::string                unique_id;
        std::string                display_name;
        const std::vector<double>* data;
};

namespace Window {
    class Statistics : public IWindow {
        private:
            std::vector<Metric> metrics;

            void processColumnDragDrop();
            void renderMenuBar();
            void renderTable();
            void renderGraph(const Metric& metric, size_t i);

        public:
            explicit Statistics(bool* isOpen = nullptr);
            virtual void render() override;
    };

} // namespace Window

#endif