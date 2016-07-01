#ifndef __G_MOVE_H__
#define __G_MOVE_H__


#include "../lib/lib_list_c.h"


/// named move 
enum PlayerMove_t {
	PM_NONE,
	PM_FORWARD,
	PM_BACKWARD,
	PM_LEFT,
	PM_RIGHT,
	PM_SHOOT,
	PM_ATTACK,			// hand or handheld weapon attack
	PM_USE,
	PM_INVENTORY,
	PM_CELLPHONE,
	PM_MENU,			// ESC key
	PM_MINIMAP, 		// display map overlay
	PM_ALT_ATTACK, 		// secondary attack: ie, grenades, smoke, 
};

// tribute to the all-pervasive idSoftware usercmd_t idiom
struct usercmd_t {
	int				created;		// time usercmd_t created
	int				serverTime;		// 
	unsigned char 	weapon; 		// which weapon currently held
	int 			buttons;		// which actions requested last frame
	signed char		axis[2]; 		// axis of requested movement
	void zero() { 
		created = 0;
		serverTime = 0;
		weapon = 0;
		buttons = 0;
		axis[0] = axis[1] = 0;
	}
	usercmd_t() {
		zero();
	}
	usercmd_t(int _t) { zero(); created = _t; }
};


struct CommandBuffer_t : public list_c<usercmd_t>, public Allocator_t  {
	memPool<node_c<usercmd_t> > internal_pool;
public:
	void init() {
		internal_pool.init( gfalse, 128 );
		list_c<usercmd_t>::init( &internal_pool ); 
	}
	void reset() { 
		list_c<usercmd_t>::reset(); 
		internal_pool.reset();
	}
	void destroy() { 
		list_c<usercmd_t>::destroy(); 
		internal_pool.destroy();
	}
	CommandBuffer_t() { init(); }
	~CommandBuffer_t() { destroy(); }
};

extern CommandBuffer_t cmdbuf;


#endif // __G_MOVE_H__
