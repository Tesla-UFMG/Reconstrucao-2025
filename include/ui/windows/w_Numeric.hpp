#ifndef NUMERIC_WINDOW_HPP
#define NUMERIC_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>
#include <limits>

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

struct ColorThresholdConfig {
    bool enabled = false;
    float bg[4] = {0.15f, 0.15f, 0.15f, 1.0f};
    float fg[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct SpecificColorRule {
    double value = 0.0;
    float bg[4] = {0.15f, 0.15f, 0.15f, 1.0f};
    float fg[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct TranslationRule {
    double value = 0.0;
    std::string text = "";
};

namespace Window {
    class Numeric : public IWindow {
        public:
            explicit Numeric(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }
            virtual std::string getDynamicType() const override { return "Numeric"; }

            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            std::string getTitle() const { return title; }
            void setTitle(const std::string& t) { title = t; }
            bool hasData() const { return m_hasData; }
            std::string getColumnName() const { return m_loadedColumns.empty() ? "" : m_loadedColumns.front().column; }
            std::string getArchiveName() const { return m_loadedColumns.empty() ? "" : m_loadedColumns.front().archive; }
            std::string getFileType() const { return m_loadedColumns.empty() ? "" : m_loadedColumns.front().fileType; }
            MetricType getMetricType() const { return m_currentMetric; }
            void setMetricType(MetricType metric) { m_currentMetric = metric; }
            const std::vector<NumericData>& getLoadedColumns() const { return m_loadedColumns; }

            // Customizable properties
            char m_prefix[64] = "";
            char m_suffix[64] = "";

            bool m_useFormula = false;
            double m_multiplier = 1.0;
            double m_offset = 0.0;

            bool m_useTranslation = false;
            std::vector<TranslationRule> m_translationRules;

            int m_colorMode = 0; // 0 = Nenhuma, 1 = Por Faixas, 2 = Valores Específicos, 3 = Gradiente Dinâmico
            double m_threshLL = 0.0;
            double m_threshL = 0.0;
            double m_threshH = 0.0;
            double m_threshHH = 0.0;
            ColorThresholdConfig m_confLL;
            ColorThresholdConfig m_confL;
            ColorThresholdConfig m_confNormal;
            ColorThresholdConfig m_confH;
            ColorThresholdConfig m_confHH;
            std::vector<SpecificColorRule> m_specificRules;

            // Continuous Gradient Mode properties
            double m_gradMinVal = 20.0;
            double m_gradMaxVal = 80.0;
            float m_gradMinColor[4] = {0.0f, 0.4f, 1.0f, 1.0f}; // Blue
            float m_gradMaxColor[4] = {1.0f, 0.1f, 0.1f, 1.0f}; // Red

            float m_fontScale = 1.0f;
            bool m_showColumnName = true;
            char m_stripPattern[64] = "";
            bool m_statModeAll = true;
            char m_customLabel[128] = "";

        private:
            void processColumnDragDrop();

            bool                     m_isOpen = true;
            bool                     m_hasData = false;
            std::vector<NumericData> m_loadedColumns;
            MetricType               m_currentMetric = MetricType::LAST;
    };
} // namespace Window

#endif // NUMERIC_WINDOW_HPP
