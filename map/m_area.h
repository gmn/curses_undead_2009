#ifndef __M_AREA_H__
#define __M_AREA_H__

/*

Area_t

	an Area_t is an arbitrarily sized box, of which there are many.  The entire
	world (map) is cut into a grid of Area_t on mapLoad.  The Area_t are axis 
	oriented
	and adjacent.  Such that you may navigate through them using north, south
	east, west pointers.  There is no guarantee that one Area_t will be smaller
	or larger than the current viewport.  Therefor a subsect of game logic
	must determine how many thinkers it must run, tiles it must draw and 
	potential collisions it must check for based on Viewport size and location
	as well as a target "thinking area", in which thinkers still think, even
	if they are not on screen.  (Eventually when you are far enough away,
	they are ignored, unless they have 'persistant' variable set, in which
	case they always think)

	An example of this would be: 
	screen size of 1280x1024, where Area_t::h == Area_t::w == 6000 in world
	coordinates.  the ratio between screen to world is 6, so in screen coords
	the Area box would be 1000x1000.  

	The game code would have a target size to 'run' per-tick of say 20000x20000
	in world coordintes, which gives 3333x3333 in screen coordinates, a little
	under 3 screen widths, and over 3 screen heights.
	it would then determine how many Area_t, relattive to the viewport's
	current position and size fall within that target, then concatenate the
	pertinent lists into its main list, sort these if necessary, and "run"  
	them, first computing all possible collisions, second by calling entities 
	"think" functions, determine the result state of each entity ( based
	on usercmd_t, collision and think.)  this determines gamestate and entity
	State. then drawing all backgrounds, the visible dispTiles, then the 
	entities.

	caveat1: the lists won't be concatenated or sorted unnecessarily.  for
	instance if the viewport doesn't move from last tick, the lists are 
	the same, (excepting for persistent entities which may walk into the frame)

	caveat2: the displayList will be culled further based on viewport coords
				before it is sorted or drawn.  the sort + draw only occurs
				after the list has been fully culled.  Also, this list can be
				re-used per gametick as well, as long as: 
			- entitieslist doesn't change
			- a change in entity state doesn't introduce a new tile.
			- the viewport x,y didn't change since last tick.


class Area_t {
	int x, y, w, h;
	
	Entity_c *Entities;
	colModel_t *ColModels;
	dispTile_t *DispTiles;

	// standard linked list for accounting
	Area_t *next, *prev; 

	// geographical linked-list for engine culling
	Area_t *north, *south, *east, *west;
};

// the world is made up of Areas
class World_t {
	Area_t *current; // area in which world.x & world.y are currently inside
	list_c<Area_t*> masterlist; // top of next-prev list of all areas
	quadTree<Area_t*> quadTree;

	Entity_c *Entities;
	colModel_t *ColModels;
	dispTile_t *DispTiles;

	main_viewport_t *vp;	// always know everything about the viewport
};

World_t - the world_t is everything.  It has the stuff that is currently 
	getting displayed, as well as links to, or arrays or whatever, all the other
	potential things that could be displayed.

mapFile_t - current map.  A mapFile_t could be the inside of just one room, first
	floor of a build, or the entire city streets view.  One map isn't 
	necessarily limited to one file, nor must 1 file contain just one map,
	in fact, 1 mapfile could contain 1-to-many maps
	
Area_t - a mapFile_t is cut into Area blocks, just like before.  except that they
	should be renamed to AreaBlocks, or just Block_t

perhaps one MapFile === 1 mapFile_t.  A mapFile_t can have 1 or more Area_t.  An Area_t
would be like putting street view over there, and then off to the side, 
put room1, hallway1, room2, underground-tunnel1, also in it, off to the side,
so that when the viewport is at its furthest possible shifted place, the areas
wouldn't overlap.  but a better way to do it, would be to define the Area
boundary, and then just Never display stuff outside of it, no matter what. ok.

Block_t - new Area_t renamed.

Area_t - a Map may have one or more of these.
mapFile_t - a world may have one or more of these.

maps should be loaded and converted into a list of Areas, such that the World
really just manages a large list of Areas.  and It is an Area that is displayed,
so that there is a Area_t *currentArea;  

Areas, like we said are cut up into blocks.  now here, I need to do a 
computation  to decide whether we are using north-south-east-west pointers
or quadTrees or both.  

If we start in a 50x50 Area, we have the pointer to the bottom left, but we
want the top-right, that is 49+49 pointers we must circumnavigate through.  
But it would be really simple to go through.  You'd just go through X east
pointers, and Y north.  simple.  then, all movement would be just 1, optimal.

A quadTree would start at the Area Size, and it's finest resolution would be
1 block.  So we'd start at the center 25x25, 1 move for top-right 13x13, 2
move for top 7x7, 3 for top 4x4, 4 for top 2x2, 5 for top 1x1.  So thats a
constant amount of moves for every block we switch, sqrt(width/2).  

The problem is that we also need to know all the adjacent blocks because we
are looking through their lists and running thinkers and things.  

so the only negative effect of Not using the quadtree, is the initial 100
pointer skips to get where we want to get, worst case.  which technically is
nothing, not to mention games since the beginning of time have had load times.
this thing wouldn't even be noticeable, not even a hiccup I bet.  and once we
make it, we wouldn't need to make it again, and all subsequent changes cost
O(1), which is perfect.  

I'm skipping the quadTree.

Area_t are loaded from the mapFile_t into the World_t.  Area_t are kept in a hash
in World_t, and are referred to by unique string identifier.  This way, a 
world can have 100's of Area_t , and they can be found very quickly.  Picture
going into and out of a doorway, very quickly, we need instantaneous loading.
I just thought of something else.  Think about doing a very basic map made out
of blocks, where there are dozens of secret doors.  All you see are blocks and
empty background, if you find the secret door a new area is revealed.  How do
we reprent that?  we're still in the same Area_t.   

well, the secret door would be a .. trigger_t.  It would be a door entity,
in particular, a secretDoor_t.  It's display would be the same block.  
secretDoor_t would extend trigger, in that it needs to have information of
what exactly needs to be done to activate it, and info on what happens when
it is activated: it's display changes, but it also effects a new subArea.
that's it, I need another distinction, subArea_t.  which is a portion of an
Area_t.

Area_t & subArea_t info will need to be stored in the mapfile format.  

Areas will also be dynamically loaded, in that when we load a map, we read
in all the tiles, and convert all the tiles to dispTiles as well.  but an
Area_t may have a door or portal into another Area_t which isn't currently
loaded.  OR, here would be a good place to declare a limitation, such as,
when a Map is loaded, we load the entire damn thing at once, no matter what
the size, and all Areas in that map will get loaded equally.  An area in a
map may refer to another Area and Map by name, in which case we will have to 
load it, and this local map data will get demolished by doing this (bad guys
will re-spawn to default positions.)  or we could have some sort of 
intermediate data type to store persistent world data as, or use sqlite..

but in general, most Area back-n-forth will take place on the same map.  Only
when a stage is completed do you do all that load stuff, so it would be rare,
and hopefully, contextually the persistence of world stuff will be irrelevant.
it'd be nice to have though.

* Robinson Crusoe - Defoe
* Life of pi - Martel
* endurance - Alfred Lancing
* Conquistador - Cortez, Montezuma, & last stand of Aztecs, Buddy Levy

// Area_t are not forced to have any subArea_t.  In the case that there are no
//  subArea, then 1 default subArea is created and everything belongs to it.  
//  otherwise, an Area contains 
//  2 or more subArea.  Each subArea contains it's stuff.  The purpose of this
//  is to have mutually existing, but not necessarily mutually visible or even
//  existing areas.  Like say, you see a man outside the closed door to a vault.
//  The vault could be a separate Area_t, but since it is just one small room
//  keep it part of the current Area, but keep it not visible and anything in
//  it not running.  So then I need to have an alternative set of displayTiles
//  to show, depending on the visibility.  the alternative set can be blank,
//  in which case, nothing is drawn and whatever background, if any, just draws
//  through.  This needs some editor mode obviously


List of Shit that needs to be added to the editor:
----------------------------------------------------
- Area_t boundary definition (just dragging a box around an area)
	- 'create_area' cmd
- subArea_t boundary definition ('addsub' command, starts a select box)
	- cycle through Area select
	- cycle through subArea
	- ability to delete any Area || subArea marking 
	- popup box asks for Area name, or have default, autonumbered name set
		at creation, and ability to customize that name in the console.
	- subArea don't get a name, and are auto numbered.
	- ability to set default subArea visibility, 
	- 'subinfo' command tells what the selected subArea number is, if it's
		visible, it's dimensions.  (so that there is a way to find out the
		number later, so that a trigger can be connected to it).
	- 'set_non_display' cmd, which removes the tiles that are contained in that
		area, and allows the user to set, in the editor, alternate tiles
		(and colModels?) to be shown when the subArea IS NOT visible
	- when a subArea is selected, you may create/delete map geometry (col
		Models will be ignored) to it.  Such that the tiles won't be drawn
		in the map editor until the subArea is selected, then they will be 
		drawn over top of whatever was there.
- in the event that no area is defined, a default Area is created
	and saved to the file when the save command is called.  This way a user
	is able to compile a very simple, one room map and not even know how to
	use area, and it will run in the engine.
- add UID support to the MapFormat.  if any subArea are created, then we
	must also store UID for each Tile.  Any addition mapTiles created under
	the subArea section will have a matching UID for the tile that they go
	to.  In fact it could just be a UID and a Material/Animation.
- 'notex' command removes the texture from the current material and downgrades
	it's type


// each subArea contains it's own blocklist.  (( so that tiles are loaded,
Area & subArea Info are loaded, and then for each subArea a blocklist is 
created and connected.  Then the subArea are added to it's Area.  

// the block in the blocklist are ultimately where the atomic data is contained

// an Area_t contains the list of all subAreas.  (I'm beginning to see a problem
here.  The whole reason that this engine design used to make sense is because
I had made an assumption that I hadn't even known I'd made, about visibility)
The problem is that all subAreas are always run because , if they are invisible
we are still attempting to run their altDisplay tiles.  the problem comes in
the Block_t matrix.  for the Block_t design to work, there must only be one
block EVER for it's given area.  A map area, then owns the blocklist.  

The subArea is a way to swap contents of a defined area.  From a Room to 
Nothing, from a jagged stone, to the inside of a vault.  obviously just what
is displayed changes.  For instance, think of the roofs in Jagged alliance, 
before you have walked into a certain section of a building you still see the
roof, then when you walk under the roof becomes transparent.  Ok, this is
how it will be, just the visual changes.  not the colModel.

so, instead of a subArea being a top-down data structure, it is more of a
meta data structure, lets go back to map load.

the map is loaded to tiles.
the displayTiles and colModel and entities are created from the tiles.
the area & subarea info are read, and data structs created, and linked up.
//The blocklist is created.
all the colModels, dispTiles and Entities are added to the blocklist.
	in the case of a subArea, for each Unit, it is determined which subArea it
	is a member of, and what is added to the

Ok, question: what is our drawing queue?  Is it a member of its block, so that
on each render pass, we go through blocks and draw their contents?  or do we
have our own list somewhere., assume that the structs live in their blocks, 
but that we have a linked list in World_t that we maintain, sort, remove from 
and add to.  so that each display pass just runs the list.  BUT..

so what we lose here is that on each move we must detect if the viewport 
changed, and that potentially we must rebuild the lists.  This involves:
	- sorting
	- fast node removal
	- fast node addition

the problem with just going through the BlockLists is draw order.  each time
we render, or do collisions, we go through these unprepared blocklists, 
(( could we prepare the blocklists anyway that would help ? ))
and have to check lots of things each time, is it visible, is it even near us,

I'm going for masterlist maintenance.  We only manage the masterlists when we
have to, so that these lists will be kept optimized, and each turn they just
run.  Entities and colModels are all collided, and disp is drawn, and Entities
are drawn.  simple.

The best way I can figure to manage subAreas, is to create subArea master 
data structs and keep those in Area_t.  Each dispTile contains a pointer to
it's subArea that it is a member of.  Then as we run the master disp list
we are forced to check it's SubAreas visibility each time we draw it?  naww,
that seems unneccesary.  

ok, instead, each dispTile has it's *mat pointer, and an *alt pointer.  
the subArea has pointers to all dispTiles managed by it.  In the event that
a subArea toggles, a routine runs that goes through all it's member dispTiles
and swaps mat & alt.  or you could just have mat[2];, and int drawing = 0;
and change that int to 1, and then back to 0.  drawing ^= 1;

There's nothing that says I need to extend tile_t.  fuckit.

ok, so each disp Tile has potentially two modes that it can display as.  You
want more?  maybe, 

ok, I just figured out that we don't need to have any subArea at all.  and
what's more, we can have zero, or 1, or many, and they can overlap, and have
any geometrical disposition that they want.  freedom == good.  So say we 

create a subArea and connect it to a trigger by number.  subAreas are numbered
sequentially from 0.  so that a subArea's index matches the array index into
the subAreas array in Area_t.  Although to ensure proper ordering, the 
subArea structure that is used in the mapEditor must actually store the 
index Number explicitly.

say you have: 0, 1, 2,and you goto delete 1, the editor will say, "are you 
sure you want to delete 1, All triggers connected to 1 & 2 will be broken? 
so You'd have to be careful.  You could run a routine that re-connects the 
triggers, sure, so you give each subArea it idnum.  and each trigger will
also have this same corresponding number.  So then, when building the triggers
list, you go through O(n) the subAreas looking for matching id_num, when you
find it, you set the trigger subArea Index, 

trigger::subAreaIndex

might as well start writing trigger now as well.

One other thing that I thought of was dynamic visibility, such as if you have
a flashlight of radius 10, and you're in the dark, that only things that are
10 away from your current position get drawn.  In which case I can re-use the 
alternate view mode in dispTile, flipping the drawing index ( drawing ^= 1 )


Movers are Thinkers, same thing.  So a bullet, or a thrown rock is a Thinker,
It is an entity that gets a turn in per-tick game-logic, which runs a function
that changes it's location.  It also has methods stored for what happens when
it collides with :
	- a wall
	- another entity
	- a thinker
	- the player
	- if it leaves the viewport (the visible area).  A bullet would get removed
		after it leaves the area defined by the shortlist, it just goes away

** colModel layers.
	well, colModels get their own layer# and are sorted by Layer.  Things
	can only collide with each other if they are on the same layer.  

Explosions.  I just realized that maybe I want to include a queue for 
temporary drawing events, such as explosions, decals of all types, things that
aren't necessarily part of the map, but will be introduced to it via action.
so, there's two I can think of 

	- Static Decal, you shoot the building, there are now chips on the concrete
	- explosion, muzzle flash.  A timed animation that only runs for a very
		finite length, and then it is removed and deleted.

You know what, both of those can be entities and can be added to the 
Block_t::entities.  if it's a static decal , it's an entity, and if it's an
explosion it gets put in thinkers, so it can move, and it's logic get's run,
and if it expires, it is removed from the queue. 

subArea

Decal List: Decals Aren't Entities.  They are Decals.  They
don't move, or think, or collide, or react, therefor they are decals, not 
requiring any of the facility that entities have.

Animation Queue, or Explosion Queue.  Where you can have a list of autonomous
animations, that run and are timed, and may run out.  (thus getting popped 
from the queue and returned to the heap).  You can put explosions here, 
muzzle flashes.  Puffs of smoke, other random things that move, but aren't
collidable entities.


:::::::::::::::::::::::
:: Drawing Algorithm ::
:::::::::::::::::::::::


:: consider the drawing algorithm from the beginning of a freshly loaded map ::

- find the Player spawn portal Entity in mapFile_t::entities list and get the 
	coordinate from it.  

- You have beginning player location.  viewport x,y is computed from this.
	- find the Area_t and set World_t::curArea
	- find the Block_t and set World_t::curBlock
		use the directional pointers (much faster: O(2n) < O(n^2) )

- create the masterlists: {
	- Drawer_t::shortlist.reset()

	- dispTiles: 
		// left
		if ( curBlock.left )
			shortlist.add( curBlock.left );
		// have to also add block to the left of the leftmost one visible
		if ( curBlock.x - view.x > 0 && curBlock.left.left ) 
			shortlist.add( curBlock.left.left );
		Block_t *rov = curBlock.left;
		if ( rov )
			rov = rov->left;
		while ( rov && rov->edgeFlag & FLAG_LEFT ) {
			rov = rov->left;
			if ( rov )
				shortlist.add( rov );
		}

		// right
		// some of the block to the right is visible
		if ( curBlock.x + curBlock.w < view.x + view.width ) {
			shortlist.add( curBlock.right );
		}
	
		// top
		if ( curBlock.y + curBlock.h < view.y + view.height ) {
			shortlist.add ( curBlock.up );	
		}

		// bottom
		if ( curBlock.down )
			shortlist.add( down );
		

		// bottom-left
		// bottom-right
		// top-right
		// top-left

		// 
		- add all the thinkers, entities, colModels, dispTiles to their
			masterlists

	- Thinkers: thinker Entities run within the given thinker radius, so first
		determine how many blocks to go in each direction to exceed that
		radius, then add the thinker Entities in that block. (combine this
		with above so that an entity doesn't get added twice).

	- sorting: 
		- dispTiles by layer.  
		- colModels by layer (colModels only collide with things on their own
			layer.  see **)
		- Entities by Layer (dispTiles can get drawn over top of entities)
		- Thinkers by Layer
}
			

:::::::::::::::::::::::::
:: Collision Detection ::
:::::::::::::::::::::::::
per gametick: compute colisions between:
	( Just on the Player's Layer # )
	- Player & colModels
	- Player & Entities
	- Player & Thinkers
	( foreach Layer # in World_t::thinkers, collide the thinker with things 
	of it's own Layer # ) 
	- Thinker Entities & colModels
	- Thinker Entities & all other Entities, including and especially other
		thinkers.

::::::::::::::::::::
:: Actual Drawing ::
::::::::::::::::::::
- glClearColor
- draw backgrounds
- get first dispTile layer #
	- draw dispTiles of that #
	- draw Entities & Thinkers of that #
	- increment layer #, get next dispTile, if NULL, Finished Drawing, break;
		else continue;

:::::::::::::::::
:: Player Move ::
:::::::::::::::::
player moves, the viewport moves, triggers a checkLists()
anytime the viewport moves, it triggers a checkLists.
- checkLists()
	if block boundary crosses over border of viewport, left, right, top, bottom,
		recompile draw lists.
	if thinkerRadius crosses over the edge of a Block that it hadn't before
		add thinkers.
	if the thinkerRadius was over into a block that it's no longer into,
		remove the thinkers that belong to that Block.
	.. etc ..
 

::::::::::::::::::::::::::::::::::::::::::
:: MapLoad / Creation / Setup Algorithm ::
::::::::::::::::::::::::::::::::::::::::::
- initialize the World_t
- read in map primitives, populating the mapFile_t structure
- foreach Area_t in mapFile_t::areas {

	- initialize Area_t internal data structures
	
	- define Area Size: get bottom-left most mapTile, bottom-right, top-left, 
		top-right.  (note: this might be done at mapSave time, automatically.
		naww, do all Area boxes by hand in the editor, so that coordinate will
		be suplied by a mapLoad()

	- figure out how many Block rows and columns are needed to span entire 
		Area_t, X & Y

	- initialize Area_t::blocklist, dimension ( X + 1 ) * ( Y + 1 ) , to
		array of NULL pointers
		
	- foreach Block position j, i: ( start at bottom-left ) over the Block Grid

		- foreach tile in mapFile_t::mapTiles 
			if ( not inside the bounds of the current Area ) 
				next, continue;

			if the ll-corner is inside of Block-j,i && mapTile::mat != NULL
				- if this is first Tile found, create Block_t at that pointer
					in Area_t::blocklist, and initialize

				- create a dispTile for the mapTile

				- if ( the tile is a background, add it to the 
					Area_t::backgrounds list )
				else
					add it to Block_t::dispTiles

				- ( pop the tile from the mapTiles list, or mark found, or 
					something )


	// rows
	- foreach Block_t row, starting at ll again, connect up east and west
		pointers.

	// cols
	- foreach Block_t col, starting at ll, connect up north and south pointers

	// edgeFlags
	- foreach Block_t:
		- if any of the tiles in the block extend OVER 1 BLOCK
			WIDTH past the right boundary of this Block, (its normal
			for tiles to extend past the boundary into an adjacent
			block, but we have to know if there are any wider than 1
			block width past the right boundary of the block), 
			Tiles will runOver in two directions, UP & Right.  
			- go through Blocks that tiles run over in and set the 
				Blocks edgeFlag, so the drawing algorithm will know to
				look for missing blocks.

	// create padding blocks
	- foreach row, add one empty Block_t onto the end
	- foreach col, add one empty Block_t onto the top
	- foreach row, bottom+1 to top, 
		- get the block-length of the current row 
		- get the block-length of the row below it
		- if the length of the row beneath it is longer , pad the current
			row out to the same length

	// finish
	- insert Area_t into World_t::areas hash
}

// subAreas
- foreach mapFile_t::subAreas:
	- figure out which Area_t the subArea belongs to
	- create an Area_t::subArea entry
	- foreach me_subAreaTile
		- walk the blocklist until we are in the correct block, 
			- find the dispTile in that block with matching UID
			if none are found, Create a fresh one w/ NULL primary display
			- set it's alternate view Tile 
			- add the found dispTile to the Area_t::subArea::tiles
	- set the subAreaIndex in all the matching Triggers to the index of
		the last created Area_t::subArea

// build entities & thinkers
- foreach mapFile_t::entities
	// add to World_t list
	if ( ent->type == ENT_THINKER ) {
		World_t::thinkers.add( ent );
	} else {
		World_t::entities.add( ent );
	}

	// 
	- find it's Area_t in World_t::areas

	- find Block_t that it is a member of 

	// add to the Area_t thinkers queue
	if ( ent->type == ENT_THINKER && ent->thinkType == THINK_ALWAYS ) {
		Area_t::thinkers.add( ent );
	} else {
		// add entity to it's block
		if ( ent->type == ENT_THINKER ) {
			Block_t::thinkers.add( ent );
		} else {
			Block_t::entities.add( ent );
		}
	}

// build colModels
- foreach mapFile_t::mapTiles
	- if there is no colModel
		next, continue;
	- find what Area_t it's in
	- find what Block_t it's in and add it to it
		World_t::Area_t::Block_t::colModels.add( colModel_t * );


- get MapSpawn Portal, and x, y spawn coords from it

:: support funcs ::
- resetVisibility() , when re-entering an area, visibilty may be reset,
	not sure how this works yet, but you would think that some times you
	want the option to reset things in an Area, and some times you want to
	NOT reset them.  
	- resetAreaEntites(), etc..
- subArea_t::toggleVis
	go through dispTiles and let drawing = (drawing+1) % MAX_DISP_MODES;


------------------------------------------------------------------------------
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
------------------------------------------------------------------------------
*/

