/*
 * gldemo.cpp
 */

#define GL_SQUARE(x,y,z,s) \
		glBegin(GL_QUADS); \
        glVertex3f((GLfloat)x,(GLfloat)y+s,(GLfloat)z); \
        glVertex3f((GLfloat)x+s,(GLfloat)y+s,(GLfloat)z); \
        glVertex3f((GLfloat)x+s,(GLfloat)y,(GLfloat)z); \
        glVertex3f((GLfloat)x,(GLfloat)y,(GLfloat)z); \
        glEnd()

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "../renderer/r_ggl.h"
#pragma comment(lib, "glu32.lib")

void GL_Demo_Init ( int w, int h ) 
{
   gglViewport (0, 0, (GLsizei) w, (GLsizei) h); 
   gglMatrixMode (GL_PROJECTION);
   gglLoadIdentity ();
   gglFrustum (-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);
   gglMatrixMode (GL_MODELVIEW);

   gglClearColor (0.0, 0.0, 0.0, 0.0);
   gglShadeModel (GL_FLAT);

   gglEnable(GL_DEPTH_TEST);
   gglDepthFunc(GL_LESS);

   gglEnable(GL_BLEND);
   gglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
}

static void GL_Demo1 ( void )
{

    gglLoadIdentity ();             
	gglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gluLookAt (0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    // 
    // wont blend because it depends on what's already there
    //

	gglDisable(GL_BLEND);
    // yellow
    gglColor4f(1.0,1.0,0.0,0.6); GL_SQUARE( 0.0, 0.0,   0,   1);
	gglEnable(GL_BLEND);
    // red
    gglColor4f(1.0,0.0,0.0,0.6); GL_SQUARE( 0.5, 0.5,  -1,   1);
    // green
    gglColor4f(0.0,1.0,0.0,0.6); GL_SQUARE( 1.0, 1.0,  -2,   1);
    // orange
    gglColor4f(1.0,0.7,0.0,0.6); GL_SQUARE( 1.5, 1.5,  -3,   1);

    gglTranslatef(2.0, 0.0, 0.0);

    //
    // Left and right will Blend because drawing furthest away from
    //  cam first.
    //

    // orange
    gglColor4f(1.0,0.7,0.0,0.6); GL_SQUARE( 1.5, 1.5,  -3,   1);
    // green
    gglColor4f(0.0,1.0,0.0,0.6); GL_SQUARE( 1.0, 1.0,  -2,   1);
    // red
    gglColor4f(1.0,0.0,0.0,0.6); GL_SQUARE( 0.5, 0.5,  -1,   1);
    // yellow
    gglColor4f(1.0,1.0,0.0,0.6); GL_SQUARE( 0.0, 0.0,   0,   1);

    gglTranslatef(-4.0, 0.0, 0.0);

    //
    // either way, the depth buffer determines which frags get
    //  drawn and which don't
    //

    // orange
    gglColor4f(1.0,0.7,0.0,0.6); GL_SQUARE( 1.5, 1.5,  -3,   1);
    // green
    gglColor4f(0.0,1.0,0.0,0.6); GL_SQUARE( 1.0, 1.0,  -2,   1);
    // red
    gglColor4f(1.0,0.0,0.0,0.6); GL_SQUARE( 0.5, 0.5,  -1,   1);
    // yellow
    gglColor4f(1.0,1.0,0.0,0.6); GL_SQUARE( 0.0, 0.0,   0,   1);

    gglFlush ();
}

void GL_Demo_Frame( void ) {
    GL_Demo1();
}
