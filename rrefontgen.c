// RRE Font generator
// Converter for .pbm or C header fonts into compressed RREs
// (c) 20017-21 Pawel A. Hernik
// based on Charles Lohr fonter util

// Recent improvements:
// - got rid of getline() function
// - code can be now compiled using TCC on Win machines
// - got rid of cw divisible by 8 limitation
// - added ascii only font bitmap support (range 0x20-0x7f only - 6 lines of 16 chars)
// - added 24bit/rect mode for 64x64 pixel fonts
// - added 24bit vertical/horizontal modes
// - added 32bit/rect mode for 256x256 pixel fonts
// - added 32bit vertical/horizontal modes
// - more debug information
// - decreased rectangle overlapping
// - instead of .pbm files C header fonts can be converted too
// - added loader for *.lcd xml fonts from GLCD Font Creator
// - <optim> added for command line
// - PBM mode for digits (1 line only)

// Usage in PBM mode:
//   rrefontgen [font.pbm] [char w] [char h] <fontName> <fontMode> <overlap> <optim>

// Usage in LCD/XML mode:
//   rrefontgen [font.lcd] <fontName> <fontMode> <overlap> <optim>

// Usage in C header font mode:
//   rrefontgen <fontName> <fontMode> <overlap> <optim>

// fontMode: 0 - rect, 1 - vert, 2 - horiz
// overlap:  0 - no overlapping quads, 1 - old overlapping, 2 - less overlapping (default)
// optim:    1 - remove empty columns and lines (left/top)

#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// -------------------------------------------------------------------

// 1-PBM file, 2-xml/lcd mode, 0-src bitmap from header included below
int fileMode = 0;

// 0-rect, 1-vert, 2-horiz
int fontMode = 0;

// 0-no overlapping quads, 1-old overlapping, 2-less overlapping (default)
int overlap = 2;

// remove empty columns before left edge of all chars
// move all chars up if there is empty space on the top of entire set
int optim = 1;


// dumps more debug info
int debug = 1;

// invert char bitmap
int invert = 0;

// -------------------------------------------------------------------
#define PROGMEM
typedef unsigned char  uint8_t;

//#include "tahoma77x77.h"
//#include "font_tahoma128x129.h"
//#include "tahoma156x193.h"
//#include "times240.h"
//#include "times150.h"
//#include "font_times122.h"
//#include "times122bold.h"
//#include "tahoma255.h"
//#include "arial72.h"
//#include "font_arialb72dig.h"
//#include "font_arial72dig.h"
//#include "c64.h"
//#include "digitek19x15.h"
//#include "chicago21x24.h"
//#include "arialdig48bold.h"
//#include "arialdig48norm.h"
//#include "bolddig13x20.h"
//#include "c64_6x7.h"
//#include "thin_5x7.h"
//#include "font_arialdig72norm.h"
///#include "font_arialdig150bold.h"
//#include "font_arialbldig54bold.h"
//#include "font_arialdig62bold.h"
//#include "font_tahoma206x255.h"
//#include "font_times80.h"
//#include "kx16x26.h"
//#include "kx9x14.h"

const uint8_t *font = NULL;
//const uint8_t *font = Tahoma77x77;
//const uint8_t *font = Tahoma128x129;
//const uint8_t *font = Tahoma156x193;
//const uint8_t *font = Times240;
//const uint8_t *font = ( uint8_t *)times125x151;
//const uint8_t *font = ( uint8_t *)times100x122;
//const uint8_t *font = ( uint8_t *)times107x121b;
//const uint8_t *font = Tahoma255;
//const uint8_t *font = ( uint8_t *)Arial65x72;
//const uint8_t *font = ( uint8_t *)Arial56x75;
//const uint8_t *font = ( uint8_t *)Arial57x72;
//const uint8_t *font = ( uint8_t *)c64;
//const uint8_t *font = ( uint8_t *)Digitek19x15;
//const uint8_t *font = ( uint8_t *)Chicago21x24;
//const uint8_t *font = Arial33x47;
//const uint8_t *font = Arial32x47n;
//const uint8_t *font = Bold13x20;
//const uint8_t *font = c64_6x7;
//const uint8_t *font = thin_5x7;
//const uint8_t *font = ( uint8_t *)Arial50x72n;
///const uint8_t *font = ( uint8_t *)Arial174x228;
//const uint8_t *font = ( uint8_t *)Arial_Black;
//const uint8_t *font = ( uint8_t *)Arial62;
//const uint8_t *font = Tahoma206x255;
//const uint8_t *font = Times80;
//const uint8_t *font = kx16x26;
//const uint8_t *font = kx9x14;

