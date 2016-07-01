//////////////////////////////////////////////////////////////////
// gvar class def
//


#include "common.h"

#define _CHECH_TAG ( _hasBeenInit == INIT_TAG )


#define _CHECK_TAG_INIT \
            do {    \
                if ( _hasBeenInit != INIT_TAG ) { \
                    init(); \
                }   \
            } while(0)


#define _GET_AS(m) \
            do {    \
                if ( !m ) { \
                    m = ( autostring * ) getmem(sizeof(autostring)); \
                }   \
            } while(0)


gbool gvar_c::init( void ) {

    if ( _hasBeenInit == INIT_TAG ) {
        // warn ?
        return gfalse;
    }

    _name = NULL;
    _desc = NULL;
    _sval = NULL;
    _fval = -1.1;
    _ival = -1;
    _bval = gfalse;
    _voidp = NULL;
    _numPointers = 0;
    // gtrue, when some value stored to gvar
    _hasBeenSet = gfalse;
    _resetString = NULL;
    _latchedString = NULL;
    _flags = GVF_ALLOWALL;
    _modified  = gfalse;
    _modificationCount = 0;

    /// FIXME: remove these and add linked list class objects
    _next = NULL;;
    _hashNext = NULL;

    // so that different memory handlers can be used later
    getmem = V_Malloc;
    freemem = V_Free;

    _hasBeenInit = INIT_TAG;
    return gtrue;
}

void gvar_c::shutdown( void ) {
    freeAutoStringP(_name);
    freeAutoStringP(_desc);
    freeAutoStringP(_sval);
    freeAutoStringP(_resetString);
    freeAutoStringP(_latchedString);
}




void gvar_c::setDescriptor( const char *name, const char *desc, const char *reset )
{
    if ( name ) {
        _GET_AS( _name );
        _name->init( name );
    }
    if ( desc ) {
        _GET_AS( _desc );
        _desc->init( desc );
    }
    if ( reset ) {
        _GET_AS( _resetString );
        _resetString->init ( reset );
    }
}


// set
// the critical assumption is that init() has been called already
// before calling set, init may call set
void gvar_c::set(   const char *value, 
                    const char *name, 
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _GET_AS( _sval );
    _sval->init( value );

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_STRING;
}

void gvar_c::set(   const gwchar_t *value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _GET_AS( _sval );
    _sval->init( value );

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_STRING;
}

void gvar_c::set(   const int value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _ival = value;

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_INT;
}

void gvar_c::set(   const float value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _fval = value;

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_FLOAT;
}

void gvar_c::set(   const double value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    set( (float) value, name, desc, reset );
}

void gvar_c::set(   const gbool value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _bval = value;

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_BOOL;
}

void gvar_c::set(   const void *value,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _voidp = (void *) value;

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_POINTER;
}

void gvar_c::set(   const float val, const float min, const float max,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    _CHECK_TAG_INIT;

    _fval   = val;
    _fmin   = min;
    _fmax   = max;

    setDescriptor( name, desc, reset );

    _hasBeenSet = gtrue;
    _flags |= GVT_FLOAT;
}

void gvar_c::setName ( const char *name ) {
    _CHECK_TAG_INIT;

    _modified = gtrue;
    _modificationCount++;
    _GET_AS( _name );
    _name->set( name );
}

void gvar_c::setDesc ( const char *desc ) {
    _CHECK_TAG_INIT;

    _modified = gtrue;
    _modificationCount++;
    _GET_AS( _desc );
    _desc->set( desc );
}

void gvar_c::setResetString ( const char *str ) {
    _CHECK_TAG_INIT;

    _GET_AS( _resetString );
    _resetString->set( str );
}

void gvar_c::setFlag( int f ) {
    _CHECK_TAG_INIT;

    _flags |= f;
}

/* char, gwchar, int, float, double, gbool, void * */
void gvar_c::init(  const char * val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const gwchar_t * val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const int val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const float val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const double val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const gbool val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const void * val,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, name, desc, reset );
}
void gvar_c::init(  const float val, const float min, const float max,
                    const char *name,
                    const char *desc, 
                    const char *reset ) 
{
    if (!init())
        return;
    set( val, min, max, name, desc, reset );
}

// related operators
//void operator= ( gvar_c& g, int j ) { g.set(j); }


