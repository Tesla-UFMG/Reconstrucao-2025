WINDOWS := 1

PROJECT := app
BUILD_FOLDER := build
OBJ_FOLDER := obj

SRC_FILES := $(wildcard src/*.cpp)
SQLITECPP_FILES := $(wildcard lib/SQLiteCpp/*.cpp)
SQLITE3_FILES := $(wildcard lib/sqlite3/*.c)
IMGUI_FILES := $(wildcard lib/imgui/*.cpp)

OBJ_FILES := $(patsubst src/%.cpp, $(OBJ_FOLDER)/%.o, $(SRC_FILES))
OBJ_FILES += $(patsubst lib/SQLiteCpp/%.cpp, $(OBJ_FOLDER)/lib/%.o, $(SQLITECPP_FILES))
OBJ_FILES += $(patsubst lib/sqlite3/%.c, $(OBJ_FOLDER)/lib/%.o, $(SQLITE3_FILES))
OBJ_FILES += $(patsubst lib/imgui/%.cpp, $(OBJ_FOLDER)/lib/%.o, $(IMGUI_FILES))

CXX_FLAGS := -Wall -Wextra -pedantic -std=c++11 
INCLUDES := -I./include -I./lib -I./lib/sqlite3 -I./lib/SQLiteCpp -I./lib/imgui -I./lib/SDL2

ifeq ($(WINDOWS), 1)
	CXX := x86_64-w64-mingw32-g++
	LINKFLAGS := -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -mconsole -static-libgcc -static-libstdc++
	LDFLAGS := -Llib/SDL2
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT).exe
else
	CXX := g++
	LINKFLAGS := -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -static-libgcc -static-libstdc++
	LDFLAGS := 
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT)
endif

all: $(BUILD_FOLDER) $(OBJ_FOLDER) $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	@$(CXX) $(INCLUDES) $(OBJ_FILES) -o $(OUTPUT) $(LDFLAGS) $(LINKFLAGS)
	@echo $@

$(OBJ_FOLDER)/%.o: src/%.cpp $(wildcard include/%.hpp)
	@$(CXX) $(INCLUDES) $(CXX_FLAGS) -c $< -o $@
	@echo $@

$(OBJ_FOLDER)/lib/%.o: lib/*/%.cpp 
	@$(CXX) $(INCLUDES) $(CXX_FLAGS) -c $< -o $@
	@echo $@

$(OBJ_FOLDER)/lib/%.o: lib/*/%.c
	@$(CXX) -x c $< $(INCLUDE_FOLDER) -c -o $@
	@echo $@

ifeq ($(WINDOWS), 1)
$(BUILD_FOLDER) :
	@mkdir -p $@
	cp lib/SDL2/*.dll $(BUILD_FOLDER)
else 
$(BUILD_FOLDER) :
	@mkdir -p $@
endif

$(OBJ_FOLDER) :
	@mkdir -p $@
	@mkdir -p $@/lib

.PHONY: clean run check

check: $(OUTPUT)
	valgrind --leak-check=full --show-leak-kinds=all $(OUTPUT)

run: $(OUTPUT)
	@./$(OUTPUT)

clean:
	rm -rf $(BUILD_FOLDER) $(OBJ_FOLDER)
