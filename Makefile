WINDOWS := 1

PROJECT := app
BUILD_FOLDER := build
OBJ_FOLDER := obj

SRC_FILES := $(wildcard src/**/*.cpp) $(wildcard src/*.cpp)
LIB_CPP_FILES := $(wildcard lib/**/*.cpp)
LIB_C_FILES := $(wildcard lib/**/*.c)

OBJ_FILES := $(patsubst src/%.cpp, $(OBJ_FOLDER)/%.o, $(SRC_FILES))
OBJ_FILES += $(patsubst lib/%.cpp, $(OBJ_FOLDER)/lib/%.o, $(LIB_CPP_FILES))
OBJ_FILES += $(patsubst lib/%.c, $(OBJ_FOLDER)/lib/%.o, $(LIB_C_FILES))

CXX_FLAGS := -Wall -Wextra -pedantic -std=c++17
INCLUDES := -I./include -I./lib -I./lib/sqlite3 -I./lib/SQLiteCpp -I./lib/imgui -I./lib/SDL2 -I./lib/implot -I./lib/tinyDialogs

ifeq ($(WINDOWS), 1)
    CXX := x86_64-w64-mingw32-g++
    LINKFLAGS := -lmingw32 -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -mconsole -static-libgcc -static-libstdc++ -lcomdlg32 -lole32
    LDFLAGS := -Llib/SDL2
    OUTPUT := $(BUILD_FOLDER)/$(PROJECT).exe
else
    CXX := g++
    LINKFLAGS := -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -static-libgcc -static-libstdc++
    LDFLAGS := 
    OUTPUT := $(BUILD_FOLDER)/$(PROJECT)
endif

all: $(BUILD_FOLDER) $(OBJ_FOLDER) $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	@echo "Compilando o executável" $@
	@$(CXX) $(INCLUDES) $(OBJ_FILES) -o $(OUTPUT) $(LDFLAGS) $(LINKFLAGS)

# Regra para compilar arquivos .cpp do src/
$(OBJ_FOLDER)/%.o: src/%.cpp $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

# Regra para compilar arquivos .cpp dentro de lib/
$(OBJ_FOLDER)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

# Regra para compilar arquivos .c dentro de lib/
$(OBJ_FOLDER)/lib/%.o: lib/%.c
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) -x c $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

# Criar pastas de compilação e copiar DLLs se for Windows
ifeq ($(WINDOWS), 1)
$(BUILD_FOLDER):
	@mkdir -p $@
	cp lib/SDL2/*.dll $(BUILD_FOLDER)
else 
$(BUILD_FOLDER):
	@mkdir -p $@
endif

$(OBJ_FOLDER):
	@mkdir -p $@
	@mkdir -p $(OBJ_FOLDER)/lib

.PHONY: clean run check copy format

check: all
	valgrind --leak-check=full --show-leak-kinds=all $(OUTPUT) 2> check.txt

run: all
	@rm -rf log.txt
	@echo "Executando."
	@./$(OUTPUT)

clean:
	rm -rf $(BUILD_FOLDER) $(OBJ_FOLDER) log.txt check.txt data.db3 Reconstrucao.zip

copy:
	zip -r Reconstrucao.zip $(BUILD_FOLDER)

format:
	@find src include -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +

