#ifndef __COM_MOUSEFRAME_H__
#define __COM_MOUSEFRAME_H__

#define MOUSE_AXIAL     BIT(1)
#define MOUSE_LBUTTONS  BIT(2)
#define MOUSE_HBUTTONS  BIT(3)

#include "com_types.h"

typedef struct mouseFrame_s {
	int frameNum;
    uint32 flags;
    int x,y,z;
    byte mb[8];   // buttons (high-bit set for down )
    int dx, dy;
	
	void reset( void ) {
		frameNum = 0;
		flags = 0;
		x = y = z = 0;
		mb[0] = 0;
		mb[1] = 0;
		mb[2] = 0;
		mb[3] = 0;
		mb[4] = 0;
		mb[5] = 0;
		mb[6] = 0;
		mb[7] = 0;
		dx = dy = 0;
	}
} mouseFrame_t;

#endif
