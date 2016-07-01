/******************************************************************************
 *
 * me_menus.cpp  - Copyright (C) 2008 Greg Naughton
 *
 * - drop menu class methods
 *
 *****************************************************************************/

#include "me_menus.h"
#include <string.h>
#include "../renderer/r_ggl.h"

#include <math.h>
#include "../client/cl_keys.h"

int global_font = 0;

//////////////////////////////////////////////////////////////////////////////
// menuBox_t
//////////////////////////////////////////////////////////////////////////////

/*
====================
 menuBox_t::newBox
====================
*/
menuBox_t * menuBox_t::newBox( const char *text, void(*func)(void), const char *desc ) {
	menuBox_t *b = (menuBox_t *) V_Malloc( sizeof(menuBox_t) );
	b->reset();

	strncpy( b->text, text, MENUBOX_STR_SIZE );
	b->text[ MENUBOX_STR_SIZE-1 ] = 0;

	b->func_f = func; // function initializer defaults this to NULL

	b->desc[0] = 0;
	if ( desc && desc[0] ) {
		strncpy( b->desc, desc, MENUBOX_STR_SIZE );
	}
	b->desc[ MENUBOX_STR_SIZE-1 ] = 0;
	return b;
}

/*
====================
 menuBox_t::reset
====================
*/
void menuBox_t::reset( void ) {
	next = 0; 
	child_con = 0; 
	text[0] = 0;
	desc[0] = 0;
	is_highlight = 0;
	parent = 0;
 
	recompute_dimension = 1; 
	num_chars = 0;	
	width = 0.f;
	x = y = height = 0.f;
	func_f = NULL;
	bfunc_f = NULL;
}

/*
=====================
 menuBox_t::getBackgroundHighlight4v
=====================
*/
const float *menuBox_t::getBackgroundHighlight4v ( void ) const {
	static float color[4];

#ifdef _DEBUG
	assert( parent );
	assert( parent->ancestor );
#endif

	if ( !parent || !parent->ancestor ) {
		return color;
	}

	switch ( parent->ancestor->highlight_mode ) {
	case MENU_USE_HL_COLOR:
		return const_cast<const float*> (parent->hlColor);
	case MENU_DARKEN:
		// FIXME: calc color, bound[0,1], 'DARKEN' pertains to bg, fg is inverted

		color[0] = parent->bgColor[0];
		color[1] = parent->bgColor[1];
		color[2] = parent->bgColor[2];
		color[3] = parent->bgColor[3];	
		return color;
	case MENU_LIGHTEN:
		// FIXME: calc color, bound[0,1], 'DARKEN' pertains to bg, fg is inverted
		color[0] = parent->bgColor[0];
		color[1] = parent->bgColor[1];
		color[2] = parent->bgColor[2];
		color[3] = parent->bgColor[3];	
		return color;
	case MENU_INVERT_COLOR:
		return const_cast<const float *> (parent->fontColor);
	}
	return color;
}

/*
=====================
 menuBox_t::getFontHighlight4v
=====================
*/
const float *menuBox_t::getFontHighlight4v ( void ) const {
	static float color[4];

#ifdef _DEBUG
	assert( parent );
	assert( parent->ancestor );
#endif

	if ( !parent || !parent->ancestor ) {
		return color;
	}

	switch ( parent->ancestor->highlight_mode ) {
	case MENU_USE_HL_COLOR:
		return const_cast<const float*> (parent->hlfColor);
	case MENU_DARKEN:
		// FIXME: calc color, bound[0,1], 'DARKEN' pertains to bg, fg is inverted
		color[0] = parent->bgColor[0];
		color[1] = parent->bgColor[1];
		color[2] = parent->bgColor[2];
		color[3] = parent->bgColor[3];	
		return color;
	case MENU_LIGHTEN:
		// FIXME: calc color, bound[0,1], 'DARKEN' pertains to bg, fg is inverted
		color[0] = parent->bgColor[0];
		color[1] = parent->bgColor[1];
		color[2] = parent->bgColor[2];
		color[3] = parent->bgColor[3];	
		return color;
	case MENU_INVERT_COLOR:
		return const_cast<const float *> (parent->bgColor);
	}
	return color;
}


//////////////////////////////////////////////////////////////////////////////
// menu_t
//////////////////////////////////////////////////////////////////////////////


/*
====================
 menu_t::print()
====================
* /
float menu_t::print( float x, float y, const char *arg, ... ) {
	return F_Printf( (int) x , (int) y , global_font, arg, ... );
} */

/*
====================
 menu_t::propagateDimensions
====================
*/
void menu_t::propagateDimensions( void ) {
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->propagateDimensions( wChar, hChar, wPad, hPad );
		bar = bar->getNext();
	}
	dimensions_computed = gtrue;
}

/*
====================
 menu_t::propagateLocation
====================
*/
void menu_t::propagateLocation( void ) {
	menuContainer_t *con = bar_head;
	while ( con ) {
		con->propagateLocation( 0.f, 0.f, 0.f, 0.f );
		con = con->getNext();
	}
	location_computed = gtrue;
}

/*
====================
 menu_t::_ownership_recurse
====================
*/
void menu_t::_ownership_recurse( menuContainer_t *con ) {
	menuBox_t *box = con->getBoxHead();
	while ( box ) {
		box->parent = con;
		menuContainer_t *con_b = box->getCon();
		if ( con_b ) {
			con_b->ancestor = this;
			_ownership_recurse( con_b );
		}
		box = box->getNext();
	}
}

/*
====================
 menu_t::propagateOwnership

	boxes don't know who owns them, bars are configured when they are connected
	all that is left to 'own' is columns. this function only affects columns
====================
*/
void menu_t::propagateOwnership( void ) {
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->ancestor = this;
		menuBox_t *box = bar->getBoxHead();
		while ( box ) {
			box->parent = bar;
			menuContainer_t *con = box->getCon();
			if ( con ) {
				con->ancestor = this;
				_ownership_recurse( con );
			}
			box = box->getNext();
		}
		bar = bar->getNext();
	}
	ownership_computed = gtrue;
}

/*
====================
 menu_t::propagateColors
====================
*/
void menu_t::propagateColors( void ) {
	setBarColor(		c.barColor[0],
						c.barColor[1],
						c.barColor[2],
						c.barColor[3] );

	setColColor(		c.colColor[0],
						c.colColor[1],
						c.colColor[2],
						c.colColor[3] );

	setBarFontColor(	c.barFont[0],
						c.barFont[1],
						c.barFont[2],
						c.barFont[3] );

	setColFontColor(	c.colFont[0],
						c.colFont[1],
						c.colFont[2],
						c.colFont[3] );

/* wait for user to opt for user-defined colors
	setColHighlightColor(	c.hlColor[0],
							c.hlColor[1],
							c.hlColor[2],
							c.hlColor[3] );

	setColHighlightFontColor(	c.hlfColor[0],
								c.hlfColor[1],
								c.hlfColor[2],
								c.hlfColor[3] );
*/
	propagate_colors = gtrue;
}

