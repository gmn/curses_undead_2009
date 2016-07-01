#ifndef __G_TILES_H__
#define __G_TILES_H__

#include "../common/com_geometry.h"
#include "g_animation.h"

// the visual description that each maptile or entity possess
class tile_t {
protected:
	animation_t *anim;
	material_t *mat;	// whichever's not null.  can't be both
	colModel_t col;
	brush_t *brush;		// dimension & location 
	int layer;			// which layer is the tile on?
};

//enttile_t := (animation_t | material_t) & colModel_t & (poly_t | brush_t)
class entTile_t : public tile_t {
};

//maptile_t := (animation_t | material_t) & colModel_t & (poly_t | brush_t)
class mapTile_t : public tile_t {
};

#endif // __G_TILES_H__
