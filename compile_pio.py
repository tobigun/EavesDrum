Import("env")
import os.path

SRC_DIR=env["PROJECT_SRC_DIR"]
PIO_INPUT_FILE=f"{SRC_DIR}/boards/rp2040/capsense.pio"
PIO_OUTPUT_FILE=f"{PIO_INPUT_FILE}.h"

def build_capsense_pio():
    HOME=env["ENV"]["USERPROFILE"]
    print(f"Build {PIO_OUTPUT_FILE}")
    env.Execute(f"{HOME}/.pico-sdk/tools/2.0.0/pioasm/pioasm.exe {PIO_INPUT_FILE} {PIO_OUTPUT_FILE}")

if not os.path.isfile(PIO_OUTPUT_FILE):
    build_capsense_pio()
