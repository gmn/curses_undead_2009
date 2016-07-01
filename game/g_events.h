// g_event.h
//
#ifndef __G_EVENTS_H__
#define __G_EVENTS_H__

enum GameEvent_t {
	GE_EMPTY,
	GE_SPAWN_ANIMATION,
	GE_START_SOUND,
	GE_TELEPORT, 			// actual map-coord re-location
	GE_PLAYER_HIT,
	GE_ENTITY_HIT,
	GE_DIALOG_BOX,
	GE_DIALOG_SCREEN,
	GE_DECAL,
	GE_PLAYER_FREEZE,		// player stops but world-entities keep going.
	GE_WORLD_FREEZE,		// like pause button
	GE_OPEN_DOOR, 			// caused by trigger, or USE_KEY
	GE_SUB_AREA, 
	GE_PLAYER_ATTRIBUTE, 	// health vial raises health

	TOTAL_EVENT_TYPES
};

/* event system */

/* note: events take the delay argument as a delay into the future, but when
the event is created, this is turned into starting time by adding to the 
current millisecond */ 

typedef int(*eventExecutor_t)(void);



// Base Event Type
class Event_t : public Allocator_t {
private:
public:
	friend int _EVENT_spawnAnim_exec( void );
	friend int _EVENT_startSound_exec( void );

	static const volatile unsigned int NAME_SIZE = 256;

	GameEvent_t 			type;

	char 					name[ NAME_SIZE ];
	float 					pt[ 2 ];
	int 					time;
	int 					start;
	eventExecutor_t			exec;

	void set( GameEvent_t, const char *, float *, int =0, int =0 );

	Event_t() : type(GE_EMPTY), time(0), start(0), exec(NULL) {
		name[ 0 ] = '\0';
		pt[ 0 ] = pt[ 1 ] = 0.f;
	} 
	virtual ~Event() {}
};



/*
======================================================================
 EventHandler_t
======================================================================
*/
class EventHandler_t : public Auto_t {
private:
	void _my_init() { events.init(); }
	void _my_reset() { events.reset(); }
	void _my_destroy() { events.destroy(); }

	// tmp for copyin by value
	Event_t e;

	// queue of events to be run
	list_c<Event_t> events;

	// remove an event from a running queue w/o losing your place in line
	node_c<Event_t> * EventHandler_t::pluck( node_c<Event_t> * );
public:

	// run the event queue
	void Run( void );

	/**
	 * anim name
	 * start point
	 * program FIXME, animators come in 3 or 4 different types, they can move
	 * length (optional)
	 * delay (optional)
	 */
	void spawnAnimation( const char *anim, float *pt, int len =0, int dly =0 ) {
		e.set( GE_SPAWN_ANIMATION, anim, pt, len, dlt );
		events.add( e );
	}
	/**
	 * sound name
	 * length (optional)
	 * delay (optional)
	 */
	void startSound( const char *snd, int len =0, delay =0 ) {
		e.set( GE_START_SOUND, snd, NULL, len, delay );
		events.add( e );
	}
	/**
	 * target Area_t
	 * coordinates
	 * delay (optional)
	 */
	void teleport( const char *areaName, float *dest, int delay =0 ) {
		e.set( GE_TELEPORT, areaName, dest, 0, delay );
		events.add( e );
	}
};

extern EventHandler_t events;

inline void G_RunEvents( void ) {
	events.Run();
}

#endif // __G_EVENTS_H__
