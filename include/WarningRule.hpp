#ifndef WARNING_RULE_HPP
#define WARNING_RULE_HPP

#include <string>

struct WarningRule {
    std::string fileType;      // CSV or Telemetry
    std::string fileName;      // filename / packetId
    std::string columnName;    // monitored column
    int conditionType = 0;     // 0 = Valor Específico, 1 = Fora da Faixa, 2 = Dentro da Faixa, 3 = Maior que, 4 = Menor que
    double targetValue = 0.0;
    double minVal = 0.0;
    double maxVal = 0.0;
    float alertColor[4] = {0.8f, 0.1f, 0.1f, 1.0f}; // Red warning color
    std::string description = "";
    int lastProcessedIndex = -1;
    bool wasTriggered = false;
    bool playSound = false;
};

#endif // WARNING_RULE_HPP