#include "../game/g_entity.h"
#include "../lib/lib_list_c.h"
#include "../mapedit/mapedit.h"

// in the mapFile, if a subArea is defined, then a subsection of alternate
//  visibility blocks may also be defined.

class me_subArea_t : public Auto_t {
private:
	void _my_init();
	void _my_reset();
	void _my_destroy();
public:

	int 						x, y;
	int 						w, h;
	int 						id;

	buffer_c<mapTile_t*> 		tiles;

	me_subArea_t() { _my_reset(); }
	me_subArea_t(me_subArea_t const &);
};

/*
// FIXME: this will probably go into g_entity
struct AreaTrigger_t : public Entity_t {
	int 		id_num;
	int 		subAreaIndex;
	poly_t		poly;
};
*/

/*
====================
 dispTile_t
====================
*/
#define MAX_DISPTILE_MODES	4 /* value may change later on */

struct dispTile_t : public Allocator_t {
	animSet_t *			anim[ MAX_DISPTILE_MODES ];
	material_t *		mat[ MAX_DISPTILE_MODES ];
	int 				drawing;	// which mode is the one currently drawing
									// defaults to 0 for no subArea, or 0 for
									//  visible, where 1 would be not-visible
									//  set by subArea, or visibility algorithm
									// 2nd pointer can be null, in which case
									//  it will simply not be drawn
	poly_t 				poly;
	AABB_t				aabb;
	OBB_t				obb;
	int 				layer;
	unsigned int		uid;

