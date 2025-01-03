import os.path
import sys
import subprocess
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from setuptools.glob import glob

ext_modules = [
    Extension(
        name="maraboupy.MarabouCore",
        sources=["maraboupy/MarabouCore.cpp"],
    ),
]


class cmake_build_ext(build_ext):
    def build_extension(self, ext: Extension):
        # Ensure build_temp exists
        self.mkpath(self.build_temp)

        # Get the path to the source
        build_temp = os.path.abspath(self.build_temp)
        path_to_source = os.path.dirname(os.path.realpath(__file__))
        print(build_temp)
        print(path_to_source)

        # Boost 및 OpenBLAS 경로 지정
        boost_root = "/content/Marabou/tools/boost-1.84.0"
        openblas_dir = "/content/Marabou/tools/OpenBLAS-0.3.19"
        protobuf_dir = "/content/Marabou/tools/protobuf-3.19.2"
        onnx_dir = "/content/Marabou/tools/onnx-1.15.0"

        # Call cmake
        args = [
            f"cmake",
            f"-DCMAKE_BUILD_TYPE=Release",
            f"-DBOOST_DIR={boost_root}",
            f"-DOPENBLAS_DIR={openblas_dir}",
            f"-DONNX_DIR={onnx_dir}",
            f"-DPROTOBUF_DIR={protobuf_dir}",
            f"-DBUILD_PYTHON=ON",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DPYTHON_LIBRARY_OUTPUT_DIRECTORY={build_temp}",
            path_to_source,
        ]
        print(" ".join(args))
        subprocess.call(args, cwd=build_temp)

        # Call make
        args = ["make", "-j2"]
        print(" ".join(args))
        subprocess.call(args, cwd=build_temp)

        # Copy library
        ext_fullpath = self.get_ext_fullpath(ext.name)
        self.mkpath(os.path.dirname(ext_fullpath))

        if sys.platform in ["win32", "cygwin"]:
            ext_fullpaths_temp = glob(os.path.join(build_temp, "MarabouCore.*.pyd"))
        else:
            ext_fullpaths_temp = glob(os.path.join(build_temp, "MarabouCore.*.so"))
        if len(ext_fullpaths_temp) == 0:
            print("Error: did not build Python extension MarabouCore")
            exit(1)
        if len(ext_fullpaths_temp) >= 2:
            print("Error: built multiple Python extensions MarabouCore")
            exit(1)
        ext_fullpath_temp = ext_fullpaths_temp[0]
        self.copy_file(ext_fullpath_temp, ext_fullpath)


def main():
    setup(
        ext_modules=ext_modules,
        cmdclass={"build_ext": cmake_build_ext},
    )


if __name__ == "__main__":
    main()
