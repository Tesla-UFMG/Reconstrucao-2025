#ifndef MATRIX_WINDOW_HPP
#define MATRIX_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Project
#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_Numeric.hpp" // For ColorThresholdConfig
#include "DB.hpp"
#include "Log.hpp"

// One column = one archive/ID. Its rows = variables dragged onto it.
struct MatrixColumn {
    std::string fileType;
    std::string archiveName;
    std::vector<std::string> variables; // rows
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

            // Color mode: 0=none, 1=gradient, 2=thresholds
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

            std::vector<MatrixColumn> m_columns;
            bool m_isOpen = true;

        private:
            void renderGrid();
            void renderColorSettings();
            void processDragDrop();
            ImVec4 getCellColor(double val) const;

            // For column/variable removal via right-click
            int  m_removeColIdx = -1;
            int  m_removeColVarIdx = -1; // col index for variable removal
            int  m_removeVarIdx = -1;
    };
} // namespace Window

#endif // MATRIX_WINDOW_HPP
