
/*
 ***************************************************************
 *  win_gl.c 
 *
 *  opengl windows platform specific stuff
 *
 ***************************************************************
 */



#include "win_local.h"

//#include "../resource.h"    // IDI_ICON1

#define WGL_WINDOW_NAME         GAME_WINDOW_TITLE
#define WGL_WINDOW_CLASS_NAME   W("OpenGL")

/*
#ifndef GL_MAX_ACTIVE_TEXTURES_ARB
    #define GL_MAX_ACTIVE_TEXTURES_ARB 1
#endif
*/

#include "wgl_driver.h"


renderState_t   rstate;
glconfig_t      glConfig;



extern void ( APIENTRY * gglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern void ( APIENTRY * gglMultiTexCoord2f )( GLenum texture, GLfloat s, GLfloat t );
extern void ( APIENTRY * gglActiveTextureARB )( GLenum texture );
extern void ( APIENTRY * gglActiveTexture )( GLenum texture );
extern void ( APIENTRY * gglClientActiveTextureARB )( GLenum texture );

extern void ( APIENTRY * gglLockArraysEXT)( GLint, GLint);
extern void ( APIENTRY * gglUnlockArraysEXT) ( void );

extern gvar_c *show_cursor;


/*
==================== 
  WGL_KillGLWindow

==================== 
*/

void WGL_KillWindow(void)		
{
    
	if (gld.fullscreen)			
	{
		ChangeDisplaySettings(NULL,0);					
		ShowCursor(TRUE);
	} else
        ShowCursor(TRUE);

	if (gld.hGLRC)									
	{
		if (!gwglMakeCurrent(NULL,NULL))		
		{
            gld.logMsg("Release Of DC And RC Failed.");
		}

		if (!gwglDeleteContext( gld.hGLRC ))
		{
			gld.logMsg("Release Rendering Context Failed.");
		}
		gld.hGLRC=NULL;
	}

	if (gld.hDC && !ReleaseDC( gld.hWnd, gld.hDC ))
	{
		gld.logMsg("Release Device Context Failed.");
		gld.hDC=NULL;
	}

	if (gld.hWnd && !DestroyWindow( gld.hWnd ))
	{
		gld.logMsg("Could Not Release hWnd.");
		gld.hWnd=NULL;
	}

	if (!UnregisterClass( L"OpenGL", gld.hInstance ))
	{
		gld.logMsg("Could Not Unregister Class.");
		gld.hInstance=NULL;	
	}
}


/*
====================
  WGL_CreatePFD
   - create pixelformatdescriptor
   - seen in quake3 gpl src
   - looks like they copied it from the same place that I first saw it
====================
*/
static void WGL_CreatePFD ( PIXELFORMATDESCRIPTOR *pPFD, int colorbits, int depthbits, int stencilbits )
{
    PIXELFORMATDESCRIPTOR src = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		COLORBITS,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer	
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };

	src.cColorBits      = (BYTE) colorbits;
	src.cDepthBits      = (BYTE) depthbits;
	src.cStencilBits    = (BYTE) stencilbits;

	*pPFD = src;
}


static gbool WGL_InitContext ( int colorbits)
{
    int depthbits, stencilbits;
    static PIXELFORMATDESCRIPTOR pfd;

    if ( gld.hDC == NULL )
    {
        if ( ( gld.hDC = GetDC ( gld.hWnd ) ) == NULL )
        {
            return gfalse;
        }
    }

    if ( colorbits == 0 )
    {
        colorbits = COLORBITS;
    }

    if ( colorbits > 16 )
        depthbits = 24;
    else
        depthbits = 16;

    if ( depthbits < 24 )
        stencilbits = 0;
    else
        // FIXME: not sure what's good bitamount
        stencilbits = 8;

    if ( !gld.pixelFormatSet )
    {
		WGL_CreatePFD ( &pfd, colorbits, depthbits, stencilbits );
        gld.pixelFormatSet = gtrue;
        gld.pfd = &pfd;
    } 

    return gtrue;
}

static int WGL_InitGLSubsystem ( void )
{
	uint32 pixelFormat;			

	if (!(gld.hDC = GetDC( gld.hWnd )))	
	{
		WGL_KillWindow();	
		Sys_Error("Can't Create A GL Device Context.");
		return -4;
	}

	if (!(pixelFormat=ChoosePixelFormat( gld.hDC, gld.pfd )))
	{
		WGL_KillWindow();
		Sys_Error("Can't Find A Suitable PixelFormat.");
		return -5;
	}

	if(!SetPixelFormat( gld.hDC, pixelFormat, gld.pfd ))
	{
		WGL_KillWindow();
		Sys_Error("Can't Set The PixelFormat.");
		return -6;
	}

	if (!(gld.hGLRC = gwglCreateContext( gld.hDC )))
	{
		WGL_KillWindow();		
		Sys_Error("Can't Create A GL Rendering Context.");
		return -7;
	}

	if(!gwglMakeCurrent( gld.hDC, gld.hGLRC ))
	{
		WGL_KillWindow();		
		Sys_Error("Can't Activate The GL Rendering Context.");
		return -8;	
	}

    return 0;
}

//LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WGL_CreateWindow (char* title, int width, int height, 
                        int colorbits, gbool fullscreenflag)
{
	WNDCLASS	wc;						
	DWORD		dwExStyle;				
	DWORD		dwStyle;			
	RECT		WindowRect;		

//	HINSTANCE	hInstance;
	DEVMODE dmScreenSettings;


	WindowRect.left   = (long)0;
	WindowRect.right  = (long)width;
	WindowRect.top    = (long)0;
	WindowRect.bottom = (long)height;
    
	gld.fullscreen      = fullscreenflag;

//	if (! gld.hInstance)
	gld.hInstance	= GetModuleHandle(NULL);

	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= gld.WndProc;

	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= gld.hInstance;

    // FIXME : load your own icon, arrow
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	//wc.hIcon			= LoadIcon(gld.hInstance, MAKEINTRESOURCE(IDI_ICON1) );
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);

    // 
	wc.hbrBackground	= NULL;	
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= (LPCWSTR)WGL_WINDOW_CLASS_NAME;

    if ( !gld.classRegistered ) 
    {
	    if (!RegisterClass(&wc))
	    {
		    Sys_Error("Failed To Register The Window Class.");
		    return -1;
	    } 
        else
            gld.classRegistered = gtrue;
    }

	
	if (gld.fullscreen)
	{
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= width;
		dmScreenSettings.dmPelsHeight	= height;
		dmScreenSettings.dmBitsPerPel	= colorbits;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			if (MessageBox(NULL,L"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?",(LPCWSTR)WGL_WINDOW_NAME,MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				gld.fullscreen = gfalse;
			}
			else
			{
			    Sys_Error("Program Will Now Close.");
				return -2;
			}
		}
	}

    // setup for fullscreen or window
	if (gld.fullscreen)			
	{
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;		
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

    if ( !show_cursor->integer() ) 
	    while ( ShowCursor(FALSE) >= 0 )
		    ;

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);



    if ( !gld.hWnd )
    {
	    // Create The actual Window
        gld.hWnd=CreateWindowEx(	
                    dwExStyle,					
                    (LPCWSTR)WGL_WINDOW_CLASS_NAME,
                    (LPCWSTR)WGL_WINDOW_NAME,				
                    dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                    // TODO: double check on how these coordinates affect
                    //  the window placement
                    // TODO: erase this comment
                    0, 0,		
                    WindowRect.right  - WindowRect.left,	
                    WindowRect.bottom - WindowRect.top,
                    NULL,							
                    NULL,						
                    gld.hInstance,	
                    NULL);

	    if (! gld.hWnd )
	    {
		    WGL_KillWindow();	
		    Sys_Error("Window Creation Error.");
		    return -3;
	    }
    }


    // setup PFD
    WGL_InitContext ( colorbits );

    WGL_InitGLSubsystem ();


	ShowWindow( gld.hWnd, SW_SHOW );
	SetForegroundWindow( gld.hWnd );
	SetFocus( gld.hWnd ) ;			

	// I just saw these, but I dont know if they do what I think that I want them to do
	// I want to get rid of the windows cursor, while the GL window has system focus, and return
	//  it when the GL window loses focus.
//	SetCapture( gld.hWnd );
//	ClipCursor( &WindowRect );
	SetCursorPos(320,240);

	return 0;	
}



