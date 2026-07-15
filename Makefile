rwildcard = $(foreach d,$(wildcard $(1)*), \
	$(call rwildcard,$(d)/,$(2)) \
	$(filter $(subst *,%,$(2)),$(d)))

.DEFAULT_GOAL := all

PROJECT := app

BUILD_FOLDER := build
CONAN_FOLDER := $(BUILD_FOLDER)/conan
OBJ_FOLDER := $(BUILD_FOLDER)/obj

CONAN_SETTINGS := $(CONAN_FOLDER)/conan_settings.mk

REQUESTED_GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)
NON_CLEAN_GOALS := $(filter-out clean distclean,$(REQUESTED_GOALS))

ifneq ($(strip $(NON_CLEAN_GOALS)),)
include $(CONAN_SETTINGS)
endif

$(CONAN_SETTINGS): conanfile.py
	@echo "Installing Conan dependencies"
	@mkdir -p $(CONAN_FOLDER)
	conan install . \
		-of $(CONAN_FOLDER) \
		--build=missing

export PKG_CONFIG_PATH := $(abspath $(CONAN_FOLDER))

PKG_CONFIG ?= pkg-config

PKG_DEPS := \
	sdl2 \
	SDL2_image \
	SDL2_mixer \
	SDL2_ttf \
	libavformat \
	libavcodec \
	libswscale \
	libavutil \
	imgui \
	implot \
	rapidcsv \
	sqlite3

SRC_FILES := $(call rwildcard,src/,*.cpp)

LIB_FILES := \
	$(wildcard lib/implot3d/*.cpp) \
	lib/serialib/lib/serialib.cpp \
	lib/tinyfiledialogs/tinyfiledialogs.c

CPP_LIB_FILES := $(filter %.cpp,$(LIB_FILES))
C_LIB_FILES := $(filter %.c,$(LIB_FILES))

OBJ_FILES := $(patsubst src/%.cpp,$(OBJ_FOLDER)/src/%.o,$(SRC_FILES))
OBJ_FILES += $(patsubst lib/%.cpp,$(OBJ_FOLDER)/lib/%.o,$(CPP_LIB_FILES))
OBJ_FILES += $(patsubst lib/%.c,$(OBJ_FOLDER)/lib/%.o,$(C_LIB_FILES))

DEP_FILES := $(OBJ_FILES:.o=.d)

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_DEPS))

PKG_LIBS := $(shell $(PKG_CONFIG) --libs --static $(PKG_DEPS))

PROJECT_INCLUDES := \
	-Iinclude \
	-Isrc \
	-Iinclude/imgui_backends \
	-Ilib/implot3d \
	-Ilib/serialib/lib \
	-Ilib/tinyfiledialogs

CPPFLAGS := $(PROJECT_INCLUDES) $(PKG_CFLAGS)

COMMON_FLAGS := \
	-Wall \
	-Wextra \
	-MMD \
	-MP

CXXFLAGS := $(COMMON_FLAGS) -std=c++17
CFLAGS := $(COMMON_FLAGS) -std=c11

ifeq ($(CONAN_BUILD_TYPE),Debug)
	CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
	CFLAGS += -O0 -g3 -fno-omit-frame-pointer
else ifeq ($(CONAN_BUILD_TYPE),RelWithDebInfo)
	CXXFLAGS += -O2 -g -DNDEBUG -fno-omit-frame-pointer
	CFLAGS += -O2 -g -DNDEBUG -fno-omit-frame-pointer
else ifeq ($(CONAN_BUILD_TYPE),MinSizeRel)
	CXXFLAGS += -Os -DNDEBUG
	CFLAGS += -Os -DNDEBUG
else
	CXXFLAGS += -O3 -DNDEBUG
	CFLAGS += -O3 -DNDEBUG
endif

ifeq ($(CONAN_COMPILER),gcc)
	ifeq ($(origin CC),default)
		CC := gcc
	endif

	ifeq ($(origin CXX),default)
		CXX := g++
	endif
else ifeq ($(CONAN_COMPILER),clang)
	ifeq ($(origin CC),default)
		CC := clang
	endif

	ifeq ($(origin CXX),default)
		CXX := clang++
	endif
else ifeq ($(CONAN_COMPILER),msvc)
	$(error This Makefile does not support MSVC. Use a MinGW/GCC profile on Windows)
endif

LDFLAGS :=
PLATFORM_LIBS :=

ifeq ($(CONAN_COMPILER),gcc)
	LDFLAGS += -static-libgcc -static-libstdc++
endif

ifeq ($(CONAN_OS),Windows)
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT).exe

	LDFLAGS += -static

	PLATFORM_LIBS += \
		-lbcrypt \
		-lcomdlg32 \
		-lole32 \
		-luuid \
		-lws2_32 \
		-lsetupapi
else
	OUTPUT := $(BUILD_FOLDER)/$(PROJECT)

GROUP_BEGIN := -Wl,--start-group
GROUP_END := -Wl,--end-group

.PHONY: all conan clean distclean

all: $(OUTPUT)

conan:
	@mkdir -p $(CONAN_FOLDER)
	conan install . \
		-of $(CONAN_FOLDER) \
		--build=missing

$(OUTPUT): $(OBJ_FILES) | $(BUILD_FOLDER)
	@echo "Linking $@"
	$(CXX) \
		$(LDFLAGS) \
		$(OBJ_FILES) \
		$(GROUP_BEGIN) \
		$(PKG_LIBS) \
		$(GROUP_END) \
		$(PLATFORM_LIBS) \
		-o $@

$(OBJ_FOLDER)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJ_FOLDER)/lib/%.o: lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_FOLDER):
	@mkdir -p $@

clean:
	rm -rf $(OBJ_FOLDER)
	rm -f $(BUILD_FOLDER)/$(PROJECT)
	rm -f $(BUILD_FOLDER)/$(PROJECT).exe

distclean:
	rm -rf $(BUILD_FOLDER)

-include $(DEP_FILES)
