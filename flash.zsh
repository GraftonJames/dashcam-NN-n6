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

# Pass -s or --sign-only to stop here: leaves FSBL_signed.bin/Appli_signed.bin in
# the project root for flashing manually via the STM32CubeProgrammer GUI instead
# of this script's CLI path.
if [[ "${1:-}" == "-s" || "${1:-}" == "--sign-only" ]]; then
  echo "flash.zsh: SUCCESS - signed only (FSBL_signed.bin @ 0x70000000, Appli_signed.bin @ 0x70100000)"
  exit 0
fi

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

# Full-range checksum, independent of -v above: compares a checksum of the exact
# bytes on device against a checksum of the local signed file. -v only checks
# during the write itself; this re-reads after the fact, over the whole image
# range, which is what actually caught corruption via the CubeProgrammer GUI
# (its "Full Flash memory checksum" option) that a plain write+verify missed.
FSBL_SIZE=$(stat -c%s "$STM_PROJ_PATH/FSBL_signed.bin")
APPLI_SIZE=$(stat -c%s "$STM_PROJ_PATH/Appli_signed.bin")

FSBL_FILE_SUM=$("$STM_PROG_PATH" -fchecksum "$STM_PROJ_PATH/FSBL_signed.bin" | tee /dev/stderr | grep "Segments total checksum" | grep -oE '0x[0-9A-Fa-f]+')
FSBL_DEV_SUM=$(sudo "$STM_PROG_PATH" -c port=SWD mode=HOTPLUG -el "$N6_LOADER" -checksum 0x70000000 "$FSBL_SIZE" | tee /dev/stderr | grep "Segments total checksum" | grep -oE '0x[0-9A-Fa-f]+')
if [[ "$FSBL_FILE_SUM" != "$FSBL_DEV_SUM" ]]; then
  echo "flash.zsh: FSBL CHECKSUM MISMATCH - file=$FSBL_FILE_SUM device=$FSBL_DEV_SUM" >&2
  exit 1
fi
echo "flash.zsh: FSBL full-range checksum OK ($FSBL_FILE_SUM)"

APPLI_FILE_SUM=$("$STM_PROG_PATH" -fchecksum "$STM_PROJ_PATH/Appli_signed.bin" | tee /dev/stderr | grep "Segments total checksum" | grep -oE '0x[0-9A-Fa-f]+')
APPLI_DEV_SUM=$(sudo "$STM_PROG_PATH" -c port=SWD mode=HOTPLUG -el "$N6_LOADER" -checksum 0x70100000 "$APPLI_SIZE" | tee /dev/stderr | grep "Segments total checksum" | grep -oE '0x[0-9A-Fa-f]+')
if [[ "$APPLI_FILE_SUM" != "$APPLI_DEV_SUM" ]]; then
  echo "flash.zsh: APPLI CHECKSUM MISMATCH - file=$APPLI_FILE_SUM device=$APPLI_DEV_SUM" >&2
  exit 1
fi
echo "flash.zsh: Appli full-range checksum OK ($APPLI_FILE_SUM)"

echo "flash.zsh: SUCCESS - signed, flashed, verified, and checksummed both images"
