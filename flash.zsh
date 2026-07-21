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

# Full-range readback verification, independent of -v above: -v only checks
# during the write itself; this re-reads the ENTIRE image after the fact and
# compares it byte-for-byte against the local signed file - what actually
# caught corruption via the CubeProgrammer GUI's "Full Flash memory checksum"
# option that a plain write+verify missed, historically. Originally done here
# via the device-side "-checksum" command, but this external NOR loader
# (MX25UM51245G_STM32N6570-NUCLEO.stldr) doesn't implement that optional API
# ("CheckSum function is not implemented in this Flashloader") - every run
# has been silently failing at this exact step and exiting non-zero, though
# the write+verify above (which relies on the same loader's more basic
# memory-read primitive, proven working) had already succeeded by that point.
# Fixed by reading the flash back with -upload instead and comparing the
# bytes directly with cmp - stronger than a checksum (exact byte compare, no
# hash-collision risk) and doesn't depend on the loader's optional API at all.
FSBL_SIZE=$(stat -c%s "$STM_PROJ_PATH/FSBL_signed.bin")
APPLI_SIZE=$(stat -c%s "$STM_PROJ_PATH/Appli_signed.bin")

# Deliberately NOT pre-created via mktemp: STM32_Programmer_CLI refuses to
# overwrite a file it doesn't already own, even when it (and the file) are
# both under sudo/root - "Unable to create the file", 0 bytes written. Clear
# any stale path instead and let the tool create it fresh (as root:root,
# mode 644 - readable back by this non-root shell for the cmp below).
# Cleanup must be "sudo rm", not plain "rm": /tmp's sticky bit means only the
# owner (root, since the file's created via sudo) or root can delete it - a
# plain "rm -f" here fails with "Operation not permitted" and (set -e) would
# abort the very next run at this exact line, confirmed by testing directly.
FSBL_READBACK="/tmp/dashcam-fsbl-readback.bin"
APPLI_READBACK="/tmp/dashcam-appli-readback.bin"
sudo rm -f "$FSBL_READBACK" "$APPLI_READBACK"
trap 'sudo rm -f "$FSBL_READBACK" "$APPLI_READBACK"' EXIT

sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG -el "$N6_LOADER" \
  -u 0x70000000 "$FSBL_SIZE" "$FSBL_READBACK"
if ! cmp -s "$STM_PROJ_PATH/FSBL_signed.bin" "$FSBL_READBACK"; then
  echo "flash.zsh: FSBL READBACK MISMATCH - device content does not match FSBL_signed.bin" >&2
  cmp "$STM_PROJ_PATH/FSBL_signed.bin" "$FSBL_READBACK" || true
  exit 1
fi
echo "flash.zsh: FSBL full-range readback OK ($FSBL_SIZE bytes match exactly)"

sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG -el "$N6_LOADER" \
  -u 0x70100000 "$APPLI_SIZE" "$APPLI_READBACK"
if ! cmp -s "$STM_PROJ_PATH/Appli_signed.bin" "$APPLI_READBACK"; then
  echo "flash.zsh: APPLI READBACK MISMATCH - device content does not match Appli_signed.bin" >&2
  cmp "$STM_PROJ_PATH/Appli_signed.bin" "$APPLI_READBACK" || true
  exit 1
fi
echo "flash.zsh: Appli full-range readback OK ($APPLI_SIZE bytes match exactly)"

echo "flash.zsh: SUCCESS - signed, flashed, verified, and full-range readback-compared both images"
