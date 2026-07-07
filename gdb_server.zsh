N6_LOADER="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin/ExternalLoader/MX25UM51245G_STM32N6570-NUCLEO.stldr"
STM_PROG_PATH="/opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.400.202601091506/tools/bin"
STM_PROJ_PATH="/home/james/proj/dashcam-NN-n6"


sudo /opt/stm32cubeide/plugins/com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.linux64_2.2.400.202601091506/tools/bin/ST-LINK_gdbserver -p 61234 -l 1 -d -s -cp "$STM_PROG_PATH" -m 1 -g
