from conans import ConanFile, CMake
import os


channel = os.getenv("CONAN_CHANNEL", "testing")
username = os.getenv("CONAN_USERNAME", "albertosm27")
version = os.environ['CONAN_REFERENCE'].split('/')[1]

class CbloscTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "c-blosc/%s@%s/%s" % (version, username, channel)
    generators = "cmake"

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")

    def test(self):
        self.run("ctest")
