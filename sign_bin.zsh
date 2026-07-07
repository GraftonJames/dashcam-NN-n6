SIGN_TOOL="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/STM32_SigningTool_CLI"
WORK_DIR="/home/james/proj/dashcam-NN-n6"
PROJ_NAME="LED_toggle"
"$SIGN_TOOL" --help
"$SIGN_TOOL" -bin "$WORK_DIR/FSBL/build/FSBL.bin" -nk -of 0x80000000 -t fsbl -o FSBL_signed.bin -hv 2.3 -dump FSBL_signed.bin -align 

"$SIGN_TOOL" -bin "$WORK_DIR/Appli/build/Appli.bin" -nk -of 0x80000000 -t fsbl -o Appli_signed.bin -hv 2.3 -dump Appli_signed.bin -align
