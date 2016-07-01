
/******************************************************************************
 *  com_gvar.cpp - a console variable system modeled after id's, q3 era
 ******************************************************************************/

//#include "common.h"
#include "com_gvar.h"
#include "../client/cl_console.h"
#include "../lib/lib.h"


static gvar_c   *gvarPool = NULL;
static gvar_c	*gvarHash[GV_HASH_SIZE];
static int      gvarInUse = 0;
#define GV_HASH_MASK (GV_HASH_SIZE-1)
static int gvarPoolBytes = -1;

/*
==================== 
 gvar_c::set()

 a subsequent call to set will change the string but only change the 
    resetstring if GV_AUTHORITATIVE
==================== 
*/
//void gvar_c::set( const char *name, const char *value =NULL, uint flags =0 ) 
void gvar_c::set( const char *name, const char *value , uint flags ) 
{
    if ( !name || !name[0] ) {
        return;
    }

    // can't be set
    if ( _flags & GV_ROM )
        return;

    C_strncpy( _name, name, GV_STRING_SIZE );

    _flags |= flags;

    // trim any quotes
    const char *p = value;
    while ( p && *p && *p == ' ' ) ++p;
    if ( *p == '"' ) {
        char * e = (char*) p;
        while ( *e ) ++e;
        while ( e != p && *e != '"' ) --e;
        if ( *e == '"' ) *e = '\0';
        value = ++p;
    }

    if ( !value ) // can set to empty string
        return;

    if ( _mcount > 0 ) {
        if ( _string[0] ) 
            C_strncpy( _oldstring, _string, GV_STRING_SIZE );
        if ( _flags & GV_AUTHORITATIVE ) 
            C_strncpy( _resetString, value, GV_STRING_SIZE );
        _modified = gtrue;
    } 
    else /* first time */ 
    {
        C_strncpy( _resetString, value, GV_STRING_SIZE );
    }

	_string[0] = 0;
    C_strncpy( _string, value, GV_STRING_SIZE );
    ++_mcount;                  // (mcount == 1) on initial call to set()
    _value = atof( _string );
    _integer = atoi( _string );
}

// just change the value by a float argument
void gvar_c::setValue( float _fval ) {
    // can't be set
    if ( _flags & GV_ROM )
        return;

    if ( _mcount > 0 ) {
        if ( _string[0] ) 
            C_strncpy( _oldstring, _string, GV_STRING_SIZE );
        if ( _flags & GV_AUTHORITATIVE ) 
			sprintf( _resetString, "%f", _fval );
//            C_strncpy( _resetString, _fval, GV_STRING_SIZE );
        _modified = gtrue;
    } 
    else /* first time */ 
    {
		sprintf( _resetString, "%f", _fval );
    }

	_string[0] = 0;
	sprintf( _string, "%f", _fval );
	_string[GV_STRING_SIZE-1] = '\0';
    ++_mcount;                  
    _value = _fval;
    _integer = (int)_fval;
}

void gvar_c::setString( const char *p_str ) 
{
    if ( _flags & GV_ROM )
        return;

    // look for quotes
    const char *p = p_str;
    const char *str = p_str;
    while ( p && *p && *p == ' ' ) ++p;
    if ( *p == '"' ) {
        char * e = (char*) p;
        while ( *e ) ++e;
        while ( e != p && *e != '"' ) --e;
        if ( *e == '"' ) *e = '\0';
        str = ++p;
    }

    if ( _mcount > 0 ) {
        if ( _string[0] ) 
            C_strncpy( _oldstring, _string, GV_STRING_SIZE );
        if ( _flags & GV_AUTHORITATIVE ) 
            C_strncpy( _resetString, str, GV_STRING_SIZE );
        _modified = gtrue;
    } 
    else /* first time */ 
    {
        C_strncpy( _resetString, str, GV_STRING_SIZE );
    }

	_string[0] = 0;
    C_strncpy( _string, str, GV_STRING_SIZE );
    ++_mcount;                  // (mcount == 1) on initial call to set()
    _value = atof( _string );
    _integer = atoi( _string );
}

