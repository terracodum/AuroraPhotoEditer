from conan import ConanFile

class Application(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "PkgConfigDeps"

    def requirements(self):
        self.requires("onnxruntime/1.18.1@aurora")
        self.requires("opencv-core-imgcodecs/4.9.0@aurora")
        self.requires("eigen/3.4.0@aurora#5004c183fe022fc122e6f936462413a7", override=True)
        self.requires("fxdiv/cci.20200417@aurora#3d2e9024da7bf8188e59fb8ab6491204", override=True)
        self.requires("nlohmann_json/3.11.3@aurora#90005bbeeff6c2749ad9071a892b4e3d", override=True)