/*
====================
 menu_t::reset
====================
*/
void menu_t::reset( void ) {
	bar_head = NULL;
	floating_head = NULL;
	wChar = hChar = wPad = hPad = 0.f;
	dimensions_computed = gfalse;
	location_computed = gfalse;
	ownership_computed = gfalse;
	propagate_colors = gfalse;
	containerListBuilt = gfalse;
	menuSystemBuilt = gfalse;
	x_res = -1.f; y_res = -1.0f;
	current_con = NULL; current_box = NULL; last_con = NULL; last_box = NULL;
	con_head = NULL;
	mouse[0] = mouse[1] = 0.f;
	highlight_mode = MENU_INVERT_COLOR;
	watch_flags = WATCH_NOTHING;
	font = FONT_VERA_BOLD;
	global_font = font;
}

/*
====================
 menu_t::_masterList_recurse_col
====================
*/
void menu_t::_masterList_recurse( menuContainer_t *con, menuContainer_t **end ) {
	if ( con_head ) {
		(*end)->mnext = con;
		*end = con;
	} else {
		con_head = con;
		*end = con_head;
	}
	menuBox_t *box = con->getBoxHead();
	while ( box ) {
		con = box->getCon();
		if ( con ) {
			_masterList_recurse( con, end );
		}
		box = box->getNext();
	}
}

/*
====================
 menu_t::buildColumnMasterList
====================
*/
void menu_t::buildContainerList( void ) {
	menuContainer_t *bar = bar_head;
	menuContainer_t *end = NULL;
	while ( bar ) {
		_masterList_recurse( bar, &end );
		bar = bar->getNext();
	}
	containerListBuilt = gtrue;
}

/*
====================
 menu_t::rebuildMenuSystem
====================
*/
void menu_t::buildMenuSystem( void ) {

	if ( !containerListBuilt )
		buildContainerList();		// put this first

	if ( !dimensions_computed )
		propagateDimensions();

	if ( !location_computed )
		propagateLocation();		// location computation requires dimensions 

	if ( !ownership_computed )
		propagateOwnership();

	if ( !propagate_colors ) 
		propagateColors();

	menuSystemBuilt = gtrue;
}

/*
====================
 menu_t::draw
====================
*/
void menu_t::draw( void ) {

	if ( !menuSystemBuilt )
		buildMenuSystem();

	menuContainer_t *b = bar_head;
	while ( b ) {
		b->draw();
		b = b->getNext();
	}

	b = floating_head;
	while ( b ) {
		b->draw();
		b = b->getNext();
	}
}

/*
====================
 menu_t::addBar
====================
*/
void menu_t::addBar( menuBar_t *bar ) {
	if ( bar_head ) {
		menuContainer_t *n = bar_head, *tmp;
		while( (tmp = n->getNext()) ) {
			n = tmp;
		}
		n->setNext( bar );
	} else {
		bar_head = bar;
	}
	bar->ancestor = this;
}

/*
====================
 menu_t::newMenuXY
	deprecated
====================
*/
menu_t * menu_t::newMenuXY( float w, float h, float pw, float ph ) { 
	assert( 0 );
	menu_t *m = (menu_t *) V_Malloc( sizeof(menu_t) );
	m->reset();
	m->setCharDim( w, h );
	m->setPadding( pw, ph );
	return m;
}

/*
====================
 menu_t::setDimensions
====================
*/
void menu_t::setDimensions( float w, float h, float pw, float ph ) {
	setCharDim( w, h );
	setPadding( pw, ph );
}

/*
====================
 menu_t::setColorScheme
====================
*/
void menu_t::setColorScheme( menuColorScheme_t *cs ) {
	c.barColor[0]   = cs->barColor[0];
	c.barColor[1]   = cs->barColor[1];
	c.barColor[2]   = cs->barColor[2];
	c.barColor[3]   = cs->barColor[3];

	c.colColor[0]   = cs->colColor[0];
	c.colColor[1]   = cs->colColor[1];
	c.colColor[2]   = cs->colColor[2];
	c.colColor[3]   = cs->colColor[3];

	c.barFont[0]	= cs->barFont[0];
 	c.barFont[1]	= cs->barFont[1];
	c.barFont[2]	= cs->barFont[2];
	c.barFont[3]	= cs->barFont[3];

	c.colFont[0]	= cs->colFont[0];
	c.colFont[1]	= cs->colFont[1];
	c.colFont[2]	= cs->colFont[2];
	c.colFont[3]	= cs->colFont[3];

	c.hlColor[0]	= cs->hlColor[0];
	c.hlColor[0]	= cs->hlColor[0];
	c.hlColor[0]	= cs->hlColor[0];
	c.hlColor[0]	= cs->hlColor[0];

	c.hlfColor[0]	= cs->hlfColor[0];
	c.hlfColor[1]	= cs->hlfColor[1];
	c.hlfColor[2]	= cs->hlfColor[2];
	c.hlfColor[3]	= cs->hlfColor[3];
}

/*
====================
 menu_t::setBarFontColor
====================
*/
void menu_t::setBarFontColor( float r, float g, float b, float a ) {
	c.barFont[0]	= r;
 	c.barFont[1]	= g;
	c.barFont[2]	= b;
	c.barFont[3]	= a;
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_BAR|MENU_FONT, r, g, b, a );
		bar = bar->getNext();
	}
}

/*
====================
 menu_t::setColFontColor
====================
*/
void menu_t::setColFontColor ( float r, float g, float b, float a ) {
	c.colFont[0]	= r;
	c.colFont[1]	= g;
	c.colFont[2]	= b;
	c.colFont[3]	= a;
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_COLUMN|MENU_FONT, r, g, b, a );
		bar = bar->getNext();
	}
}

/*
====================
 menu_t::setGlobalFontColor
====================
*/
void menu_t::setGlobalFontColor ( float r, float g, float b, float a ) {
	setBarFontColor( r, g, b, a );
	setColFontColor( r, g, b, a );
}

/*
====================
 menu_t::setBarColor
	sets all bars that are children of menu ancestor
====================
*/
void menu_t::setBarColor( float r, float g, float b, float a ) {
	c.barColor[0]   = r;
	c.barColor[1]   = g;
	c.barColor[2]   = b;
	c.barColor[3]   = a;
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_BAR|MENU_BACKGROUND, r, g, b, a );
		bar = bar->getNext();
	}
}

/*
====================
 menu_t::setColColor
	sets all menuColumns
====================
*/
void menu_t::setColColor( float r, float g, float b, float a ) {
	c.colColor[0]   = r;
	c.colColor[1]   = g;
	c.colColor[2]   = b;
	c.colColor[3]   = a;
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_COLUMN|MENU_BACKGROUND, r, g, b, a );
		bar = bar->getNext();
	}
}