	void reset() { 
		for ( int i = 0; i < MAX_DISPTILE_MODES; i++ ) {
			if ( anim[i] ) { delete anim[i]; }
			anim[i] = NULL;
		}
		memset( mat, 0, sizeof(mat) );
		drawing = 0; 
		poly.zero();
		AABB_zero(aabb);
		OBB_zero(obb);
		layer = 0;
		uid = (unsigned)-1;
	}
	void destroy() { reset(); }

	dispTile_t() { reset(); }
	dispTile_t( dispTile_t const & );
	dispTile_t( mapTile_t const & );
	~dispTile_t() { destroy(); }
	
	// already allocated in tiles.
	void SetAnimation( animSet_t *primary, unsigned int sub =0 ) {
		if ( sub > MAX_DISPTILE_MODES || !primary )
			return;
		anim[ sub ] = primary;
	}
	// already allocated in tiles.
	void SetMaterial( material_t *primary, unsigned int sub =0 ) {
		if ( sub > MAX_DISPTILE_MODES || !primary )
			return;
		mat[ sub ] = primary;
	}
};

enum { 
	FLAG_LEFT = 1,
	FLAG_DOWN = 2,
};


/*
====================
 Block_t
====================
*/
class Block_t : public Auto_t { 
private:
	void _my_init() ;
	void _my_reset() ;
	void _my_destroy() ;
public:
	int						id;		// unique id in that Area
	float 					p1[2]; 	// location of bot-left corner
	float					p2[2];	// top-right corner