void gvar_c::setInt( int ival ) {
    if ( _flags & GV_ROM )
        return;

    if ( _mcount > 0 ) {
        if ( _string[0] ) 
            C_strncpy( _oldstring, _string, GV_STRING_SIZE );
        if ( _flags & GV_AUTHORITATIVE ) 
			sprintf( _resetString, "%i", ival );
        _modified = gtrue;
    } 
    else /* first time */ 
    {
		sprintf( _resetString, "%i", ival );
    }
	_string[0] = 0;
	sprintf( _string, "%i", ival );
	_string[GV_STRING_SIZE-1] = '\0';
    ++_mcount;                  
    _value = (float)ival;
    _integer = ival;
}

static void Gvar_HashRemove( gvar_c * );

/*
==================== 
 gvar_c::reset
==================== 
*/
void gvar_c::reset( void )
{
    if ( this->inuse ) {
        Gvar_HashRemove( this );
    }

	this->_comment[0] = 0;
	this->_flags = 0;
	this->_integer = 0;
	this->_mcount = 0;
	this->_modified = gfalse;
	this->_name[0] = 0;
	this->_oldstring[0] = 0;
	this->_resetString[0] = 0;
	this->_string[0] = 0;
	this->_value = 0.0f;

 //   C_memset( this, 0, sizeof(this) );
}

void gvar_c::setComment( const char *str ) {
	strncpy( _comment, str, GV_STRING_SIZE );
}


/*
==================== 
 Gvar_HashName
==================== 
*/
static long Gvar_HashName( const char *name ) { 
    int i = 0;
    long hash = 0;
    char a;
    
    while ( name[i] != '\0' ) 
    {
        a = tolower( name[i] ); 
        hash += (long) a * (i++ + 119);
    }
    hash &= GV_HASH_MASK;
    return hash;
}

/*
==================== 
 Gvar_HashInsert
==================== 
*/
static void Gvar_HashInsert( gvar_c *g ) 
{
    gvar_c *slot;
    if ( !g )
        return;

	if ( !g->checkName() )
		return;

    // if zero, get again to make sure
    if ( !g->hash ) {
        g->hash = Gvar_HashName( g->name() );
    }

    g->next = NULL;

    if ( !gvarHash[g->hash] )
    {
        gvarHash[g->hash] = g;
        return;
    }

    slot = gvarHash[g->hash];

    while( slot->next ) {
        slot = slot->next;
    }

    slot->next = g;
}

/*
==================== 
 Gvar_HashRemove
==================== 
*/
static void Gvar_HashRemove( gvar_c *g )
{
    gvar_c *slot, *nslot;
    char name[GV_STRING_SIZE];

    if ( !g )
        return;
    if ( !g->inuse )
        return;

    C_strncpy( name, g->name(), GV_STRING_SIZE );

    if ( !name || !name[0] ) {
        return;
    } 
    if ( !g->hash ) {
        g->hash = Gvar_HashName( g->name() );
    }

    // none
    if ( !gvarHash[g->hash] )
        return;

    slot = gvarHash[g->hash];

    // only one
    if ( !slot->next ) 
    {
        if ( C_strncmp( name, slot->name(), GV_STRING_SIZE ) )
            return;

        gvarHash[g->hash] = NULL;
        return; 
    }

    // more
    nslot = slot->next;
    while ( nslot ) 
    {
        if ( !C_strncmp( name, nslot->name(), GV_STRING_SIZE ) ) {
            slot->next = nslot->next;
            return;
        }

        slot = nslot;
        nslot = nslot->next;
    }
}

/*
==================== 
 Gvar_Find
==================== 
*/
gvar_c *Gvar_Find( const char *name ) 
{
    gvar_c *s;
    uint hash;

    if ( !name || !name[0] )
        return NULL;

    hash = Gvar_HashName( name );

    s = gvarHash[hash];
    while ( s ) 
    { 
       if ( ! C_strncmp( name, s->name(), GV_STRING_SIZE ) ) 
       {
           return s;
       }
       s = s->next;
    }

    return NULL;
}

