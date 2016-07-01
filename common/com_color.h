#ifndef __COM_COLOR_H__
#define __COM_COLOR_H__


#define C_BLACK     0x000000FF
#define C_WHITE     0xFFFFFFFF
#define C_RED       0xFF0000FF
#define C_GREEN     0x00FF00FF
#define C_BLUE      0x0000FFFF
#define C_YELLOW    0xFFFF00FF
#define C_PURPLE    0x990099FF
#define C_VIOLET    0xFF00FFFF
#define C_ORANGE    0xFF6600FF
#define C_AQUA      0x0099CCFF
#define C_NAVY      0x003399FF
#define C_DARKGREEN 0x006600FF
#define C_HOTPINK   0xFF3399FF
#define C_DEEPPURPLE    0x660066FF
#define C_LIME      0x339900FF
#define C_BURGUNDY  0x990033FF
#define C_INDIGO    0x3300CCFF
#define C_BEIGE     0x996633FF

#define CON_COLOR_WHITE ^1

struct colorSet_t {
	unsigned int color;
	char *name;
	float fc[4];
};

extern colorSet_t colors[];
extern const int TOTAL_COLORS;

class color_c { 
public:
	static colorSet_t *colors;
	static void StringToColor4v( float *, const char * );
	color_c() {
		this->colors = ::colors;
	}
	static int GetColorName4v( float *, const char * );

	static int GetColorNamei( unsigned int *, const char * );
	static void StringToUInt( unsigned int *, const char * );
	static float * TokenToFloat( unsigned int );
};

extern color_c color;






#endif // __COM_COLOR_H__

