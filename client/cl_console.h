#ifndef __CL_CONSOLE_H__
#define __CL_CONSOLE_H__ 1


#include "../common/common.h"





enum {
	CON_CLOSED,
	CON_EXPANDING,
	CON_OPEN,
	CON_RETRACTING,
};

#define CON_LINE_SIZE	1024*4

class console_t {
private:
	int state;
	int height;				// absolute height
	int anim_height;		// scrolling height, for animation
	float frac_tick;		// millisecond per pixel increase/decrease in anim_height
	float next_tick;
	int border_width;		// width of little line that borders the entry area from the output area
	int input_area_pad;		// input area needs some padding to make it the right size

	uint32 keyCatchSave;
	
	// console output
	list_c<char*> lines;

	// command history
	buffer_c<char*> history;
	
	struct {
		float x, y;			// top left corner of the viewport location
		float w, h; 		// dimensions of viewing area
		float wpad, hpad;	// font character padding
		float xpad, ypad;	// window padding around viewport perimeter
	} view;

	void pollDynamicSize( void );
	void on( void );
	void off( void );
	class timer_c timer;
	static const int TRANSITION_TIME = 250;

	char current_line[ CON_LINE_SIZE ];
	int current_char; // where input cursor is on the current line

	int shift;
	bool capslock;
	int font;

	int checkCmd( void );
	int checkGvar( void );

	int scroll ;
	static const int SCROLL_AMOUNT = 9;
public:
	bool is_exposed;

	void reset( void );
	void init( void ) { reset(); }
	void destroy( void );

	void Clear( void );				// only clears the display area
	void ClearHistory( void );		// actually removes the history
	void Draw( void );
	void Toggle( void );
	void pushMsg( const char * );
	void Printf( const char *, ... );
	void keyHandler( int, int );
	void mouseHandler( mouseFrame_t * ) ;

	// dumps to file
	void dumpcon( const char * =NULL );
	void dump( const char * =NULL );

	// load text into the console from file
	bool load( const char * );

	int histLine;
	void SaveLine( void ); // save line into history
	void HistoryBack( void );
	void HistoryNext( void );
	void PrintHistory( void );
	void drawScreenBuffer( void ); // for drawing in game
};


extern class console_t console;


#endif /* !__CL_CONSOLE_H__ */
