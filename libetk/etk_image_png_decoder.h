#ifndef _ETK_IMAGE_PNG_DECODER_H
#define _ETK_IMAGE_PNG_DECODER_H
#include "etk_image_decoder.h"


EtkImageDecoder* etk_image_png_decoder_create(void);
void etk_image_png_decoder_destroy(EtkImageDecoder* thiz);
EtkBitmap* etk_image_png_decoder_decode(EtkImageDecoder* thiz, const char* filename);

#endif