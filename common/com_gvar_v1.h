
/***********************************************************************
 *  GVAR system defines
 ***********************************************************************/
#define HOPEFULLY_UNIQUE_INIT_TAG 0x55378008
#define INIT_TAG HOPEFULLY_UNIQUE_INIT_TAG



enum { 
    // types
    GVT_PLACEHOLDER     =   BIT(0),
    GVT_STRING          =   BIT(1),
    GVT_FLOAT           =   BIT(2),
    GVT_INT             =   BIT(3),
    GVT_BOOL            =   BIT(4),
    GVT_POINTER         =   BIT(5),
    GVT_POINTER_ARRAY   =   BIT(6),

    // flags
    GVF_NOMODIFYVAL     =   BIT(12),
    GVF_NOCLIENTREAD    =   BIT(13),
    GVF_NOMODIFYFLAG    =   BIT(14),
    GVF_NOCLIENTSET     =   BIT(15),
    GVF_NOMODIFYALL     =   BIT(16),

    GVF_ALLOWALL        =   0,
    // 
    GVT_FLUSH           =  -1
};

/*
==================== 
   GVAR class
==================== 
*/
class gvar_c {

private:

    // identity
	autostring *    _name;
    autostring *    _desc;

    // values 
	autostring *    _sval;
	float           _fval;				
	int	            _ival;			    
    gbool           _bval;
    void *          _voidp;
    float           _fmin;
    float           _fmax;
    const char **   _valueStrings;

    // bookkeeping
    int             _numPointers;
    int             _hasBeenInit;
    gbool           _hasBeenSet;
	autostring *    _resetString;		
	autostring *    _latchedString;		
	int	            _flags;
	gbool           _modified; // default gfalse.  set gfalse on reset
	int	            _modificationCount;	

    /// FIXME: remove these and add linked list class objects instead
	gvar_c *        _next;
	gvar_c *        _hashNext;

    // utilities
    void *(*getmem) ( size_t );
    void (*freemem) ( void * );
    void setDescriptor ( const char *, const char *, const char * );

public:

    gbool           on;

    gbool init( void );
    void init( const char *,    const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const gwchar_t *, const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const int,        const char* =NULL, const char* =NULL, const char* =NULL ); 
    void init( const float,      const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const double,     const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const gbool,      const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const void *,     const char* =NULL, const char* =NULL, const char* =NULL );
    void init( const float, const float, const float, const char* =NULL, const char* =NULL, const char* =NULL );

    // value, [name, description, resetstring]
    void set( const char *,       const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const gwchar_t *,   const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const int,          const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const float,        const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const double,       const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const gbool,        const char* =NULL, const char* =NULL, const char* =NULL );
    void set( const void *,       const char* =NULL, const char* =NULL, const char* =NULL );
    // val, min, max, [name, description, resetstring]
    void set( const float, const float, const float, const char* =NULL, const char* =NULL, const char* =NULL );

    // accessors
    const gwchar_t * getName( void )        const { return _name->getcp(); }
    const gwchar_t * getDescription( void ) const { return _desc->getcp(); }
    const gwchar_t * getString( void )      const { return _sval->getcp(); }
    int              getInt( void )         const { return _ival; }
    float            getFloat( void )       const { return _fval; }
    gbool            getBool( void )        const { return _bval; }
    int              timesModified( void )  const { return _modificationCount;}
    gbool            isModified( void )     const { return _modified; }

    void setName ( const char * );
    void setDesc ( const char * );
    void setResetString ( const char * );
    void setInt ( int i ) { _ival = i; }
    void setFloat ( float f ) { _fval = f; }
    void setFloat ( double d ) { _fval = (float)d; }
    void setBool ( gbool b ) { _bval = b; }
    void setFlag( int );
    void resetFlag( void ) { _flags = 0; }

    void reset( void );
    void shutdown( void );

    // Never use the default constructor.
    gvar_c( void ) { assert( typeid( this ) != typeid( gvar_c ) ); }
    virtual ~gvar_c( void ) {};

};

// gvar operators
//void operator= ( gvar_c &, int );
/**********************************************************************/
