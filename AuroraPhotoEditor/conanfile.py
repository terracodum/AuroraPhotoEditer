from conan import ConanFile

class Application(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "PkgConfigDeps"
    default_options = {"*:shared": False}

    def requirements(self):
        self.requires("tensorflow-lite/2.16.2@aurora")
        self.requires("opencv-core-imgcodecs/4.9.0@aurora")
