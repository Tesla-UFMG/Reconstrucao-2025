#ifndef NUMERIC_WINDOW_HPP
#define NUMERIC_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>

// Project
#include "ui/windows/iWindow.hpp"
#include "DB.hpp"
#include "Log.hpp"

struct NumericData {
    std::string         column;
    std::string         archive;
    std::string         fileType;
    const std::vector<double>* data = nullptr;
};

enum class MetricType {
    LAST,
    AVERAGE,
    MIN,
    MAX
};

namespace Window {
    class Numeric : public IWindow {
        public:
            explicit Numeric(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }

            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            std::string getTitle() const { return title; }
            bool hasData() const { return m_hasData; }
            std::string getColumnName() const { return m_loadedData.column; }
            std::string getArchiveName() const { return m_loadedData.archive; }
            std::string getFileType() const { return m_loadedData.fileType; }
            MetricType getMetricType() const { return m_currentMetric; }
            void setMetricType(MetricType metric) { m_currentMetric = metric; }

        private:
            void processColumnDragDrop();

            bool        m_isOpen = true;
            bool        m_hasData = false;
            NumericData m_loadedData;
            MetricType  m_currentMetric = MetricType::LAST;
    };
} // namespace Window

#endif // NUMERIC_WINDOW_HPP
