#ifndef __G_COLLISION_H__
#define __G_COLLISION_H__

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ALL POSSIBLE COLLISION OUTCOMES:

- player wounded - (moved to change entity attribute)
- npc wounded - ( moved to entity attribute )
- item picked up
- dialog box spawned ("are you sure you would like to leave this area?),
	("do you want to pick up the Twinkies?")
- player attribute changed (health vials)
- trigger some event
	- door opens
	- move to new area
	- trigger a subArea 
	- end level
- noise caused (ie when punching a wall, you hear a thud, when punching chain)
	link fence you hear a chishsh of chainlink. ) 
- bullet hits wall, decal added to wall
- player/entity walk into wall, just not allowed to pass through wall
	or furniture, or closed door.  these all act as simple barriers.
- an animation could spawn nearby
- Dialog Screen triggered Talk to NPC, NPC talks to you.  see **note:
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

// all possible outcomes of a collision
enum CollisionOutcome_t {
	COL_NONE,
	COL_ATTRIBUTE, 			// change entity attribute
	COL_ITEM, 				// entity gets item
	COL_DIALOGBOX,			// create a dialog box
	COL_EVENT, 				// add an event
	COL_SOUND,				// play a sound
	COL_DECAL,				// leave a decal
	COL_BLOCK,				// hit a wall, no pass
	COL_ANIMATOR,			// spawn animators
	COL_DIALOG, 			// trigger dialog screen mode from NPC encounter
};

// 
// collision handler for an entity
//
// what do do, who to call, if there is a collision
//
class collisionHandler_t : public Auto_t {
private:
	void _my_init() { }
	void _my_reset() { }
	void _my_destroy() { }
public:


};


#endif //