/*
====================
 menu_t::setGlobalBackgroundColor
====================
*/
void menu_t::setGlobalBackgroundColor( float r, float g, float b, float a ) {
	setBarColor( r, g, b, a );
	setColColor( r, g, b, a );
}

/*
====================
 menu_t::setGlobalBackgroundColorGreyScale
====================
*/
void menu_t::setGlobalBackgroundColorGreyScale( float x, float a ) {
	setGlobalBackgroundColor( x, x, x, a );
}

/*
====================
 menu_t::mouseClick
====================
*/
void menu_t::mouseClick( float x, float y ) {


	// first determines which bar, column & box the mouse cursor is in
	mouseOver( x, y, true );


	// clicking in a box that is currently Active
	if ( current_box && current_con && current_box == current_con->activated_box ) {
		// deActivateChildren()
		current_con->setActive( NULL );
	// different box in the same Container
	} else if ( current_box && current_con && current_box != current_con->activated_box ) {
		// deActivateChildren()
		if ( current_box->func_f ) {
			current_box->func_f();
			deActivateAll();
		} else
			current_con->setActive( current_box );
	// 
	} else if ( current_con ) {
		current_con->unHighlightBoxes();
		current_con = NULL;
	}

	if ( !current_con && !current_box ) {
		deActivateAll();
	}


	/* run down again: roving mouse around, any box that is covered by the mouse pointer,
	shows a highlight.  when you leave any box the highlight automatically goes away.  no
	boxes under cursor, no highlight.

	click! you clicked on a box.  if its in a bar, the column-child of that box display
	is enabled.  Its display is semi-perminent

	+ if you mouse-over another box in that bar, that is NOT the same as the currently
	active one, the displayed column is disabled and hidden again.  

	+ if you click ANYWHERE NOT in that bar, the displayed column is hidden.

	+ if you mouse-over a box in that column that has a child-column of menuColumn type,
	the menuColumn is displayed.  other types may have different behaviors.

	+ if you click a box in that column with a function: the function is executed and
	the display of that column is disabled.
	*/
}

/*
====================
 menu_t::mouseOver
====================
*/
void menu_t::mouseOver( float _x, float _y, bool click ) {
	static timer_c timer;

	mouse[0] = _x; mouse[1] = _y;

	menuContainer_t *	found_con = NULL;
	menuBox_t *			found_box = NULL;

	if ( watch_flags & WATCH_FLOATING ) {
		// depends on the blocking flag whether it would return or not, or just go
		// on about it's business as normal.  but I'm implementing the blocking first, so
		// we'll do it this way for now.
		return ;
	}

	// check the containers
	menuContainer_t *con = con_head;
	while ( con ) {
		if ( con->is_exposed ) {
			if ( con->checkMouse( mouse[0], y_res-mouse[1] ) ) {
				found_con = con;
				break;
			}
		}
		con = con->mnext;
	}

	// none found  
	if ( !found_con ) {
		if ( current_con ) {
			current_con->unHighlightBoxes();
		}

		if ( current_box ) {
			current_box->is_highlight = 0;
		}

		if ( click ) {
			timer.reset();
			current_con = NULL;
			current_box = NULL;
			return;
		}

		if ( timer.flags == MOUSE_IN_STASIS ) {
			if ( timer.timeup() ) {
				timer.reset();
			} else {
				return;
			}
		} else if ( timer.flags == MOUSE_ON_MENU ) {
			timer.set( MENU_LATENCY, MOUSE_IN_STASIS );	
			return;
		}

		// times up, lose current
		current_con = NULL;
		current_box = NULL;
		return;
	}		
	
	// for sure now there is a found_con
	timer.flags = MOUSE_ON_MENU;

	// different container this time, unset the old container
	if ( current_con != found_con ) {
		if ( current_con ) {
			current_con->unHighlightBoxes();

			// cant unset menuBars
			if ( typeid(*current_con) != typeid(menuBar_t) ) {
				// if found_con is not a child of a current_con
				bool _found = false;
				menuBox_t *rov = current_con->getBoxHead();

				while ( rov ) {
					if ( found_con == rov->getCon() ) {
						_found = true;
						break;
					}
					rov = rov->getNext();
				}
				if ( !_found ) {
					if ( current_con->parent )
						current_con->parent->setChildColumn( NULL );
					current_con->is_exposed = NULL;
					current_con->activated_box = NULL;
				}
			}
		}

		if ( current_box ) {
			current_box->is_highlight = 0;
		}
		current_con = NULL;
		current_box = NULL;		
	}

	// Next, having either a column or a bar, we run its boxes to determine 
	//  if the pointer is also within a box. 
	menuBox_t *box = found_con->getBoxHead();
	while ( box ) {
		if ( box->checkMouse( mouse[0], y_res-mouse[1] ) ) {
			found_box = box;
			break;
		}
		box = box->getNext();
	}

	if ( !found_box ) {
		if ( (MOUSE_IN_STASIS == timer.flags || MOUSE_ON_MENU == timer.flags) && !click ) {
			return;
		}

		// hide old column, if there is one
		if ( current_box && current_box->getCon() ) {
			current_box->getCon()->is_exposed = false;
		}

		current_con = found_con;
		current_box = found_box;
		return;
	}

	// for sure now there is a found_box


	// different box, same column
	if ( current_con == found_con && found_box != current_box ) {
		// unset the rest of the column, and set new active box (below)
		if ( current_con ) {
			current_con->unHighlightBoxes();
		}

		// hide old column, if there is one
		if ( current_box && current_box->getCon() ) {
			current_box->getCon()->is_exposed = false;
		}

	// just different box
	} else if ( found_box != current_box ) {
		if ( current_box && current_box->getCon() ) {
			current_box->getCon()->is_exposed = false;
		}

		if ( found_box->parent ) {
			if ( found_box->parent->activated_box != found_box ) {
				found_box->parent->activated_box = ( click ) ? found_box : NULL;
			}
		}
	}

	// if its a box, set box->is_highlight
	found_box->is_highlight = 1;	

	// auto-expose the child column of the column box on mouse over
	if ( found_box->getCon() && found_con ) {
		if ( typeid ( *found_con ) != typeid( menuBar_t ) ) {
			found_box->getCon()->is_exposed = true;
			found_con->activated_box = found_box;
		}
	}

	current_con = found_con;
	current_box = found_box;
}

