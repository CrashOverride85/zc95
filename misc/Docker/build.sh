#!/bin/sh

# Default board types
BOARD_TYPE_ZC95="pico_w"
BOARD_TYPE_ZC624="pico"
BUILD_BOOTLOADERS=1

if [ "$#" -gt 1 ]; then
    echo "Error: Too many arguments"
    exit 1
fi

if [ "$#" -eq 1 ]; then
    case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
        pico)
            BOARD_TYPE_ZC95="pico_w"
            BOARD_TYPE_ZC624="pico"
            BUILD_BOOTLOADERS=1
            ;;
        pico2)
            BOARD_TYPE_ZC95="pico2_w"
            BOARD_TYPE_ZC624="pico2"
            BUILD_BOOTLOADERS=0
            ;;
        *)
            echo "Error: Invalid argument '$1' (expected 'pico' or 'pico2')"
            exit 1
            ;;
    esac
fi

echo "Building with board types:"
echo "  zc95 : $BOARD_TYPE_ZC95"
echo "  zc624: $BOARD_TYPE_ZC624"
echo "  bootloaders: $([ "$BUILD_BOOTLOADERS" -eq 1 ] && echo "yes" || echo "no")"
echo

mkdir -p /src/source/CompiledUF2

# --- build zc95 bootloader (orignal pico only) ---
if [ "$BUILD_BOOTLOADERS" -eq 1 ]; then
    cd /src/source/zc95Bootloader || exit 1
    rm -f CMakeCache.txt
    mkdir -p build
    cd build || exit 1
    rm -f CMakeCache.txt
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DPICO_BOARD="$BOARD_TYPE_ZC95" ..
    make
fi

# --- build zc95 ---
cd /src/source/zc95 || exit 1
rm -f CMakeCache.txt
mkdir -p build
cd build || exit 1
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DPICO_BOARD="$BOARD_TYPE_ZC95" ..
make -j8
cp zc95.uf2 /src/source/CompiledUF2/zc95.$BOARD_TYPE_ZC95.uf2

# --- build zc624 bootloader (original pico only) ---
if [ "$BUILD_BOOTLOADERS" -eq 1 ]; then
    cd /src/source/zc624Bootloader || exit 1
    rm -f CMakeCache.txt
    mkdir -p build
    cd build || exit 1
    rm -f CMakeCache.txt
    cmake ..
    make -j8
fi

# --- build zc624 ---
cd /src/source/zc624 || exit 1
rm -f CMakeCache.txt
mkdir -p build
cd build || exit 1
rm -f CMakeCache.txt
cmake -DPICO_BOARD="$BOARD_TYPE_ZC624" ..
make -j8
cp OutputZc.uf2 /src/source/CompiledUF2/OutputZc.$BOARD_TYPE_ZC624.uf2
