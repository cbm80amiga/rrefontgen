# rrefontgen
RRE fonts generator utility

### Usage in PBM mode:
   ` rrefontgen [pbm file] [char w] [char h] <fontName> <fontMode> <overlap>`
  
  PBM file should be B&W, 1-bit. Typical file should contain 16x6 or 32x3 characters (96 characters mode) or 16x8 (256 characters)
  
### Usage in C header font mode:

   `rrefontgen <fontName> <fontMode> <overlap>`
  
  In this mode you should add your bitmap font header to *rrefontgen* source code, compile the tool and generate the font
  
- fontName: name used in source code
- fontMode: 0 - rectangles (default), 1 - vertical lines, 2 - horizozntal lines
- overlap:  0 - no overlapping quads, 1 - old overlapping, 2 - less overlapping (default)
