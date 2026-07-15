import re

with open("src/ui/windows/w_Pedal.cpp", "r") as f:
    content = f.read()

# Fix throttleIndex block
content = re.sub(r'if \(!pedalData\.data->empty\(\)\) {',
                 r'const std::vector<double>* dataPtr = nullptr;\n        if (pedalData.fileType == "CSV") dataPtr = &DB::getInstance().getCSVData(pedalData.archive, pedalData.column);\n        else if (pedalData.fileType == "Telemetry") dataPtr = &DB::getInstance().getTelemetryData(pedalData.archive, pedalData.column);\n        if (dataPtr && !dataPtr->empty()) {',
                 content)
content = re.sub(r'pedalData\.data->back\(\)', r'dataPtr->back()', content)
content = re.sub(r'pedalData\.data->size\(\)', r'dataPtr->size()', content)

# Fix addColumn
content = re.sub(r'pd\.data = &DB::getInstance\(\)\.getCSVData\(fileName, columnName\);',
                 r'const std::vector<double>* dataPtr = &DB::getInstance().getCSVData(fileName, columnName);',
                 content)
content = re.sub(r'pd\.data = &DB::getInstance\(\)\.getTelemetryData\(fileName, columnName\);',
                 r'const std::vector<double>* dataPtr = &DB::getInstance().getTelemetryData(fileName, columnName);',
                 content)
content = re.sub(r'if \(!pd\.data\)', r'if (!dataPtr)', content)
content = re.sub(r'pd\.data->begin\(\)', r'dataPtr->begin()', content)
content = re.sub(r'pd\.data->end\(\)', r'dataPtr->end()', content)
content = re.sub(r'pd\.data->size\(\)', r'dataPtr->size()', content)

with open("src/ui/windows/w_Pedal.cpp", "w") as f:
    f.write(content)
