# rrefontgen
RRE fonts generator utility

### Usage in PBM mode:
   ` rrefontgen [pbm file] [char w] [char h] <fontName> <fontMode> <overlap> <optim>`
  
  PBM file should be B&W, 1-bit. The file should contain 16x6 or 32x3 characters (96 characters mode), 16x8 (128 characters) or 1 line for digits and following characters.  Use irfanview to convert bitmaps in other formats
  
### Usage in XML/LCD mode:
   ` rrefontgen [font.lcd] <fontName> <fontMode> <overlap>  <optim>`
  
  Direct loader of GLCD Font Creator XML format
  
### Usage in C header font mode:

   `rrefontgen <fontName> <fontMode> <overlap>  <optim>`
  
  In this mode you should add your bitmap font header to *rrefontgen* source code, compile the tool and generate the font
  
- fontName: struct name used in the source code
- fontMode: 0 - rectangles (default), 1 - vertical lines, 2 - horizontal lines
- overlap:  0 - no overlapping quads, 1 - old overlapping, 2 - less overlapping (default)
- optim:    1 - remove empty columns and lines (left/top)

### Examples: ###
   `rrefontgen times.pbm 32 64 Times32x64`
   
   `rrefontgen times.pbm 32 64 Times32x64 1`
   
   `rrefontgen times.pbm 32 64 Times32x64 0 0`
   
   `rrefontgen times.lcd Times32x64 0 2`