/*
====================
 menu_t::setBoxDimensions
====================
*/
void menu_t::setBoxDimensions( menuBox_t *box, float w, float h, float wpad, float hpad, const float *padding ) {

	if ( !box->text || !box->text[0] ) {
		box->height = 4.0f;
		box->width = 4.f;		// something, so you can at least see you have an empty box
		box->num_chars = 0;
		box->recompute_dimension = 0;
		return ;
	}

	box->num_chars = (int) strlen ( box->text ) ;

	box->width = wpad + box->num_chars * ( w + wpad ) + padding[0];
	box->height = h + hpad * 2.0f                     + padding[1];

	// let F_Printf tell us how wide the string is
	box->width = F_Printf( 0, 0, global_font, "%s", box->text ) + padding[ 0 ];

	box->recompute_dimension = 0;
}

void menu_t::setColHighlightColor( float r, float g, float b , float a ) {
	if ( highlight_mode != MENU_USE_HL_COLOR ) {
		c.hlfColor[0] = c.colColor[0];
		c.hlfColor[1] = c.colColor[1];
		c.hlfColor[2] = c.colColor[2];
		c.hlfColor[3] = c.colColor[3];
	}

	c.hlColor[0]   = r;
	c.hlColor[1]   = g;
	c.hlColor[2]   = b;
	c.hlColor[3]   = a;

	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_ALL|MENU_BACKGROUND_HIGHLIGHT, r, g, b, a );
		bar->setFontColorAll( MENU_ALL|MENU_FONT_HIGHLIGHT, c.hlfColor[0], c.hlfColor[1], c.hlfColor[2], c.hlfColor[3] );
		bar = bar->getNext();
	}

	this->highlight_mode = MENU_USE_HL_COLOR;
}

void menu_t::setColHighlightFontColor( float r, float g, float b, float a ) {
	if ( highlight_mode != MENU_USE_HL_COLOR ) {
		c.hlColor[0] = c.colFont[0];
		c.hlColor[1] = c.colFont[1];
		c.hlColor[2] = c.colFont[2];
		c.hlColor[3] = c.colFont[3];
	}

	c.hlfColor[0]	= r;
	c.hlfColor[1]	= g;
	c.hlfColor[2]	= b;
	c.hlfColor[3]	= a;
	
	menuContainer_t *bar = bar_head;
	while ( bar ) {
		bar->setColorAll( MENU_ALL|MENU_FONT_HIGHLIGHT, r, g, b, a );
		bar->setFontColorAll( MENU_ALL|MENU_BACKGROUND_HIGHLIGHT, c.hlfColor[0], c.hlfColor[1], c.hlfColor[2], c.hlfColor[3] );
		bar = bar->getNext();
	}
	this->highlight_mode = MENU_USE_HL_COLOR;
}

void menu_t::registerFloating( menuContainer_t * con ) {
	if ( floating_head ) {
		con->fnext = floating_head;
		floating_head = con;
	} else {
		floating_head = con;
		con->fnext = NULL;
	}
	con->ancestor = this;

	if ( typeid( *con ) == typeid( textEntryBox_t ) ) {
		textEntryBox_t * t = reinterpret_cast<textEntryBox_t*>(con);
		if ( t->flag & TE_BLOCKING ) {
			watch_flags |= WATCH_FLOATING;
		}
	}
	else if ( typeid( *con ) == typeid( messageBox_t ) ) {
		messageBox_t * t = reinterpret_cast<messageBox_t*>(con);
		if ( t->flag & TE_BLOCKING ) {
			watch_flags |= WATCH_FLOATING;
		}
	}

	con->propagateDimensions( wChar, hChar, wPad, hPad );
	con->propagateLocation( x_res, y_res, 0.f, 0.f );

	for ( int i = 0; i < 4; i++ ) {
		con->bgColor[i] = c.colColor[i];
		con->fontColor[i] = c.barFont[i];
	}
}

/*
====================
 menu_t::keyboard

 returns true if menu ate the key event
====================
*/
bool menu_t::keyboard( int key, int down ) {
	if ( watch_flags & WATCH_FLOATING ) {
		menuContainer_t *con = floating_head;
		while ( con ) {
			if ( typeid( *con ) == typeid( textEntryBox_t ) ) {
				reinterpret_cast<textEntryBox_t*>(con)->keys( key, down );
			} else if ( typeid( *con ) == typeid( messageBox_t ) ) {
				reinterpret_cast<messageBox_t*>(con)->keys( key, down );
			}
			con = con->fnext;
		}
		return true;
	}
	return false;
}

/*
====================
 menu_t::mouseInput
====================
*/
void menu_t::mouseInput( int x, int y, int z, int state ) {

	// pop-ups 
	if ( watch_flags & WATCH_FLOATING ) {
		menuContainer_t *con = floating_head;
		while ( con ) {
			if ( typeid( *con ) == typeid( textEntryBox_t ) ) {
				reinterpret_cast<textEntryBox_t*>(con)->mouse( x, y_res-y, z, state );
			} else if ( typeid( *con ) == typeid( messageBox_t ) ) {
				reinterpret_cast<messageBox_t*>(con)->mouse( x, y_res-y, z, state );
			}
			con = con->fnext;
		}
		return;
	}

	// normal
	if ( state & MOUSE_LMB_DOWN ) {
		mouseClick( (float)x, (float)y );
		return;
	}
	mouseOver( (float)x, (float)y );
}

/*
====================
 menu_t::clearFloatingBoxes

 gets rid of pop-ups and unsets blocking behavior  
====================
*/
void menu_t::clearFloatingBoxes( void ) {
	watch_flags = 0;
	floating_head = NULL;
}

/*
====================
 menu_t::destroy() 

	return all memory held by menus
====================
*/
void menu_t::destroy( void ) {

	// FIXME: some day when I have a million dollars, unlimited time, or I
	//  run out of memory, whatever comes first, write the body of this 
	//  function

}

//////////////////////////////////////////////////////////////////////////////
// menuContainer_t::  master class for menuBox and menuColumn
//////////////////////////////////////////////////////////////////////////////



/*
====================
 menuContainer_t::reset
====================
*/
void menuContainer_t::reset( void ) {
	x = y = 0.f;
	w = h = wpad = hpad = 20.f;
	height = width = 20.f;
	stringLength = 0;

	bgColor[0] = bgColor[1] = bgColor[2]       = MENU_DEF_BG_COLOR;
	fontColor[0] = fontColor[1] = fontColor[2] = MENU_DEF_FONT_COLOR;
	hlColor[0] = hlColor[1] = hlColor[2]       = MENU_DEF_BG_HL_COLOR;
	hlfColor[0] = hlfColor[1] = hlfColor[2]    = MENU_DEF_FONT_HL_COLOR;
	bgColor[3] = fontColor[3] = hlColor[3] = hlfColor[3] = 1.0f; // default no alpha blend

	padding[0] = padding[1] = 4.0f;
	box_head = NULL;
	_trigbox = NULL;
	next = NULL;
	mnext = NULL;
	parent = NULL;
	ancestor = NULL;
	activated_box = NULL;

	is_exposed = false;
	fnext = NULL;
}

