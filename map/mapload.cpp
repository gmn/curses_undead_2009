
#include "mapdef.h"
#include "../mapedit/mapedit.h"

// 
int MapLoad( const char *map ) {

	if ( ME_LoadMap( map ) ) {
		return -1;
	}
}

