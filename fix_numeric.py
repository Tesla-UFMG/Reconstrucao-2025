import re

with open("src/ui/windows/w_Numeric.cpp", "r") as f:
    content = f.read()

# Replace col.data usage
content = re.sub(r'if \(col\.data && !col\.data->empty\(\)\)',
                 r'const std::vector<double>* dataPtr = nullptr;\n                        if (col.fileType == "CSV") dataPtr = &DB::getInstance().getCSVData(col.archive, col.column);\n                        else if (col.fileType == "Telemetry") dataPtr = &DB::getInstance().getTelemetryData(col.archive, col.column);\n                        if (dataPtr && !dataPtr->empty())',
                 content)
content = re.sub(r'col\.data->begin\(\)', r'dataPtr->begin()', content)
content = re.sub(r'col\.data->end\(\)', r'dataPtr->end()', content)
content = re.sub(r'col\.data->back\(\)', r'dataPtr->back()', content)
content = re.sub(r'col\.data->size\(\)', r'dataPtr->size()', content)
content = re.sub(r'\*col\.data', r'*dataPtr', content)

# Fix it->data usage
content = re.sub(r'if \(it->data && !it->data->empty\(\)\)',
                 r'const std::vector<double>* dataPtr = nullptr;\n                if (it->fileType == "CSV") dataPtr = &DB::getInstance().getCSVData(it->archive, it->column);\n                else if (it->fileType == "Telemetry") dataPtr = &DB::getInstance().getTelemetryData(it->archive, it->column);\n                if (dataPtr && !dataPtr->empty())',
                 content)
content = re.sub(r'it->data->back\(\)', r'dataPtr->back()', content)

# Fix colData.data = ...
content = re.sub(r'colData\.data = &DB::getInstance\(\)\.getCSVData\(fileName, columnName\);',
                 r'const std::vector<double>* dataPtr = &DB::getInstance().getCSVData(fileName, columnName);',
                 content)
content = re.sub(r'colData\.data = &DB::getInstance\(\)\.getTelemetryData\(fileName, columnName\);',
                 r'const std::vector<double>* dataPtr = &DB::getInstance().getTelemetryData(fileName, columnName);',
                 content)
content = re.sub(r'if \(colData\.data\)', r'if (dataPtr)', content)
content = re.sub(r'colData\.data->size\(\)', r'dataPtr->size()', content)

with open("src/ui/windows/w_Numeric.cpp", "w") as f:
    f.write(content)