	Block_t 				*up, 
							*down, 
							*left, 
							*right;

	// entities separated into lists based on their entity class
	list_c<Entity_t*>		entities[ TOTAL_ENTITY_CLASSES ];


	buffer_c<dispTile_t*>	dispTiles;
	buffer_c<colModel_t*>	colModels;

	int						edgeFlag;	// if any tiles go through the adjacent
										// block

	Block_t() : up(0), down(0), left(0), right(0) { _my_reset(); }
	Block_t( Block_t const & );
	~Block_t() { _my_destroy(); }

	bool isOccupied( void );
};


/*
====================
 subArea_t
====================
*/
class subArea_t : public Auto_t {
private:
	void _my_init();
	void _my_reset();
	void _my_destroy();
public:
	subArea_t() : id_num(0), visibility(0) {}
	subArea_t( subArea_t const & );

	// ctor can take me_subArea_t, translate from one type to another
	subArea_t( me_subArea_t const & );

	int 					id_num;
	int 					visibility;
	buffer_c<dispTile_t*> 	tiles;
};



/*
====================
 Area_t
====================
*/
#define AREA_NAME_SIZE		32

class Area_t : public Allocator_t {
private:
//	void _my_init();
//	void _my_reset();
//	void _my_destroy();

public:
	void init();
	void reset();
	void destroy();