/*
==================== 
 Gvar_ConPrint
==================== 
*/
void Gvar_ConPrint( void  ) {
	console.pushMsg( "" );
	console.pushMsg( "listing Gvars" );
	console.pushMsg( "----------------------------------" );

	list_c<gvar_c*> tmplist;
	tmplist.init();

	for ( int i = 0; i < GV_HASH_SIZE; i++ ) {
		gvar_c *s = gvarHash[ i ];
		while ( s ) {
			tmplist.add( s );
			s = s->next;
		}
	}

// HACKAJAWEEA
//	tmplist.gethead()->next->next->next = NULL;

// shitty bubblesort for the linked list
//====================================
	node_c<gvar_c *> *rov, *bof, *start, *end, *head, *tail;

	head = tmplist.gethead();
	tail = tmplist.gettail();
	int length = tmplist.size() * 2;


	while ( tail && head != tail ) {

		rov = head;
		bof = ( rov ) ? rov->next : NULL;

		while ( rov && bof ) {

			if ( O_strncasecmp( rov->val->name(), bof->val->name(), GV_NAME_SIZE ) > 0 ) {

				rov->next = bof->next;
				bof->prev = rov->prev;

				if ( rov->prev )
					rov->prev->next = bof;
				else
					head = bof;
	
				if ( bof->next )
					bof->next->prev = rov;
//				else
//					tail = rov;
	
				bof->next = rov;
				rov->prev = bof;
	
				// increment 
				bof = rov->next;
			} else {
				// increment 
				rov = rov->next;
				bof = ( rov ) ? rov->next : NULL;
			}
			
		} // end while()
		
		if ( tail )
			tail = tail->prev;

	} // end while()
//====================================

	node_c<gvar_c*> *node = head;
	while ( node ) {
		gvar_c *s = node->val;
		if ( s->comment()[0] )
			console.Printf( "%s \"%s\" (%s)", s->name(), s->string(), s->comment() );
		else
			console.Printf( "%s \"%s\"", s->name(), s->string() );
		node = node->next;
	}

	tmplist.destroy();
}

/*
==================== 
 Gvar_Get

 sets as well as gets. hey, 2 for the price of one
==================== 
*/
gvar_c *Gvar_Get( const char *name, const char *value =NULL, uint flags, const char *comment )
{
    int i;
    gvar_c *g;
    
    if ( !name || !name[0] )
        return NULL;

	if ( !gvarPool ) {
		Gvar_Init();
	}

    // get gvar if it is already created
    if ( !(g = Gvar_Find( name )) )
    {
        // get an available gvar
        if ( gvarInUse == GV_POOL_SIZE ) {
            Com_Error( ERR_FATAL, "out of gvars\n" );
        }

		// FIXME: this is stupid, save a pointer to the end
        for ( i = 0; i < GV_POOL_SIZE; i++ ) {
            if ( !gvarPool[i].inuse ) {
                break;
            }
        }

		Assert( i < GV_POOL_SIZE );
        if ( i == GV_POOL_SIZE ) {
            Com_Error( ERR_FATAL, "no free gvar\n" );
        }

        g = &gvarPool[i];
		g->reset();
        ++gvarInUse;
        g->inuse = 1;
        g->hash = Gvar_HashName( name );
		g->set( name, value, flags );
		if ( comment ) {
			g->setComment( comment );
		}
		Gvar_HashInsert( g );
	} else {
		g->set( name, value, flags );
		if ( comment ) {
			g->setComment( comment );
		}
	}
	return g;
}

/*
==================== 
 Gvar_Free
==================== 
*/
// basically does the same thing as g->reset()
void Gvar_Free( gvar_c *g )
{
    if ( !g )
        return;

    if ( !g->inuse )
        return;

    g->reset();
}

/*
==================== 
 Gvar_Reset
==================== 
*/
// zero out the whole everything
void Gvar_Reset( void ) {
    C_memset( gvarPool, 0, gvarPoolBytes );
    C_memset( gvarHash, 0, sizeof(gvarHash) );
}

/*
==================== 
 Gvar_Init
==================== 
*/
void Gvar_Init( void ) {
	static gbool didalready = gfalse;
	
	if ( didalready )
		return;

	didalready = gtrue;
	gvarPool = (gvar_c *) V_Malloc(sizeof(gvar_c) * GV_POOL_SIZE);
	gvarPoolBytes = sizeof(gvar_c) * GV_POOL_SIZE;
    Gvar_Reset();
}