/*
====================
 menuContainer_t::addTextBox
====================
*/
menuBox_t * menuContainer_t::addTextBox( const char * text, void(*func)(void), const char *desc ) {
	menuBox_t *b = menuBox_t::newBox( text, func, desc );
	if ( box_head ) {
		menuBox_t *n = box_head, *tmp;
		while ( (tmp = n->getNext()) ) {
			n = tmp;
		}
		n->setNext( b );
	} else {
		box_head = b;
	}
	return b;
}

/*
====================
 menuContainer_t::addBox() 
====================
*/
void menuContainer_t::addBox( menuBox_t *box ) {
	if ( box_head ) {
		menuBox_t *n = box_head, *tmp;
		while ( (tmp = n->getNext()) ) {
			n = tmp;
		}
		n->setNext( box );
	} else {
		box_head = box;
	}
	box->recompute_dimension = 1;
}

/*
====================
 menuContainer_t::setActiveByTxt
====================
*/
void menuContainer_t::setActiveNode( const char *str ) {
	menuBox_t *box = box_head;
	while ( box ) {
		if ( !strncmp( str, box->text, MENUBOX_STR_SIZE ) ) {
			activated_box = box;
			break;
		}
		box = box->getNext();
	}
}

/*
====================
 menuContainer_t::setActive
====================
*/
void menuContainer_t::setActive( menuBox_t *box ) {
	if ( box ) {
		if ( box->getCon() )
			box->getCon()->is_exposed = true;
		activated_box = box;
	} else {
		activated_box = NULL;
	}
}

/*
====================
 menuContainer_t::setColorAll
====================
*/
void menuContainer_t::setColorAll( unsigned int type, float r, float g, float b, float a ) {
	bool set = false;
	switch ( type & ~COLOR_MASK ) {
	case MENU_BAR:		
		if ( typeid(*this) == typeid(menuBar_t) ) 
			set = true; 
		break;
	case MENU_COLUMN:
		if (	typeid(*this) == typeid(menuColumn_t) || 
				typeid(*this) == typeid(textArea_t) ) 
			set = true; 
		break;
	case MENU_ALL:		
		set = true; 
		break;
	}
	if ( set ) {
		switch ( type & COLOR_MASK ) {
		case MENU_BACKGROUND:
			bgColor[0] = r; bgColor[1] = g; bgColor[2] = b; bgColor[3] = a; 
			break;
		case MENU_FONT:
			fontColor[0] = r; fontColor[1] = g; fontColor[2] = b; fontColor[3] = a; 
			break;
		case MENU_BACKGROUND_HIGHLIGHT:
			hlColor[0] = r; hlColor[1] = g; hlColor[2] = b; hlColor[3] = a; 
			break;
		case MENU_FONT_HIGHLIGHT:
			hlfColor[0] = r; hlfColor[1] = g; hlfColor[2] = b; hlfColor[3] = a; 
			break;
		}
	}
	menuBox_t *box = box_head;
	while ( box ) {
		menuContainer_t * con = box->getCon();
		if ( con ) {
			con->setColorAll( type, r, g, b, a );
		}
		box = box->getNext();
	}
}

/*
====================
 menuContainer_t::setFontColorAll
====================
*/
void menuContainer_t::setFontColorAll( unsigned int type, float r, float g, float b, float a ) {
	bool set = false;
	switch ( type ) {
	case MENU_BAR:		
		if ( typeid(*this) == typeid(menuBar_t) ) set = true; 
		break;
	case MENU_COLUMN:	
		if ( typeid(*this) == typeid(menuColumn_t) ) set = true; 
		break;
	case MENU_ALL:
		if ( typeid(*this) == typeid(menuContainer_t) ) set = true; 
		break;
	}
	if ( set ) {
		fontColor[0] = r; fontColor[1] = g; fontColor[2] = b; fontColor[3] = a;
	}
	menuBox_t *box = box_head;
	while ( box ) {
		menuContainer_t * con = box->getCon();
		if ( con ) {
			con->setFontColorAll( type, r, g, b, a );
		}
		box = box->getNext();
	}
}

/*
====================
 menuContainer_t::setHighlightColorAll
====================
*/
void setHighlightColorAll( unsigned int type, float r, float g, float b, float a ) {
}

/*
====================
 menuContainer_t::setHighlightFontColorAll
====================
*/
void setHighlightFontColorAll( unsigned int type, float r, float g, float b, float a ) {
}



//////////////////////////////////////////////////////////////////////////////
// menuColumn_t
//////////////////////////////////////////////////////////////////////////////

/*
====================
 menuColumn_t::propagateLocation
====================
*/
void menuColumn_t::propagateLocation( float bar_x, float bar_y, float bar_xOffset, float bar_yOffset ) {
	x = bar_x + bar_xOffset;
	y = bar_y + bar_yOffset;

	// start column boxes with a Y offset
	float y_ofst = y + 0.5f * padding[1];

	menuBox_t *box = box_head;
	while ( box ) {
		box->x = x;
		box->y = y_ofst;
		menuContainer_t *con = box->getCon();
		if ( con ) {
			con->propagateLocation( x, y_ofst, width, 0 /* same height as it's trigger */ );
		}
		
		y_ofst += box->height;

		box = box->getNext();
	}
}

/*
====================
 menuColumn_t::propagateDimensions
====================
*/
void menuColumn_t::propagateDimensions( float _w, float _h, float _wp, float _hp ) {

	// set font dimensions
	w = _w; h = _h; wpad = _wp; hpad = _hp;

	int count = 0;
	int maxlen = 0;
	float widest = 0.f;

	// set box widths and find the widest box
	menuBox_t *b = box_head;
	while ( b ) {
		++count;
		menu_t::setBoxDimensions( b, w, h, wpad, hpad, padding );
		if ( b->num_chars > maxlen ) {
			maxlen = b->num_chars;
			widest = b->width;
		}
		b = b->getNext();
	}

	stringLength = maxlen;
	width = widest;
	height = padding[1] + ((float)count) * ( h + 2.0f * hpad + padding[1] );

	// recurse the boxes' columns, if any
	b = box_head;
	while ( b ) {
		menuContainer_t *col_b = b->getCon();
		if ( col_b ) {
			col_b->propagateDimensions( w, h, wpad, hpad );
		}
		b = b->getNext();
	}
}

