#ifndef WARNING_WINDOW_HPP
#define WARNING_WINDOW_HPP

// C++
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

// Project
#include "ui/windows/iWindow.hpp"
#include "DB.hpp"
#include "Log.hpp"

#include "WarningRule.hpp"

struct LoggedWarning {
    std::string timestamp;     // formatted date/time
    std::string variableName;
    std::string archiveName;
    std::string conditionText;
    double valueReached = 0.0;
    float color[4] = {0.8f, 0.1f, 0.1f, 1.0f};
    std::string description = "";
};

namespace Window {
    class Warning : public IWindow {
        public:
            explicit Warning(bool* isOpen = nullptr);
            virtual ~Warning();
            virtual void render() override;

            static Warning* getInstance();

            const std::vector<WarningRule>& getRules() const { return m_rules; }
            void setRules(const std::vector<WarningRule>& rules) { m_rules = rules; }

            void addRule(const WarningRule& rule);
            void removeRule(size_t index);
            void clearLogs();
            void exportToCSV(const std::string& filepath);

        private:
            void evaluateRules();
            void triggerWarning(const WarningRule& rule, double value);

            static Warning* s_instance;

            std::vector<WarningRule> m_rules;
            std::vector<LoggedWarning> m_logs;

            // UI Temporary States for Rule Creator
            int m_selectedFileIdx = -1;
            std::vector<bool> m_selectedCols;
            int m_selectedCondType = 3;   // Default to 'Maior que'
            double m_tempTargetValue = 100.0;
            double m_tempMinVal = 0.0;
            double m_tempMaxVal = 100.0;
            float m_tempColor[4] = {0.8f, 0.1f, 0.1f, 1.0f};
            char m_tempDesc[128] = "";
    };
} // namespace Window

#endif // WARNING_WINDOW_HPP
