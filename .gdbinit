
set print pretty on
set logging on

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
target extended-remote localhost:61234
tui enable
load_fsbl

