all: BuildBootLoader BuildKernel32 BuildKernel64 BuildUtility Disk.img

BuildBootLoader:
	make -C BootLoader
	
BuildKernel32:
	make -C Kernel32

BuildKernel64:
	make -C Kernel64
	
BuildUtility:
	make -C Utility
	
Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin Kernel64/Kernel64.bin
	./Utility/ImageMaker/ImageMaker.exe $^

clean:
	make -C BootLoader clean
	make -C Kernel32 clean
	make -C Kernel64 clean
	make -C Utility clean
	rm -f Disk.img
