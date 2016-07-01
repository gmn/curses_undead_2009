
#include "../win32/win_public.h"
#include "../mapedit/mapedit.h"

static void InitOpenGL ( void ) 
{

    WGL_Init();


    //GLimp_Init(); // in id's win_glimp.c
//           -> GLW_StartOpenGL();
//              -> GLW_LoadOpenGL("opengl32"); // attempts to load dll
//                  -> QGL_Init("opengl32")
//                      gld->hinstOpenGL = LoadLibrary ( dllname )
//                      // load a ton of func_pointers here
//                      qglAccum = dllAccum = GPA( "glAccum" );
//                  -> GLW_StartDriverAndSetMode
//                      -> GLW_SetMode
//                          -> R_GetModeInfo
//                          // check desktop attributes *** Useful code ***
//                          -> hDC = GetDC( GetDesktopWindow() );




    //qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );

    // R_InitCommandBuffers();

    //GfxInfo_f(); // print info
    
    // GL_SetDefaultState();
}



void R_Init ( void ) {

    // init function tables
    /*

#define DEG2RAD(a) (((a)*M_PI)/180.0f)

    for ( i = 0; i < FUNCTABLE_SIZE; i++ ) {     // 1024
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];
                 
    } 
    */


    
    //R_Register(); means set the proponderance of cvars


    
    InitOpenGL();
}
