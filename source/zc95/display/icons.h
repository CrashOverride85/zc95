/*

FONTX version of the Public Domain X11 misc-fixed typeface.
https://www.cl.cam.ac.uk/~mgk25/ucs-fonts.html

*/

#ifndef _ICONS_H
#define _ICONS_H

#include <inttypes.h>

// Bluetooth icon
const uint8_t bt_logo[] = {0x18, 0x94, 0x52, 0x34, 0x18, 0x34, 0x52, 0x94, 0x18}; // height = 9, width = 8

// Battery icon
const uint8_t bat_logo[3][9] =  // height = 9, width = 8*3=24
{
{0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF},
{0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},
{0xFE, 0x02, 0x02, 0x03, 0x03, 0x03, 0x02, 0x02, 0xFE}
};

/*

const unsigned char font6x9[] = {
  
};
const unsigned int font6x9_size = 12213;
*/

#endif
