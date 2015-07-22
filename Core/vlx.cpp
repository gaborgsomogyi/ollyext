#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include "vlx.h"

u32 rbitstart;		// read buffer bit start
u8 *rbuf;
u8 *wbuf;
u32 rbufi;

// -------------------------------------------------
//  read_bits
// -------------------------------------------------
inline u32 read_bits()
{
	u32 i,bits=0;
	
	for( i=0; i<4; ++i ) bits=(bits<<8)+(u32)rbuf[rbufi+i];
	bits<<=rbitstart;
	return bits;
}


// -------------------------------------------------
//  skip_bits
// -------------------------------------------------
inline void skip_bits(u32 skipnum)
{
	for( rbitstart+=skipnum; rbitstart>7; rbitstart-=8 ) ++rbufi;
}


u32 vlxpowtab[]=
{
	1,2,4,8,16,32,64,128,256,512,1024,2048,4096
};

// -------------------------------------------------
//  vlx_decompress
// -------------------------------------------------
bool vlx_decompress(void* pData, DWORD size, void** pUncompressedData, DWORD* pUncompressedSize)
{
	u32 iflen,wflen,wfpos,hi;
	vlxdecomptab lbit[12],wbit[12];
	u32 i,j,lbi,wbi;
	u32 bits,wrel,wlen;

	// read file
	rbuf = (u8*)pData;
	iflen = size;
	if ( rbuf==NULL )
	{
		return false;
	}
	
	// header
	if ( rbuf[0]>>4!=0 )
	{
		return false;
	}
	hi=0;
	if ( (rbuf[0]&15)==1 )
	{
		wflen=rbuf[1];
		hi=2;
	}
	else if ( (rbuf[0]&15)==2 )
	{
		wflen=rbuf[1]+(((long)rbuf[2])<<8);
		hi=3;
	}
	else
	{
		wflen=rbuf[1]+(((long)rbuf[2])<<8)+(((long)rbuf[3])<<16)+(((long)rbuf[4])<<16);
		hi=5;
	}

	lbi=rbuf[hi]>>4;
	wbi=rbuf[hi]&15;
	++hi;
	for( i=0; i<lbi; ++i )
	{
		lbit[i].s=rbuf[hi]+(((u16)rbuf[hi+1])<<8);
		lbit[i].b=lbit[i].s>>12;
		lbit[i].h=lbit[i].s&0xfff;
		for( j=1<<11,lbit[i].l=11,lbit[i]; (lbit[i].h&j)==0; j>>=1,--lbit[i].l ) ;
		lbit[i].h<<=32-lbit[i].l;
		lbit[i].m=0xffffffff<<(32-lbit[i].l);
		hi+=2;
	}
	for( i=0; i<wbi; ++i )
	{
		wbit[i].s=rbuf[hi]+(((u16)rbuf[hi+1])<<8);
		wbit[i].b=wbit[i].s>>12;
		wbit[i].h=wbit[i].s&0xfff;
		for( j=1<<11,wbit[i].l=11,wbit[i]; (wbit[i].h&j)==0; j>>=1,--wbit[i].l ) ;
		wbit[i].h<<=32-wbit[i].l;
		wbit[i].m=0xffffffff<<(32-wbit[i].l);
		hi+=2;
	}

	wbuf=new u8[wflen];

	rbufi=hi;
	wfpos=0;
	rbitstart=0;
	while( wfpos<wflen )
	{
		bits=read_bits();
		for( i=0; i<lbi; ++i ) if ( (bits&lbit[i].m)==lbit[i].h ) break;
		skip_bits(lbit[i].l+lbit[i].b);

		if ( lbit[i].b==0 )
		{
			wbuf[wfpos++]=bits<<lbit[i].l>>24;
			skip_bits(8);
		}
		else
		{
			wlen=(bits<<lbit[i].l>>(32-lbit[i].b))+vlxpowtab[lbit[i].b];
			bits=read_bits();

			for( i=0; i<wbi; ++i ) if ( (bits&wbit[i].m)==wbit[i].h ) break;
			skip_bits(wbit[i].l+wbit[i].b);
			wrel=(bits<<wbit[i].l>>(32-wbit[i].b))+vlxpowtab[wbit[i].b]-1;
			//memmove(wbuf+wfpos,wbuf+wfpos-wrel,wlen);
			for( i=0; i<wlen; ++i ) wbuf[wfpos+i]=wbuf[wfpos-wrel+i];
			wfpos+=wlen;
		}
	}

	*pUncompressedData = wbuf;
	*pUncompressedSize = wflen;

	return true;
}
