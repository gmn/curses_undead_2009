#ifndef __G_ITEM_H__
#define __G_ITEM_H__

#include "g_entity.h"

/*
==============================================================================

    Item definitions

==============================================================================
*/


enum {
    // tools
    ITEM_PLIERS,
    ITEM_GASCAN,
    ITEM_303AMMO,
    ITEM_LOCKBOX,
    ITEM_BAR_OF_SOAP,
    ITEM_TENT,
    ITEM_FRENCH_PRESS,
    ITEM_DIGITAL_CAMERA,
    ITEM_CALCULATOR_WATCH,
    ITEM_SUNGLASSES,
    ITEM_WINTER_COAT,
    ITEM_RAINCOAT,
    ITEM_CAMPING_STOVE,
    ITEM_BINOCULARS,
    ITEM_FLASHLIGHT,
    ITEM_BASSOMATIC3000,
    ITEM_POCKET_FISHERMAN,
    ITEM_TIRE_IRON,
    ITEM_KITCHENAID_MIXER,
    ITEM_CUISINART,
    ITEM_SHOVEL,
    ITEM_DUSTY_OLD_BOOK,
    ITEM_KEYCARD_RED,
    ITEM_KEYCARD_YELLOW,
    ITEM_KEYCARD_BLUE,
    ITEM_KEYCARD_WHITE,
    ITEM_KEYCARD_GREEN,
    ITEM_BOAT_KEYS,
    ITEM_SLEEPING_BAG,
    ITEM_JEWELRY,
    ITEM_MONEY,
    ITEM_WATER_JUG,
    ITEM_ROPE,
    ITEM_GENERATOR,
    ITEM_WRENCHES,
    ITEM_WATER_PURIFIER,
    ITEM_WATER_TABLETS,
    ITEM_ANTIBIOTICS,
    ITEM_LAPTOP,
    ITEM_CELLPHONE,
    ITEM_RADIO,
    ITEM_NEWSPAPER,
    ITEM_LEATHERMAN,
    ITEM_MAGAZINE,
    ITEM_DUCTTAPE,
    ITEM_FLAREGUN, 

    // weapons
    ITEM_RIFLE,
    ITEM_BASEBALL_BAT,
    ITEM_COMPOUND_BOW,
    ITEM_CHEFS_KNIFE,
    ITEM_ZOMBIE_SYRUM,

    // food
    ITEM_BEANS,
    ITEM_TUNA,
    ITEM_BOX_OF_WINE,
    ITEM_SARDINES,
    ITEM_CHOCOLATE,
    ITEM_COFFEE,
    ITEM_VODKA,
    ITEM_IBUPROFIN,
    ITEM_TEQUILA,
    ITEM_SALAMI,
    ITEM_RAVIOLI,
    ITEM_CRACKERS,
    ITEM_CHIPS,
    ITEM_MARIJUANA,
    ITEM_SCOTCH,
    ITEM_BEER,
    ITEM_SALMON,
    ITEM_PISTACIOS,
    ITEM_CANDYBAR,
    ITEM_SODAPOP,
    ITEM_BOTTLED_WATER,
    ITEM_CIGARETTES,
    ITEM_PEACHES,
    ITEM_BEEFJERKY,

    TOTAL_ITEM_TAGS
};

enum itemClass_e {
    ITEM_TEMPLATE           =       -2,
    ITEM_NOTYPE             =       -1,

    ITEM_WEAPON             =       1,
    ITEM_FOOD,
    ITEM_TOOL,

    ITEM_SPECIAL, // save this 
};

enum itemParms_e {
    ITEM_PICKUP         = 1, // must be set in order to pickup item
    ITEM_INSTANT        = 2, // has instant effect
    ITEM_MSG_ONLY       = 4
};

enum itemEffectParms_e {
    ITEM_HITPOINTS      = 1,
    ITEM_MESSAGE        = 2,
    ITEM_AMMOINC_303    = 4,

