from conans import ConanFile, CMake, tools


class AsyOpConan(ConanFile):
    name = "asyop"
    description = "C++17 library for asynchronous operations"
    license = "https://github.com/maksym-lepekh/asyop/blob/master/LICENSE"
    url = "https://maksym-lepekh.github.io/asyop/"
    version = "0.2"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_paths"

    options = {
        "build_tests": [False, True],
        "asio_support": [False, True]
    }

    default_options = {
        "build_tests": False,
        "asio_support": True
    }

    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def requirements(self):
        if self.options.build_tests:
            self.requires("Catch2/2.9.0@catchorg/stable")
        if self.options.asio_support:
            self.requires("asio/1.13.0@bincrafters/stable")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder='.' if self.options.build_tests else 'lib')
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        self.copy("LICENSE", dst="licenses")
