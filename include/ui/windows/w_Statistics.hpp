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

struct Metric {
        std::string                unique_id;
        std::string                display_name;
        std::string                fileName;
        std::string                fileType;
        const std::vector<double>* data;
};

namespace Window {
    class Statistics : public IWindow {
        private:
            std::vector<Metric> metrics;
            bool                m_isOpen = true;

            void processColumnDragDrop();
            void renderTable();

        public:
            explicit Statistics(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }
            virtual std::string getDynamicType() const override { return "Tabela"; }

            std::string getTitle() const { return title; }
            const std::vector<Metric>& getMetrics() const { return metrics; }
            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
    };

} // namespace Window

#endif