// -------------------------------------------------------------------

int firstChar = 0, lastChar = 255; // initial for PBM or automatically set from header font
const char *fontName = "TempFont";  // set from command line
const char *fontModeTxt = "0";
int rectSize = 2;   // set automatically to 3 if wd,ht >16 or 4 if >64

int fontMinX=0,fontMaxX=0,fontMinY=0,fontMaxY=0,fontW,fontH;
int cw, ch;
int w, h;
int bytes;
uint8_t *buff;

struct MRect { uint8_t x, y, w, h; };

#define MAX_CHARS 256
#define MAX_RECTS_PER_CHAR 300
uint16_t charOffs[MAX_CHARS+1];
struct MRect MRects[MAX_CHARS*MAX_RECTS_PER_CHAR];
int numRects = 0;
    
int numRectPixelsTotal = 0;
int numPixelsTotal = 0;

// -------------------------------------------------------------------

int ReadPBMFile(const char *rn)
{
  char ct[1024];
  int r;
  FILE * f = fopen( rn, "r" );

  if( !f ) {
    fprintf( stderr, "Error: Cannot open file '%s'.\n", rn );
    return -11;
  }

  if( fscanf( f, "%1023s\n", ct ) != 1 || strcmp( ct, "P4" ) != 0 ) {
    fprintf( stderr, "Error: Expected P4 header.\n" );
    return -2;
  }

  char c = fgetc(f);
  while(c!=10 && !feof(f)) c=fgetc(f);

  if( (r = fscanf( f, "%d %d\n", &w, &h )) != 2 || w <= 0 || h <= 0 ) {
    fprintf( stderr, "Error: Need w and h in pbm file.  Got %d params.  (%d %d)\n", r, w, h );
    return -4;
  }
  if(debug) printf("PBM: %dx%d\n",w,h);
  w=(w+7)&~7;
  bytes = (w*h)>>3;
  buff = (uint8_t*)malloc( bytes );
  r = fread( buff, 1, bytes, f );
  if( r != bytes ) {
    fprintf( stderr, "Error: Ran into EOF when reading file.  (%d)\n",r  );
    return -5;
  }

  fclose( f );
  return 0;
}

// -------------------------------------------------------------------
void calcCharSize(int ch, int *minx, int *maxx, int *miny, int *maxy)
{
  int x,y,offs=(ch-firstChar)*fontW*fontH;
  for(y=0;y<fontH;y++)    for(x=0;x<fontW;x++) if(buff[offs+x+y*fontW]) { *miny=y; y=fontH; break;}
  for(y=fontH-1;y>=0;y--) for(x=0;x<fontW;x++) if(buff[offs+x+y*fontW]) { *maxy=y; y=-1; break; }
  for(x=0;x<fontW;x++)    for(y=0;y<fontH;y++) if(buff[offs+x+y*fontW]) { *minx=x; x=fontW; break; }
  for(x=fontW-1;x>=0;x--) for(y=0;y<fontH;y++) if(buff[offs+x+y*fontW]) { *maxx=x; x=-1; break; }
}

