////////////////////////////////
// r_public.h
//
#ifndef __R_PUBLIC_H__
#define __R_PUBLIC_H__

//#pragma once
#include "r_ggl.h"

//#include "../map/m_area.h" // Block_t
#include "../lib/lib_list_c.h" // buffer_c<>



#include "../common/common.h"



class renderExport_t {
public:
    void (*init) ( void );

    void start ( void ) {
        init = R_Init;
    }

private:
    void (*internalInit) ( void );

    friend void R_Init ( void );

};



class renderState_t {
public: 
    gbool allowExtensions;

    void start ( void ) {
        allowExtensions = gtrue;
    }

private:
};



/////////////////////////////////
/////////////////////////////////
// glconfig_t
/////////////////////////////////
/////////////////////////////////

// TODO: Get:   - 2007 values for hardware & driver
//              - modern textureCompression types 


typedef enum {
	TC_NONE,
	TC_S3TC
} textureCompression_t;

typedef enum {
	GLDRV_ICD,					// driver is integrated with window system
								// WARNING: there are tests that check for
								// > GLDRV_ICD for minidriverness, so this
								// should always be the lowest value in this
								// enum set
	GLDRV_STANDALONE,			// driver is a non-3Dfx standalone driver
	GLDRV_VOODOO				// driver is a 3Dfx standalone driver
} glDriverType_t;

typedef enum {
	GLHW_GENERIC,			// where everthing works the way it should
	GLHW_3DFX_2D3D,			// Voodoo Banshee or Voodoo3, relevant since if this is
							// the hardware type then there can NOT exist a secondary
							// display adapter
	GLHW_RIVA128,			// where you can't interpolate alpha
	GLHW_RAGEPRO,			// where you can't modulate alpha on alpha textures
	GLHW_PERMEDIA2			// where you don't have src*dst
} glHardwareType_t;

#define OPENGL_STRING_SIZE 4096
#define Zero_String(s) memset( (s), 0, sizeof(s) )

class glconfig_t {
public:
    char vendor_string[OPENGL_STRING_SIZE];
    char renderer_string[OPENGL_STRING_SIZE];
    char version_string[OPENGL_STRING_SIZE];
    char extensions_string[OPENGL_STRING_SIZE];

	int						maxTextureSize;			// queried from GL
	int						maxActiveTextures;		// multitexture ability

	int						colorBits, depthBits, stencilBits;

	glDriverType_t			driverType;
	glHardwareType_t		hardwareType;

	gbool				    deviceSupportsGamma;
	textureCompression_t	textureCompression;
	gbool				    textureEnvAddAvailable;

	int						vidWidth, vidHeight;

	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float					windowAspect;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	gbool				    isFullscreen;
	gbool				    stereoEnabled;
	gbool				    smpActive;		// dual processor

    void start( void ) {
        Zero_String ( vendor_string );
        Zero_String ( renderer_string );
        Zero_String ( version_string );
        Zero_String ( extensions_string );
    }
};


//extern glconfig_t glConfig;
//extern renderState_t rstate;

/* responsible for drawing */
class Renderer_t {
private:
public:
    void Init( void );
    void DrawFrame( void );
};

extern Renderer_t render;


#endif /* __R_PUBLIC_H__ */

// func prototypes
void R_Init ( void );

void R_DrawFrame( void );

//void R_PrintBlocks( buffer_c<Block_t*>&, float *, unsigned int, unsigned int );

