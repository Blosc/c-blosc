from conans import ConanFile, CMake, tools
import os


class CbloscConan(ConanFile):
    name = "c-blosc"
    description = "An extremely fast, multi-threaded, meta-compressor library"
    try:
        if os.name == 'nt':
            version = os.environ['APPVEYOR_REPO_TAG_NAME']
        else:
            version = os.environ['TRAVIS_TAG']
    except:
        pass
    license = "BSD"
    url = "https://github.com/Blosc/c-blosc"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    
    def source(self):
        self.run("git clone https://github.com/Blosc/c-blosc.git")
        # patch to ensure compatibility
        tools.replace_in_file("c-blosc/CMakeLists.txt", "PROJECT(blosc)", '''PROJECT(blosc)
            include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
            conan_basic_setup()''')
    
    def build(self):
        cmake = CMake(self)
        shared = "-DBUILD_SHARED_LIBS=ON" if self.options.shared else ""
        self.run('cmake c-blosc -DBUILD_TESTS=OFF %s %s' %
                 (cmake.command_line, shared))
        self.run("cmake --build . --config Release %s" % cmake.build_config)

    def package(self):
        self.run("rm liblosc_testing*")
        self.copy("blosc.h", dst="include", src="c-blosc/blosc")
        self.copy("blosc-export.h", dst="include", src="c-blosc/blosc")
        self.copy("*.lib", dst="lib", src="blosc/Release", keep_path=False)
        self.copy("*.dll", dst="bin", src="blosc/Release", keep_path=False)
        self.copy("*.dylib", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["blosc"]
