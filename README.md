# SAV1-OpenGL-example

A very basic example for integrating SAV1 with OpenGL. Uses a provided video as a texture for either a sphere or a cylinder.

## Dependencies

-   OpenGL: This should already be installed on whatever operating system you are using
-   GLFW: https://www.glfw.org/download.html
-   GLM: A version of this is already included in the `windows_include` folder, but it can also be downloaded from https://github.com/g-truc/glm/tags
-   GLEW: This is only required on Windows and can be downloaded from https://glew.sourceforge.net/
-   SAV1: https://github.com/SAV1-org/SAV1

If you using something like Visual Studio, many of these dependencies are handled for you automatically, but if you are compiling manually with g++ on Windows then you will need the dlls for all of the above (except for GLM) at the project root.

## Build

The makefile contains a g++ command to compile on windows. Compile with the command

```
mingw32-make windows
```

Instructions for building on Mac and Linux are forthcoming.

## Run

The make command will create a file named `sav3dplay.exe` at the project root. Run this and provide a path to a video to play as a command line argument. Example

```
sav3dplay test_files/climate.webm
```

By default, the example program textures the requested video around a cylinder, but if the `USE_SPHERE` macro is set to 1 then it will be textured onto a sphere instead.
