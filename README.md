1.	Install CMake
a.	https://cmake.org/download/
b.	Add it to your path
2.	Install Raylib
a.	Windows
i.	https://github.com/raysan5/raylib/releases/download/3.5.0/raylib_installer_v350.tcc.exe
b.	Linux
i.	git clone https://github.com/raysan5/raylib.git
3.	Build Raylib
a.	Navigate to raylib /src/ directory
i.	Ex: C:/raylib/raylib/
b.	Create directory for building files
i.	mkdir build # Create a build directory
ii.	cd build && cmake .. # Build from that directory so the build files are in one place
iii.	cmake --build . # Build the project
4.	Run the new executable
