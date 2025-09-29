#!/bin/sh

set -e

# INPUTS

PICO1_BUILD_DIR=${PICO1_BUILD_DIR:-".pio/build/pico"}
PICO2_BUILD_DIR=${PICO2_BUILD_DIR:-".pio/build/pico2"}
FS_IMAGE_DIR=${FS_IMAGE_DIR:-"$PICO1_BUILD_DIR"}
OUTPUT_DIR=${OUTPUT_DIR:-.uf2-out}

PICOTOOL_DIR=${PICOTOOL_DIR:-"$HOME/.platformio/packages/tool-picotool-rp2040-earlephilhower"}
export PATH=$PICOTOOL_DIR:$PATH

mkdir -p $OUTPUT_DIR

PICO1_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico1_with_fs.uf2
python merge-uf2.py --build $PICO1_BUILD_DIR --fsbin $FS_IMAGE_DIR/littlefs.bin --target $PICO1_COMBINED_UF2

PICO2_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico2_with_fs.uf2
python merge-uf2.py --build $PICO2_BUILD_DIR --fsbin $FS_IMAGE_DIR/littlefs.bin --target $PICO2_COMBINED_UF2

# Make universal (Pico 1 + 2) UF2
echo "Create Universal UF2 for Pico1 + Pico2"
cat $PICO1_COMBINED_UF2 $PICO2_COMBINED_UF2 > $OUTPUT_DIR/eavesdrum-pico-universal.uf2