/*
==================== 
 WGL_InitExtensions
==================== 
*/
static void WGL_InitExtensions( void )
{
	if ( !rstate.allowExtensions )
	{
		gld.logMsg( "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	gld.logMsg( "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc
	glConfig.textureCompression = TC_NONE;
	if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
        /*
		if ( r_ext_compressed_textures->integer )
		{
			glConfig.textureCompression = TC_S3TC;
			gld.logMsg( "...using GL_S3_s3tc\n" );
		}
		else */
		{
			glConfig.textureCompression = TC_NONE;
			gld.logMsg( "...ignoring GL_S3_s3tc\n" );
		}
	}
	else
	{
		gld.logMsg( "...GL_S3_s3tc not found\n" );
	}

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = gfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
        /*
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = gtrue;
			gld.logMsg( "...using GL_EXT_texture_env_add\n" );
		}
		else */
		{
			glConfig.textureEnvAddAvailable = gfalse;
			gld.logMsg( "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		gld.logMsg( "...GL_EXT_texture_env_add not found\n" );
	}

	// WGL_EXT_swap_control
	gwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) gwglGetProcAddress( "wglSwapIntervalEXT" );
	if ( gwglSwapIntervalEXT )
	{
		gld.logMsg( "...using WGL_EXT_swap_control\n" );
		//r_swapInterval->modified = gtrue;	// force a set next frame
	}
	else
	{
		gld.logMsg( "...WGL_EXT_swap_control not found\n" );
	}

    gwglGetSwapIntervalEXT = ( int (WINAPI *)(void) ) gwglGetProcAddress( "wglGetSwapIntervalEXT" );
    if ( gwglGetSwapIntervalEXT ) {
        gld.logMsg( "...using wglGetSwapIntervalEXT" );
    } else {
        gld.logMsg( "...NOT using wglGetSwapIntervalEXT" );
    }

	// GL_ARB_multitexture
	gglMultiTexCoord2fARB = NULL;
	gglActiveTextureARB = NULL;
	gglMultiTexCoord2f = NULL;
	gglActiveTexture = NULL;
	gglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
        
        /*
		if ( r_ext_multitexture->integer )
		{ */

			gglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) gwglGetProcAddress( "glMultiTexCoord2fARB" );
			gglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) gwglGetProcAddress( "glActiveTextureARB" );
			gglMultiTexCoord2f = ( PFNGLMULTITEXCOORD2FARBPROC ) gwglGetProcAddress( "glMultiTexCoord2f" );
			gglActiveTexture = ( PFNGLACTIVETEXTUREARBPROC ) gwglGetProcAddress( "glActiveTexture" );
			gglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) gwglGetProcAddress( "glClientActiveTextureARB" );

            /*
			if ( gglActiveTextureARB )
			{
				gglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					gld.logMsg( "...using GL_ARB_multitexture\n" );
				}
				else
				{
					gglMultiTexCoord2fARB = NULL;
					gglActiveTextureARB = NULL;
					gglClientActiveTextureARB = NULL;
					gld.logMsg( "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
            */


            /*
		}
		else 
		{
			gld.logMsg( "...ignoring GL_ARB_multitexture\n" );
		}
        */
	}
	else
	{
		gld.logMsg( "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	gglLockArraysEXT = NULL;
	gglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) && ( glConfig.hardwareType != GLHW_RIVA128 ) )
	{
        /*
		if ( r_ext_compiled_vertex_array->integer )
		{
            */



			gld.logMsg( "...using GL_EXT_compiled_vertex_array\n" );
			gglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) gwglGetProcAddress( "glLockArraysEXT" );
			gglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) gwglGetProcAddress( "glUnlockArraysEXT" );
			if (!gglLockArraysEXT || !gglUnlockArraysEXT) {
				gld.logMsg("bad getprocaddress");
			}


            /*
		}
		else
		{
			gld.logMsg( "...ignoring GL_EXT_compiled_vertex_array\n" );
		} */


	}
	else
	{
		gld.logMsg( "...GL_EXT_compiled_vertex_array not found\n" );
	}

	// WGL_3DFX_gamma_control
	gwglGetDeviceGammaRamp3DFX = NULL;
	gwglSetDeviceGammaRamp3DFX = NULL;

	if ( strstr( glConfig.extensions_string, "WGL_3DFX_gamma_control" ) )
	{
        /*
		if ( !r_ignorehwgamma->integer && r_ext_gamma_control->integer )
		{
			gwglGetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) ) gwglGetProcAddress( "wglGetDeviceGammaRamp3DFX" );
			gwglSetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) ) gwglGetProcAddress( "wglSetDeviceGammaRamp3DFX" );

			if ( gwglGetDeviceGammaRamp3DFX && gwglSetDeviceGammaRamp3DFX )
			{
				gld.logMsg( "...using WGL_3DFX_gamma_control\n" );
			}
			else
			{
				gwglGetDeviceGammaRamp3DFX = NULL;
				gwglSetDeviceGammaRamp3DFX = NULL;
			}
		}
		else
		{
			gld.logMsg( "...ignoring WGL_3DFX_gamma_control\n" );
		}
        */
	}
	else
	{
		gld.logMsg( "...WGL_3DFX_gamma_control not found\n" );
	}
}

//extern int VSYNC_state;
extern gvar_c * r_vsync;

int VSyncEnabled( void )
{
    return gwglGetSwapIntervalEXT() > 0;
}
 
/*
void Set_VSYNC( bool enable )
{
    if ( enable )
       gwglSwapIntervalEXT( 1 ); //set interval to 1 -> enable
    else
       gwglSwapIntervalEXT( 0 ); //disable

    r_vsync->set( "r_vsync", enable ? "1" : "0" ) ;

	int e = (int)enable;

	if ( VSyncEnabled() != e ) {
		Sys_Error( "Failed to set VSYNC" );
	}
}
*/


void WGL_Init ( void )
{
    int code = -1;

	rstate.start();

//    if ( ! GGL_Init( W("nvoglnt") ) )
    if ( ! GGL_Init( W("opengl32") ) )
        Sys_Error("couldn't load opengl driver");

    gld.WndProc = Win_GetWndProc();

    code = WGL_CreateWindow( gld.window_title, gld.width, gld.height, 
                            COLORBITS, gld.fullscreen );

    if ( code < 0 )
        Sys_Error ( "unable to create GL Window, code %d", code );


	C_strncpy ( glConfig.vendor_string, (const char *) gglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	C_strncpy ( glConfig.renderer_string, (const char *) gglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	C_strncpy ( glConfig.version_string, (const char *) gglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	C_strncpy ( glConfig.extensions_string, (const char *) gglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );


    // chipset specific configuration
    

    // second stage of wgl function pointer inits...
    WGL_InitExtensions();

    
    // check hardware gamma
    
    // set VSYNC
    //Set_VSYNC( 1 );   
}


