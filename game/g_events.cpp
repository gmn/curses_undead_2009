// g_event.cpp
//

node_c<Event_t> * _EVENT_spawnAnim_exec( void ) {
	if ( now() < start ) {
		return NULL;
	}

	// create/get new Animator
	// location
	// timer
	// delay
	// put animator in it's correct block->entities
}

node_c<Event_t> * _EVENT_startSound_exec( void ) {
	// if delay until start, check enough time is passed
	if ( now() < start ) { 
		return NULL;
	}

	// call the sound code
	// time limit is optional
}

node_c<Event_t> * _EVENT_teleport_exec( void ) {
}
node_c<Event_t> * _EVENT_playerHit_exec( void ) {
}
node_c<Event_t> * _EVENT_entityHit_exec( void ) {
}
node_c<Event_t> * _EVENT_dialogBox_exec( void ) {
}
node_c<Event_t> * _EVENT_dialogScreen_exec( void ) {
}
node_c<Event_t> * _EVENT_decal_exec( void ) {
}
node_c<Event_t> * _EVENT_playerFreeze_exec( void ) {
}
node_c<Event_t> * _EVENT_worldFreeze_exec( void ) {
}
node_c<Event_t> * _EVENT_activateDoor_exec( void ) {
}
node_c<Event_t> * _EVENT_subArea_exec( void ) {
}
node_c<Event_t> * _EVENT_playerAttribute_exec( void ) {
}

void Event_t::set( 	GameEvent_t _type,
					const char *_name,
					float *_v,
					int _t,
					int _d ) {

	// always a type	
	Assert( _type < TOTAL_EVENT_TYPES );
	type = _type;

	// may be NULL
	if ( _name ) {
		strncpy( name, _name, NAME_SIZE );
		name[ NAME_SIZE - 1 ] = '\0';
	} else {
		_name[ 0 ] = '\0';
	}

	// may be NULL
	if ( _v ) { 
		pt[ 0 ] = _v[ 0 ];
		pt[ 1 ] = _v[ 1 ];
	} else {
		pt[ 0 ] = pt[ 1 ] = 0.f;
	}

	// default arg to 0
	time = _t;
	start = ( _d == 0 ) 0 : _d + now();

	switch ( type ) {
	case GE_SPAWN_ANIMATION:
		exec = _EVENT_spawnAnim_exec; break;
	case GE_START_SOUND:
		exec = _EVENT_startSound_exec; break;
	case GE_TELEPORT:
		exec = _EVENT_teleport_exec; break;
	case GE_PLAYER_HIT:
		exec = _EVENT_playerHit_exec; break;
	case GE_ENTITY_HIT:
		exec = _EVENT_entityHit_exec; break;
	case GE_DIALOG_BOX:
		exec = _EVENT_dialogBox_exec; break;
	case GE_DIALOG_SCREEN:
		exec = _EVENT_dialogScreen_exec; break;
	case GE_DECAL:
		exec = _EVENT_decal_exec; break;
	case GE_PLAYER_FREEZE:
		exec = _EVENT_playerFreeze_exec; break;
	case GE_WORLD_FREEZE:
		exec = _EVENT_worldFreeze_exec; break;
	case GE_ACTIVATE_DOOR:
		exec = _EVENT_activateDoor_exec; break;
	case GE_SUB_AREA:
		exec = _EVENT_subArea_exec; break;
	case GE_PLAYER_ATTRIBUTE:
		exec = _EVENT_playerAttribute_exec; break;
	default:
		exec = NULL; break;
	}
}

node_c<Event_t> * EventHandler_t::pluck( node_c<Event_t> *node ) {
	if ( !node )
		return NULL;
	node_c<Event_t> *tmp_next = node->next;
	events.popnode( node );
	return tmp_next;
}

void EventHandler_t::Run( void ) {
	node_c<Event_t> *node = events.gethead();
	node_c<Event_t> *next;
	while ( node ) {
		// remove null event from run queue
		if ( !node->exec ) {
			node = pluck( node );
			continue;
		}

		// successful return means, event was executed and removed. next is
		//  returned
		if ( (next=node->exec()) ) {
			node = next;
			continue;
		}

		node = node->next;
	}
}

