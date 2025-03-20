#!/bin/sh

BUILD_DIR="build"
BUILD_TYPE="Release"  # or "Debug"

show_help() {
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  (缺省)           Release构建"
    echo "  --debug          Debug构建"
    echo "  --preset <name>  根据CMakePresets.json指定Preset项进行配置&构建"
    echo "  --clean          删除构建目录"
    echo "  --help           帮助信息"
}

configure_and_build() {
    BUILD_FULL_PATH="${BUILD_DIR}/${BUILD_TYPE}"
    mkdir -p "${BUILD_FULL_PATH}"
    cmake -B" ${BUILD_FULL_PATH}" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "${BUILD_FULL_PATH}" --target "$1" -j 8
}

# 解析命令行参数
while [ $# -gt 0 ]; do
    case $1 in
        --debug)
            echo "Build Type: Debug"
            BUILD_TYPE="Debug"
            ;;
        --clean)
            echo "Clean build directory.."
            rm -rf "$BUILD_DIR"
            exit 0
            ;;
        --preset)
            shift
            if [ -z "$1" ]; then
                echo "Error: --preset requires a preset name."
                exit 1
            fi
            PRESET="$1"
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
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
    shift
done

echo "Configure and build: ${BUILD_TYPE}" 
configure_and_build "http_server"