/*
====================
 menuColumn_t::draw() 
====================
*/
void menuColumn_t::draw( void ) {

	if ( !ancestor )
		return;

	//float y_res = ancestor->getYRes() - 1.0f;
	float y_res = ancestor->getYRes();

// FIXME: use vertex arrays
	// draw menu bar background
	gglColor4fv( bgColor );
	gglBegin( GL_QUADS );
	gglVertex3f( x,		 y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - y,			0.f );
	gglVertex3f( x,		 y_res - y,			0.f );
	gglEnd();

	// for each box, draw the text and button details
	menuBox_t *b = box_head;
	float x_ofst = x + 0.5f * padding[0] + wpad;
	float y_ofst = y_res - ( y + hpad + 0.5f * padding[1] );

	while ( b ) {

		b->recomputeIfChanged( w, h, wpad, hpad, padding );

		// check if the box is highlighted, if so, draw highlight
		if ( b->is_highlight ) {
			gglColor4fv( b->getBackgroundHighlight4v() );			
			gglBegin( GL_QUADS );
			gglVertex3f( b->x,		 y_res - (b->y + b->height), 0.f );
			gglVertex3f( b->x + width, y_res - (b->y + b->height), 0.f );
			gglVertex3f( b->x + width, y_res - b->y, 0.f );
			gglVertex3f( b->x ,		y_res - b->y, 0.f );
			gglEnd();
		}

		if ( b->is_highlight ) {
			gglColor4fv( b->getFontHighlight4v() );
		} else {
			gglColor4fv( fontColor );
		}

		F_Printf( x_ofst, y_ofst-hpad, global_font, "%s", b->text );
		//CL_DrawString( (int)x_ofst, (int)(y_ofst-h-hpad), (int)w, (int)h, b->text, (int)wpad );
		
		y_ofst -= b->height;

		// next box in this column
		b = b->getNext();
	}

	if ( activated_box ) {
		activated_box->drawContainer();
	}
}


//////////////////////////////////////////////////////////////////////////////
// menuBar_t
//////////////////////////////////////////////////////////////////////////////

/*
====================
 menuBar_t::draw
====================
*/
void menuBar_t::draw( void ) {

	if ( !ancestor )
		return;

//	float y_res = ancestor->getYRes() - 1.0f;
	float y_res = ancestor->getYRes();

// FIXME: use vertex arrays
	// draw menu bar background
	gglColor4fv( bgColor );
	gglBegin( GL_QUADS );
	gglVertex3f( x,		 y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - y,			0.f );
	gglVertex3f( x,		 y_res - y,			0.f );
	gglEnd();

	// for each box, draw the text and button details
	menuBox_t *b = box_head;
	float x_ofst = x + 0.5f * padding[0] + wpad;
	x_ofst = x + padding[0] + wpad; // fudge
	float y_ofst = y_res - ( y + hpad  + 0.5f * padding[1]);

	float fudge = 0.5f * padding[0];
	fudge = 0.f; // this is right, but it looks worse, I think I need to add another padding[0]

	while ( b ) {
	 
		b->recomputeIfChanged( w, h, wpad, hpad, padding );

		// check if the box is highlighted, if so, draw highlight
		if ( b->is_highlight ) {
			gglColor4fv( b->getBackgroundHighlight4v() );			
			gglBegin( GL_QUADS );
			gglVertex3f( b->x - fudge,				y_res - (b->y + b->height), 0.f );
			gglVertex3f( b->x - fudge + b->width,	y_res - (b->y + b->height), 0.f );
			gglVertex3f( b->x - fudge + b->width,	y_res - b->y, 0.f );
			gglVertex3f( b->x - fudge,				y_res - b->y, 0.f );
			gglEnd();
		}

		if ( b->is_highlight ) {
			gglColor4fv( b->getFontHighlight4v() );
		} else {
			gglColor4fv( fontColor );
		}

		F_Printf( x_ofst, y_ofst, global_font, "%s", b->text );
		//CL_DrawString( (int)x_ofst, (int)(y_ofst-h), (int)w, (int)h, b->text, (int)wpad );
		
		x_ofst += b->width;

		b = b->getNext();
	}

	// draw the active column, if any
	if ( activated_box ) {
		activated_box->drawContainer();
	}
}

/*
====================
 menuBar_t::propagateDimensions
====================
*/
void menuBar_t::propagateDimensions( float _w, float _h, float _wp, float _hp ) { 
	// set font dimensions
	w = _w; h = _h; wpad = _wp; hpad = _hp;

	int count = 0;
	width = 0.5f * padding[0];
	width = padding[0];
	
	menuBox_t *b = box_head;
	while ( b ) {
		++count;

		// set trigger box width
		menu_t::setBoxDimensions( b, w, h, wpad, hpad, padding );

		// propagate the columns
		menuContainer_t * c = b->getCon();
		if ( c ) {
			c->propagateDimensions( w, h, wpad, hpad );
		}

		// update the menuBar dimensions
		if ( b->num_chars > 0 ) {
			width += b->width;
		}
		b = b->getNext();
	}

	// same as box
	height = h + hpad * 2.0f + padding[1];
}

/*
====================
 menuBar_t::propagateLocation
	menuBar location (x,y) set explicitly by the user
====================
*/
void menuBar_t::propagateLocation( float d1, float d2, float d3, float d4 ) {
	menuBox_t *b = box_head;
	float prev_col_width = 0.5f * padding[0];
	while ( b ) {
		b->x = x + prev_col_width;
		b->y = y;
		menuContainer_t *c = b->getCon();
		if ( c ) {
			c->propagateLocation( x, y, prev_col_width, height );
		}
		prev_col_width += b->width;
		b = b->getNext();	
	}
}



//////////////////////////////////////////////////////////////////////////////
// textArea_t
//////////////////////////////////////////////////////////////////////////////

/*
====================
 textArea_t::draw
====================
*/
void textArea_t::draw( void ) {
#ifdef _DEBUG
	assert(ancestor);
#endif
	if ( !ancestor ) 
		return;
	float y_res = ancestor->getYRes() - 1.0f;
	gglColor4fv( bgColor );
	gglBegin( GL_QUADS );
	gglVertex3f( x,		 y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - y,			0.f );
	gglVertex3f( x,		 y_res - y,			0.f );
	gglEnd();
	float x_ofst = x + wpad + 0.5f * padding[0];
	float y_ofst = y_res - ( y + hpad + 0.5f * padding[1] );
	gglColor4fv( fontColor );

	F_Printf( x_ofst, y_ofst, global_font, "%s", this->text );
	//CL_DrawString( (int)x_ofst, (int)(y_ofst-h), (int)w, (int)h, this->text, (int)wpad );
}

/*
====================
 textArea_t::propagateDimensions
====================
*/
void textArea_t::propagateDimensions( float _w, float _h, float _wpad, float _hpad ) { 

	// set font dimensions
	w = _w; h = _h; wpad = _wpad; hpad = _hpad;

	// look for line breaks
	int i = 0;
	int lines = ( text[i] ) ? 1 : 0;
	int line_len = 0;
	int longest = 0;

	if ( !lines ) {
		width = height = 0;
		return;
	}

	while ( i < MENUBOX_TEXTAREA_STR_SIZE && text[i] ) {
		if ( text[i] == '\n' ) {
			++lines;
			if ( line_len > longest ) {
				longest = line_len;
			}
			line_len = 0;
		} else {
			line_len++;
		}		
		++i;
	}

	//width = padding[0] + wpad + ((float)longest) * ( w + wpad );
	width = F_Printf( 0, 0, global_font, "%s", text ) + padding[ 0 ] + wpad;

	height = 2.0f * padding[1] + hpad + ((float)lines) * ( h + hpad );
	height = 1.5f * padding[1] + hpad + ((float)lines) * ( h + hpad );
}


