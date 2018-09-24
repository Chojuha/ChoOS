all: BootLoader Kernel32 Kernel64 Utility Disk.img

BootLoader:
	make -C 00.BootLoader
	
Kernel32:
	make -C 01.Kernel32

Kernel64:
	make -C 02.Kernel64
	
Utility:
	make -C 04.Utility
	
Disk.img: 00.BootLoader/BootLoader.bin 01.Kernel32/Kernel32.bin 02.Kernel64/Kernel64.bin
	./04.Utility/00.ImageMaker/ImageMaker.exe $^

clean:
	make -C 00.BootLoader clean
	make -C 01.Kernel32 clean
	make -C 02.Kernel64 clean
	make -C 04.Utility clean
	rm -f Disk.img
