// arty.h

#pragma once
#include "SDL.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


int ARTY_Init(int src_w,
              int src_h,
              int disp_w,
              int disp_h,
              float fps);

void ARTY_Update(const uint8_t *indexed,
                 const SDL_Color *palette);

void ARTY_Shutdown(void);

#ifdef __cplusplus
}
#endif