/*
====================
 textArea_t::propagateLocation
	this is easy because textAreas have no children
====================
*/
void textArea_t::propagateLocation( float _x, float _y, float x_ofst, float y_ofst ) {
	x = _x + x_ofst;
	y = _y + y_ofst;
	y = _y + y_ofst - 1; // fudge
}

/*
====================
 textArea_t::setText
====================
*/
void textArea_t::setText( const char *txt ) {
	strncpy( this->text, txt, MENUBOX_TEXTAREA_STR_SIZE );
	this->text[ MENUBOX_TEXTAREA_STR_SIZE - 1 ] = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// TEXT ENTRY BOX
/////////////////////////////////////////////////////////////////////////////////////////////

/*
====================
====================
*/
void textEntryBox_t::setLabel( const char *txt ) {
	strncpy( this->label, txt, MENUBOX_TEXTAREA_STR_SIZE );
	this->label[ MENUBOX_TEXTAREA_STR_SIZE - 1 ] = 0;
}


/*
====================
====================
*/
#define PUTCHAR( c ) { if ( _elen < max_elen ) { entry[_elen++] = (c); entry[_elen] = '\0'; } }

void textEntryBox_t::keys( int key, int down ) {

	// DOWN
	if ( down ) {
		switch ( key ) {
		case KEY_ESCAPE:
			if ( b_cancel->func_f )
				b_cancel->func_f();
			break;
		case KEY_SPACEBAR:
			PUTCHAR( '_' );
			break;
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			shift = true;
			break;
		case KEY_BACKSPACE:
			if ( _elen > 0 ) 
				entry[--_elen] = '\0';
			break;
		case KEY_ENTER:
			b_ok->bfunc_f( entry, _elen, 0 );
			break;
		case KEY_CAPSLOCK:
			caps = !caps;
			break;
		case KEY_TAB:
			PUTCHAR( '_' );
			break;
		case KEY_MINUS:
			if ( shift ) {
				PUTCHAR( '_' );
			} else {
				PUTCHAR( '-' );
			}
			break;
		case KEY_PERIOD:
			PUTCHAR( '.' );
			break;
		default:
			if ( key >= '0' && key <= '9' ) {
				PUTCHAR( key );
			} else if ( key >= 'a' && key <= 'z' ) {
				if ( shift ) {
					PUTCHAR( key - ('a'-'A') );
				} else {
					PUTCHAR( key );
				}
			}
		}
	// UP 
	} else {
		switch ( key ) {
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			shift = false;
		break;
		}
	}
}

void textEntryBox_t::mouse( int x, int y, int z, int state ) {
	if ( state & MOUSE_LMB_DOWN ) {
		if ( b_ok->checkMouse( (float)x, (float)y ) ) {
			if ( b_ok->bfunc_f ) 
				b_ok->bfunc_f( entry, _elen, 0 );
		}
		if ( b_cancel->checkMouse( (float)x, (float)y ) ) {
			if ( b_cancel->func_f )
				b_cancel->func_f();
		}
	}
}

void textEntryBox_t::draw( void ) {
#ifdef _DEBUG
	assert(ancestor);
#endif
	if ( !ancestor ) 
		return;
	//
	// Draw The Main Box
	//
	float y_res = ancestor->getYRes() - 1.0f;
	gglColor4fv( bgColor );
	gglBegin( GL_QUADS );
	gglVertex3f( x,		 y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - y,			0.f );
	gglVertex3f( x,		 y_res - y,			0.f );
	gglEnd();

	// 
	// Draw The Text Label
	// 
	float x_ofst = x + wpad + 0.5f * padding[0];
	float y_init = y + hpad + 0.5f * padding[1];
	float y_ofst = y_res - y_init;

	gglColor4fv( fontColor );

	F_Printf( x_ofst, y_ofst, global_font, "%s", this->label );
	//CL_DrawString( (int)x_ofst, (int)(y_ofst-h), (int)w, (int)h, this->label, (int)wpad );

	// 
	// Draw the Text Entry Area
	// 
	float _w = width - wpad - 4.0f * padding[0];
	float _h = 4.0f * hpad + h;

	x_ofst = x + wpad + 0.5f * padding[0]	+ 3.0f * ( wpad + padding[0] );
	y_ofst = y_init + ((float)label_lines)*(h+hpad+2) + 4.0f;


	gglColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
	gglBegin( GL_QUADS );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglEnd();

	gglColor4f( 1.f, 1.f, 1.f, 1.0f );
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();

	//
	// Draw the text in the entry form
	//
	x_ofst = x_ofst + 1.0f;
	y_ofst = y_res - ( y_ofst + ( h + hpad ) + 3.0f );

	gglColor4f( 0.0f, 0.0f, 0.0f, 1.0f );

	F_Printf( x_ofst, y_ofst+h, global_font, "%s", entry );
	//CL_DrawString( (int)x_ofst, (int)(y_ofst), (int)w, (int)h, entry, (int)wpad );

	float len = (float) _elen;
	len = len * ( w + wpad + wpad + 2.0f ) + 4.0f;

	// set max_elen dynamically based on box width
	if ( len >= _w - 10.f ) {
		max_elen = _elen;
	}

	// color of cursor is cosine function 
	float ms = (float)( Com_Millisecond() % 1000 );
	float g = cosf( ms * 6.3 / 1000.0f ) / 2.0f + 0.5f;
	gglColor4f( 0.2f, 0.2f, 0.2f, g );

	x_ofst += len;
	
	gglBegin( GL_LINES );
	gglVertex2f( x_ofst, y_ofst );
	gglVertex2f( x_ofst, y_ofst + h + hpad );
	gglEnd();



	//
	// boxes
	//

	x_ofst = b_ok->x;
	y_ofst = b_ok->y;

	_w = b_ok->width;

	// "OK" Box
	gglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	gglBegin( GL_QUADS );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglEnd();

	gglColor4f( 0.f, 0.f, 0.f, 1.0f );
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	F_Printf( x_ofst+16, (y_res-y_ofst-5), global_font, "%s", "OK" );
	//CL_DrawString( (int)x_ofst+16.f, (int)(y_res-y_ofst-h-5.f), (int)w, (int)h, "OK", (int)wpad );


	// "Cancel" Box
	_w = b_cancel->width;
	x_ofst += b_ok->width + 6.f;

	gglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	gglBegin( GL_QUADS );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglEnd();

	gglColor4f( 0.f, 0.f, 0.f, 1.0f );
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();
		
	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	F_Printf( x_ofst+10, y_res - y_ofst - 5, global_font, "%s", "Cancel" );
	//CL_DrawString( (int)x_ofst+10.f, (int)(y_res-y_ofst-h-5.f), (int)w, (int)h, "Cancel", (int)wpad );



	/*

	// draw outline of button boxes, 
	gglColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
	x_ofst = b_ok->x;
	y_ofst = b_ok->y;
	_w = b_ok->width;
	_h = b_ok->height;
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();

	x_ofst = b_cancel->x;
	y_ofst = b_cancel->y;
	_w = b_cancel->width;
	_h = b_cancel->height;
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();
*/
}

void textEntryBox_t::propagateDimensions( float _w, float _h, float _wpad, float _hpad ) {
	w = _w; h = _h; wpad = _wpad; hpad = _hpad;
	_slen = strlen( label );

	int i = 0;
	int lines = ( label[i] ) ? 1 : 0;
	int line_len = 0;
	int longest = 0;

	if ( !lines ) {
		width = height = 20;
		return;
	}

	while ( i < MENUBOX_TEXTAREA_STR_SIZE && label[i] ) {

		if ( line_len > longest ) {
			longest = line_len;
		}
		
		if ( label[i] == '\n' ) {
			++lines;
			line_len = 0;
		} else {
			line_len++;
		}		
		++i;
	}

	label_lines = lines;

	width = padding[0] + wpad + ((float)longest) * ( w + wpad ) + 100.0f;
	height = 2.0f * padding[1] + hpad + ((float)lines) * ( h + hpad );


	// add in height for text entry area
	height += padding[1] + 2.0f * hpad + h;

	// add in height for OK & Cancel buttons
	height += padding[1] + 2.0f * hpad + h;
	height += 20;

	// set box dimensions
	b_ok->width = 58.f;
	b_ok->height = h + hpad * 4.0f;
	b_cancel->width = 81.f;
	b_cancel->height = h + hpad * 4.0f;
}

void textEntryBox_t::propagateLocation( float screen_x, float screen_y, float d3, float d4 ) {
	x = screen_x / 2.0f - 0.5f * width;
	y = screen_y / 2.0f - 0.5f * height;


	// set box locations
	b_ok->x = x + this->width - b_ok->width - b_cancel->width - 16;
	b_ok->y = y + this->height - b_ok->height - 6;
	b_cancel->x = b_ok->x + b_ok->width + 8;
	b_cancel->y = b_ok->y;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  MESSAGE BOX
/////////////////////////////////////////////////////////////////////////////////////////////

/*
====================
====================
*/
void messageBox_t::keys( int key , int down ) {
	if ( down ) {
		switch ( key ) {
		case KEY_ESCAPE:
		case KEY_ENTER:
			if ( this->b_ok->func_f )
				this->b_ok->func_f();
			break;
		}
	}
}

/*
====================
====================
*/
void messageBox_t::mouse( int x, int y, int z, int state ) {
	if ( state & MOUSE_LMB_DOWN ) {
		if ( b_ok->checkMouse( (float)x, (float)y ) ) {
			if ( b_ok->func_f ) 
				b_ok->func_f();
		}
	}
}

/*
====================
====================
*/
void messageBox_t::draw( void ) {

	if ( !ancestor )
		return;
	//
	// Draw The Main Box
	//
	float y_res = ancestor->getYRes() - 1.0f;
	gglColor4fv( bgColor );
	gglBegin( GL_QUADS );
	gglVertex3f( x,		 y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - (y + height), 0.f );
	gglVertex3f( x + width, y_res - y,			0.f );
	gglVertex3f( x,		 y_res - y,			0.f );
	gglEnd();

	// 
	// Draw The Text Label
	// 
	float x_ofst = x + wpad + 0.5f * padding[0] + 10.f;
	float y_init = y + hpad + 0.5f * padding[1] + 10.f;
	float y_ofst = y_res - y_init;

	gglColor4fv( fontColor );

	F_Printf( x_ofst, y_ofst, global_font, "%s", this->label );
	//CL_DrawString( (int)x_ofst, (int)(y_ofst-h), (int)w, (int)h, this->label, (int)wpad );

	x_ofst = b_ok->x;
	y_ofst = b_ok->y;

	float _w = b_ok->width;
	float _h = 4.0f * hpad + h;

	// "OK" Box
	gglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
	gglBegin( GL_QUADS );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglEnd();

	gglColor4f( 0.f, 0.f, 0.f, 1.0f );
	gglBegin( GL_LINE_STRIP );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst+_h), 0.f );
	gglVertex3f( x_ofst + _w,	y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst), 0.f );
	gglVertex3f( x_ofst,		y_res - (y_ofst+_h), 0.f );
	gglEnd();

	gglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	F_Printf( x_ofst+16, y_res-y_ofst-5, global_font, "%s", "OK" );
	//CL_DrawString( (int)x_ofst+16.f, (int)(y_res-y_ofst-h-5.f), (int)w, (int)h, "OK", (int)wpad );
}

