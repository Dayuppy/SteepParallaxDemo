//**************************************************************************************
// Copyright 2004, Huamin Wang
//**************************************************************************************
// File BMP_IO.h
// Only consider 24 bits RGB color BMP files here.
// Ignore the alpha channel.
// The storage sequence in BITMAP is BGR, not RGB.
//**************************************************************************************
#ifndef __FILE_IO_BMP_IO_H__
#define __FILE_IO_BMP_IO_H__
#include <stdio.h>
#include <windows.h>


bool BMP_Read(const char *filename, BYTE** pixels, int &width, int &height)
{
	HBITMAP bmp_handle = (HBITMAP)LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
	if(bmp_handle==INVALID_HANDLE_VALUE) return false;
	BITMAP bitmap;
	GetObject(bmp_handle, sizeof(BITMAP), (LPSTR)&bitmap);
	if(bitmap.bmPlanes * bitmap.bmBitsPixel!=24)
	{printf("Error: The bmp image depth is not 24.\n"); DeleteObject(bmp_handle); return false;}

	width=bitmap.bmWidth;
	height=bitmap.bmHeight;
    if(*pixels) delete *pixels;
	*pixels=new BYTE[bitmap.bmWidth * bitmap.bmHeight *3];

	unsigned char *ptr=(unsigned char *)(bitmap.bmBits);
	for(int j=0; j<height; j++)
	{
		unsigned char *line_ptr=ptr;
		for(int i=0; i<width; i++) 
		{
			(*pixels)[3*(i*height+j)  ]=line_ptr[2];
			(*pixels)[3*(i*height+j)+1]=line_ptr[1];
			(*pixels)[3*(i*height+j)+2]=line_ptr[0];
			line_ptr+=3;
		}
		ptr+=bitmap.bmWidthBytes;
	}
	DeleteObject(bmp_handle);
	return true;
}

#endif //__FILE_IO_BMP_IO_H__
