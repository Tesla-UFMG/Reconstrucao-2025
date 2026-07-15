import os

from conan import ConanFile
from conan.tools.files import copy, save


class App(ConanFile):
    required_conan_version = ">=2.0"

    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    generators = "PkgConfigDeps"

    default_options = {
        "*:shared": False,
    
        # SDL remains static.
        "sdl/*:shared": False,
        "sdl_image/*:shared": False,
        "sdl_mixer/*:shared": False,
        "sdl_ttf/*:shared": False,
    
        # System-facing Wayland/EGL boundary.
        "wayland/*:shared": True,
        "xkbcommon/*:shared": True,
        "libdecor/*:shared": True,
        "libffi/*:shared": True,
    
        "sdl/*:x11": True,
        "sdl/*:wayland": True,
        "sdl/*:alsa": True,
        "sdl/*:pulse": False,
    
        "ffmpeg/*:with_pulse": False,
        "ffmpeg/*:with_xcb": True,
        "ffmpeg/*:with_xlib": True,
    }

    def requirements(self):
        self.requires("imgui/1.92.5-docking", override=True)
        self.requires("implot/0.17")

        self.requires("sdl/2.32.10")
        self.requires("sdl_image/2.8.8")
        self.requires("sdl_mixer/2.8.0")
        self.requires("sdl_ttf/2.24.0")

        self.requires("ffmpeg/8.1.2")
        self.requires("rapidcsv/8.80")
        self.requires("sqlite3/3.45.3")

    def generate(self):
        self._copy_imgui_backends()
        self._generate_make_settings()

    def _copy_imgui_backends(self):
        imgui = self.dependencies["imgui"]

        bindings_dir = os.path.join(
            imgui.package_folder,
            "res",
            "bindings",
        )

        destination_include = os.path.join(
            self.recipe_folder,
            "include",
            "imgui_backends",
        )

        destination_source = os.path.join(
            self.recipe_folder,
            "src",
            "imgui_backends",
        )

        copy(
            self,
            "imgui_impl_sdl2.h",
            bindings_dir,
            destination_include,
        )

        copy(
            self,
            "imgui_impl_sdlrenderer2.h",
            bindings_dir,
            destination_include,
        )

        copy(
            self,
            "imgui_impl_sdl2.cpp",
            bindings_dir,
            destination_source,
        )

        copy(
            self,
            "imgui_impl_sdlrenderer2.cpp",
            bindings_dir,
            destination_source,
        )

    def _generate_make_settings(self):
        settings = (
            f"CONAN_OS := {self.settings.os}\n"
            f"CONAN_ARCH := {self.settings.arch}\n"
            f"CONAN_COMPILER := {self.settings.compiler}\n"
            f"CONAN_BUILD_TYPE := {self.settings.build_type}\n"
        )

        save(
            self,
            os.path.join(
                self.generators_folder,
                "conan_settings.mk",
            ),
            settings,
        )
