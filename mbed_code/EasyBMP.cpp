/*************************************************
*                                                *
*  EasyBMP Cross-Platform Windows Bitmap Library * 
*                                                *
*  Author: Paul Macklin                          *
*   email: macklin01@users.sourceforge.net       *
* support: http://easybmp.sourceforge.net        *
*                                                *
*          file: EasyBMP.cpp                     * 
*    date added: 03-31-2006                      *
* date modified: 12-01-2006                      *
*       version: 1.06                            *
*                                                *
*   License: BSD (revised/modified)              *
* Copyright: 2005-6 by the EasyBMP Project       * 
*                                                *
* description: Actual source file                *
*                                                *
*************************************************/

#include "EasyBMP.h"

/* These functions are defined in EasyBMP_DataStructures.h */

int ceildiv(int n, int d)
{
  return 1 + ((n - 1) / d);
}

int IntPow( int base, int exponent )
{
 int i;
 int output = 1;
 for( i=0 ; i < exponent ; i++ )
 { output *= base; }
 return output;
}

BMFH::BMFH()
{
 bfType = 19778;
 bfReserved1 = 0;
 bfReserved2 = 0;
}

BMIH::BMIH()
{
 biPlanes = 1;
 biCompression = 0;
 biXPelsPerMeter = DefaultXPelsPerMeter;  
 biYPelsPerMeter = DefaultYPelsPerMeter;
 biClrUsed = 0;
 biClrImportant = 0;
}


/* These functions are defined in EasyBMP_BMP.h */

unsigned char BMP::GetPixel( int i, int j ) const
{
  if(PixelsBW[i / 32][j] & ( 0x1 << (31 - i % 32))){
    return 1; // white
  }else{
    return 0; // black
  }
}

bool BMP::SetPixel( int i, int j )
{
  PixelsR[i / 32][j] |= ( 0x1 << (31 - i % 32));
  return true;
}

BMP::BMP()
{
 Width = 1;
 Height = 1;
 BitDepth = 24;
 PixelsBW = new unsigned int* [1];
 PixelsBW[0] = new unsigned int [1];
 PixelsR = new unsigned int* [1];
 PixelsR[0] = new unsigned int [1];

 Colors = NULL;
 
 XPelsPerMeter = 0;
 YPelsPerMeter = 0;
 
 MetaData1 = NULL;
 SizeOfMetaData1 = 0;
 MetaData2 = NULL;
 SizeOfMetaData2 = 0;
}


BMP::~BMP()
{
 int i;
 for(i=0;i<ceildiv(Width,32);i++)
 { delete [] PixelsBW[i]; }
 delete [] PixelsBW;

 for(i=0;i<ceildiv(Width,32);i++)
 { delete [] PixelsR[i]; }
 delete [] PixelsR;

 if( Colors )
 { delete [] Colors; }
 
 if( MetaData1 )
 { delete [] MetaData1; }
 if( MetaData2 )
 { delete [] MetaData2; }
} 

// int BMP::TellBitDepth( void ) const
int BMP::TellBitDepth( void )
{ return BitDepth; }

// int BMP::TellHeight( void ) const
int BMP::TellHeight( void )
{ return Height; }

// int BMP::TellWidth( void ) const
int BMP::TellWidth( void )
{ return Width; }

// int BMP::TellNumberOfColors( void ) const
int BMP::TellNumberOfColors( void )
{
 int output = IntPow( 2, BitDepth );
 return output;
}

bool BMP::SetBitDepth( int NewDepth )
{
 using namespace std;
 if( NewDepth != 1 && NewDepth != 4)
 {
  return false;
 }
 
 BitDepth = NewDepth;
 if( Colors )
 { delete [] Colors; }
 int NumberOfColors = IntPow( 2, BitDepth );

 { Colors = new RGBApixel [NumberOfColors]; }
 { CreateStandardColorTable(); }
 
 return true;
}

bool BMP::SetSize(int NewWidth , int NewHeight )
{
 using namespace std;
 if( NewWidth <= 0 || NewHeight <= 0 )
 {
  return false;
 }

 int i,j; 

 for(i=0;i<ceildiv(Width,32);i++)
 { delete [] PixelsBW[i]; }
 delete [] PixelsBW;

 for(i=0;i<ceildiv(Width,32);i++)
 { delete [] PixelsR[i]; }
 delete [] PixelsR;

 Width = NewWidth;
 Height = NewHeight;
 PixelsBW = new unsigned int* [ ceildiv(Width,32) ];
 PixelsR  = new unsigned int* [ ceildiv(Width,32) ];

 for(i=0;i<ceildiv(Width,32);i++){
   PixelsBW[i] = new unsigned int [ Height ];
   PixelsR[i] = new unsigned int [ Height ];
   for( j=0 ; j < Height ; j++ ){
     PixelsBW[i][j] = 0;
     PixelsR[i][j] = 0;
   }
 }

 return true; 
}
#include<iostream>

