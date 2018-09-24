all: ImageMaker SerialTransfer

ImageMaker:
	make -C 00.ImageMaker
SerialTransfer:
	make -C 01.SerialTransfer
clean:
	make -C 00.ImageMaker clean
	make -C 01.SerialTransfer clean
