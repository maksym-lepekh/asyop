from conans import ConanFile, CMake, tools


class AsyOpConan(ConanFile):
    name = "asyop"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_paths"
    requires = "Catch2/2.7.1@catchorg/stable", "asio/1.13.0@bincrafters/stable"

    def source(self):
        self.run("git clone https://github.com/maksym-lepekh/asyop.git")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="hello")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)