// -------------------------------------------------------------------
int ReadXMLFile(const char *rn)
{
  int r;
  FILE *f = fopen( rn, "rb" );
  if( !f ) {
    fprintf( stderr, "Error: Cannot open file '%s'.\n", rn );
    return -11;
  }

  fseek(f,0,SEEK_END);
  int filesize = ftell(f);
  fseek(f,0,SEEK_SET);

  char *xml = (char*)malloc(filesize+1);
  if(!xml) {
    fprintf(stderr, "ERROR: Cannot allocate %d bytes\n",filesize);
    exit(1);
  }
  long res = fread( xml, 1, filesize, f );
  if( res != filesize ) {
    fprintf(stderr, "ERROR: Read bytes %d!=%d\n",res,filesize);
    return -5;
  }
  fclose(f);

  *(xml+filesize) = 0;
  char *p = xml;
  while(*p && strncmp(p,"WIDTH=",6)) p++;
  p+=7;
  fontW=atoi(p);
  while(*p && strncmp(p,"HEIGHT=",7)) p++;
  p+=8;
  fontH=atoi(p);
  while(*p && strncmp(p," FROM=",6)) p++;
  p+=7;
  firstChar=atoi(p);
  while(*p && strncmp(p," TO=",4)) p++;
  p+=5;
  lastChar=atoi(p);

  fprintf(stderr,"LCD/XML: font %dx%d  loading chars %d to %d ...\n",fontW,fontH,firstChar,lastChar);
  int numCh = lastChar-firstChar+1;
  buff = (uint8_t*)malloc(fontW*fontH*numCh);
  //memset(buff,0,fontW*fontH*numCh);
  for(int c=0;c<numCh; c++) {
    //fprintf(stderr, "[%d]\n",c+firstChar);
    int offs = c*fontH*fontW;
    while(p<xml+filesize && *p && strncmp(p," PIXELS=",8)) p++;
    p+=9;
    //printf("---ch=%d\n",c+firstChar);
    for(int j=0;j<fontW;j++) {
      for(int i=0;i<fontH;i++) {
        buff[offs+i*fontW+j]=(atoi(p)?0:1);
        //printf("%d",buff[offs+i*fontW+j]);
        while(p<xml+filesize && *p && *p!=',' && *p!='\"') p++;
        p++;
      }
      //printf("\n");
    }
  }
  free(xml);

  if(!optim) {
    cw = fontW;
    ch = fontH;
    fontMinX = fontMinY = 0;
    if(cw<0 || ch<0) { fprintf(stderr, "Wrong cw,ch [%d,%d]!\n",cw,ch); exit(1); } 
    fprintf(stderr, "Non-Optimized xml font: cw,ch=%dx%d\n",cw,ch);
    return 0;
  }
  
  fontMinX=999,fontMaxX=0,fontMinY=999,fontMaxY=0;
  int minX=999,maxX=0,minY=999,maxY=0;
  for(int c=firstChar;c<=lastChar; c++) {
    calcCharSize(c,&minX,&maxX,&minY,&maxY);
    if(minX<fontMinX) fontMinX=minX;
    if(maxX>fontMaxX) fontMaxX=maxX;
    if(minY<fontMinY) fontMinY=minY;
    if(maxY>fontMaxY) fontMaxY=maxY;
  }
  cw = fontMaxX-fontMinX+1;
  ch = fontMaxY-fontMinY+1;
  if(cw<0 || ch<0) { fprintf(stderr, "Wrong cw,ch [%d,%d]!\n",cw,ch); exit(1); } 
  fprintf(stderr, "Optimized xml font: minx,miny=%d,%d cw,ch=%dx%d\n",fontMinX,fontMinY,cw,ch);
  return 0;
}

// -------------------------------------------------------------------
int TryCover( uint8_t *ibuff, struct MRect *r )
{
  int x, y;
  int count = 0;
  for( y = r->y; y < r->y+r->h; y++ ) {
    for( x = r->x; x < r->x+r->w; x++ ) {
      if( ibuff[x+y*cw] == 1 ) count++;
      else if( ibuff[x+y*cw]==0 || (overlap==0 &&  ibuff[x+y*cw]!= 1) ) return -1;
    }
  }
  return count;
}

void DoCover( uint8_t *ibuff, struct MRect *r )
{
  int x, y;
  for( y = r->y; y < r->y+r->h; y++ ) {
    for( x = r->x; x < r->x+r->w; x++ ) {
      if( ibuff[x+y*cw] > 0 ) ibuff[x+y*cw]++;
    }
  }
}

