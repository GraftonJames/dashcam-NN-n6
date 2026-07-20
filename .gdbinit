
set print pretty on
set logging on
set confirm off

set history save on
set history size 10000

file /home/james/proj/dashcam-NN-n6/FSBL/build/FSBL.elf

define add_breaks
end
define rst
	monitor reset
	load_fsbl
end
define load_fsbl
	load /home/james/proj/dashcam-NN-n6/FSBL/build/FSBL.elf
	hbreak stm32_boot_lrun.c:161
	continue
end

define load_appli
	add-symbol-file /home/james/proj/dashcam-NN-n6/Appli/build/Appli.elf
	break Appli/Src/main.c:main
	continue
end

# Attach to whatever the target is CURRENTLY doing - no reset, no reload, no
# breakpoints. Use this to inspect an existing hang/fault in place (the state
# has already happened; a reset would destroy it before you get to look).
# This is what runs by default at the bottom of this file, so plain
# `gdb -x .gdbinit` (no -nx needed) now attaches non-destructively.
define attach_live
	add-symbol-file /home/james/proj/dashcam-NN-n6/Appli/build/Appli.elf
end

# Dump the Cortex-M fault/status registers we keep reading by hand:
# ICSR (active exception + pending bits), AIRCR (incl. TrustZone PRIS/PRIGROUP),
# CFSR/HFSR (what faulted and why), MMFAR/BFAR (fault addresses),
# SFSR/SFAR (Secure fault status/address - TrustZone-specific).
define fault_regs
	printf "ICSR   = 0x%08x\n", *(unsigned int*)0xE000ED04
	printf "AIRCR  = 0x%08x\n", *(unsigned int*)0xE000ED0C
	printf "CFSR   = 0x%08x\n", *(unsigned int*)0xE000ED28
	printf "HFSR   = 0x%08x\n", *(unsigned int*)0xE000ED2C
	printf "MMFAR  = 0x%08x\n", *(unsigned int*)0xE000ED34
	printf "BFAR   = 0x%08x\n", *(unsigned int*)0xE000ED38
	printf "SFSR   = 0x%08x\n", *(unsigned int*)0xE000EDE4
	printf "SFAR   = 0x%08x\n", *(unsigned int*)0xE000EDE8
end

target extended-remote localhost:61234
tui enable
attach_live
