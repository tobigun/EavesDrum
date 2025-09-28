#!/bin/sh

set -e

# INPUTS

PICO1_BUILD_DIR=${PICO1_BUILD_DIR:-".pio/build/pico"}
PICO2_BUILD_DIR=${PICO2_BUILD_DIR:-".pio/build/pico2"}
UF2TOOL_DIR=${UF2TOOL_DIR:-"uf2tool/_build/default/bin"}
FS_IMAGE_DIR=${FS_IMAGE_DIR:-"$PICO1_BUILD_DIR"}
PICOTOOL_DIR=${PICOTOOL_DIR:-"$HOME/.platformio/packages/tool-picotool-rp2040-earlephilhower"}
OUTPUT_DIR=${OUTPUT_DIR:-.uf2-out}

UF2TOOL=$UF2TOOL_DIR/uf2tool

# More info about UF2 format and families:
#  https://github.com/microsoft/uf2
#  https://github.com/microsoft/uf2/blob/master/utils/uf2families.json
#  https://github.com/raspberrypi/pico-bootrom-rp2040/blob/master/bootrom/virtual_disk.c#L445
#  https://github.com/raspberrypi/pico-bootrom-rp2350/blob/master/src/nsboot/usb_virtual_disk.c#L607
PICOTOOL=$PICOTOOL_DIR/picotool
FAMILY_RP2040=0xe48bff56
FAMILY_RP2350_ARM_S=0xe48bff59
FAMILY_RP2350_ABS=0xe48bff57

get_fs_offset() {
    ld_file=$1
    offset_dec=$(grep '_FS_start' $ld_file | sed -E 's/.*= *([0-9]+).*/\1/')
    printf '0x%x\n' $offset_dec
}

# Create combined UF2 from executable UF2 and filesystem image (config + UI)
combine_executable_and_fs_uf2() {
    name=$1
    build_dir=$2
    family=$3
    output_uf2=$4

    fs_uf2=$OUTPUT_DIR/eavesdrum-${name}-fs.uf2

    # Convert filesystem image to UF2
    fs_start_offset=$(get_fs_offset $build_dir/memmap_default.ld)
    echo "Create Filesystem UF2 for ${name} (offset: $fs_start_offset): $fs_uf2"
    $PICOTOOL uf2 convert $FS_IMAGE_DIR/littlefs.bin $fs_uf2 --family $family --offset $fs_start_offset

    # Combine filesystem image UF2 with executable UF2
    echo "Create Combined (executable + filesystem) UF2 for ${name}: $output_uf2"
    $UF2TOOL join -o $output_uf2 $build_dir/firmware.uf2 $fs_uf2
}

mkdir -p $OUTPUT_DIR

PICO1_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico1.uf2
combine_executable_and_fs_uf2 pico1 $PICO1_BUILD_DIR $FAMILY_RP2040 $PICO1_COMBINED_UF2

# Note: $FAMILY_RP2350_ABS works too for the FS but cannot be combined with the FAMILY_RP2350_ARM_S executable without restart during flash
PICO2_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico2.uf2
combine_executable_and_fs_uf2 pico2 $PICO2_BUILD_DIR $FAMILY_RP2350_ARM_S $PICO2_COMBINED_UF2

# Make universal (Pico 1 + 2) UF2
echo "Create Universal UF2 for Pico1 + Pico2"
cat $PICO1_COMBINED_UF2 $PICO2_COMBINED_UF2 > $OUTPUT_DIR/eavesdrum-pico-universal.uf2
