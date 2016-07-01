#ifndef __LIB_BUFFER_H__
#define __LIB_BUFFER_H__

#ifndef NULL
#define NULL 0
#endif

/*
==============================================================================

	buffer_c

	two-part class structure:

	part 1:
		a buffer class that is an array and has the ability to grow dynamically
	when using the inline mechanism add() or push(), which inserts elts onto
	the end of the data.  if the maximum size is reached it is automatically
	increased

	part 2: bufferPool_c
		a way to manage a set of buffers, so that simple buffers can be 
	easily re-used between different extractors, instead of each extractor
	claiming its own memory span

==============================================================================
*/
template <typename type>
class buffer_c {
private:
	static const unsigned int bMag = 0x1BA55094;
	unsigned int magic;
protected:
	type *					free_p;		// pointer to next free elt
	unsigned int			_size;		// byte amount held
public:
	type *					data;		// master data pointer

	void init ( unsigned int sz =2048 ) {
		if ( magic == bMag ) {
			free_p = data; // implicit reset
			return;
		}
		_size = sz * sizeof(type);
		data = (type * ) V_Malloc ( _size );
		free_p = data;
		magic = bMag;
	}

	buffer_c( void ) : free_p(0),_size(0),data(0) {
		//init();
	}

	void add( type t ) {
		if ( (length()+1)*sizeof(type) > _size ) {
			unsigned int newsz = _size << 1;
			type *tmp = (type*) V_Malloc( newsz );
			memcpy( tmp, data, _size );
			_size = newsz;
			int free_p_ofst = free_p - data;
			V_Free( data );
			data = tmp;
			free_p = data + free_p_ofst;
		}

		*free_p++ = t;
	}

	void push( type t ) { add ( t ); }
	void reset_keep_mem( void ) { free_p = data; }

	// copy in an array
	void copy_in( const type *a, unsigned int sz ) 
	{
		unsigned int needed = ( length() + sz ) * sizeof(type);

		if ( needed > _size ) {
			unsigned int newsz = _size << 1;
			while ( newsz < needed ) { 
				newsz <<= 1; 
			} 
			type *tmp = (type*) V_Malloc( newsz );
			memcpy( tmp, data, _size );
			_size = newsz;
			int free_p_ofst = free_p - data;
			V_Free( data );
			data = tmp;
			free_p = data + free_p_ofst;
		}
		//memcpy( free_p, a, sz * sizeof(data) );
		for ( int i = 0; i < sz ; i++ ) {
			free_p[ i ] = a[ i ];
		}
		free_p += sz;
	}

	// static class constructor method
	static buffer_c<type> * newBuffer( unsigned int sz =128 ) {
		buffer_c<type> *a = (buffer_c<type>*) V_Malloc( sizeof(buffer_c<type>));
		a->_size = sz * sizeof(type);
		a->data = ( type * ) V_Malloc( a->_size );
		a->free_p = a->data;
		return a;
	} 

	void destroy( void ) {
		if ( magic == bMag ) {
			if ( this->data ) {
				V_Free( this->data );
			}
		}
		this->data = NULL;
		_size = 0;
		free_p = NULL;
		magic = 0;
	}

	~buffer_c( void ) { /* destroy(); */ }

	void zero_out( void ) {
		memset( data, 0, _size );
		free_p = data;
	}
	void reset( void ) {
		if ( data ) {
			zero_out();
		}
	}

	// # of elts in use
	unsigned int length( void ) const { return (uint)(free_p-data); }
	// # of bytes alloc'd internally
	unsigned int size( void ) const { return _size; }

	void set_by_ref( buffer_c<type> const& obj ) {
		unsigned int elts = obj.length();
		init( elts );
		copy_in( obj.data, elts  );
	}

	type operator[]( unsigned int index ) {
//		if ( index >= length() )
//			return 0;
		return data[ index ];
	}
	bool isstarted() { return data != NULL; }

	void removeIndex( unsigned int index ) {
		if ( !data || index >= length() || length() == 0 )
			return;
		type * src = data + index + 1;
		type * dest = data + index;
		size_t bytes = data + length() - src; 
		memmove( dest, src, bytes );
		--free_p;
	}

	// call delete on each element of the contents
	void delete_contents( void ) {
		if ( !data )
			return;

		if ( !V_isStarted() ) {
			return;
		}

		const unsigned int len = length();
		for ( int i = 0; i < len; i++ ) {
			if ( data[i] )
				delete data[i];
			data[i] = NULL;
		}
	}

	void delete_reset() {
		delete_contents();
		reset();
	}

	void delete_destroy() {
		delete_contents();
		destroy();
	}
};

/*
====================
 buffer_delete

	takes a double-pointer to a templated buffer_c type, and calls delete
	on all its members, as well as deleting the main buffer and setting
	its pointer to NULL. 
====================
*/
template<typename type>
inline void buffer_delete( buffer_c<type> ** buffer_pp ) {
	if ( !buffer_pp || !(*buffer_pp) )
		return;
	buffer_c<type> & buffer = **buffer_pp;
	
	if ( !buffer.isstarted() )
		return;

	const unsigned int len = buffer.length();
	for ( int i = 0; i < len; i++ ) {
		delete buffer.data[i];
#ifdef _DEBUG
		buffer.data[i] = (type)0xcdcdcdcd;
#endif
	}

	delete *buffer_pp;
	*buffer_pp = NULL;
}



#endif // __LIB_BUFFER_H__

