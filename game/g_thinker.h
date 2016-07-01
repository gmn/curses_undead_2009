
class thinker_t : public entity_t {

protected:
	int mode;
public:
	void think( void ) = 0;
	void damage( void ) = 0;
};

class pidgeon_t : public thinker_t {
	
	void stop_and_peck( void );
	void look_at_player( void );
	void do_walking( void ); // will walk back in forth in a certain area,
							// every so often turning.
	
	// if hit by another entity
	void damage( void );    // base damage method is in entity_t
	
	void fly_away( void ); 	// if hit, but not killed

public:

	void think( void );	// current function to "think" in

	/* note, think() could also be a static func w/ a switch instead,
	void think( void ) {
		switch ( mode ) {
		case PIDG_MODE_DO_WALKING:		do_walking(); break;
		case PIDG_MODE_STOP_AND_PECK:	stop_and_peck(); break;
		case PIDG_MODE_LOOK_AT_PLAYER:	look_at_player(); break;
		case PIDG_MODE_FLY_AWAY:		fly_away(); break;
		}
	}
	*/
};