void Fill( uint8_t *ibuff, struct MRect *r, int val )
{
  int x, y;
  for( y = r->y; y < r->y+r->h; y++ )
    for( x = r->x; x < r->x+r->w; x++ )
      ibuff[x+y*cw] = val;
}

// -------------------------------------------------------------------
int CoverR( uint8_t *ibuff, struct MRect *rs )
{
  int i;
  int x, y, w, h;

  for( i = 0; i < MAX_RECTS_PER_CHAR; i++ )
  {
    struct MRect most_efficient, tmp;
    int most_efficient_count = 0;
    for( y = 0; y < ch; y++ )
    for( x = 0; x < cw; x++ )
    for( h = 1; h <= ch-y; h++ )
    for( w = 1; w <= cw-x; w++ ) {
      tmp.x = x; tmp.y = y; tmp.w = w; tmp.h = h;
      int ct = TryCover( ibuff, &tmp );
      // tmp.w*tmp.h->total number of pixels (including overlapping with count>1)
      // ct->number unique pixels (with count==1), so it is better to take less overlapping
      if( ct>most_efficient_count || (overlap==2 && ct==most_efficient_count && tmp.w*tmp.h<most_efficient.w*most_efficient.h) )
      {
        memcpy( &most_efficient, &tmp, sizeof( tmp ) );
        most_efficient_count = ct;
      }
    }

    if( most_efficient_count == 0 ) return i;

    DoCover( ibuff, &most_efficient );
    memcpy( &rs[i], &most_efficient, sizeof( struct MRect ) );
  }
  fprintf( stderr, "Error: too many rects per char.\n ");
  exit( -22 );
}

int CoverV( uint8_t *ibuff, struct MRect *rs )
{
  int cnt=0;
  int x, y;
  int ystart = -1;

  for( x = 0; x < cw; x++ )
    for( y = 0; y < ch; y++ ) {
      if(ibuff[x+y*cw] > 0 && ystart < 0) ystart = y;
      if((ibuff[x+y*cw] == 0 && ystart >= 0) || (ibuff[x+y*cw] > 0 && y==ch-1) ) {
        if(ystart<0) continue;
        rs[cnt].x = x;
        rs[cnt].y = ystart;
        rs[cnt].w = 1;
        rs[cnt].h = y-ystart;
        if(ibuff[x+y*cw] > 0 && y==ch-1) ++rs[cnt].h;
        cnt++;
        ystart = -1;
      }
    }

  return cnt;
}

int CoverH( uint8_t * ibuff, struct MRect * rs )
{
  int cnt=0;
  int x, y;
  int xstart = -1;

  for( y = 0; y < ch; y++ )
    for( x = 0; x < cw; x++ ) {
      if(ibuff[x+y*cw] > 0 && xstart < 0) xstart = x;
      if((ibuff[x+y*cw] == 0 && xstart >= 0) || (ibuff[x+y*cw] > 0 && x==cw-1) ) {
        if(xstart<0) continue;
        rs[cnt].y = y;
        rs[cnt].x = xstart;
        rs[cnt].h = 1;
        rs[cnt].w = x-xstart;
        if(ibuff[x+y*cw] > 0 && x==cw-1) ++rs[cnt].w;
        cnt++;
        xstart = -1;
      }
    }
  return cnt;
}

