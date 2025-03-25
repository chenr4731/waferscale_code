/*************************************************
*                                                *
*  EasyBMP Cross-Platform Windows Bitmap Library * 
*                                                *
*  Author: Paul Macklin                          *
*   email: macklin01@users.sourceforge.net       *
* support: http://easybmp.sourceforge.net        *
*                                                *
*          file: EasyBMP.h                       * 
*    date added: 01-31-2005                      *
* date modified: 12-01-2006                      *
*       version: 1.06                            *
*                                                *
*   License: BSD (revised/modified)              *
* Copyright: 2005-6 by the EasyBMP Project       * 
*                                                *
* description: Main include file                 *
*                                                *
*************************************************/

//#include <iostream>
#include <cmath>
#include <cctype>
#include <cstring>

#include "mbed.h"

#ifndef EasyBMP
#define EasyBMP
//static LocalFileSystem local("local"); 

#include "pinout.h"

#include <cstdio>


#ifndef _DefaultXPelsPerMeter_
#define _DefaultXPelsPerMeter_
#define DefaultXPelsPerMeter 3780
// set to a default of 96 dpi 
#endif

#ifndef _DefaultYPelsPerMeter_
#define _DefaultYPelsPerMeter_
#define DefaultYPelsPerMeter 3780
// set to a default of 96 dpi
#endif


int IntPow( int base, int exponent );
int ceildiv(int n, int d);

#ifndef _EasyBMP_Defined_WINGDI
#define _EasyBMP_Defined_WINGDI
 typedef unsigned char  ebmpBYTE;
 typedef unsigned short ebmpWORD;
 typedef unsigned int  ebmpDWORD;
#endif

#ifndef _EasyBMP_DataStructures_h_
#define _EasyBMP_DataStructures_h_


// it's easier to use a struct than a class
// because we can read/write all four of the bytes 
// at once (as we can count on them being continuous 
// in memory

typedef struct RGBApixel {
    ebmpBYTE Blue;
    ebmpBYTE Green;
    ebmpBYTE Red;
    ebmpBYTE Alpha;
} RGBApixel; 

class BMFH{
public:
 ebmpWORD  bfType;
 ebmpDWORD bfSize;
 ebmpWORD  bfReserved1;
 ebmpWORD  bfReserved2;
 ebmpDWORD bfOffBits; 

 BMFH();
};

class BMIH{
public:
 ebmpDWORD biSize;
 ebmpDWORD biWidth;
 ebmpDWORD biHeight;
 ebmpWORD  biPlanes;
 ebmpWORD  biBitCount;
 ebmpDWORD biCompression;
 ebmpDWORD biSizeImage;
 ebmpDWORD biXPelsPerMeter;
 ebmpDWORD biYPelsPerMeter;
 ebmpDWORD biClrUsed;
 ebmpDWORD biClrImportant;

 BMIH();
};

#endif

#ifndef _EasyBMP_BMP_h_
#define _EasyBMP_BMP_h_


bool SafeFread( char* buffer, int size, int number, FILE* fp );
bool EasyBMPcheckDataSize( void );

class BMP
{private:

 char BitDepth;
 short Width;
 short Height;

 RGBApixel* Colors;
 short XPelsPerMeter;
 short YPelsPerMeter;

 ebmpBYTE* MetaData1;
 short SizeOfMetaData1;
 ebmpBYTE* MetaData2;
 short SizeOfMetaData2;

 bool Read1bitRow(  ebmpBYTE* Buffer, int BufferSize, int Row );
 bool Write4bitRow(  ebmpBYTE* Buffer, int BufferSize, int Row );  

 public: 

 unsigned int** PixelsBW;
 unsigned int** PixelsR;

 int TellBitDepth( void );
 int TellWidth( void );
 int TellHeight( void );
 int TellNumberOfColors( void );
  
 BMP();
 ~BMP();
 
 unsigned char GetPixel( int i, int j ) const;
 bool SetPixel( int i, int j);
 
 bool CreateStandardColorTable( void );
 
 bool SetSize( int NewWidth, int NewHeight );
 bool SetBitDepth( int NewDepth );
 bool WriteToFile( const char* FileName );
 bool ReadFromFile( const char* FileName );

};

#endif


#endif

