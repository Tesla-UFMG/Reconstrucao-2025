WINDOWS := 1
CARD_VIDEO_RENDEREING := 1

PROJECT := app
BUILD_FOLDER := build
OBJ_FOLDER := obj

SRC_FILES := $(wildcard src/**/**/*.cpp) $(wildcard src/**/*.cpp) $(wildcard src/*.cpp)
LIB_CPP_FILES := $(wildcard lib/**/*.cpp)
LIB_C_FILES := $(wildcard lib/**/*.c)

OBJ_FILES := $(patsubst src/%.cpp, $(OBJ_FOLDER)/%.o, $(SRC_FILES))
OBJ_FILES += $(patsubst lib/%.cpp, $(OBJ_FOLDER)/lib/%.o, $(LIB_CPP_FILES))
OBJ_FILES += $(patsubst lib/%.c, $(OBJ_FOLDER)/lib/%.o, $(LIB_C_FILES))

CXX_FLAGS := -Wall -Wextra -pedantic -std=c++17 -g
INCLUDES := -I./include \
    -I./lib\
  	-I./lib/imgui\
   	-I./lib/SDL2\
    -I./lib/implot\
	-I./lib/tinyDialogs\
	-I./lib/rapidcsv\
	-I./lib/implot3d\
	-I./src/ui/windows\
	-I./lib/serialib\
	-I./lib/sqlite3

ifeq ($(WINDOWS), 1)
	CXX := x86_64-w64-mingw32-g++
	LINKFLAGS := -lmingw32 -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lbcrypt -mconsole -static-libgcc -static-libstdc++ -lcomdlg32 -lole32
	LDFLAGS := -Llib/SDL2
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT).exe
else
	CXX := g++
	LINKFLAGS := -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lz -lpthread -lm -static-libgcc -static-libstdc++
	LDFLAGS := 
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT)
endif

ifeq ($(CARD_VIDEO_RENDEREING), 1)
CXX_FLAGS += -DACCELERATED
endif


all: $(BUILD_FOLDER) $(OBJ_FOLDER) $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	@echo "Compilando o executável" $@
	@$(CXX) $(OBJ_FILES) $(LDFLAGS) $(LINKFLAGS) -o $(OUTPUT)

$(OBJ_FOLDER)/%.o: src/%.cpp $(wildcard include/**/*.hpp) $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: lib/%.c
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) -x c $(INCLUDES) -c $< -o $@

ifeq ($(WINDOWS), 1)
$(BUILD_FOLDER):
	@mkdir -p $@ $@/assets
	cp lib/SDL2/*.dll $(BUILD_FOLDER)
	cp assets/* $(BUILD_FOLDER)/assets/ 

	@mkdir -p $@ $@/maps
	cp maps/* $(BUILD_FOLDER)/maps/
else 
$(BUILD_FOLDER):
	@mkdir -p $@ $@/assets
	cp assets/* $(BUILD_FOLDER)/assets/

	@mkdir -p $@ $@/maps
	cp maps/* $(BUILD_FOLDER)/maps/
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
	rm -rf $(BUILD_FOLDER) $(OBJ_FOLDER) log.txt check.txt data.db3 Reconstrucao.zip cache telemetry output

copy:
	zip -r Reconstrucao.zip $(BUILD_FOLDER)

format:
	@find src include -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +