from conans import ConanFile, CMake, tools


class FastrtpsConan(ConanFile):
    name = "fastrtps"
    version = "1.10.0"
    license = "Apache License 2.0"
    author = "Vilas Chitrakaran chitrakaran@arrival.com"
    url = "https://git.tra.ai/robohive/conan-fastrtps"
    description = "An implementation of RTPS protocol"
    topics = ("dds", "fast-rtps", "omg")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": True}
    generators = "cmake"

    def requirements(self):
        self.requires.add('asio/1.14.0@arrival/stable')
        self.requires.add('zlib/1.2.11@arrival/stable')
        self.requires.add('openssl/1.1.1f@arrival/stable')
        self.requires.add('tinyxml2/8.0.0@arrival/stable')
        self.requires.add('fastcdr/1.0.12@arrival/stable')
        self.requires.add('foonathan_memory/0.6-2@arrival/stable')

    def source(self):
        git = tools.Git()
        git.clone("https://github.com/eProsima/Fast-RTPS.git", "v1.10.0")

    def build(self):
        cmake = CMake(self, parallel=True)
        cmake.configure(source_folder="")
        cmake.build()
        cmake.install()

    def package(self):
        pass
    
    def package_info(self):
        self.cpp_info.libs = ["fastrtps"]