/*
====================
====================
*/
void messageBox_t::propagateDimensions( float _w, float _h, float _wpad, float _hpad ) {
	w = _w; h = _h; wpad = _wpad; hpad = _hpad;

	_slen = strlen( label );

	int i = 0;
	int lines = ( label[i] ) ? 1 : 0;
	int line_len = 0;
	int longest = 0;

	if ( !lines ) {
		width = height = 20;
		return;
	}

	while ( i < MENUBOX_TEXTAREA_STR_SIZE && label[i] ) {

		if ( line_len > longest ) {
			longest = line_len;
		}
		
		if ( label[i] == '\n' ) {
			++lines;
			line_len = 0;
		} else {
			line_len++;
		}		
		++i;
	}

	label_lines = lines;

	width = 2.0f * padding[0] + wpad + ((float)longest) * ( w + wpad ) + 100.f;
	height = 2.0f * padding[1] + hpad + ((float)lines) * ( h + hpad );
	height += padding[1] + 2.0f * hpad + h;
	height += 20;

	b_ok->width = 58.f;
	b_ok->height = h + hpad * 4.0f;
}

/*
====================
====================
*/
void messageBox_t::propagateLocation( float scn_x, float scn_y, float d3, float d4 ) {
	x = scn_x / 2.0f - 0.5f * width;
	y = scn_y / 2.0f - 0.5f * height;

	// set OK button loc
	b_ok->x = x + this->width - b_ok->width - 16;
	b_ok->y = y + this->height - b_ok->height - 6;
}

/*
====================
====================
*/
void messageBox_t::setLabel( const char *txt ) {
	strncpy( this->label, txt, MENUBOX_TEXTAREA_STR_SIZE );
	this->label[ MENUBOX_TEXTAREA_STR_SIZE - 1 ] = 0;
}
