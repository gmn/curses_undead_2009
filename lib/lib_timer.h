#ifndef __LIB_TIMER_H__
#define __LIB_TIMER_H__

#include "../common/com_types.h"

int Com_Millisecond( void );

//==================================================================
//   Timer  :  timer_c
//==================================================================
class timer_c {
public:
    uint32 flags;
    uint32 start;
    uint32 length;

    void reset ( void ) {
        flags = start = length = 0;
    }
    timer_c( void ) {
        flags = start = length = 0;
    }
    bool check( void ) {
        return Com_Millisecond() - start > length;
    }
    int timeup( void ) {
        return check();
    }
    void set( uint32 len =0 ) {
        length = len;
        start = Com_Millisecond();
        flags = 1;
    }
    void set( timer_c const & t ) {
		flags = t.flags;
		start = t.start;
		length = t.length;
    }

    void set( uint32 len, uint32 flg ) {
        length = len;
        start = Com_Millisecond();
        flags = flg;
    }
	uint32 delta( void ) {
		return Com_Millisecond() - start;
	}
	float ratio( void ) {
		return (float) delta() / (float) length;
	}
    uint32 time( void ) {
        return delta();
    }
};

#endif