// -------------------------------------------------------------------
int GreedyChar( int chr, struct MRect * RectSet )
{
  int x, y, i;
  uint8_t cbuff[ch*cw];
  uint8_t rbuff[ch*cw];
  int rectCount;

  for( i = 0; i < ch*cw; i++ ) cbuff[i] = rbuff[i] = 0;
  // copy char image to cbuff
  if(fileMode==1) {
    // PBM mode
    for( y = 0; y < ch; y++ ) {
      for( x = 0; x < cw; x++ ) {
        int ppcx = w/cw;
        int xpos = ((chr-firstChar) % ppcx)*cw;
        int ypos = ((chr-firstChar) / ppcx)*ch;
        int idx = ((ypos+y)*w+xpos+x)/8;
        int xbit = (xpos+x)&7;
        cbuff[x+y*cw] = (buff[idx]&(1<<(7-xbit)))?0:1;
      }
    }
  } else if(fileMode==2) {
    // lcd/xml mode
    int idx = (chr-firstChar)*fontW*fontH;
    for( y = 0; y < ch; y++ )
      for( x = 0; x < cw; x++ )
        cbuff[x+y*cw] = buff[idx+(y+fontMinY)*fontW+x+fontMinX];
  } else {
    // font in header mode
    int ch8 = (ch+7)/8;
    uint8_t *b = (uint8_t*)font + 4 + 1+(chr-firstChar)*(cw*ch8+1);
    for( x = 0; x < cw; x++ ) {
      for( y = 0; y < ch; y++ ) {
        int idx = x*ch8+y/8;
        int ybit = y&7;
            cbuff[x+y*cw] = (b[idx]&(1<<ybit))?1:0;
      }
    }
  }

  if(invert)
    for( x = 0; x < cw; x++ ) for( y = 0; y < ch; y++ ) cbuff[x+y*cw] = cbuff[x+y*cw] ? 0 : 1;

  //Greedily find the minimum # of rectangles that can cover this.
  switch(fontMode) {
    case 0: rectCount = CoverR( cbuff, RectSet ); break;
    case 1: rectCount = CoverV( cbuff, RectSet ); break;
    case 2: rectCount = CoverH( cbuff, RectSet ); break;
  }

  int numRectPixels = 0;
  int numPixels = 0;
  for( i = 0; i < rectCount; i++ ) {
    numRectPixels+=RectSet[i].w*RectSet[i].h;
    Fill(rbuff,&RectSet[i],i+1);
  }
  for( i=0; i<ch*cw; i++ ) if(rbuff[i]>0) numPixels++;
  numRectPixelsTotal+=numRectPixels;
  numPixelsTotal+=numPixels;
  if(debug) {
    printf( "=========================================================================\n");
    printf( "Char 0x%02x '%c' %d rects, %d pixels (%d overlapping), %d bytes (vs bmp %db)\n", chr, chr<32?'.':chr, rectCount, numRectPixels, numRectPixels-numPixels, rectCount*rectSize, cw*((ch+7)/8));
    if(rectCount>0) printf( "\n   X   Y   W   H   [*]   Rect\n");
    for( i=0; i<rectCount; i++ )
      printf( " %3d %3d %3d %3d   [%c] %3d\n", RectSet[i].x, RectSet[i].y, RectSet[i].w, RectSet[i].h, (i+1)<10 ? i+'1' : ((i-10+1)&0x3f)+'A', i);

    if(rectCount>0) {
      printf( "\nOverlap buffer / Rects\n" );
      for( y=0; y<ch; y++ ) {
        for( x=0; x<cw; x++ ) printf( "%c", cbuff[x+y*cw]>0 ? cbuff[x+y*cw]+'0'-1 : '.' );
        printf( "    " );
        for( x=0; x<cw; x++ ) printf( "%c", rbuff[x+y*cw]==0 ? '.' : (rbuff[x+y*cw]<10 ? rbuff[x+y*cw]+'0' : ((rbuff[x+y*cw]-10)&0x3f)+'A' ));
        printf( "\n" );
      }
    }
  }

  return rectCount;
}