	char 						name[ AREA_NAME_SIZE ];

	// storing poly as AABB format, ll-corner == p1, top-right corner == p2
	float 						p1[2], p2[2];

	// blocklist dimension (ie, 10 x 50 blocks)
	int 						bx, by; 

	// the block widths for this area
	float 						block_dim[2];

	// ultimate boundary of the camera location.  usually padded by 1 or 2
	//  extra block widths on the left & bottom, wider than the width of 
	//  just the blocks themselves
	// note: possibly deprecated
	float 						boundary[4];

	// the number of blocks on the perimeter to run thinkers. default 2
	int 						ent_perim;
	// the number of blocks on the perimeter to draw. default 1
	int							draw_perim;
	// ditto, how many blocks radius/perimeter to get colModels.  good idea
	// to get 1 more than entities 
	int							col_perim;

	// master list of all blocks, stored as static array
	buffer_c<Block_t*> 			blocks;

	// master list of subAreas
	buffer_c<subArea_t *> 		subAreas;	// all subAreas, if any

	// dispTiles
	buffer_c<dispTile_t*> 		dispTiles;

	// colModel
	buffer_c<colModel_t*>		colModels;

	// entlist_t : 
	// 	- linked list
	// 	- all entity types, mixed types, unsorted
	list_c<Entity_t*>			entities;

