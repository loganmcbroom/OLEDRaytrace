# OLEDRaytrace
Single file OLED raytracing renderer in c++.

Usage: From main, call printOledDataToFile, renderOledDataToConsole, or renderASCIIDataToConsole, passing a uv function and a normal function. 
See the example default and torus functions just above main for how this should look. 
The uv funcion will take u and v values on [-1,1] and return xyz coordinates. 
The integer input in the uv function can be used for surfaces made of multiple uv sheets. See "defaultUV".
The normal funcion takes xyz coordinates and should return a normalized normal vector to the surface at the input point.
There are a handful of global settings at the top of the file. 
For my oled I needed 32 screenWidth, 128 screenHeight, and 11 numFrames.
You may want to modify lightCurve depending on the shape you're drawing.

This is using windows.h for showing a higher quality render on the console during clearScreen(). 