// -------------------------------------------------------------------
// remove empty columns before left edge of all chars
// move all chars up if there is empty space on the top of entire set
// for each char move rect with xmax to the end of the list for easier/faster char width calculation
int optimize()
{
  int xmin,xmax,cwMax = 0, cwMaxCh=0;
  int ymin = 9999;
  int ymax = 0;
  int i,c;
  int yminch='1',ymaxch='1';
  for( c=0; c<256; c++ ) {
    xmin = 9999;
    xmax = 0;
    int recIdx = charOffs[c];
    int recNum = charOffs[c+1]-recIdx;
    int xmaxi=0;
    if(recNum==0) continue;
    for( i = 0; i < recNum; i++ ) {
      int x = MRects[i+recIdx].x;
      int y = MRects[i+recIdx].y;
      int w = MRects[i+recIdx].w;
      int h = MRects[i+recIdx].h;
      if(x<xmin) xmin = x;
      if(x+w-1>xmax) { xmax = x+w-1; xmaxi = i; }
      if(y<ymin) { ymin = y; yminch = c; }
      if(y+h-1>ymax) { ymax = y+h-1; ymaxch = c; }
    }
    int w = xmax-xmin+1;
    if(w>cwMax) { cwMax = w; cwMaxCh=c; }
    if(optim && xmin>0) for(i = 0; i < recNum; i++) MRects[i+recIdx].x -= xmin;
    if(xmaxi<recNum-1) { // move rect with xmax to the end of list for easier char width calculation
      struct MRect tmp = MRects[recIdx+xmaxi];
      for( i = xmaxi; i < recNum-1; i++ ) MRects[recIdx+i]=MRects[recIdx+i+1];
      MRects[recIdx+i] = tmp;
      //memcpy( &tmp, &(MRects[recIdx+xmaxi]), sizeof( tmp ) );
      //memcpy( &(MRects[recIdx+xmaxi]), &(MRects[recIdx+recNum-1]), sizeof( tmp ) );
      //memcpy( &(MRects[recIdx+recNum-1]), &tmp, sizeof( tmp ) );
    }
  }
  if(debug) printf("\norig cw,ch = %dx%d\n",cw,ch);
  if(optim && ymin>0) {
    for(i = 0; i < numRects; i++) MRects[i].y -= ymin;
    cw = cwMax;
    ch = ymax-ymin+1;
  }
  if(cw<0 || ch<0) { fprintf(stderr,"Wrong cw,ch=%d,%d!",cw,ch); exit(1); }
  if(debug) {
    printf("ymin = %3d for [0x%02x] '%c'\n",ymin,yminch,yminch);
    printf("ymax = %3d for [0x%02x] '%c'\n",ymax,ymaxch,ymaxch);
    printf("wmax = %3d for [0x%02x] '%c'\n",cwMax,cwMaxCh,cwMaxCh);
    printf("optim cw,ch = %dx%d\n\n",cw,ch);
  }
}

// -------------------------------------------------------------------
// rect
void DumpX4Y4W4H4()
{
  printf( "const unsigned short font%s_Rects[%d] PROGMEM = {", fontName, numRects );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    unsigned int val = x | (y<<4) | (w<<8) | (h<<12);
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%04x, ", val );
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_16B";
}

void DumpX6Y6W6H6()
{
  printf( "const unsigned char font%s_Rects[%d] PROGMEM = {", fontName, numRects*3 );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%02x,0x%02x,0x%02x, ",
     x | ((h<<6) & 0xc0),
     y | ((h<<4) & 0xc0),
     w | ((h<<2) & 0xc0) );
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_24B";
}

void DumpX8Y8W8H8()
{
  printf( "const unsigned char font%s_Rects[%d] PROGMEM = {", fontName, numRects*4 );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%02x,0x%02x,0x%02x,0x%02x, ", x,y,w,h);
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_32B";
}

// -------------------------------------------------------------------
// vertical
void DumpX6Y5W0H5()
{
  printf( "const unsigned short font%s_Rects[%d] PROGMEM = {", fontName, numRects );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    unsigned int val = x | (y<<6) | (h<<11);
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%04x, ", val );
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_V16B";
}

void DumpX8Y8W0H8()
{
  printf( "const unsigned char font%s_Rects[%d] PROGMEM = {", fontName, numRects*3 );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%02x,0x%02x,0x%02x, ", x,y,h);
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_V24B";
}
// -------------------------------------------------------------------
// horizontal
void DumpX5Y6W5H0()
{
  printf( "const unsigned short font%s_Rects[%d] PROGMEM = {", fontName, numRects );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    unsigned int val = x | (y<<5) | (w<<11);
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%04x, ", val );
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_H16B";
}

