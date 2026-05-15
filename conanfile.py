from conan import ConanFile
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.env import VirtualBuildEnv

class IoTHubConan(ConanFile):
    name = "iot-hub"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"

    generators = "PkgConfigDeps"

    def requirements(self):
        self.requires("nlohmann_json/3.12.0")  # JSON-протокол IPC / конфігурація
        self.requires("spdlog/[>=1.14 <2]")    # логування
        self.requires("fmt/[>=12.1]")          # форматування рядків, часто використовується зі spdlog
        self.requires("cli11/[>=2.4 <3]")      # аргументи CLI-утиліти
        #self.requires("yaml-cpp/[>=0.8 <1]")   # конфігураційні файли YAML
        self.requires("asio/[>=1.30 <2]")      # async I/O

    def tool_requirements(self):
        self.tool_requires("meson/[>=1.4 <2]")
        self.tool_requires("ninja/[>=1.11 <2]")

    def generate(self):
        tc = MesonToolchain(self)
        tc.generate()

        env = VirtualBuildEnv(self)
        env.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()