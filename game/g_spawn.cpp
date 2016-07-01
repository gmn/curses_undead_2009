//
//
#include "g_entity.h"

extern bool com_freeze_thinkers;



int Spawn_t::id = 0;

void Spawn_t::setBlockWait( int wait_period ) {
	timer.set( wait_period );
	state = SPAWN_BLOCKING;
	::com_freeze_thinkers = true;
}

void Spawn_t::think( void ) {
	if ( SPAWN_NORMAL == state )
		return;
	if ( timer.check() ) {
		::com_freeze_thinkers = false;
		state = SPAWN_NORMAL;
	}
}
