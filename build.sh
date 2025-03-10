#!/bin/sh

BUILD_DIR="build"
BUILD_TYPE="Debug"  # or "Release"

BUILD_FULL_PATH="${BUILD_DIR}/${BUILD_TYPE}"

PRESET="Debug"   # or "Release"

show_help() {
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  缺省            配置&构建"
    echo "  --preset       根据CMakePresets.json进行配置&构建"
    echo "  --clean        删除构建目录"
    echo "  --help         帮助信息"
}

configure_and_build() {
    mkdir -p "${BUILD_FULL_PATH}"
    cmake -B" ${BUILD_FULL_PATH}" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "${BUILD_FULL_PATH}" --target "$1" -j 8
}

# 解析命令行参数
while [ $# -gt 0 ]; do
    case $1 in
        --clean)
            echo "Clean build directory.."
            rm -rf "$BUILD_DIR"
            exit 0
            ;;
        --preset)
            echo "Configure and build according to CMakePresets.json"
            cmake --preset "$PRESET"
            cmake --build --preset "$PRESET"
            exit 0
            ;;
        --test)
            echo "Configure and build all tests exec..."
            configure_and_build "all_test_exec"
            exit 0
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
    esac
    shift
done

echo "Configure and build according"
configure_and_build "http_server"
