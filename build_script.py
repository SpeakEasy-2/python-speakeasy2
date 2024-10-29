"""Script to build C extensions."""

import os
import shutil
import subprocess

from setuptools import Extension
from setuptools.command.build_clib import build_clib
from setuptools.command.build_ext import build_ext
from setuptools.command.develop import develop
from setuptools.errors import CompileError

extensions = [
    Extension(
        "speakeasy2._speakeasy2",
        sources=["speakeasy2/_speakeasy2.c"],
        include_dirs=[
            "vendor/speakeasy2/include",
            "vendor/speakeasy2/vendor/igraph/include",
            "vendor/python-igraph/src/_igraph",
        ],
    )
]


class CLibBuilder(build_clib):
    _cmake_cmd = shutil.which("cmake")

    def build_libraries(self, libraries):
        if not self._cmake_cmd:
            raise CompileError("CMake must be installed to compile extension")

        for lib_name, lib_dict in libraries:
            lib_name = f"lib{lib_name}"
            if not os.path.exists(os.path.join(self.build_temp, lib_name)):
                self.cmake_config(lib_name, lib_dict["source"])

            self.cmake_build(lib_name)

    def _cmake(self, args):
        args.insert(0, self._cmake_cmd)
        subprocess.call(args)

    def cmake_config(self, name, source):
        args = [f"-B {os.path.join(self.build_temp, name)}", f"-S {source}"]

        if shutil.which("ninja"):
            args.append("-G Ninja")

        package_version = os.getenv("CMAKE_PACKAGE_VERSION")
        if package_version:
            args.append(f"-D CMAKE_PACKAGE_VERSION={package_version}")

        self._cmake(args)

    def cmake_build(self, name):
        self._cmake(["--build", os.path.join(self.build_temp, name)])


class ExtBuilder(build_ext):
    def run(self):
        self.include_dirs.append(
            os.path.join(self.build_temp, "libSpeakEasy2", "include")
        )
        self.library_dirs.append(
            os.path.join(self.build_temp, "libSpeakEasy2", "src", "speakeasy2")
        )
        build_ext.run(self)


class CustomDevelop(develop):
    def run(self):
        self.run_command("build_ext")
        super().run()


def build(setup_kwargs):
    setup_kwargs.update(
        {
            "libraries": [("SpeakEasy2", {"source": "vendor/speakeasy2"})],
            "ext_modules": extensions,
            "cmdclass": {
                "develop": CustomDevelop,
                "build_clib": CLibBuilder,
                "build_ext": ExtBuilder,
            },
        }
    )
