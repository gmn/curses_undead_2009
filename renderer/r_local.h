#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__

// local files always include public , first line
#include "r_public.h"

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT
} renderCommand_t;

/*
typedef struct {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_ENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	renderCommandList_t	commands;
} backEndData_t;
*/

//extern volatile renderCommandList_t	*renderCommandList;


#define	MAX_RENDER_COMMAND_BYTES	0x40000

typedef struct {
	byte	cmds[MAX_RENDER_COMMAND_BYTES];
	int		used;
} renderCommandList_t;

/* responsible for drawing * /
 * moved to r_public
class Renderer_t {
private:
public:
	void Init( void );
	void DrawFrame( void );
};
*/


#endif