bool BMP::WriteToFile( const char* FileName )
{
    
 using namespace std;
 
 cout << "called" << endl;
 if( !EasyBMPcheckDataSize() )
 {
  return false; 
 }
 
 FILE* fp = fopen( FileName, "wb" );
 

 if( fp == NULL )
 {
    cout << "can't write\r\n" << endl;
  fclose( fp );
  return false;
 }
  
 // some preliminaries
 
 double dBytesPerPixel = ( (double) BitDepth ) / 8.0;
 double dBytesPerRow = dBytesPerPixel * (Width+0.0);
 dBytesPerRow = ceil(dBytesPerRow);
  
 int BytePaddingPerRow = 4 - ( (int) (dBytesPerRow) )% 4;
 if( BytePaddingPerRow == 4 )
 { BytePaddingPerRow = 0; } 
 
 double dActualBytesPerRow = dBytesPerRow + BytePaddingPerRow;
 
 double dTotalPixelBytes = Height * dActualBytesPerRow;
 
 double dPaletteSize = 0;
 if( BitDepth == 1 || BitDepth == 4)
 { dPaletteSize = IntPow(2,BitDepth)*4.0; }

 double dTotalFileSize = 14 + 40 + dPaletteSize + dTotalPixelBytes;
 
 // write the file header 
 
 BMFH bmfh;
 bmfh.bfSize = (ebmpDWORD) dTotalFileSize; 
 bmfh.bfReserved1 = 0; 
 bmfh.bfReserved2 = 0; 
 bmfh.bfOffBits = (ebmpDWORD) (14+40+dPaletteSize);  
 
 fwrite( (char*) &(bmfh.bfType) , sizeof(ebmpWORD) , 1 , fp );
 fwrite( (char*) &(bmfh.bfSize) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmfh.bfReserved1) , sizeof(ebmpWORD) , 1 , fp );
 fwrite( (char*) &(bmfh.bfReserved2) , sizeof(ebmpWORD) , 1 , fp );
 fwrite( (char*) &(bmfh.bfOffBits) , sizeof(ebmpDWORD) , 1 , fp );
 
 // write the info header 
 
 BMIH bmih;
 bmih.biSize = 40;
 bmih.biWidth = Width;
 bmih.biHeight = Height;
 bmih.biPlanes = 1;
 bmih.biBitCount = BitDepth;
 bmih.biCompression = 0;
 bmih.biSizeImage = (ebmpDWORD) dTotalPixelBytes;
 if( XPelsPerMeter )
 { bmih.biXPelsPerMeter = XPelsPerMeter; }
 else
 { bmih.biXPelsPerMeter = DefaultXPelsPerMeter; }
 if( YPelsPerMeter )
 { bmih.biYPelsPerMeter = YPelsPerMeter; }
 else
 { bmih.biYPelsPerMeter = DefaultYPelsPerMeter; }

 bmih.biClrUsed = 0;
 bmih.biClrImportant = 0;

 // indicates that we'll be using bit fields for 16-bit files
 if( BitDepth == 16 )
 { bmih.biCompression = 3; }
 
 fwrite( (char*) &(bmih.biSize) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biWidth) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biHeight) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biPlanes) , sizeof(ebmpWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biBitCount) , sizeof(ebmpWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biCompression) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biSizeImage) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biXPelsPerMeter) , sizeof(ebmpDWORD) , 1 , fp );
 fwrite( (char*) &(bmih.biYPelsPerMeter) , sizeof(ebmpDWORD) , 1 , fp ); 
 fwrite( (char*) &(bmih.biClrUsed) , sizeof(ebmpDWORD) , 1 , fp);
 fwrite( (char*) &(bmih.biClrImportant) , sizeof(ebmpDWORD) , 1 , fp);
 
 // write the palette 
 if( BitDepth == 1 || BitDepth == 4 )
 {
  int NumberOfColors = IntPow(2,BitDepth);
  
  // if there is no palette, create one 
  if( !Colors )
  {
   if( !Colors )
   { Colors = new RGBApixel [NumberOfColors]; }
   CreateStandardColorTable(); 
  }
   
  int n;
  for( n=0 ; n < NumberOfColors ; n++ )
  { fwrite( (char*) &(Colors[n]) , 4 , 1 , fp ); }
 }
 
 // write the pixels 
 int j;
 {  
  ebmpBYTE* Buffer;
  int BufferSize = (int) ( (Width*BitDepth)/8.0 );
  while( 8*BufferSize < Width*BitDepth )
  { BufferSize++; }
  while( BufferSize % 4 )
  { BufferSize++; }
  
  Buffer = new ebmpBYTE [BufferSize];
  for( j=0 ; j < BufferSize; j++ )
  { Buffer[j] = 0; }
    
  j=Height-1;
  
  while( j > -1 )
  {
   bool Success = false;
   if( BitDepth == 4  )
   { Success = Write4bitRow( Buffer, BufferSize, j ); }
   if( Success )
   {
    int BytesWritten = (int) fwrite( (char*) Buffer, 1, BufferSize, fp );
    if( BytesWritten != BufferSize )
    { Success = false; }
   }
   if( !Success )
   {
    j = -1; 
   }
   j--; 
  }
  
  delete [] Buffer;
 }
 
 cout << "done " << endl;
 fclose(fp);
 return true;
}

