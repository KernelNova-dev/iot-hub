# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

from conan import ConanFile
from conan.tools.gnu import PkgConfigDeps
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.env import VirtualBuildEnv

class WireGateConan(ConanFile):
    name = "wiregate"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("nlohmann_json/3.12.0")  # JSON-протокол IPC / конфігурація
        self.requires("spdlog/[>=1.14 <2]")    # логування
        self.requires("cli11/[>=2.4 <3]")      # аргументи CLI-утиліти
        self.requires("asio/1.29.0")           # async I/O shared by IPC and Crow
        self.requires("crowcpp-crow/1.3.2")    # embedded web server for dashboard

    def build_requirements(self):
        self.tool_requires("meson/[>=1.4 <2]")
        self.tool_requires("ninja/[>=1.11 <2]")

    def generate(self):
        # Meson
        tc = MesonToolchain(self)
        tc.generate()
        # Venv
        env = VirtualBuildEnv(self)
        env.generate()
        # PkgConfig
        deps = PkgConfigDeps(self)
        deps.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()
