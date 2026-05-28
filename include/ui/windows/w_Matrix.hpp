#ifndef MATRIX_WINDOW_HPP
#define MATRIX_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>

// Project
#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_Numeric.hpp" // For ColorThresholdConfig, SpecificColorRule, TranslationRule
#include "DB.hpp"
#include "Log.hpp"

// One column = one archive/ID.
struct MatrixColumn {
    std::string fileType;
    std::string archiveName;
    std::vector<std::string> variables; // Kept to match existing serialization structures
};

namespace Window {
    class Matrix : public IWindow {
        public:
            explicit Matrix(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }
            virtual std::string getDynamicType() const override { return "Matrix"; }

            std::string getTitle() const { return title; }
            void setTitle(const std::string& t) { title = t; }

            // Color mode: 0=none, 1=thresholds, 2=specific, 3=gradient
            int    m_colorMode = 0;
            double m_minVal    = 0.0;
            double m_maxVal    = 100.0;
            float  m_minColor[4] = {0.0f, 0.4f, 1.0f, 1.0f}; // cold = blue
            float  m_maxColor[4] = {1.0f, 0.1f, 0.1f, 1.0f}; // hot  = red

            double m_threshLL = 3.0;
            double m_threshL  = 3.2;
            double m_threshH  = 4.0;
            double m_threshHH = 4.2;
            ColorThresholdConfig m_confLL;
            ColorThresholdConfig m_confL;
            ColorThresholdConfig m_confNormal;
            ColorThresholdConfig m_confH;
            ColorThresholdConfig m_confHH;
            std::vector<SpecificColorRule> m_specificRules;

            std::vector<MatrixColumn> m_columns;
            std::vector<std::string> m_rowVariables; // Global aligned variables (rows)
            float m_fontScale = 1.0f;
            bool m_showVariableName = true;
            bool m_isOpen = true;

            // Customization Properties
            char m_suffix[64] = "";
            bool m_useFormula = false;
            double m_multiplier = 1.0;
            double m_offset = 0.0;
            bool m_useTranslation = false;
            std::vector<TranslationRule> m_translationRules;

        private:
            void renderGrid();
            void processDragDrop();
            ImVec4 getCellColor(double val) const;
            ImVec4 getCellTextColor(double val) const;
            std::string getIDDisplayName(const std::string& archiveName, const std::string& fileType) const;
    };
} // namespace Window

#endif // MATRIX_WINDOW_HPP