bool BMP::ReadFromFile( const char* FileName )
{ 
 using namespace std;
 if( !EasyBMPcheckDataSize() )
 {
  return false; 
 }

 FILE* fp = fopen( FileName, "rb" );
 if( fp == NULL )
 {

  SetBitDepth(1);
  SetSize(1,1);
  return false;
 }
 
 // read the file header 
 
 BMFH bmfh;
 bool NotCorrupted = true;
 
 NotCorrupted &= SafeFread( (char*) &(bmfh.bfType) , sizeof(ebmpWORD), 1, fp);
 
 bool IsBitmap = false;
 
 if( bmfh.bfType == 19778 )
 { IsBitmap = true; }
 
 if( !IsBitmap ) 
 {
  fclose( fp ); 
  return false;
 }

 NotCorrupted &= SafeFread( (char*) &(bmfh.bfSize) , sizeof(ebmpDWORD) , 1, fp); 
 NotCorrupted &= SafeFread( (char*) &(bmfh.bfReserved1) , sizeof(ebmpWORD) , 1, fp);
 NotCorrupted &= SafeFread( (char*) &(bmfh.bfReserved2) , sizeof(ebmpWORD) , 1, fp);
 NotCorrupted &= SafeFread( (char*) &(bmfh.bfOffBits) , sizeof(ebmpDWORD) , 1 , fp);
 
 // read the info header

 BMIH bmih; 
 
 NotCorrupted &= SafeFread( (char*) &(bmih.biSize) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biWidth) , sizeof(ebmpDWORD) , 1 , fp); 
 NotCorrupted &= SafeFread( (char*) &(bmih.biHeight) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biPlanes) , sizeof(ebmpWORD) , 1, fp); 
 NotCorrupted &= SafeFread( (char*) &(bmih.biBitCount) , sizeof(ebmpWORD) , 1, fp);

 NotCorrupted &= SafeFread( (char*) &(bmih.biCompression) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biSizeImage) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biXPelsPerMeter) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biYPelsPerMeter) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biClrUsed) , sizeof(ebmpDWORD) , 1 , fp);
 NotCorrupted &= SafeFread( (char*) &(bmih.biClrImportant) , sizeof(ebmpDWORD) , 1 , fp);
 
 // a safety catch: if any of the header information didn't read properly, abort
 // future idea: check to see if at least most is self-consistent
  
 if( !NotCorrupted )
 {
  SetSize(1,1);
  SetBitDepth(1);
  fclose(fp);
  return false;
 } 
 
 XPelsPerMeter = bmih.biXPelsPerMeter;
 YPelsPerMeter = bmih.biYPelsPerMeter;

 // set the bit depth
 
 int TempBitDepth = (int) bmih.biBitCount;
 if(    TempBitDepth != 1  && TempBitDepth != 4 )
 {
  SetSize(1,1);
  SetBitDepth(1);
  fclose(fp);
  return false;
 }
 SetBitDepth( (int) bmih.biBitCount ); 
 
 // set the size

 if( (int) bmih.biWidth <= 0 || (int) bmih.biHeight <= 0 ) 
 {
  SetSize(1,1);
  SetBitDepth(1);
  fclose(fp);
  return false;
 } 
 SetSize( (int) bmih.biWidth , (int) bmih.biHeight );
  
 // some preliminaries
 
 double dBytesPerPixel = ( (double) BitDepth ) / 8.0;
 double dBytesPerRow = dBytesPerPixel * (Width+0.0);
 dBytesPerRow = ceil(dBytesPerRow);
  
 int BytePaddingPerRow = 4 - ( (int) (dBytesPerRow) )% 4;
 if( BytePaddingPerRow == 4 )
 { BytePaddingPerRow = 0; }  

 // skip blank data if bfOffBits so indicates
 
 int BytesToSkip = bmfh.bfOffBits - 54;;
 if( BitDepth == 16 && bmih.biCompression == 3 )
 { BytesToSkip -= 3*4; }
 if( BytesToSkip < 0 )
 { BytesToSkip = 0; }
 if( BytesToSkip > 0 && BitDepth != 16 )
 {
  ebmpBYTE* TempSkipBYTE;
  TempSkipBYTE = new ebmpBYTE [BytesToSkip];
  SafeFread( (char*) TempSkipBYTE , BytesToSkip , 1 , fp);   
  delete [] TempSkipBYTE;
 } 
  
 // This code reads 1, 4, 8, 24, and 32-bpp files 
 // with a more-efficient buffered technique.

 int j;
 if( BitDepth != 16 )
 {
  int BufferSize = (int) ( (Width*BitDepth) / 8.0 );
  while( 8*BufferSize < Width*BitDepth )
  { BufferSize++; }
  while( BufferSize % 4 )
  { BufferSize++; }
  ebmpBYTE* Buffer;
  Buffer = new ebmpBYTE [BufferSize];
  j= Height-1;
  while( j > -1 )
  {
   int BytesRead = (int) fread( (char*) Buffer, 1, BufferSize, fp );
   if( BytesRead < BufferSize )
   {
    j = -1; 
   }
   else
   {
    bool Success = false;
    if( BitDepth == 1  )
    { Success = Read1bitRow(  Buffer, BufferSize, j ); }
    if( !Success )
    {
     j = -1;
    }
   }   
   j--;
  }
  delete [] Buffer; 
 }
 fclose(fp);
 return true;
}

