all:
	cc -O3 -o ocr ocr.c 
clean:
	rm -f *.o *.core *.jpg ocr
