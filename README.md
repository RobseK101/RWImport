# RWImport
A multithreaded dynamic library (DLL) to stream in and convert RenderWare Binary Stream Files like models (DFF files) and texture dictionaries (TXD) found in the 3D era GTA games. The conversion queue runs concurrently on a separate thread. 

# Features: 
- Converts DFF models, TXD texture dictionaries and COL collision model bundles. Supports files from GTA 3 at this stage only.
- Streaming architecture: Runs a concurrent conversion queue into which load jobs are commissioned. Converted data can be retrieved when the job has been finished. The commission order is kept at all times.
- C API: The DLL was written to enable <b>runtime</b> model loading in Unity game engine. The C API is architecture agnostic, however, and may be used in other contexts as well. 
- Loads files from an internal "virtual filesystem" (a list, essentially), into which IMG archives (so the files within them), but also single files can be mounted. 

# Project state
The DLL was tested with Unity game engine and enables runtime loading of DFF files, TXD files and collision object. A few files have been successfully tested at this point.

# Dependencies
External dependencies:
- GLM (OpenGL Mathematics): A header-only math library with a permissive license (MIT). Despite its name, the fact that it has "OpenGL" in it is secondary; it's just a good math library.

# Building / Compilation
The code can be compiled using MSVC, using the C++17 standard. You can add all the code files (so not the "docs") to a Visual Studio project and set the default include directory to the "include" directory. Choose DLL/dynamic library as a compilation target. 
