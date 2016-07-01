// g_item.cpp
//

#define __MAPEDIT_H__ 1
#define __M_AREA_H__ 1
#include "g_entity.h"
#include "g_item.h"

/*
==============================================================================

    Item_t

==============================================================================
*/
int Item_t::id = 0;


// this is only useful for the mapEditor, where we are still allowed to
//  change the non-const item_id.  which later can be read from a map,
//  and the proper item be rebuilt from that single id.
// obviously later on it would be a good idea to store item definition
//  tables in script or XML, something read in by the program, to allow
//  for modding donchaknow, but for now they are hard-coded in itemDefs.h
//  the only way you can create a fully formed Item_t is by using the 
//  ItemHandler_c::CreateItemFromID factory method.  Items created in 
//  the map editor are just placeholders with the correct id. 
Item_t::Item_t() : Entity_t() {
    baseInit();
}

/*
====================
 Item_t

create Item_t from itemDef_s
====================
*/
Item_t::Item_t( int _iclass,
                int _item_id,
                int _iparms,
                const char *_name, 
                const char *_proxMsg,
                const char *_useMsg,
                const char *_detailInfo,
                const itemEffect_t * _ieffect ) :
            Entity_t(),
            proxMsg(_proxMsg),
            useMsg(_useMsg),
            detailInfo(_detailInfo),
            ieffect(_ieffect) {
    baseInit(); 
    if ( _name )
        strcpy( name, _name );
    iclass = _iclass;
    item_id = _item_id;
    iparms = _iparms;
}

/*
====================
 Item_t( & )
====================
*/
Item_t::Item_t( Item_t const & I ) : 
                        Entity_t( I ),
                        proxMsg(I.proxMsg),
                        useMsg(I.useMsg),
                        detailInfo(I.detailInfo),
                        ieffect(I.ieffect) {
    strcpy( name, I.name );
    iclass = I.iclass;
    item_id = I.item_id;
    iparms = I.iparms;
}



/*   OLD

// item's identity is determined solely by it's material

Item_t::Item_t( Item_t const & I ) : Entity_t( I ) {
	basicShit();

	name[ ENTITY_NAME_SIZE - 1 ] = '\0';

	if ( !I.mat )
		return;

	// turkey leg
	if ( !strcmp( "turkeyLeg.tga", I.mat->name ) ) {
		iclass = ITEM_FOOD;
		itype = ITEM_TURKEY_LEG;
		_snprintf( name, ENTITY_NAME_SIZE-1, "Turkey Leg" );

		health = 5;
		uses = 1;
	} 

	// large medkit 
	else if ( !strcmp( "medkit01.tga", I.mat->name ) ) {
		iclass = ITEM_TOOL;
		itype = ITEM_MEDKIT1;
		_snprintf( name, ENTITY_NAME_SIZE-1, "Large MedKit" );

		health = 10;
		uses = 1;
	}

	// 
}
*/

/*
====================
 Item_t::removeFromWorld
====================
*/
void Item_t::removeFromMap( void ) {
}


/*
==============================================================================

    ItemHandler_c

==============================================================================
*/

/*
====================
 ItemHandler_c::Handle

 called by a player when in contact with an Item on the map
====================
*/
void ItemHandler_c::Handle( Player_t *p, Item_t *I ) {
    if ( I->iparms & ITEM_PICKUP ) {
        /* player picks up item, print STANDARD MESSAGES,
        - food items
        - various gear */
        if ( p->pickupItem( I ) )
            I->removeFromMap();
    } else if ( I->iparms & ITEM_INSTANT ) { 
        /* handle instant effect, and remove */
        handleItemEffect( p, I->ieffect );
        I->removeFromMapAndDelete();
    } else if ( I->iparms & ITEM_MSG_ONLY ) {
        /* print useMsg when you walk over it, but dont pick it up, 
        and dont remove it from the map.
        if ( I->useMsg ) 
            entMessages.Print( I->useMsg );

        might be useful for signs, or placards on the walls of buildings,
        gravestones perhaps..
        */
    }
}

/*
====================
 ItemHandler_c::CreateItemFromID()

 loops through table, finds match, passes in all params to const constructor
====================
*/
Item_t * ItemHandler_c::CreateItemFromID( int id ) {
    int i;
    for ( i = 0; i < TOTAL_ITEM_DEFS; i++ ) {
        if ( id == itemDefsTable[i].item_id ) 
            break;
    }

    if ( i == TOTAL_ITEM_DEFS )
        return NULL;

    itemDef_s * D = &itemDefsTable[ i ]; 
    
    Item_t *I = new Item_t( D->iclass,
                            D->item_id, 
                            D->iparms,
                            D->name,
                            D->proxMsg,
                            D->useMsg,
                            D->detailInfo,
                            &D->ieffect );
    return I;
}

/*
====================
 handleItemEffect

    FIXME: v2, move this to Player_t, because some items effects may be blocked
        or enhanced by: other items, playerState, etc... therefor, manage
        within the player class where we have full access to player data
====================
*/
void ItemHandler_c::handleItemEffect( Player_t * p, const itemEffect_t * E ) {
    if ( E->parms & ITEM_HITPOINTS ) {
        p->adjustHitpoints( E->hitpoints );
    }
    if ( E->parms & ITEM_MESSAGE ) {
//        if ( E->message ) 
//            entMessages.putMsg( E->message );
    }
}
