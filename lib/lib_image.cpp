/*
==================== 
image.c -  old image loading c code from pacman.  pretty archaic but works.
            bitmap and targa only.
==================== 
*/

#include <string.h>
#ifndef snprintf
#define snprintf _snprintf
#endif

#include "lib_image.h"

#include "../client/cl_console.h"

#include "../renderer/r_ggl.h"

#include "lib.h"

textureMode_t modes[] = {
	{ "GL_NEAREST", GL_NEAREST, GL_NEAREST, "Far Filtering off.  Mipmapping off" },
	{ "GL_LINEAR", GL_LINEAR, GL_LINEAR, "Bilinear filtering, no mipmap" },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, "Far Filtering off, standard mipmap" },
	{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, "Bilinear filtering with standard mipmap" },
	{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST, "Far Filtering off.  Mipmaps use Trilinear" },
	{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, "Bilinear Filtering.  Mipmaps use Trilinear" },
};

gbool IMG_getBmpInfo (image_t *img, bmp_info_t *info)
{
    img->w = info->biWidth;
    img->h = info->biHeight;
    img->bpp = info->biBitCount / 8;

    // check for stuff we dont want
    if (img->bpp != 3 && img->bpp != 4) {
        Com_Printf("bitmap only supports 24 & 32-bit formats! : %d\n", img->bpp);
        return gfalse;
    }
    
    return gtrue;
}


image_t *IMG_readfileBMP ( const char *fullpath, image_t *img )
{
    FILE *fp;
    int i, bytesread;
    int datasize;
    int infosize;
    byte buf[128];
	bmp_header_t *h;
    bmp_info_t *info;
    register byte temp;

    // open
    if ((fp = fopen(fullpath, "rb")) == NULL) {
        Com_Printf("cant read %s \n", fullpath);
        return NULL;
    }

    // read header straight in
    if (fread(buf, 14, 1, fp) == NULL) {
        Com_Printf("error reading header\n");
        return NULL;
    }

	h = &img->header.bmp;
	h->magic = (unsigned short) *(unsigned short *)buf;
	h->totalBytes = (unsigned int) *(unsigned int *)&buf[2];
	h->reserved1 = h->reserved2 = 0;
	h->dataOffsetBytes = (unsigned int) *(unsigned int *)&buf[10];

    // check magic number
    if (img->header.bmp.magic != 19778) {
        Com_Printf("Not a Bitmap File\n");
        return NULL;
    }

    // read info portion of header
    memset( buf , 0, sizeof(buf) );
    //infosize = img->header.bmp.dataOffsetBytes - sizeof(bmp_header_t);
    infosize = img->header.bmp.dataOffsetBytes - 14;

    info = (bmp_info_t *) buf;

    if (fread(info, infosize, 1, fp) == NULL) {
        Com_Printf("error reading bitmap info from file\n");
        return NULL;
    } 

    // record info
    if (! IMG_getBmpInfo(img, info) ) {
        return NULL;
    }
    
    // malloc data portion
	
	//   2 bytes padding (0x00, 0x00) on the end of bmp
    datasize = img->header.bmp.totalBytes - img->header.bmp.dataOffsetBytes;
    img->data = (byte *) V_Malloc (datasize);
    memset (img->data, 0, datasize);
    img->numbytes = datasize;
	img->type = IMG_BMP;

    // get data
    fseek ( fp, img->header.bmp.dataOffsetBytes, SEEK_SET );

	/*
    do {
        i = fread(img->data, 1, 1024, fp);
        bytesread += i;
        if (i <= 0)
            break;
    } while(1);
	*/

	// read image data
    bytesread = 0;
	while ((i = fread(&img->data[bytesread], 1, 8192, fp)) > 0)
		bytesread += i;

    if ( bytesread != datasize ) {
        Com_Printf("bitmap: incorrect datasize: %d\n", bytesread);
        return NULL;
    }

    return img;
}





void IMG_MakeGLTexture (image_t *img)
{
	GLenum err = GL_NO_ERROR;

	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "***GL error: %s\n", gluErrorString( err ) );
	}
	err = GL_NO_ERROR;

    gglPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "***GL error: %s\n", gluErrorString( err ) );
	}
	err = GL_NO_ERROR;

    gglGenTextures ( 1, &img->texhandle );

	int BIG_STUPID_DEBUG_INT = img->texhandle;

	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "***GL error: %s\n", gluErrorString( err ) );
	}
	err = GL_NO_ERROR;

    gglBindTexture( GL_TEXTURE_2D, img->texhandle );

	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "***GL error: %s\n", gluErrorString( err ) );
	}
	err = GL_NO_ERROR;

