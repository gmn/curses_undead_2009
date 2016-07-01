/******************************************************************************
 *
 * me_menus.h  - Copyright (C) 2008 Greg Naughton
 *
 *****************************************************************************/

/*
Coordinates: Menus use screen coords (0,0) = top-left corner, however everything is drawn in OpenGL coords which are (0,0) = bot-left.

You set the top-left x,y of each horizontal menubar.  There are two real 
practical uses of menu bars, top of screen, or bottom of screen, but we set it 
manually, so it really doesn't matter.

the width of each column is automatically determined based on the width of
the longest child-menubox-string.  the top-left x,y position of subsequent 
columns depends on the width of the previous columns, going in a left to right 
fashion.

the top-left position of the first column is the same as the top-left x,y of
its owner menubar.

menu_t
|
+-- menuBar_t 
|      |
|      + menuBox_t +----------+ menuBox_t +-->
|           +                      +
|           menuColumn_t           menuColumn_t
|              |                      | 
|              |                      +--> 
|              +-- menuBox_a           
|              +-- menuBox_t --+-- menuColumn_t
|              |                      |
|              |                      +-- menuBox_t
|              |                      +-- menuBox_t
|              +-- menuBox_t
|              +-- menuBox_t
|
+-- menuBar_t
|

menuColumns can be wider than the menuBox that is its head node, because only
one menuColumn may be open at a time.

boxes are always flush against one-another in a column.  any spacings are 
determined in x,y pad.  

mouse-over colors are determined automatically, they either darken the box, 
lighten it or invert the colors.  manual highlight colors are in the works,
but currently un-finished.

menuBox has no drawing information

a box highlight can come from mouseover or from left click.  in the case of 
mouseOver, the box is NOT activated, though a tool tip will appear after a few
seconds.  when the mouse leaves mouseOver, the highlight is removed.  

onMouseClick, the box is activated and it exposes whatever is attached to it,
or it executes whatever function is bound to it.  If a function is bound, it
is executed and the menu-structure visibility is reset.
*/

#ifndef __ME_MENUS__
#define __ME_MENUS__ 1

#include "../common/common.h"


#define MENUBOX_STR_SIZE 128
#define MENUBOX_TEXTAREA_STR_SIZE 1024


typedef enum {
	MENU_DARKEN,
	MENU_LIGHTEN,
	MENU_USE_HL_COLOR,
	MENU_INVERT_COLOR,
} menuHighlightMode_t;

typedef enum {
	
	MENU_BAR                    = BIT(0),
	MENU_COLUMN                 = BIT(1),
	MENU_ALL                    = BIT(2),

	MENU_BACKGROUND             = BIT(3),
	MENU_FONT                   = BIT(4),
	MENU_BACKGROUND_HIGHLIGHT   = BIT(5),
	MENU_FONT_HIGHLIGHT         = BIT(6),

} menuNode_t;

#define COLOR_MASK (MENU_BACKGROUND|MENU_FONT|MENU_BACKGROUND_HIGHLIGHT|MENU_FONT_HIGHLIGHT)

typedef enum {
	MENU_NONE,
} menuFace_t;

#define MENU_DEF_BG_COLOR               1.0f
#define MENU_DEF_FONT_COLOR             0.0f
#define MENU_DEF_BG_HL_COLOR            0.5f
#define MENU_DEF_FONT_HL_COLOR          0.75f

// 
#define MOUSE_ON_MENU                   33
#define MOUSE_IN_STASIS                 34
#define MENU_LATENCY                    1000

enum { 
	MOUSE_NORMAL			= 0,
	MOUSE_LMB_DOWN		= BIT(0),
	MOUSE_LMB_UP		= BIT(1),
	MOUSE_RMB_DOWN		= BIT(2),
	MOUSE_RMB_UP		= BIT(3),
	MOUSE_MMB_DOWN		= BIT(4),
	MOUSE_MMB_UP		= BIT(5),
};



/*
====================
 menuColorScheme_t
====================
*/
struct menuColorScheme_t {
	float barColor[4];
	float colColor[4];
	float barFont[4];
	float colFont[4];
	float hlColor[4];
	float hlfColor[4];
	menuColorScheme_t () {
		barColor[0] = barColor[1] = barColor[2] = colColor[0] = colColor[1] = colColor[2] = MENU_DEF_BG_COLOR;
		barFont[0] = barFont[1] = barFont[2] = colFont[0] = colFont[1] = colFont[2] = MENU_DEF_FONT_COLOR;
		hlColor[0] = hlColor[1] = hlColor[2] = MENU_DEF_BG_HL_COLOR;
		hlfColor[0] = hlfColor[1] = hlfColor[2] = MENU_DEF_FONT_HL_COLOR;
		barColor[3] = colColor[3] = barFont[3] = colFont[3] = hlColor[3] = hlfColor[3] = 1.0f;
	}
};


