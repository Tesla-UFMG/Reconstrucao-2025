import re

with open("src/ui/windows/w_Pedal.cpp", "r") as f:
    content = f.read()

content = re.sub(
    r'if \(fileType == "CSV"\) \{\n\s*const std::vector<double>\* dataPtr = &DB::getInstance\(\)\.getCSVData\(fileName, columnName\);\n\s*\} else if \(fileType == "Telemetry"\) \{\n\s*const std::vector<double>\* dataPtr = &DB::getInstance\(\)\.getTelemetryData\(fileName, columnName\);\n\s*\}',
    r'const std::vector<double>* dataPtr = nullptr;\n    if (fileType == "CSV") {\n        dataPtr = &DB::getInstance().getCSVData(fileName, columnName);\n    } else if (fileType == "Telemetry") {\n        dataPtr = &DB::getInstance().getTelemetryData(fileName, columnName);\n    }',
    content
)

with open("src/ui/windows/w_Pedal.cpp", "w") as f:
    f.write(content)
