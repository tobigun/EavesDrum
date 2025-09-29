# Copyright 2014-present PlatformIO <contact@platformio.org>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Modified version of https://github.com/maxgerhardt/platform-raspberrypi/blob/develop/builder/main.py

# Simple merge for two UF2 files, rewriting them to have sequentual block numbers and a combined numBlocks.
import re
import subprocess
import struct
from dataclasses import dataclass
from os.path import isfile
import argparse

# More info about UF2 format and families:
#  https://github.com/microsoft/uf2
#  https://github.com/microsoft/uf2/blob/master/utils/uf2families.json
#  https://github.com/raspberrypi/pico-bootrom-rp2040/blob/master/bootrom/virtual_disk.c#L445
#  https://github.com/raspberrypi/pico-bootrom-rp2350/blob/master/src/nsboot/usb_virtual_disk.c#L607

UF2_BLOCK_SIZE = 512
UF2_HEADER_FORMAT = "<IIIIIIII"  # 8 unsigned little-endian 32-bit ints
UF2_HEADER_SIZE = struct.calcsize(UF2_HEADER_FORMAT)

@dataclass
class UF2Header:
    magicStart0: int
    magicStart1: int
    flags: int
    targetAddr: int
    payloadSize: int
    blockNo: int
    numBlocks: int
    fileSize: int # or familyID if 0x00002000 present in flags

    @classmethod
    def from_bytes(cls, data: bytes) -> "UF2Header":
        fields = struct.unpack(UF2_HEADER_FORMAT, data[:UF2_HEADER_SIZE])
        return cls(*fields)

    def to_bytes(self) -> bytes:
        return struct.pack(
            UF2_HEADER_FORMAT, self.magicStart0, self.magicStart1,
            self.flags, self.targetAddr, self.payloadSize,
            self.blockNo, self.numBlocks, self.fileSize)

    def __str__(self) -> str:
        return (f"Block {self.blockNo}/{self.numBlocks-1} | Addr=0x{self.targetAddr:08X} | Size={self.payloadSize} | "
                f"Flags=0x{self.flags:X} | FileID=0x{self.fileSize:X} | NumBlocks={self.numBlocks}")

def merge_uf2(fw_uf2: str, fs_uf2: str, outfile: str):
    def read_blocks(filename):
        with open(filename, "rb") as f:
            data = f.read()
        if len(data) % UF2_BLOCK_SIZE != 0:
            raise ValueError(f"{filename} is not a valid UF2 file")
        return [data[i:i+UF2_BLOCK_SIZE] for i in range(0, len(data), UF2_BLOCK_SIZE)]

    # Read both input UF2 files
    blocks_fw = read_blocks(fw_uf2)
    blocks_fs = read_blocks(fs_uf2)

    # merge blocks
    # Note: although the order should not make a difference there seems to be a problem with Pico 1(W) (Booloader v2 and v3)
    # but not with Pico2(W) if the FW is flashed first
    all_blocks = blocks_fs + blocks_fw
    total_blocks = len(all_blocks)

    # the UF2 file has to have the same family ID for the entire file. Take family ID of the firmware UF2
    first_header = UF2Header.from_bytes(blocks_fw[0])
    chosen_file_id = first_header.fileSize

    new_blocks = []
    for new_block_no, raw in enumerate(all_blocks):
        header = UF2Header.from_bytes(raw)
        # Update block number + total blocks + family ID
        header.blockNo = new_block_no
        header.numBlocks = total_blocks
        header.fileSize = chosen_file_id
        # Rebuild block
        new_block = header.to_bytes() + raw[UF2_HEADER_SIZE:]
        assert len(new_block) == UF2_BLOCK_SIZE
        new_blocks.append(new_block)

    # Write merged UF2
    with open(outfile, "wb") as f:
        for blk in new_blocks:
            f.write(blk)
    return total_blocks

def get_fs_offset(memmap_file_path):
    with open(memmap_file_path, 'r') as f:
        for line in f:
            match = re.search(r'_FS_start\s*=\s*([0-9]+)', line)
            if match:
                offset_dec = int(match.group(1))
                return offset_dec
    raise ValueError("No valid _FS_start entry found in file {memmap_file_path}")

def convert_bin_to_uf2(bin_file, uf2_file, offset):
    print(f"Convert bin to UF2 (offset={offset}/{hex(offset)}): {uf2_file}")
    subprocess.run(["picotool", "uf2", "convert", bin_file, uf2_file, "--offset", hex(offset)])
    if not isfile(uf2_file):
        print(f"Error: Cannot find UF2 file {uf2_file}")
        exit(1)

def merge_firmware_and_filesystem_to_one_uf2(firmware_uf2, filesystem_bin, memmap_file_path, target_file):
    fs_start_addr = get_fs_offset(memmap_file_path)

    filesystem_uf2 = filesystem_bin.replace(".bin", ".uf2")
    convert_bin_to_uf2(filesystem_bin, filesystem_uf2, fs_start_addr)

    num_blocks = merge_uf2(firmware_uf2, filesystem_uf2, target_file)
    print(f"Built combined UF2 file: {target_file} ({num_blocks} blocks)")


parser = argparse.ArgumentParser("merge-uf2")
parser.add_argument("--build", help="Build input directory.", type=str, required=True)
parser.add_argument("--target", help="Merged UF2 target file.", type=str)
parser.add_argument("--fsbin", help="Path to littlefs.bin.", type=str)
args = parser.parse_args()

# target is a firmware_with_fs.uf2 file, binary concatenate
build_dir = args.build
firmware_uf2 = f"{build_dir}/firmware.uf2"
if not isfile(firmware_uf2):
    print(f"Error: Cannot find firmware UF2 file {firmware_uf2}")
    exit(1)

memmap_file_path = f"{build_dir}/memmap_default.ld"
if not isfile(memmap_file_path):
    print(f"Error: Cannot find memmap_default.ld file {memmap_file_path}")
    exit(1)

filesystem_bin = f"{args.build}/littlefs.bin" if args.fsbin == None else args.fsbin
target_file = f"{args.build}/firmware_with_fs.uf2" if args.target == None else args.target
merge_firmware_and_filesystem_to_one_uf2(firmware_uf2, filesystem_bin, memmap_file_path, target_file)
