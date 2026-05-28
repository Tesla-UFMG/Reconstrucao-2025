#ifndef BAR_WINDOW_HPP
#define BAR_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

// Project
#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_Numeric.hpp"
#include "DB.hpp"
#include "Log.hpp"

struct BarData {
    std::string         column;
    std::string         archive;
    std::string         fileType;
    const std::vector<double>* data = nullptr;
};

struct BarThresholdConfig {
    bool enabled = false;
    float color[4] = {0.0f, 0.8f, 0.0f, 1.0f};
};

namespace Window {
    class Bar : public IWindow {
        public:
            explicit Bar(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }
            virtual std::string getDynamicType() const override { return "Bar"; }

            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            std::string getTitle() const { return title; }
            void setTitle(const std::string& t) { title = t; }
            bool hasData() const { return m_hasData; }
            std::string getColumnName() const { return m_loadedData.column; }
            std::string getArchiveName() const { return m_loadedData.archive; }
            std::string getFileType() const { return m_loadedData.fileType; }
            MetricType getMetricType() const { return m_currentMetric; }
            void setMetricType(MetricType metric) { m_currentMetric = metric; }

            // Configurations
            int m_orientation = 0; // 0 = Vertical, 1 = Horizontal
            bool m_useManualLimits = false;
            double m_minVal = 0.0;
            double m_maxVal = 100.0;

            float m_barColor[4] = {0.0f, 0.8f, 0.0f, 1.0f};
            float m_bgColor[4] = {0.15f, 0.15f, 0.15f, 1.0f};
            float m_fgColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};

            float m_fontScale = 1.0f;
            bool m_showPercentage = true;
            bool m_showValue = true;
            bool m_showColumnName = true;
            char m_stripPattern[64] = "";
            char m_prefix[64] = "";
            char m_suffix[64] = "";

            // Thresholds
            bool m_useThresholds = false;
            double m_threshLL = 0.0;
            double m_threshL = 0.0;
            double m_threshH = 0.0;
            double m_threshHH = 0.0;
            BarThresholdConfig m_confLL;
            BarThresholdConfig m_confL;
            BarThresholdConfig m_confNormal;
            BarThresholdConfig m_confH;
            BarThresholdConfig m_confHH;

            // Continuous Gradient Configs
            bool   m_useGradient  = false;  // kept for serialization compat
            int    m_colorBarMode  = 0;     // 0=none, 1=gradient, 2=thresholds (unified)
            double m_gradMinVal = 0.0;
            double m_gradMaxVal = 100.0;
            float m_gradMinColor[4] = {0.0f, 0.4f, 1.0f, 1.0f}; // Blue/Cyan
            float m_gradMaxColor[4] = {1.0f, 0.1f, 0.1f, 1.0f}; // Red

        private:
            void processColumnDragDrop();

            bool        m_isOpen = true;
            bool        m_hasData = false;
            BarData     m_loadedData;
            MetricType  m_currentMetric = MetricType::LAST;
    };
} // namespace Window

#endif // BAR_WINDOW_HPP
