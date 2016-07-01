
#include "../common/com_geometry.h"

// plots points, lines, polygons to the screen as instructed
//  for instance, if we want to see where the computed centroid is in relation
//  to a group of selected, when it is computed just call plotter.plot(x,y,4);
//  which will plot a point at x,y for 4 seconds, then it will fade out.
//  the plotter is called by the update code.  via the Update(); handle, which
//  draws to GL directly.  Maintains it's own internal vertex arrays for each
//  points, lines, Triangles, Quads.  if ( array.size() > 0 ) array.draw(); 
class plotter_c {
	buffer_c<point_t> points;
	buffer_c<line_t> lines;
	buffer_c<quad_t> quads;
	buffer_c<texquad_t> texquads;
	buffer_c<point_t> tris;
private:

	// plots in world coord
	void plot( float x, float y,  int color =~0, float dur =0.f );

	// draw any/all
	void Update( void );

	// reset everything
	void Clear( void ) {
		points.init();
		lines.init();
		quads.init();
		texquads.init();
		tris.init();
	}

	// basic line
	void line( float, float, float, float, int color =~0, float dur =0.f ); 

	// vector
	void arrow( float, float, float, float, int color =~0, float dur =0.f );

	// floating text
	void message( const char *, float x, float y, int color =~0, float dur =0.f );

	// quad
	void quad( float *, int color =~0, float dur =0.f );

	//
	void textured_quad( float *v, float *rstq, int texnum );
};


	// plots in world coord
inline void plotter_c::plot( float x, float y,  int color, float dur ) {
	point_t p;
	p.u[0] = x; p.u[1] = y; p.c = color;
	points.add( p );
}
	// basic line
inline void plotter_c::line( float x1, float y1, float x2, float y2,  int color, float ) {
	line_t l;	
	l.u[0] = x1; l.u[1] = y1;
	l.v[0] = x2; l.v[1] = y2;
	l.c = l.c2 = color;
	lines.add( l );
}

	// vector
inline void plotter_c::arrow( float x1, float y1, float x2, float y2, int color, float dur ) {
	line_t l;
	point_t v1, v2, v3;
	l.u[0] = x1; l.u[1] = y1;
	l.v[0] = x2; l.v[1] = y2;
	l.c = l.c2 = color;
	lines.add( l );
	v1.u[0] = x2; v1.u[1] = y2;
	v1.c = color;
	//v2.
}

	// quad
inline void plotter_c::quad( float *, int color, float dur ) {
}
	// textured quad
inline void plotter_c::textured_quad( float *v, float *rstq, int texnum ) {
}