/*	// vanilla bilinear filtering, no mipmap
	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); */

	// gvar specified filtering
	gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, modes[tex_mode->integer()].minimize );
    gglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, modes[tex_mode->integer()].maximize );
	
	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "GL error: %s \"%s\"\n", gluErrorString( err ), img->name );
	}
	
	if ( img->bpp == 1 ) {
		gglTexImage2D ( GL_TEXTURE_2D, 0, GL_ALPHA, img->w, img->h, 
		              0, GL_ALPHA, GL_UNSIGNED_BYTE, img->compiled );
	} else {
		gglTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 
		              0, GL_RGBA, GL_UNSIGNED_BYTE, img->compiled );
	}

	err = gglGetError();
	if (err != GL_NO_ERROR) {
		Com_Printf( "gl error: %s \n", gluErrorString(err) );
		console.Printf( "GL error: %s \"%s\"\n", gluErrorString( err ), img->name );
	}
}


void IMG_copyHeaderTGA (tga_header_t *tga, byte *buf_p)
{
    tga->indentsize     = *buf_p++;
    tga->colormaptype   = *buf_p++;
    tga->imagetype      = *buf_p++;
    tga->colormapstart  = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->colormaplength = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->colormapbits   = *buf_p++;
    tga->xstart         = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->ystart         = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->width          = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->height         = (unsigned short) *(short *)buf_p; buf_p += 2;
    tga->pixel_size     = *buf_p++;
    tga->descriptor     = *buf_p;
}


image_t * IMG_readfileTGA (const char *fullpath, image_t *img)
{
    //FILE *fp;
    byte tga_header_buf[18];
    int sz;
 //   int i,j;
    filehandle_t fh;

    if ( ! FS_FOpenReadOnly( fullpath, &fh ) ) 
        Com_Error ( ERR_FATAL, "couldn't open file: %s\n", fullpath);

    if ( ! FS_Read( (void *)tga_header_buf, 18, fh ) )
        Com_Error ( ERR_FATAL, "couldn't read file: %s\n", fullpath);

    IMG_copyHeaderTGA (&img->header.tga, tga_header_buf);

	sz = FS_GetFileSize( fh );
	if ( sz < 0 )
		Com_Error( ERR_FATAL, "returned bad size, file not found?\n" );

	img->data = (byte *)V_Malloc( sz - 18 );

	img->numbytes = FS_ReadAll( img->data, fh );
    if ( !img->numbytes )
        Com_Error ( ERR_FATAL, "error reading rest of file: %s\n", fullpath);
	if ( img->numbytes != sz-18 )
		Com_Error ( ERR_WARNING, "sizes don't match %i %i\n", img->numbytes, sz );

    img->type = IMG_TGA;
    
    img->h = img->header.tga.height;
    img->w = img->header.tga.width;
    img->bpp = img->header.tga.pixel_size / 8;

	FS_FClose( fh );

	return img;
}

/*
==================== 
  IMG_compileGLImage()
==================== 
*/
int IMG_compileGLImage (image_t *img)
{
    int i, j, rgba_sz;

    byte *compiled;

	if ( img->bpp != 1 && img->bpp != 3 && img->bpp != 4 ) {
        //Com_Error ( ERR_FATAL, "only handling 24 or 32-bit right now");
        Com_Printf( "%s", "only handling 8, 24 or 32-bit right now" );
		return -1;
	}

	if ( img->bpp == 1 ) {
		rgba_sz = img->numbytes;
	} else if ( img->bpp == 3 ) {
	    rgba_sz = img->numbytes * 4;
	    rgba_sz /= 3;
    } else {
        rgba_sz = img->numbytes;
    }

    rgba_sz += 16; // pad

    compiled = (byte *) V_Malloc ( rgba_sz );
    memset (compiled, 0, rgba_sz);

    if ( img->bpp == 1 ) {
		for ( i = 0; i < (int)img->numbytes; i++ ) {
			compiled[ i ] = img->data[ i ];
		}
	} else if (img->bpp == 3) {
        for (i = 0, j = 0; i < (int)img->numbytes; i += img->bpp, j+=4)
        {
            if (img->data[i] || img->data[i+1] || img->data[i+2]) {
                compiled[j+2] = img->data[i+0];
                compiled[j+1] = img->data[i+1];
                compiled[j+0] = img->data[i+2];
                compiled[j+3] = 255;
            }
        }
    } else {
        for (i = 0, j = 0; i < (int)img->numbytes; i += img->bpp, j+=4)
        {
            compiled[j+2] = img->data[i+0];
            compiled[j+1] = img->data[i+1];
            compiled[j+0] = img->data[i+2];
            compiled[j+3] = img->data[i+3];
        }
    }
    img->compiled = compiled;

	return 0;
}

