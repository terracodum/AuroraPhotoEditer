from conan import ConanFile

class Application(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "PkgConfigDeps"
    default_options = {"*:shared": False}

    def requirements(self):
        self.requires("onnxruntime/1.18.1@aurora")