    ITEM_STATE_DRUNK    = 8,
    ITEM_STATE_SICK     = 16,
    ITEM_STATE_HEALTH_BOOST = 32,
    ITEM_STATE_ARMOR_BOOST = 64,
    ITEM_STATE_TOHIT_BOOST = 128,
    ITEM_STATE_CURE_ILLNESS = 256,
};

struct itemEffect_t {
    const int parms;    // itemEffectParms_e
    const int hitpoints;
    const char *message;
};

#define _nullEffect { 0, 0, NULL }

/*
*** ITEMS, Furniture & NPCs get both 
    - PROXIMITY MESSAGE                 (prints EntityMessage_t)
    - USE MESSAGE                       (
    - USE EFFECTS (eg. grant health, alter weapon damage, make drunk, 
                        hurt health, destroy item, )
    - DETAIL INFO (items get this in inventory screen)
*/

// static table to hold definitions for all items in the game
struct itemDef_s {
	int iclass;                  
	int item_id;                
    int iparms;
    int value;
    int weight;
    int uses;
	const char * name;
    const char * proxMsg;   // phrase that comes after: "you see "
    const char * useMsg;
    const char * detailInfo;
    itemEffect_t ieffect;
} ;

extern const unsigned int _totalItemDefs;
#define TOTAL_ITEM_DEFS _totalItemDefs

#ifndef NULL
    #define NULL 0
#endif

extern itemDef_s itemDefsTable[];



/*
==============================================================================

    Item_t

==============================================================================
*/
class Item_t : public Entity_t {
private:

	static int id;

	void _my_reset() {
		Entity_t::_my_reset();
		entType = ET_ITEM;
		entClass = EC_ITEM;
	}
	
public:

	void baseInit() { // sets base class to vanilla Item_t
		entType = ET_ITEM;
		entClass = EC_ITEM;
		ioType = OUT_TYPE;
		collidable = true;
		clipping = true;
        triggerable = false;
		AutoName( "Item", &Item_t::id );
		anim = NULL;
        mat = NULL;

		// isn't fully typed until running through copy-ctor
		item_id = ITEM_NOTYPE;
		iclass = ITEM_NOTYPE;
	}
		

    Item_t();
    Item_t( int iclass,
            int item_id,
            int iparms,
            const char *name = NULL,
            const char *proxMsg = NULL, 
            const char *useMsg = NULL, 
            const char *detailInfo = NULL,     
            const itemEffect_t * ieffect = NULL );

	Item_t( Item_t const & );
    ~Item_t() { /* all shared data, nothing to delete */ }

	int                     iclass;         // food || tool || weapon
    int                     item_id;        
    int                     iparms;

    const char *            proxMsg;
    const char *            useMsg;
    const char *            detailInfo;

    const itemEffect_t *    ieffect;    // either an useEffect or touchEffect

    int                     value;
    int                     weight;
    int                     uses;


    // just removes the item from map blocks only, it still exists
    void removeFromMap( void );

    // item will be removed next server pass
    void markForDeletion( void );

    void removeFromMapAndDelete( void ) ;
    
};

inline void Item_t::markForDeletion( void ) {

}

inline void Item_t::removeFromMapAndDelete( void ) {
    removeFromMap();
    markForDeletion();
}



/*
==============================================================================

    ItemHandler_c

==============================================================================
*/
// singleton
class ItemHandler_c : public Allocator_t {
    static void Handle( Player_t *p, Item_t *I );

    // loops through table, finds match, passes in all params to constructor
    static Item_t * CreateItemFromID( int id ) ;

    static int getClassFromID( int id ) ;
    
    static void handleItemEffect( Player_t * p, const itemEffect_t * E );
};

inline int ItemHandler_c::getClassFromID( int id ) {
    for ( int i = 0; i < TOTAL_ITEM_DEFS; i++ ) {
        if ( id == itemDefsTable[ i ].item_id )
            return itemDefsTable[i].iclass;
    }
    return -1;
}


#endif /* !__G_ITEM_H__ */