static const char *IMG_Error( int typenum ) {
    switch (typenum) {
    case IMG_NONE:      return "IMG_NONE";
    case IMG_BMP:       return "IMG_BMP";
    case IMG_TGA:       return "IMG_TGA";
    case IMG_GIF:       return "IMG_TGA";
    case IMG_JPG:       return "IMG_JPG";
    case IMG_PCX:       return "IMG_PCX";
    case IMG_PPM:       return "IMG_PPM";
    case IMG_PNG:       return "IMG_PNG";
    case IMG_OPENEXR:   return "IMG_OPENEXR";
    }
    return "UNKNOWN";
}

void IMG_compileImageFile ( const char *path, image_t **ip )
{
    int type;
    int i;

    *ip = (image_t *) V_Malloc( sizeof(image_t) );

    // get type from filename extension
    for (i = 0; path[i] != '\0'; i++)
        ;

    do {
        --i;
    } while ( path[i] != '.' );
    ++i;


    type = IMG_NONE;
    if ( !C_strncasecmp( &path[i], "BMP", 3 ) )
        type = IMG_BMP;
    else if ( !C_strncasecmp( &path[i], "TGA", 3 ) )
        type = IMG_TGA;


    // 
    switch(type) {
    case IMG_TGA:
        IMG_readfileTGA( path, *ip ); 
        break;
    case IMG_BMP:
        IMG_readfileBMP( path, *ip );
        break;
    case IMG_GIF:
    case IMG_JPG:
    case IMG_PCX:
    case IMG_PPM:
    case IMG_PNG:
    default:
        Com_Printf( "image type: %s not supported\n", IMG_Error(type) );
        V_Free(*ip);
        (*ip)=NULL;
        return;
    }

	(*ip)->name[0] = 0;
	(*ip)->syspath[0] = 0;

	// get basename
	strcpy( (*ip)->name, strip_path( &path[0] ) );

	// get relative name
	strcpy( (*ip)->syspath, strip_gamepath( &path[0] ) );

	int err_cond = 0;
    if ( IMG_compileGLImage( *ip ) )
		err_cond = 1;

    V_Free( (*ip)->data );
    (*ip)->data = NULL;

	if ( !err_cond ) {
		IMG_MakeGLTexture( *ip );

		V_Free( (*ip)->compiled );
		(*ip)->compiled = NULL;
	}
}

