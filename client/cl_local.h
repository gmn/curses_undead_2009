#ifndef __CL_LOCAL_H__
#define __CL_LOCAL_H__


#include "cl_public.h"

//#include "../common/common.h"
#include "../map/mapdef.h"

#include "cl_console.h"
#include "cl_keys.h"
#include "cl_sound.h"

//#include "../renderer/r_public.h"
//#include "../map/m_area.h"

extern clientState_c    cls;
extern class renderExport_t   re;

int CL_LoadMap( const char * );

void CL_GameKeyEvent( int, int, unsigned int, int );

#endif /* __CL_LOCAL_H__ */

