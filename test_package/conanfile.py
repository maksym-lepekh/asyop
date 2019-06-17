from conans import ConanFile, CMake, tools

class AsyOpSampleConan(ConanFile):
    name = "asyop-test"
    generators = "cmake_paths"

    def test(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
