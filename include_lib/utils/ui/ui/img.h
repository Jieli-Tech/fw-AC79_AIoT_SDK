#ifndef UI_IMG_H
#define UI_IMG_H

#include "ui/ui_core.h"




//struct element_image {
//struct element elm;           //must be first
//const struct image_info *info;
//u8  index;
//u32 src;
////int flag;
//};





#define image_set_css(img, css) \
	ui_core_set_element_css(&(img)->elm, css)









#endif
