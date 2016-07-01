

#include "../common/com_types.h"

// TODO: actually implement timer data-structure, and functions to
//          govern and manage the running of timers.  I found in my last 
//          game that I started doing many many events that used a sort of
//          home-made timer, with a start-timer, duration  and other 
//          information (perhaps a function pointer), and was polled by
//          functions who were called in the display or logic code.  timers
//          usually would expire and then be "turned off", if any timers were
//          "on", they would be run.
//
//
// timers are a list of finite events that start and run for a duration
//  once that duration is reached they are shutdown and discarded
typedef struct timer_s {
    char *name;
    int id;
    int start;
    int duration;

    // selects which func to call
    int action;
    // args for the func stored in order
    void *args[3];
    // the funcs
    void (*action0) (void);
    void (*action1) (void *);
    void (*action2) (void *, void *);
    void (*action3) (void *, void *, void *);
    // to disable set on to gfalse
    gbool on;
    struct timer_s *prev;
    struct timer_s *next;
} timer_t;

/*
#define _default_timer { NULL, 0, 0, 0, 0, { NULL, NULL, NULL }, NULL, \
                        NULL, NULL, NULL, gtrue, NULL, NULL }
                        */


