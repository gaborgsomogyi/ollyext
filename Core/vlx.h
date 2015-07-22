#ifndef _VLX_H_
#define _VLX_H_

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <string.h>

#define MAX_LOOK_LENGTH				((u32)8190)
#define MAX_WINDOW_LENGTH			((u32)4095)
#define MIN_WINDOW_LENGTH			2

#define MAX(v1,v2)		(v1)>=(v2)?(v1):(v2)
#define MIN(v1,v2)		(v1)<=(v2)?(v1):(v2)

typedef unsigned char			u8;
typedef unsigned short int		u16;
typedef unsigned int			u32;
typedef signed char				s8;
typedef signed short int		s16;
typedef signed int				s32;

struct vlxdecomptab
{
	u16		b,l,s;
	u32		h,m;
	// l - bit length of hufman code
	// h - huffman code
	// b - number of bits to fill in with window length or relpos
	// m - hufmann mask
	// s - save to header
};

bool vlx_decompress( void* pData, DWORD size, void** pUncompressedData, DWORD* pUncompressedSize );

#endif // _VLX_H_
