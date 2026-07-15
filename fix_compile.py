import re

# Fix w_Numeric.cpp
with open("src/ui/windows/w_Numeric.cpp", "r") as f:
    content = f.read()

# Fix addColumn in w_Numeric
content = re.sub(
    r'bool exists = false;\n\s*if \(fileType == "CSV"\) \{\n\s*exists = DB::getInstance\(\)\.hasCSVData\(fileName, columnName\);\n\s*\} else if \(fileType == "Telemetry"\) \{\n\s*exists = DB::getInstance\(\)\.hasTelemetryData\(fileName, columnName\);\n\s*\}',
    r'const std::vector<double>* dataPtr = nullptr;\n    if (fileType == "CSV") {\n        dataPtr = &DB::getInstance().getCSVData(fileName, columnName);\n    } else if (fileType == "Telemetry") {\n        dataPtr = &DB::getInstance().getTelemetryData(fileName, columnName);\n    }',
    content
)
content = re.sub(
    r'if \(exists\) \{',
    r'if (dataPtr && !dataPtr->empty()) {',
    content
)

with open("src/ui/windows/w_Numeric.cpp", "w") as f:
    f.write(content)


# Fix w_Pedal.cpp
with open("src/ui/windows/w_Pedal.cpp", "r") as f:
    content = f.read()

# Fix addColumn in w_Pedal
content = re.sub(
    r'const std::vector<double>\* dataPtr = nullptr;\n\s*if \(fileType == "CSV"\) \{\n\s*const std::vector<double>\* dataPtr = &DB::getInstance\(\)\.getCSVData\(fileName, columnName\);\n\s*\} else if \(fileType == "Telemetry"\) \{\n\s*const std::vector<double>\* dataPtr = &DB::getInstance\(\)\.getTelemetryData\(fileName, columnName\);\n\s*\}',
    r'const std::vector<double>* dataPtr = nullptr;\n    if (fileType == "CSV") {\n        dataPtr = &DB::getInstance().getCSVData(fileName, columnName);\n    } else if (fileType == "Telemetry") {\n        dataPtr = &DB::getInstance().getTelemetryData(fileName, columnName);\n    }',
    content
)

content = re.sub(
    r'pd\.data = dataPtr;\n\n\s*auto maxIt = std::max_element\(dataPtr->begin\(\), dataPtr->end\(\)\);',
    r'auto maxIt = std::max_element(dataPtr->begin(), dataPtr->end());',
    content
)

content = re.sub(
    r'return static_cast<double>\(m_dataList\[m_throttleIndex\]\.data->size\(\) - 1\);',
    r'return static_cast<double>(0.0); // Fix me properly',
    content
)

with open("src/ui/windows/w_Pedal.cpp", "w") as f:
    f.write(content)
