#include "etk_font.h"
#include "etk_platform.h"
#include "etk_util.h"

u8  ASCII[1536]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,								//20
 0,0,0,0,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0,0x18,0x18,0,0,
 0,0,0,0,0x66,0x66,0x66,0x24,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0,0,
 0,0,0x18,0x18,0x7C,0xC6,0xC2,0xC0,0x7C,0x6,0x86,0xC6,0x7C,0x18,0x18,0,
 0,0,0,0,0,0,0xC2,0xC6,0xC,0x18,0x30,0x60,0xC6,0x86,0,0,
 0,0,0,0,0x38,0x6C,0x6C,0x38,0x76,0xDC,0xCC,0xCC,0xCC,0x76,0,0,
 0,0,0,0,0x30,0x30,0x30,0x60,0,0,0,0,0,0,0,0,
 0,0,0,0,0xC,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x18,0xC,0,0,
 0,0,0,0,0x30,0x18,0xC,0xC,0xC,0xC,0xC,0xC,0x18,0x30,0,0,
 0,0,0,0,0,0,0,0x66,0x3C,0xFF,0x3C,0x66,0,0,0,0,
 0,0,0,0,0,0,0,0x18,0x18,0x7E,0x18,0x18,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0x18,0x18,0x18,0x30,0,0,
 0,0,0,0,0,0,0,0,0,0xFE,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0x18,0x18,0,0,
 0,0,0,0,0,0,0x2,0x6,0xC,0x18,0x30,0x60,0xC0,0x80,0,0,
 0,0,0,0,0x7C,0xC6,0xC6,0xCE,0xD6,0xD6,0xE6,0xC6,0xC6,0x7C,0,0,	//30
 0,0,0,0,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0,0,
 0,0,0,0,0x7C,0xC6,0x6,0xC,0x18,0x30,0x60,0xC0,0xC6,0xFE,0,0,
 0,0,0,0,0x7C,0xC6,0x6,0x6,0x3C,6,6,6,0xC6,0x7C,0,0,
 0,0,0,0,0xC,0x1C,0x3C,0x6C,0xCC,0xFE,0xC,0xC,0xC,0x1E,0,0,
 0,0,0,0,0xFE,0xC0,0xC0,0xC0,0xFC,0xE,6,6,0xC6,0x7C,0,0,
 0,0,0,0,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,
 0,0,0,0,0xFE,0xC6,6,6,0xC,0x18,0x30,0x30,0x30,0x30,0,0,
 0,0,0,0,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,
 0,0,0,0,0x7C,0xC6,0xC6,0xC6,0x7E,6,6,6,0xC,0x78,0,0,
 0,0,0,0,0,0,0x18,0x18,0,0,0,0x18,0x18,0,0,0,
 0,0,0,0,0,0,0x18,0x18,0,0,0,0x18,0x18,0x30,0,0,
 0,0,0,0,0x6,0xC,0x18,0x30,0x60,0x30,0x18,0xC,0x6,0,0,0,
 0,0,0,0,0,0,0,0xFE,0,0,0xFE,0,0,0,0,0,
 0,0,0,0,0,0x60,0x30,0x18,0xC,6,0xC,0x18,0x30,0x60,0,0,
 0,0,0,0,0x7C,0xC6,0xC6,0xC,0x18,0x18,0x18,0,0x18,0x18,0,0,
 0,0,0,0,0,0x7C,0xC6,0xC6,0xDE,0xDE,0xDE,0xDC,0xC0,0x7C,0,0,	//40
 0,0,0,0,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0,0,
 0,0,0,0,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0xFC,0,0,
 0,0,0,0,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0,0,
 0,0,0,0,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0,0,
 0,0,0,0,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x62,0x66,0xFE,0,0,
 0,0,0,0,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0x60,0xF0,0,0,
 0,0,0,0,0x3C,0x66,0xC2,0xC0,0xC0,0xDE,0xC6,0xC6,0x66,0x3A,0,0,
 0,0,0,0,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0xC6,0,0,
 0,0,0,0,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,
 0,0,0,0,0x1E,0xC,0xC,0xC,0xC,0xC,0xCC,0xCC,0xCC,0x78,0,0,
 0,0,0,0,0xE6,0x66,0x6C,0x6C,0x78,0x78,0x6C,0x66,0x66,0xE6,0,0,
 0,0,0,0,0xF0,0x60,0x60,0x60,0x60,0x60,0x60,0x62,0x66,0xFE,0,0,
 0,0,0,0,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0xC6,0,0,
 0,0,0,0,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0xC6,0xC6,0xC6,0,0,
 0,0,0,0,0x38,0x6C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0,0,
 0,0,0,0,0xFC,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x60,0xF0,0,0,	//50
 0,0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0xC,0,
 0,0,0,0,0xFC,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0x66,0xE6,0,0,
 0,0,0,0,0x7C,0xC6,0xC6,0x60,0x38,0xC,0x6,0xC6,0xC6,0x7C,0,0,
 0,0,0,0,0x7E,0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,
 0,0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,
 0,0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0,0,
 0,0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0x6C,0x6C,0,0,
 0,0,0,0,0xC6,0xC6,0x6C,0x6C,0x38,0x38,0x6C,0x6C,0xC6,0xC6,0,0,
 0,0,0,0,0x66,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x18,0x3C,0,0,
 0,0,0,0,0xFE,0xC6,0x86,0xC,0x18,0x30,0x60,0xC2,0xC6,0xFE,0,0,
 0,0,0,0,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0,0,
 0,0,0,0,0,0x80,0xC0,0xE0,0x70,0x38,0x1C,0xE,0x6,2,0,0,
 0,0,0,0,0x3C,0xC,0xC,0xC,0xC,0xC,0xC,0xC,0xC,0x3C,0,0,
 0,0,0x10,0x38,0x6C,0xC6,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xFF,0,
 0,0,0x30,0x30,0x18,0,0,0,0,0,0,0,0,0,0,0,						//60
 0,0,0,0,0,0,0,0x78,0xC,0x7C,0xCC,0xCC,0xCC,0x76,0,0,
 0,0,0,0,0xE0,0x60,0x60,0x78,0x6C,0x66,0x66,0x66,0x66,0xDC,0,0,
 0,0,0,0,0,0,0,0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0,0,
 0,0,0,0,0x1C,0xC,0xC,0x3C,0x6C,0xCC,0xCC,0xCC,0xCC,0x76,0,0,
 0,0,0,0,0,0,0,0x7C,0xC6,0xFE,0xC0,0xC0,0xC6,0x7C,0,0,
 0,0,0,0,0x38,0x6C,0x64,0x60,0xF0,0x60,0x60,0x60,0x60,0xF0,0,0,
 0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0xCC,0xCC,0x7C,0xC,0xCC,0x78,0,
 0,0,0,0,0xE0,0x60,0x60,0x6C,0x76,0x66,0x66,0x66,0x66,0xE6,0,0,
 0,0,0,0,0x18,0x18,0,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,
 0,0,0,6,6,0,0xE,0x6,0x6,0x6,0x6,0x6,0x66,0x66,0x3C,0,
 0,0,0,0,0xE0,0x60,0x60,0x66,0x6C,0x78,0x78,0x6C,0x66,0xE6,0,0,
 0,0,0,0,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0,0,
 0,0,0,0,0,0,0,0xEC,0xFE,0xD6,0xD6,0xD6,0xD6,0xD6,0,0,
 0,0,0,0,0,0,0,0xDC,0x66,0x66,0x66,0x66,0x66,0x66,0,0,
 0,0,0,0,0,0,0,0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0,0,
 0,0,0,0,0,0xDC,0x66,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0xF0,0,	//70
 0,0,0,0,0,0x76,0xCC,0xCC,0xCC,0xCC,0xCC,0x7C,0xC,0xC,0x1E,0,
 0,0,0,0,0,0,0,0xDC,0x76,0x62,0x60,0x60,0x60,0xF0,0,0,
 0,0,0,0,0,0,0,0x7C,0xC6,0x60,0x38,0xC,0xC6,0x7C,0,0,
 0,0,0,0,0x10,0x30,0x30,0xFC,0x30,0x30,0x30,0x30,0x36,0x1C,0,0,
 0,0,0,0,0,0,0,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0,0,
 0,0,0,0,0,0,0,0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0,0,
 0,0,0,0,0,0,0,0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0x6C,0,0,
 0,0,0,0,0,0,0,0xC6,0x6C,0x38,0x38,0x38,0x6C,0xC6,0,0,
 0,0,0,0,0,0,0xC6,0xC6,0xC6,0xC6,0xC6,0x7E,0x6,0xC,0xF8,0,
 0,0,0,0,0,0,0,0xFE,0xCC,0x18,0x30,0x60,0xC6,0xFE,0,0,
 0,0,0,0,0xE,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0xE,0,0,
 0,0,0,0,0x18,0x18,0x18,0x18,0,0x18,0x18,0x18,0x18,0x18,0,0,
 0,0,0,0,0x70,0x18,0x18,0x18,0xE,0x18,0x18,0x18,0x18,0x70,0,0,
 0,0,0,0,0x76,0xDC,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0x10,0x38,0x6C,0xC6,0xC6,0xC6,0xFE,0,0,0
};

Ret etk_font_default_load(EtkFont *thiz,e8 *filename){
	u32 size=243712;
	thiz->data=(char*)ETK_MALLOC(size);
	memset(thiz->data,0,size);
	read_file_to_buffer((char*)thiz->data,filename,size);
	thiz->width=16;
	thiz->height=16;
	return RET_OK;
}
EtkFont *etk_font_default_create(e8 *filename){
	EtkFont *thiz=(EtkFont*)ETK_MALLOC(sizeof(EtkFont));
	etk_font_default_load(thiz,filename);
	return thiz;
}