void DumpX8Y8W8H0()
{
  printf( "const unsigned char font%s_Rects[%d] PROGMEM = {", fontName, numRects*3 );
  for( int i = 0; i < numRects; i++ ) {
    int x = MRects[i].x;
    int y = MRects[i].y;
    int w = MRects[i].w-1;
    int h = MRects[i].h-1;
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%02x,0x%02x,0x%02x, ", x,y,w);
  }
  printf( "\n};\n\n" );
  fontModeTxt="RRE_H24B";
}

// -------------------------------------------------------------------
void DumpOffs16()
{
  printf( "const unsigned short font%s_CharOffs[%d] PROGMEM = {", fontName, lastChar-firstChar+2 );
  for( int i=firstChar; i<=lastChar+1; i++ ) {
    if( (i%16) == 0 ) printf( "\n\t" );
    printf( "0x%04x, ", charOffs[i] );
  }
  printf( "\n};\n\n" );
}

// -------------------------------------------------------------------
void calcRectSize()
{
  if(fontMode==0) // rect mode
    rectSize = (cw>16 || ch>16) ? ((cw>64 || ch>64) ? 4 : 3) : 2;
  else if(fontMode==1)  // vertical lines
    rectSize = (cw>64 || ch>32) ? ((cw>256 || ch>256) ? 4 : 3) : 2;
  else // horizontal lines
    rectSize = (cw>32 || ch>64) ? ((cw>256 || ch>256) ? 4 : 3) : 2;
}