/*
====================
 menuBox_t
====================
*/
class menuBox_t {
private:
	menuBox_t *next;
	class menuContainer_t * child_con;  // child column, if any

public:
	char    text[ MENUBOX_STR_SIZE ];
	char    desc[ MENUBOX_STR_SIZE ];   // not sure, might use this for tool-tip
	int     is_highlight;

	menuContainer_t *parent;

	int     recompute_dimension;
	int     num_chars;   
	float   width;                      // text-portion
	float   x, y, height;

	void(*func_f)(void);
	bool(*bfunc_f)( const char *, int, int );

	// explicit constructor
	static menuBox_t * newBox( const char *, void(*)(void) =NULL, const char * =NULL );

	void setChildColumn( class menuContainer_t * c ) { child_con = c; }

	class menuContainer_t * getCon( void ) const { return child_con; }

	menuBox_t * getNext() const { return next; }
	void setNext( menuBox_t *a ) { next = a; }

	void reset( void );

	menuBox_t ( void ) { reset(); }

	// inlines
	void drawContainer( void );
	void recomputeIfChanged( float, float, float, float, const float * );
	bool checkMouse( float mx, float my ) const;

	const float *getFontHighlight4v ( void ) const ;
	const float *getBackgroundHighlight4v ( void ) const;
};


/*
====================
 menuContainer_t

	abstract base class for all container types.  (a container is what is
	triggered by a box)
====================
*/
class menuContainer_t {
private:

protected:

	float x, y; 
	float w, h, wpad, hpad;     // font dimensions
	float height, width;        // extended column dimensions
	int stringLength;

	float padding[2];

	menuBox_t *box_head;        // head of the child-node list
	menuBox_t *_trigbox;        // box that exposes this column
	menuContainer_t *next;

public:

	float bgColor[4];           // background color
	float fontColor[4];         // font color
	float hlColor[4];           // highlight color for mouse-over: background
	float hlfColor[4];          // highlight color for mouse-over: font

	menuContainer_t *mnext;     // used by master list
	class menu_t *ancestor; 
	class menuBox_t *parent;    // menuBars don't have parents, they are immediatly
                                //  decended from menu
	menuBox_t *activated_box;   // child box of this container if child is activated

	bool is_exposed;

	menuContainer_t *fnext;

	// basic initialization
	virtual void reset( void );
	menuContainer_t() { reset(); }

	// must be provided
	virtual void draw( void ) = 0;
	virtual void propagateDimensions( float, float, float =0.f, float =0.f ) = 0;
	virtual void propagateLocation( float, float, float, float ) = 0;


	// creates child-node box with text-initializer and adds it
	menuBox_t * addTextBox( const char *, void(*)(void) =NULL, const char * =NULL );

	void addBox( menuBox_t * );
	void connectTriggerBox( menuBox_t *b ) { _trigbox = b; b->setChildColumn( this ); }
	const menuBox_t * getTriggerBox( void ) const { return const_cast<const menuBox_t *>( _trigbox ); }
	void unHighlightBoxes( void );

	// accessors
	menuBox_t * getBoxHead( void ) const { return box_head; }

	void setColor( float r, float g, float b, float a ) {
		bgColor[0] = r; bgColor[1] = g; bgColor[2] = b; bgColor[3] = a; 
	}
	void setFontColor( float r, float g, float b, float a ) {
		fontColor[0] = r; fontColor[1] = g; fontColor[2] = b; fontColor[3] = a; 
	}
	void setHighlightColor( float r, float g, float b, float a ) {
		hlColor[0] = r; hlColor[1] = g; hlColor[2] = b; hlColor[3] = a; 
	}
	void setHighlightFontColor( float r, float g, float b, float a ) {
		hlfColor[0] = r; hlfColor[1] = g; hlfColor[2] = b; hlfColor[3] = a; 
	}

	void setColorAll( unsigned int, float r, float g, float b, float a );
	void setFontColorAll( unsigned int, float r, float g, float b, float a );
	void setHighlightColorAll( unsigned int, float r, float g, float b, float a );
	void setHighlightFontColorAll( unsigned int, float r, float g, float b, float a );

	float getWidth( void ) const { return width; }
	menuContainer_t * getNext( void ) const { return next; }
	void setNext( menuContainer_t *a ) { next = a; }

	void setPadding( float x, float y ) { padding[0] = x; padding[1] = y; }
	void setActiveNode( const char * );
	void setActive( menuBox_t * );

	bool checkMouse( float mx, float my ) const { return mx >= x && mx <= x+width && my >= y && my <= y+height; }
	void setLocation( float _x, float _y ) { x = _x; y = _y; }

};

