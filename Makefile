windows:
	g++ -Iinclude -Iwindows_include -L. sav3dplay.cpp -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32 -lsav1 -o sav3dplay.exe

mac:
	echo "coming soon..."
