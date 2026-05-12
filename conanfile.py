from conan import ConanFile
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.env import VirtualBuildEnv

class IoTHubConan(ConanFile):
    name = "iot-hub"
    version = "1.0"

    settings = "os", "compiler", "build_type", "arch"

    generators = "PkgConfigDeps"

    def requirements(self):
        # Example dependencies (add later as needed)
        # self.requires("fmt/10.2.1")
        pass

    def generate(self):
        tc = MesonToolchain(self)
        tc.generate()

        env = VirtualBuildEnv(self)
        env.generate()

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()