	// all backgrounds for this Area
	buffer_c<dispTile_t*> 		backgrounds;


	Area_t( memPool<node_c<Entity_t*> > *ep =NULL ) { 
		reset(); 
		//blocks.init(); // init later with the # of actual blocks needed
		subAreas.init();
		dispTiles.init();
		colModels.init();
		entities.init( ep );
		backgrounds.init();
	}
	Area_t( Area_t const & );
	Area_t( Area_t const &A, memPool<node_c<Entity_t*> > *ep ); // special 
	~Area_t() { destroy(); }
	
	// named constructor idiom (old way of doing it, deprecated)
	static Area_t * New( void ) { return new Area_t(); }
};


#define MAP_NAME_SIZE 		64

/*
====================
	mapFile_t

	we populate this when we read in a map from file.  mapFile is a sort of
	translation layer.  

	
====================
*/
class mapFile_t : public Auto_t {
private:

	void _my_reset();
	void _my_init();
	void _my_destroy();
public:
	char 						name[ MAP_NAME_SIZE ];

	buffer_c<mapTile_t *> 		mapTiles;  // read in as mapTiles

	buffer_c<Entity_t *> 		entities;

	// mapLoad will create an Areas list, but will not initialize the structures
	//  until we are in the postProcessing phase
	buffer_c<Area_t *> 			areas;

