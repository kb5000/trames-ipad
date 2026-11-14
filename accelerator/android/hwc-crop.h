// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TRAMES_ANDROID_HWC_CROP_H
#define TRAMES_ANDROID_HWC_CROP_H
#include <stdbool.h>
#include <stdint.h>
// HWC2-on-1 can pair the new display extent with an old client target during
// resize. Wayland requires the source to fit the actually attached buffer.
static bool trames_hwc_crop(float crop[4],uint32_t width,uint32_t height) {
    float w=width,h=height;
    if(crop[0]>=0 && crop[1]>=0 && crop[2]<=w && crop[3]<=h)return false;
    float left=crop[0]<0?0:crop[0]>w-1?w-1:crop[0];
    float top=crop[1]<0?0:crop[1]>h-1?h-1:crop[1];
    float right=crop[2]>w?w:crop[2]<left+1?left+1:crop[2];
    float bottom=crop[3]>h?h:crop[3]<top+1?top+1:crop[3];
    crop[0]=left;crop[1]=top;crop[2]=right;crop[3]=bottom;return true;
}
#endif
