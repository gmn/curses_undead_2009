#ifndef __R_GLMACRO_H__
#define __R_GLMACRO_H__

#define GL_DRAW_OUTLINE_4i( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3i( (v)[0], (v)[1], 0 ); \
        gglVertex3i( (v)[0], (v)[3], 0 ); \
        gglVertex3i( (v)[2], (v)[3], 0 ); \
        gglVertex3i( (v)[2], (v)[1], 0 ); \
        gglVertex3i( (v)[0], (v)[1], 0 ); \
		gglEnd(); }
	
#define GL_DRAW_OUTLINE_8i( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3i( (v)[0], (v)[1], 0 ); \
        gglVertex3i( (v)[2], (v)[3], 0 ); \
        gglVertex3i( (v)[4], (v)[5], 0 ); \
        gglVertex3i( (v)[6], (v)[7], 0 ); \
        gglVertex3i( (v)[0], (v)[1], 0 ); \
		gglEnd(); }

#define GL_LINES_FANCYPANTS( v, o, h ) { \
		gglBegin( GL_LINES );\
		gglVertex3i( (v)[0]-(h), (v)[1]-(o), 0 );\
		gglVertex3i( (v)[0]-(h), (v)[3],     0 );\
		\
		gglVertex3i( (v)[0]-(o), (v)[3]+(h), 0 );\
		gglVertex3i( (v)[2]    , (v)[3]+(h), 0 );\
		\
		gglVertex3i( (v)[2]+(h), (v)[1]    , 0 );\
		gglVertex3i( (v)[2]+(h), (v)[3]+(o), 0 );\
		\
		gglVertex3i( (v)[0]    , (v)[1]-(h), 0 );\
		gglVertex3i( (v)[2]+(o), (v)[1]-(h), 0 );\
		gglEnd(); }
	
#define GL_DRAW_OUTLINE_4V( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
		gglEnd(); }

#define GL_DRAW_OUTLINE_8V( v ) { \
	    gglBegin( GL_LINE_STRIP ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[4], (v)[5], 0.0f ); \
        gglVertex3f( (v)[6], (v)[7], 0.0f ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
		gglEnd(); }

#define GL_DRAW_QUAD_4V( v ) { \
        gglBegin( GL_QUADS ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[0], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[2], (v)[1], 0.0f ); \
		gglEnd(); }

#define GL_DRAW_QUAD_8V( v ) { \
        gglBegin( GL_QUADS ); \
        gglVertex3f( (v)[0], (v)[1], 0.0f ); \
        gglVertex3f( (v)[2], (v)[3], 0.0f ); \
        gglVertex3f( (v)[4], (v)[5], 0.0f ); \
        gglVertex3f( (v)[6], (v)[7], 0.0f ); \
		gglEnd(); }

#define UP_ARROW( v ) { \
		float _r, _s, _o = 12.f;\
		gglTranslatef( -_o, 0.f, 0.f );\
		_r = v[0] + (v[2]-v[0])/2.f;\
		_s = v[3];\
		gglBegin( GL_LINE_STRIP );\
		gglVertex3f( _r, _s-_o, 0.f );\
		gglVertex3f( _r, _s+_o, 0.f );\
		gglVertex3f( _r-_o/4.f, _s+_o-_o/4.f, 0.f );\
		gglVertex3f( _r+_o/4.f, _s+_o-_o/4.f, 0.f );\
		gglVertex3f( _r, _s+_o, 0.f );\
		gglEnd();\
		gglTranslatef( +_o, 0.f, 0.f ); }

#define DOWN_ARROW( v ) { \
		float _r, _s, _o = 12.f;\
		gglTranslatef( +_o, 0.f, 0.f );\
		_r = v[0] + (v[2]-v[0])/2.f;\
		_s = v[1];\
		gglBegin( GL_LINE_STRIP );\
		gglVertex3f( _r, _s+_o, 0.f );\
		gglVertex3f( _r, _s-_o, 0.f );\
		gglVertex3f( _r+_o/4.f, _s-_o+_o/4.f, 0.f );\
		gglVertex3f( _r-_o/4.f, _s-_o+_o/4.f, 0.f );\
		gglVertex3f( _r, _s-_o, 0.f );\
		gglEnd();\
		gglTranslatef( -_o, 0.f, 0.f ); }

#define LEFT_ARROW( v ) {\
		float _r, _s, _o = 12.f;\
		gglTranslatef( 0.f, +_o, 0.f );\
		_r = v[0];\
		_s = v[1] + (v[3]-v[1]) / 2.f;\
		gglBegin( GL_LINE_STRIP );\
		gglVertex3f( _r+_o, _s, 0.f );\
		gglVertex3f( _r-_o, _s, 0.f );\
		gglVertex3f( _r-_o+_o/4.f, _s-_o/4.f, 0.f );\
		gglVertex3f( _r-_o+_o/4.f, _s+_o/4.f, 0.f );\
		gglVertex3f( _r-_o, _s, 0.f );\
		gglEnd();\
		gglTranslatef( 0.f, -_o, 0.f ); }

#define RIGHT_ARROW( v ) {\
		float _r, _s, _o = 12.f;\
		gglTranslatef( 0.f, -_o, 0.f );\
		_r = v[2];\
		_s = v[1] + (v[3]-v[1]) / 2.f;\
		gglBegin( GL_LINE_STRIP );\
		gglVertex3f( _r-_o, _s, 0.f );\
		gglVertex3f( _r+_o, _s, 0.f );\
		gglVertex3f( _r+_o-_o/4.f, _s+_o/4.f, 0.f );\
		gglVertex3f( _r+_o-_o/4.f, _s-_o/4.f, 0.f );\
		gglVertex3f( _r+_o, _s, 0.f );\
		gglEnd();\
		gglTranslatef( 0.f, +_o, 0.f ); }

#endif // __R_GLMACRO_H__
