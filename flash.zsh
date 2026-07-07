N6_LOADER="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/ExternalLoader/MX25UM51245G_STM32N6570-NUCLEO.stldr"
STM_PROG_PATH="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI"
STM_PROJ_PATH="/home/james/proj/dashcam-NN-n6"


"$STM_PROG_PATH" --help | less

# Flash FSBL
sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG reset=SWrst\
  -el "$N6_LOADER" \
  -w "$STM_PROJ_PATH/"FSBL_signed.bin 0x70000000
sudo "$STM_PROG_PATH" \
  -c port=SWD mode=HOTPLUG reset=SWrst\
  -el "$N6_LOADER" \
  -w "$STM_PROJ_PATH/"Appli_signed.bin 0x70100000
