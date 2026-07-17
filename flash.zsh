#!/usr/bin/env zsh
# Sign FSBL/Appli build outputs and flash them to external XSPI NOR.
# Aborts immediately on ANY failure (signing, erase, write, verify) - a partial
# run must never look like a success (stale/corrupt flash content caused the
# July 2026 intermittent-hang saga).
set -e
set -o pipefail

SIGN_TOOL="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_SigningTool_CLI"
N6_LOADER="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/ExternalLoader/MX25UM51245G_STM32N6570-NUCLEO.stldr"
STM_PROG_PATH="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI"
STM_PROJ_PATH="/home/james/proj/dashcam-NN-n6"

# Sign. Output paths are ABSOLUTE on purpose: previously they were relative, so
# running this script from any directory other than the project root signed into
# the wrong place while the flash step below silently used STALE binaries from
# the project root.
"$SIGN_TOOL" -bin "$STM_PROJ_PATH/FSBL/build/FSBL.bin" -nk -of 0x80000000 -t fsbl \
  -o "$STM_PROJ_PATH/FSBL_signed.bin" -hv 2.3 -dump "$STM_PROJ_PATH/FSBL_signed.bin" -align -s

"$SIGN_TOOL" -bin "$STM_PROJ_PATH/Appli/build/Appli.bin" -nk -of 0x80000000 -t fsbl \
  -o "$STM_PROJ_PATH/Appli_signed.bin" -hv 2.3 -dump "$STM_PROJ_PATH/Appli_signed.bin" -align -s

# Pass -e or --erase to do a full chip erase before flashing (slower, but rules out stale flash content)
if [[ "${1:-}" == "-e" || "${1:-}" == "--erase" ]]; then
  echo "Performing full chip erase..."
  sudo "$STM_PROG_PATH" \
    -c port=SWD mode=HOTPLUG -el "$N6_LOADER" -e all
fi

# Flash + verify. -v reads flash back against the file after writing: catches
# marginal/corrupted programming immediately (the classic failure signature is a
# write onto unerased cells, e.g. the 0x05-vs-0x8D AND-pattern mismatch we hit).
sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG reset=SWrst \
  -el "$N6_LOADER" \
  -w "$STM_PROJ_PATH/FSBL_signed.bin" 0x70000000 \
  -v
sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG reset=SWrst \
  -el "$N6_LOADER" \
  -w "$STM_PROJ_PATH/Appli_signed.bin" 0x70100000 \
  -v

echo "flash.zsh: SUCCESS - signed, flashed, and verified both images"
