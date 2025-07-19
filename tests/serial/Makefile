WINDOWS := 1
CARD_VIDEO_RENDEREING := 1

PROJECT      := app
BUILD_FOLDER := build
OBJ_FOLDER   := obj

LIB_FOLDER := ../../lib

SRC_FILES      := $(wildcard src/**/**/*.cpp) $(wildcard src/**/*.cpp) $(wildcard src/*.cpp)
LIB_CPP_FILES  := $(wildcard $(LIB_FOLDER)/serialib/*.cpp)
LIB_C_FILES    := $(wildcard $(LIB_FOLDER)/serialib/*.c)

OBJ_FILES := $(patsubst src/%.cpp, $(OBJ_FOLDER)/%.o, $(SRC_FILES))
OBJ_FILES += $(patsubst $(LIB_FOLDER)/%.cpp, $(OBJ_FOLDER)/lib/%.o, $(LIB_CPP_FILES))
OBJ_FILES += $(patsubst $(LIB_FOLDER)/%.c,   $(OBJ_FOLDER)/lib/%.o, $(LIB_C_FILES))

CXX_FLAGS := -Wall -Wextra -pedantic -std=c++17
INCLUDES  := -I./include \
             -I$(LIB_FOLDER) \
             -I$(LIB_FOLDER)/serialib 

ifeq ($(WINDOWS), 1)
  CXX       := x86_64-w64-mingw32-g++
  LINKFLAGS := -lmingw32 -mconsole -static-libgcc -static-libstdc++ 
  LDFLAGS   := 
  OUTPUT    := $(BUILD_FOLDER)/$(PROJECT).exe
else
  CXX       := g++
  LINKFLAGS := -static-libgcc -static-libstdc++
  LDFLAGS   := 
  OUTPUT    := $(BUILD_FOLDER)/$(PROJECT)
endif

ifeq ($(CARD_VIDEO_RENDEREING), 1)
  CXX_FLAGS += -DACCELERATED
endif

all: $(BUILD_FOLDER) $(OBJ_FOLDER) $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	@echo "Compilando o executável $@"
	@$(CXX) $(OBJ_FILES) $(LDFLAGS) $(LINKFLAGS) -o $@

$(OBJ_FOLDER)/%.o: src/%.cpp $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: $(LIB_FOLDER)/%.cpp
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: $(LIB_FOLDER)/%.c
	@mkdir -p $(dir $@)
	@echo $@
	@$(CXX) -x c $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

# Cria pastas de build e copia DLLs no Windows
ifeq ($(WINDOWS), 1)
$(BUILD_FOLDER):
	@mkdir -p $@
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
	@rm -f log.txt
	@echo "Executando."
	@./$(OUTPUT)

clean:
	rm -rf $(BUILD_FOLDER) $(OBJ_FOLDER) cache

copy:
	zip -r Reconstrucao.zip $(BUILD_FOLDER)

format:
	@find src include -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