	buffer_c<me_subArea_t *>	subAreas;

	// i never know if I want these 
	mapFile_t() { init(); }
	~mapFile_t() { if ( V_isStarted() ) destroy(); }
};




/*
====================
 World_t
====================
*/

#define BLOCK_DIM			6000
#define DRAW_DIAMETER		18000
#define ENT_DIAMETER		30000

class World_t : public Allocator_t {
private:

	//
	// memory
	//

	// used for per-frame blocklist construction
	buffer_c<Block_t*> 			dispBlocks;
	buffer_c<Block_t*> 			entBlocks;
	buffer_c<Block_t*> 			colBlocks;
	

	/* Auto_t methods we must implement */
	//void _my_init( void ); 			// allocates
	//void _my_reset( void ); 		// sets virgin state
	//void _my_destroy( void ); 		// deallocates

	bool						default_area;

public:
	void init( void ) ;
	void reset( void ) ;
	void destroy( void ) ;

	static const unsigned int WORLD_MEMORY_SIZE = 4 * 1024 * 1024; // 4 MB

	memPool<node_c<Entity_t*> >	entPool;

	// per-frame blocklists used in actual collision & rendering. 
	// built out of Area::blocks
	buffer_c<dispTile_t*>		dispList;
	buffer_c<Entity_t*>			entList;
	buffer_c<colModel_t*>		colList;

