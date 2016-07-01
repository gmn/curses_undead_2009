#ifndef __GAME_H__
#define __GAME_H__

#include "g_loader.h"


extern Game_t MainGame;

void GL_Demo_Init( int, int );

void GL_Demo_Frame( void );

void G_InitPlayer( void );

void G_InitGame( void ); // sets up thinker states

#endif // __GAME_H__