bool BMP::CreateStandardColorTable( void )
{
 using namespace std;
 if( BitDepth != 1 && BitDepth != 4)
 {
  return false;
 }

 if( BitDepth == 1 )
 {
  int i;
  for( i=0 ; i < 2 ; i++ )
  {
   Colors[i].Red = i*255;
   Colors[i].Green = i*255;
   Colors[i].Blue = i*255;
   Colors[i].Alpha = 0;
  } 
  return true;
 } 

 for(int i=0; i<16; i++){
   Colors[i].Red = 0;
   Colors[i].Blue = 0;
   Colors[i].Green = 0;
   Colors[i].Alpha = 0;
 }

 Colors[1].Red = 255;

 Colors[15].Red = 255;
 Colors[15].Blue = 255;
 Colors[15].Green = 255;

 return true;
}

bool SafeFread( char* buffer, int size, int number, FILE* fp )
{
 using namespace std;
 int ItemsRead;
 if( feof(fp) )
 { return false; }
 ItemsRead = (int) fread( buffer , size , number , fp );
 if( ItemsRead < number )
 { return false; }
 return true;
}

#include<iostream>
bool BMP::Read1bitRow(  ebmpBYTE* Buffer, int BufferSize, int Row )
{
  using namespace std;
 if( Width > 8*BufferSize )
 { return false; }

 int i=0;
 for(i=0; i < BufferSize; i++){
   PixelsBW[i/4][Row] |= Buffer[i] << (8 * (3 - i%4));
 }
 return true;
}

bool BMP::Write4bitRow(  ebmpBYTE* Buffer, int BufferSize, int Row )
{ 
 int PositionWeights[2]  = {16,1};
 
 int i=0;
 int j;
 int k=0;
 if( Width > 2*BufferSize )
 { return false; }
 while( i < Width )
 {
  j=0;
  int Index = 0;
  while( j < 2 && i < Width )
  {
    int closest = 0; // black
    if(PixelsBW[i / 32][Row] & ( 0x1 << (31 - i % 32))){
      closest = 15; // white
    }
    if(PixelsR[i / 32][Row] & ( 0x1 << (31 - i % 32))){
      closest = 1; // red
    }
    Index += ( PositionWeights[j]* closest ); 
    i++; j++;   
  }
  Buffer[k] = (ebmpBYTE) Index;
  k++;
 }
 return true;
}

bool EasyBMPcheckDataSize( void )
{
 using namespace std;
 bool ReturnValue = true;
 if( sizeof( ebmpBYTE ) != 1 )
 {
  ReturnValue = false;
 }
 if( sizeof( ebmpWORD ) != 2 )
 {
  ReturnValue = false;
 }
 if( sizeof( ebmpDWORD ) != 4 )
 {
  ReturnValue = false;
 }
 return ReturnValue;
}