/*
====================
 menuColumn_t
	
	vertical row which contains stacked menuboxes
====================
*/
class menuColumn_t : public menuContainer_t {
private:

protected:

public:

	void draw( void );
	void propagateLocation( float, float, float, float );
	void propagateDimensions( float, float, float, float );

	menuColumn_t() { this->reset(); }

};


/*
====================
 menuBar_t

	horizontal arrangement of menuboxes
====================
*/
class menuBar_t : public menuContainer_t {

private:

protected:

public:

	void reset( void ) {
		menuContainer_t::reset();
		is_exposed = true;
	}

	void draw( void );
	void propagateLocation( float, float, float, float );
	void propagateDimensions( float, float, float, float );

	menuBar_t( void ) { reset(); }

};


/*
====================
 textWin_t

	Window of static text, activated by menubox click.  A click anywhere
	outside of the window perimeter will minimize it.
====================
*/
class textArea_t : public menuContainer_t {
private:
public:
	char text[ MENUBOX_TEXTAREA_STR_SIZE ];
	int _slen;

	void draw( void );
	void propagateLocation( float, float, float, float );
	void propagateDimensions( float, float, float, float );


	void setText( const char * );

	void reset( void ) {
		menuContainer_t::reset();
		_slen = 0;
	}
};

enum {
	WATCH_NOTHING = 0,
	WATCH_FLOATING = 1,
};

/*
====================
 menu_t

	composed of one or more menubars
====================
*/
class menu_t {

private:

	// helper function for ownership
	void _ownership_recurse( menuContainer_t * );

	// helper for buildColumnMasterList
	void _masterList_recurse( menuContainer_t *, menuContainer_t ** );

	menuBar_t *bar_head;

	// floating boxes 
	menuContainer_t *floating_head;

	// data
	float wChar;
	float hChar;
	float wPad;
	float hPad;
	menuColorScheme_t c;

	gbool dimensions_computed; 
	gbool location_computed;
	gbool ownership_computed;
	gbool propagate_colors;
	gbool containerListBuilt;
	// one to rule them all
	gbool menuSystemBuilt;

	float x_res, y_res;

	class menuContainer_t *	current_con;
	class menuBox_t *		current_box;
	class menuContainer_t *	last_con;
	class menuBox_t *		last_box;

	menuContainer_t *con_head;  // linked list of _all_ Containers in the 
                                // entire menu system. ( using mnext pointer )

	void buildMenuSystem( void );
	void buildContainerList( void );

	int watch_flags ;

	int font;

public:

	static float print( float, float, const char * , ... );

	float mouse[2];

	unsigned int highlight_mode;

	void reset( void );

	menu_t( void ) : c() { reset() ; }

	void setScreenDim( float w, float h ) { x_res = w; y_res = h; }

	// methods
	void draw( void ) ;
	void addBar( menuBar_t * );

	// chars in a menu are all the same dimensions. set once here
	void setCharDim( float w, float h ) { wChar = w; hChar = h; }
	void setPadding( float w, float h ) { wPad = w; hPad = h; }

	// args == wChar, hChar, wPad, hPad
	static menu_t * newMenuXY( float, float, float =0.f, float =0.f );

	// state
	void mouseClick( float, float );
	void mouseOver( float, float, bool =false );

	void propagateDimensions( void ) ;

	void setColorScheme( menuColorScheme_t * );

	void setBarFontColor( float, float, float, float );
	void setColFontColor( float, float, float, float );

	void setBarColor( float, float, float, float );
	void setColColor( float, float, float, float );

	void setGlobalFontColor( float, float, float, float );
	void setGlobalBackgroundColor( float, float, float, float );

	void setGlobalBackgroundColorGreyScale( float, float =1.0f );

	void setColHighlightColor( float, float, float, float );
	void setColHighlightFontColor( float, float, float, float );

	const menuColorScheme_t * getColorScheme( void ) const { return const_cast<const menuColorScheme_t *>( &c ) ; }

	void propagateLocation( void );
	void propagateOwnership( void );
	void propagateColors( void );

	float getXRes() const { return x_res; }
	float getYRes() const { return y_res; }

	static void setBoxDimensions( menuBox_t *box , float, float, float, float, const float * );
	void setDimensions( float, float , float, float );
	void deActivateAll( void );

	void registerFloating( menuContainer_t * );
	// for containers that require or can use key input
	bool keyboard( int, int );
	void mouseInput( int, int, int, int );

	bool blocking( void ) const { return watch_flags & WATCH_FLOATING; }

	void clearFloatingBoxes( void );
	
	void destroy( void );
};

/*
===================
 menuContainer_t::unHighlightBoxes
===================
*/
inline void menuContainer_t::unHighlightBoxes( void ) {
	menuBox_t *box = box_head;
	while ( box ) {
		box->is_highlight = 0;
		box = box->getNext();
	}
}

