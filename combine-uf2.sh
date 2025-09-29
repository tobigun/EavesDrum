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

PICO1_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico1.uf2
python merge-uf2.py --build $PICO1_BUILD_DIR --fsbin $FS_IMAGE_DIR/littlefs.bin --target $PICO1_COMBINED_UF2

PICO2_COMBINED_UF2=$OUTPUT_DIR/eavesdrum-pico2.uf2
python merge-uf2.py --build $PICO2_BUILD_DIR --fsbin $FS_IMAGE_DIR/littlefs.bin --target $PICO2_COMBINED_UF2

# copy filesystem for articat upload
cp $FS_IMAGE_DIR/littlefs.uf2 $OUTPUT_DIR/eavesdrum-fs.uf2
cp $PICO1_BUILD_DIR/firmware.uf2 $OUTPUT_DIR/eavesdrum-pico1_no_fs.uf2
cp $PICO2_BUILD_DIR/firmware.uf2 $OUTPUT_DIR/eavesdrum-pico2_no_fs.uf2

# Make universal (Pico 1 + 2) UF2
# Note: deactivated as the Windows Explorer will show an error message that copying the UF2 file failed as the
# Pico will reboot when it sees a firmware that is not compatible with the board. Hence the user experience is not really improved.
# Seems the universal UF2 approach is only suitable when the images are small.
#echo "Create Universal UF2 for Pico1 + Pico2"
#cat $PICO1_COMBINED_UF2 $PICO2_COMBINED_UF2 > $OUTPUT_DIR/eavesdrum-pico-universal.uf2