// -------------------------------------------------------------------
int main( int argc, char ** argv )
{
  int r, i, x, y, j;

  if(argc>1 && strlen(argv[1])>4) {
    int slen = strlen(argv[1]);
    if(strncmp(&argv[1][slen-4],".pbm",4)==0) fileMode=1; else
    if(strncmp(&argv[1][slen-4],".lcd",4)==0) fileMode=2;
  }
  if(fileMode==1) {
    if(argc<4) {
      fprintf( stderr, "Got %d args.\nUsage: rrefontgen [font.pbm] [char w] [char h] <fontName> <fontMode> <overlap>\n", argc );
      return -1;
    }
    if((r = ReadPBMFile(argv[1]))) return r;
    cw = atoi( argv[2] );
    ch = atoi( argv[3] );
    if((w/cw) * (h/ch) < 256) {
       if(h/ch==1) { firstChar='0'; lastChar='0'+(w/cw)-1; } // only digits 0..9
       else        { firstChar=32; lastChar=(w/cw)*(h/ch)+32-1; } // ascii only mode, 64 chars
    }  
    if( argc>4 ) fontName = argv[4];
    if( argc>5 ) fontMode = atoi(argv[5]);
    if( argc>6 ) overlap = atoi(argv[6]);
    if( argc>7 ) optim = atoi(argv[7]);
  } else if(fileMode==2) {
    if(argc<2) {
      fprintf( stderr, "Got %d args.\nUsage: rrefontgen [font.lcd] <fontName> <fontMode> <overlap>\n", argc );
      return -1;
    }
    if((r = ReadXMLFile(argv[1]))) return r;
    if( argc>2 ) fontName = argv[2];
    if( argc>3 ) fontMode = atoi(argv[3]);
    if( argc>4 ) overlap = atoi(argv[4]);
    if( argc>5 ) optim = atoi(argv[5]);
  } else {
    if(!font) { 
      fprintf( stderr, "Font not defined!\n" );
      return -1;
    }
    //cw = ((char*)font)[0]; if(cw<0) cw=-cw;
    cw = font[0]; if(cw<0) cw=-cw;
    ch = font[1];
    firstChar = font[2];
    lastChar  = font[3];
    // Usage for C header mode: rrefontgen <fontName> <fontMode> <overlap>
    if( argc>1 ) fontName = argv[1];
    if( argc>2 ) fontMode = atoi(argv[2]);
    if( argc>3 ) overlap = atoi(argv[3]);
    if( argc>4 ) optim = atoi(argv[4]);
  }

  calcRectSize(); // for cw/ch before optimization

  if(debug) {
    printf("fileMode       = %d (0-header,1-pbm,2-xml)\n",fileMode);
    printf("fontMode       = %d (0-rect,1-vert,2-horiz)\n",fontMode);
    printf("overlap        = %d (0-none,1-normal,2-less/optimal)\n",overlap);
    printf("optim          = %d (0-none,1-optimize wd/ht)\n",optim);
    printf("orig cw x ch   = %dx%d\n",cw,ch);
    printf("orig rectSize  = %d bytes/rect\n",rectSize);
    printf("firstCh,lastCh = 0x%02x to 0x%02x\n",firstChar,lastChar);
    printf("fontName       = [%s]\n\n",fontName);
  }

  //i = 'H';
  for( i=firstChar; i<=lastChar; i++) {
    fprintf( stderr, "%02x [%c]\n", i, i<' '?' ':i );
    charOffs[i] = numRects;
    numRects += GreedyChar( i, &MRects[numRects] );
  }
  charOffs[i] = numRects;

  optimize();
  calcRectSize(); // for cw/ch after optimization

  fprintf( stderr, "Total rects: %d\n", numRects );

  if(debug) printf( "----><--------><--------><--------><--------><--------><----\n\n" );

  printf( "#ifndef __font_%s_h__\n", fontName );
  printf( "#define __font_%s_h__\n\n", fontName );

  int numChars = lastChar-firstChar+1;
  printf( "/*\n  *** Generated by rrefontgen by Pawel A. Hernik ***\n" );
  printf( "  Font:         [%s] %dx%d\n", fontName, cw,ch );
  printf( "  Total chars:  %d", numChars );
  if(firstChar>=0x20) printf( " ('%c' to ", firstChar ); else printf( " (0x%02x to ", firstChar );
  if(lastChar>=0x20 && lastChar<0x7f) printf( "'%c')\n", lastChar ); else printf( "0x%02x)\n", lastChar );
  printf( "  Total rects:  %d * %d bytes\n", numRects, rectSize );
  printf( "  Total pixels: %d (%d overlapping)\n", numRectPixelsTotal, numRectPixelsTotal-numPixelsTotal );
  printf( "  Total bytes:  %d (%d rects + %d offs)\n", numRects*rectSize+numChars*2, numRects*rectSize, numChars*2 );
  printf( "  Bitmap size:  %d (%dx%d * %d) (+%d opt)\n*/\n\n",  numChars*cw*((ch+7)/8), cw,ch,numChars,numChars );
  
  if(fontMode==0) { // rect
    if(cw<=16 && ch<=16) DumpX4Y4W4H4();        // max 16x16, 4+4+4+4=16b
    else if(cw<=64 && ch<=64) DumpX6Y6W6H6();   // max 64x64, 6+6+6+6=24b
    else if(cw<=256 && ch<=256) DumpX8Y8W8H8(); // max 256x256, 8+8+8+8=32b
    else printf("cw or ch greater than 256 !\n");
  } else
  if(fontMode==1) { // vertical
    if(cw<=64 && ch<=32) DumpX6Y5W0H5();               // max 64x32,  6+5+5=16b
    else if(cw<=256 && ch<=256) DumpX8Y8W0H8();        // max 256x256 8+8+8=24b
    //else if(cw<=1024 && ch<=1024) DumpX10Y10W0H10(); // max 1024x1024 10+10+10=30b, not implemented
    else printf("cw or ch greater than 256 !\n");
  } else
  if(fontMode==2) { // horizontal
    if(cw<=32 && ch<=64) DumpX5Y6W5H0();               // max 32x64,  5+6+5=15b
    else if(cw<=256 && ch<=256) DumpX8Y8W8H0();        // max 256x256 8+8+8=24b
    //else if(cw<=1024 && ch<=1024) DumpX10Y10W10H0(); // max 1024x1024 10+10+10=30b, not implemented
    else printf("cw or ch greater than 256 !\n");
  }

  DumpOffs16();

  printf( "RRE_Font rre_%s = { %s, %d,%d, 0x%x, 0x%x, (const uint8_t*)font%s_Rects, (const uint16_t*)font%s_CharOffs };\n", fontName, fontModeTxt, cw,ch, firstChar, lastChar, fontName, fontName);
  printf( "\n#endif\n\n" );

  return 0;
}