	// current Block (the block that is mostly under the viewport
	Block_t *					curBlock;
	Block_t *					prevBlock;
	Area_t *					curArea;
	Area_t *					prevArea;
	
	// Area_t 
	buffer_c<Area_t*>			areas;

	// hash of Areas
	//hash_c<Area_t*> 			ahash;


	// force a per-frame rebuild of frameLists
	bool 						force_rebuild;	

	// client id for memory area
	int 						client_id;
	int 						idSave; // for saving the original
	
	//
	// methods
	//
	World_t() : curBlock(0), prevBlock((Block_t*)-1), curArea(0), prevArea(0), doPrintBlocks(0), default_area(0), client_id(-1), idSave(-1) { reset(); }
	~World_t() { destroy(); }

	void BuildWorld( mapFile_t const & ) ;
	void BuildWorldFromMapFile( mapFile_t const & );	
	void BuildBlocklist( Area_t * );
	void CreateDefaultArea( mapFile_t const & );
	Block_t * NewBlock(); // blocks are allocated by World_t because they share it's ent pool

	void BuildFrame( void ); // construct the lists each frame
	void BuildFrameLists( void ) ;
	
	void printBlocks( void ); 

	void setPrintBlocks( bool B ) { doPrintBlocks = B; }
	bool doPrintBlocks;

	void SetupViewport( Area_t * );
	void setCurBlock( Block_t *b ) {
		prevBlock = curBlock;
		curBlock = b;
	}
	void setCurArea( Area_t *A ) {
		prevArea = curArea;
		curArea = A;
	}

	// re-builds entire world from mapFile, completely wiping the memory space
	void rebuildPristine( mapFile_t const & ); 
	void startMemClient();
	void restoreMemClient();
};

inline void World_t::BuildFrame( void ) {

    // determine if camera moved, if so, determine if curBlock needs to
    //  be changed, if so change it


	BuildFrameLists();
}
    
extern World_t world;



struct BlockExport_t {
	colModel_t **cmpp;
	unsigned int cmln;
	list_c<Entity_t*> * elst;
};

// client can specify a point and a radius, and this will return an array
// of pointers to colModels and entities in all the blocks that this radius
// might cover (via BlockExport_t type).  
BlockExport_t * M_GetBlockExport( float *pt, float radius, int *ioLen );



#endif // __M_AREA_H__
