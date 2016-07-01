#ifndef __COM_OBJECT_H__
#define __COM_OBJECT_H__

void * V_Malloc( size_t );
void V_Free( void * );
int  V_isStarted( void );

// objects that all other zombiecurses game objects can inherit from. 
// providing standard interfaces that all objects tend to use.

/*
====================
 Allocator_t
====================
*/
class Allocator_t {
public:
	static void * operator new( size_t size ) { return V_Malloc( size ); }
	static void operator delete( void *ptr ) { V_Free( ptr ); }
	static void * operator new[]( size_t bytes ) { return V_Malloc( bytes ); }
	static void operator delete[]( void *ptr ) { V_Free( ptr ); }
};

/*
====================
 Auto_t

 manages internal state conditions automatically.  Good for classes that 
	allocate buffers
====================
*/
class Auto_t : public Allocator_t {
private:
	unsigned int magic;
	static const unsigned int AUTO_MAGIC = 0x499602D2;

protected:

	// Init Allocates
	virtual void _my_init( void ) = 0;
	// Reset sets virgin state
	virtual void _my_reset( void ) = 0;
	// Destroy DeAllocates
	virtual void _my_destroy( void ) = 0;

public:
	
	// sets up internal allocated buffers, only called once before any use.  subsequent calls will have no effect on allocation, though will call reset.
	virtual void init( void ) { 
		if ( magic == AUTO_MAGIC ) {
			_my_reset();
			return;
		}
		_my_reset(); 	// set state
		_my_init();		// use state to build
		magic = AUTO_MAGIC;
	}

	// sets variables initial state, does no allocation, although, if called before an init, will call init implicitly to do allocation
	virtual void reset( void ) {
		if ( magic != AUTO_MAGIC ) {
			init();
			return;
		}
		_my_reset();
	}

	virtual void destroy( void ) {
		if ( magic == AUTO_MAGIC ) {
			if ( V_isStarted() )
				_my_destroy();
			magic = 0;
		}
		_my_reset();
	}

	// synonyms
	void clear( void ) { reset(); }
	void start( void ) { init(); }

	/* should be provided by inheritors
	Auto_t() { _my_reset(); }
	virtual ~Auto_t() { destroy(); } 
	*/
};

/*
// FIXME: use standard copy constructor nomenclature instead
class Copyable_t : public Allocator_t {
public:
	virtual void Destroy( void ) = 0;
	static virtual Copyable_t * New( void ) = 0;
	virtual Copyable_t * Copy( void ) = 0;
};
*/


#endif // __COM_OBJECT_H__
