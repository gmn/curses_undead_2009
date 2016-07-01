
#include <math.h>
#include "lib.h"

#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif

#define DEG_TO_RAD	(M_PI/180.0f)
#define RAD_TO_DEG 	(180.0/M_PI)

/* matrix storage method is (different from OpenGL which is column major)
	m1	m2	m3	m4
	m5	m6	m7	m8
	m9	m10	m11	m12
	m13	m14	m15	m16
*/

// does a O = AB
void M_MatMult16( float *O, float *A, float *B ) {
}

void M_MultVec4( float *O, float *M, float *V ) {
	for ( unsigned int j = 0; j < 4; j++ ) {
		O[j] = 0.f;
		for ( unsigned int i = 0; i < 4; i++ ) {
			O[j] += M[j*4+i] * V[i];
		}
	}
}

// do a 2D translation in a 4x4 mat op
void M_Rotate2d( float *point, float angle ) {
	float m[16];
	m[0] = cosf( angle * DEG_TO_RAD );
	m[1] = sinf( angle * DEG_TO_RAD );
	m[2] = 0.f;
	m[3] = 0.f;
	m[4] = -1.f * sinf( angle * DEG_TO_RAD );
	m[5] = cosf( angle * DEG_TO_RAD );
	m[6] = 0.f;
	m[7] = 0.f;
	m[8] = 0.f;
	m[9] = 0.f;
	m[10] = 0.f;
	m[11] = 1.f;
	m[12] = 0.f;
	m[13] = 0.f;
	m[14] = 0.f;
	m[15] = 1.f;

	float v[4];
	v[0] = point[0];
	v[1] = point[1];
	v[2] = 0.f;
	v[3] = 1.f;

	float r[4];
	M_MultVec4( r, m, v );

	point[0] = r[0];
	point[1] = r[1];
}

// do a 2D translation _with_ a 4x4 matrix
void M_Translate2d( float *point, float *t ) {
	float m[16];
	m[0] = 1.f;
	m[1] = 0.f;
	m[2] = 0.f;
	m[3] = t[0];
	m[4] = 0.f;
	m[5] = 1.f;
	m[6] = 0.f;
	m[7] = t[1];
	m[8] = 0.f;
	m[9] = 0.f;
	m[10] = 1.f;
	m[11] = 0.f;
	m[12] = 0.f;
	m[13] = 0.f;
	m[14] = 0.f;
	m[15] = 1.f;

	float v[4];
	v[0] = point[0];
	v[1] = point[1];
	v[2] = 0.f;
	v[3] = 1.f;

	float t2[4];
	M_MultVec4( t2, m, v );

	point[0] = t2[0];
	point[1] = t2[1];
}

// 
void M_TranslateRotate2d( float *point, float *dist, float rot ) {
//	if ( rot <= 0.01f )
//		return;
	float x[2];
	x[0] = -dist[0];
	x[1] = -dist[1];
	M_Translate2d( point, x );
	M_Rotate2d( point, rot );
	M_Translate2d( point, dist );
}


// input vec will be 2d
void M_CrossProd3d( float *a, float *b, float *c ) {
	a[0] = b[1] * c[2] - b[2] * c[1];
	a[1] = b[2] * c[0] - b[0] * c[2];
	a[2] = b[0] * c[1] - b[1] * c[0];
}

void M_TranslateRotate2DVertSet( float *point, float *dist, float rot ) {
	float x[2];
	x[0] = -dist[0]; 
	x[1] = -dist[1];
	M_Translate2d( &point[0], x );
	M_Translate2d( &point[2], x );
	M_Translate2d( &point[4], x );
	M_Translate2d( &point[6], x );

	M_Rotate2d( &point[0], rot );
	M_Rotate2d( &point[2], rot );
	M_Rotate2d( &point[4], rot );
	M_Rotate2d( &point[6], rot );

	M_Translate2d( &point[0], dist );
	M_Translate2d( &point[2], dist );
	M_Translate2d( &point[4], dist );
	M_Translate2d( &point[6], dist );
}

float M_GetAngleDegrees( float *r1, float *r2 ) {
	// dot product for angle between 2 clicks
	float dot = M_DotProduct2d( r1, r2 );
	
	// get magnitudes for scale
	float a = M_Magnitude2d( r1 );
	float b = M_Magnitude2d( r2 );
		
	// actual rotation 
	return acosf( dot / (a * b) ) * RAD_TO_DEG;
}

float M_GetAngleRadians( float *r1, float *r2 ) {		
	// dot product for angle between 2 clicks
	float dot = M_DotProduct2d( r1, r2 );
	
	// get magnitudes for scale
	float a = M_Magnitude2d( r1 );
	float b = M_Magnitude2d( r2 );
		
	// actual rotation 
	return acosf( dot / (a * b) );
}

void M_ClampAngle360( float *A ) {
	while ( *A < 0.0f ) {
		*A += 360.0f;
	}
	while ( *A >= 360.0f ) {
		*A -= 360.0f;
	}
}
