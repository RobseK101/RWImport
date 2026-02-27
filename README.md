# RWImport
A multithreaded dynamic library (DLL) to decode and convert RenderWare Binary Stream Files like models (DFF files) and texture dictionaries (TXD) found in the 3D era GTA games. The conversion queue runs concurrently on a separate thread. 

# Features: 
- Converts DFF models, TXD texture dictionaries and COL collision model bundles. Supports files from GTA 3 at this stage only.
- Runs a concurrent conversion queue into which load jobs are commissioned. Converted data can be retrieved when the job has been finished. The commission order is kept at all times.
- C API: The DLL was written to enable <b>runtime</b> model loading in Unity game engine. The C API is architecture agnostic, however, and may be used in other contexts as well. 
- Loads files from an internal "virtual filesystem" (a list, essentially), into which IMG archives (so the files within them), but also single files can be mounted. 
