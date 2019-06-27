lol.exe: main.c
	i686-w64-mingw32-gcc main.c -o lol.exe
start: lol.exe
	wineconsole lol.exe
clean:
	rm -f lol.exe