// tex and mask should already be created.  we'll re-read in each of their
//  file's data, and then create another image and GL compile it, storing
//  all of the new info to ip.  Only supports textures that have an equal
//  width and height value, and that value must be a powerof 2.
int IMG_CombineTextureMaskPow2( image_t *tex, image_t *mask, image_t **ipp ) {
    int i;
	
	if ( !( tex && mask && ipp ) )
		return -1;
	if ( tex->h != tex->w || mask->h != mask->w )
		return -2;
	if ( !ISPOWEROF2( tex->h ) )
		return -3;

	// alloc the image
    image_t *ip = (image_t *) V_Malloc( sizeof(image_t) );
	image_t *m_ip = (image_t*) V_Malloc( sizeof(image_t) );
	memset( ip, 0, sizeof(image_t) );
	memset( m_ip, 0, sizeof(image_t) );

	char path[ 512 ];
	snprintf( path, 512, "%s\\%s", fs_gamepath->string(), tex->syspath );

    // read the TEX file 
    switch ( tex->type ) {
    case IMG_TGA:
        IMG_readfileTGA( path, ip ); 
        break;
    case IMG_BMP:
        IMG_readfileBMP( path, ip );
        break;
    case IMG_GIF:
    case IMG_JPG:
    case IMG_PCX:
    case IMG_PPM:
    case IMG_PNG:
    default:
        Com_Printf( "image type: %s not supported\n", IMG_Error(tex->type) );
        V_Free(ip);
        V_Free(m_ip);
        return -6;
    }

	// save path info of the texture image
	ip->name[0] = 0;
	ip->syspath[0] = 0;
	strcpy( ip->name, strip_path( &path[0] ) );
	strcpy( ip->syspath, strip_gamepath( &path[0] ) );

	// read in the MASK file
	snprintf( path, 512, "%s\\%s", fs_gamepath->string(), mask->syspath );
    switch ( mask->type ) {
    case IMG_TGA:
        IMG_readfileTGA( path, m_ip ); 
        break;
    case IMG_BMP:
        IMG_readfileBMP( path, m_ip );
        break;
    case IMG_GIF:
    case IMG_JPG:
    case IMG_PCX:
    case IMG_PPM:
    case IMG_PNG:
    default:
        Com_Printf( "image type: %s not supported\n", IMG_Error(mask->type) );
		if ( ip->data )
			V_Free( ip->data );
        V_Free(ip);
        V_Free(m_ip);
        return -7;
    }

	int err_cond = 0;

	// allocates compiled array & formats data into it
    if ( IMG_compileGLImage( ip ) )
		err_cond = 1;
    if ( IMG_compileGLImage( m_ip ) )
		err_cond = 1;

	byte *rsmp = NULL;

	if ( !err_cond ) {

		int scale = ip->w / m_ip->w;	// difference between scales

		// single channel greyscale mask
		int r_sz = ip->h * ip->w; // 1 byte for each pixel
		rsmp = (byte *) V_Malloc( ip->h * ip->w );
		memset( rsmp, 0, r_sz );

		// convert mask from it's resident format to be the same dimension
		//  as the texture image
		int h, i, j, k, rsmp_i, rsmp_j, pix;

		/// foreach Row in Mask

		// for each row in m_ip ==> j
		for ( j = 0; j < m_ip->h; j++ ) {

			/// foreach col in Mask

			// for each col in m_ip ==> i
			for ( i = 0; i < m_ip->w; i++ ) {

				/// extrapolate Mask Pixel to Cover all of it's corresponding pixels
				///  in the new re-sampled mask

				// ROWS: ( j * scale ) to ( j * scale + scale ) 
				for ( k = 0; k < scale; k++ ) {

					rsmp_j = j * scale + k;

					// COLS: set i to i + scale pixels to value of m_ip->compiled[ i ]
					for ( h = 0; h < scale; h++ ) {

						rsmp_i = i * scale + h;

						// index into new mask
						pix = rsmp_j * ip->w + rsmp_i;

						if ( pix < r_sz ) {
							rsmp[ pix ] = m_ip->compiled[ j * m_ip->w + i ];
						}
					}
				}
			}
		}

		if ( ip->bpp == 1 ) {
			for ( i = 0; i < (int)ip->numbytes; i += 1 ) {
				if ( rsmp[ i ] == 0 ) {
					ip->compiled[ i + 0 ] = 0;
				}
			}
		} else if ( 0 ) {
			// set to zero any texels that didn't pass
			for ( i = 0; i < (int)r_sz; i++ ) {
				if ( rsmp[ i ] == 0 ) {
					ip->compiled[ i * 4 + 0 ] = 0;
					ip->compiled[ i * 4 + 1 ] = 0;
					ip->compiled[ i * 4 + 2 ] = 0;
					ip->compiled[ i * 4 + 3 ] = 255;
				}
			}
		} // if bpp == 4
		
		IMG_MakeGLTexture( ip );
	}

	if ( ip->data ) {
    	V_Free( ip->data );
    	ip->data = NULL;
	}
	if ( ip->compiled ) {
		V_Free( ip->compiled );
		ip->compiled = NULL;
	}
	if ( m_ip->data ) {
    	V_Free( m_ip->data );
    	m_ip->data = NULL;
	}
	if ( m_ip->compiled ) {
		V_Free( m_ip->compiled );
		m_ip->compiled = NULL;
	}
	V_Free( m_ip );
	if ( rsmp )
		V_Free( rsmp );

	*ipp = NULL;
	if ( err_cond )
		return -4;

	*ipp = ip;
	return 0;
}