/*
===================
 menu_t::deActivateAll
	happens when you , for instance, click outside the menu areas all-together
===================
*/
inline void menu_t::deActivateAll( void ) {
	menuContainer_t *con = con_head;
	while ( con ) {
		if ( typeid( *con ) != typeid( menuBar_t ) ) // bars are always exposed
			con->is_exposed = false;
		con->activated_box = NULL;
		con = con->mnext;
	}
}

/*
===================
menuBox_t::drawContainer
===================
*/
inline void menuBox_t::drawContainer( void ) { 
	if ( child_con && child_con->is_exposed ) {
		child_con->draw(); 
	}
}

/*
===================
menuBox_t::recomputeIfChanged
===================
*/
inline void menuBox_t::recomputeIfChanged( float f, float g, float h, float i, const float *padding ) {
	if ( recompute_dimension ) {
		menu_t::setBoxDimensions( this, f, g, h, i, padding );
	}
}

/*
===================
 menuBox_t::checkMouse

 FIXME: perhaps typeid ops should be replaced with a static id tag
===================
*/
inline bool menuBox_t::checkMouse( float mx, float my ) const {
	if ( parent && typeid(*parent) == typeid(menuColumn_t) ) {
		return mx >= x && mx < x+parent->getWidth() && my >= y && my < y+height; 
	} else {
		return mx >= x && mx < x+width && my >= y && my < y+height; 
	}
}


#define TE_BLOCKING 1
#define TEXT_ENTRY_LENGTH	64

/*
====================
 messageBox_t

 like a windows messageBox, customizeable number of buttons, default is "OK", also pass a 
 flag to make a confirmation box "OK/CANCEL"
====================
*/
class messageBox_t : public menuContainer_t {
	unsigned int magic;
	static const unsigned int TE_MAGICK = 0x7AFF7355;
public:
	char label[ MENUBOX_TEXTAREA_STR_SIZE ];
	int label_lines;
	int _slen;

	void keys( int , int );
	void mouse( int, int, int, int );

	void draw( void );
	void propagateLocation( float, float, float, float );
	void propagateDimensions( float, float, float, float );

	void setLabel( const char * );

	void setOKFunc( void(*_ok)(void) ) {
		if ( b_ok ) {
			b_ok->func_f = _ok;
		}
	}

	int flag;

	menuBox_t *b_ok;

	void reset( void ) {
		label[0] = '\0';
		label_lines = 1;
		_slen = 0;
		if ( magic != TE_MAGICK ) {
			b_ok = menuBox_t::newBox( "OK" );
			addBox( b_ok );
		}
		magic = TE_MAGICK;
	}
};


/*
====================
 textEntryBox_t

 single row text entry field, like messageBox, queries menu_t for it's color scheme, text
 field is of fixed width, but the blinking cursor pushes hidden text into the open

 // also with OK/CANCEL buttons
====================
*/
class textEntryBox_t : public menuContainer_t {
	static const unsigned int TE_MAGICK = 0x71577355;
	unsigned int magic;
public:
	char label[ MENUBOX_TEXTAREA_STR_SIZE ];
	int label_lines;
	int _slen;
	char entry[ 64 ];
	int _elen;
	int max_elen;
	bool caps, shift;

	void draw( void );
	void propagateLocation( float, float, float, float );
	void propagateDimensions( float, float, float, float );


	void setLabel( const char * );

	void setOKCancelFuncs( bool(*_ok)(const char*,int,int), void(*_cancel)(void) ) {
		if ( b_ok ) {
			b_ok->bfunc_f = _ok;
		}
		if ( b_cancel ) {
			b_cancel->func_f = _cancel;
		} 
	}

	int flag;

	menuBox_t *b_ok;
	menuBox_t *b_cancel;
	menuBox_t *b_entry;

	void reset( void ) {
		label_lines = 1;
		caps = shift = false;
		menuContainer_t::reset();
		_elen = 0;
		_slen = 0;
		max_elen = 24;
		flag = 0;
		label[0] = 0;
		entry[0] = '\0';
		this->is_exposed = true;
		if ( magic != TE_MAGICK ) {
			b_ok = menuBox_t::newBox( "OK" );
			b_cancel = menuBox_t::newBox( "Cancel" );
			addBox( b_ok );
			addBox( b_cancel );
		}
		magic = TE_MAGICK;
	}

	/* fucking default ctor debacle!!!
	textEntryBox_t() {
		reset();
	}
	textEntryBox_t(int flags) {
		this->flag |= flags;
		reset();
	}*/

	void keys( int , int );
	void mouse( int, int, int, int );
};


#endif /* !__ME_MENUS__ */
