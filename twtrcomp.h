#include <string.h>
#include <glib.h>
#include <adpcore.h> // NB: Both Client and Component need ADPCore

#define MAX_STR_CHARS 256 // limit the chance for buffer overruns


// The TIMELINE_SIZE tells the number of tweets that may be cached locally
// MAX_RECV_ITEMS is a flag to end the main loop, it will not quit the loop
// until MAX_RECV_ITEMS-TIMELINE_SIZE newly updated items are received.

#define TIMELINE_SIZE 10
#define MAX_RECV_ITEMS TIMELINE_SIZE+2

// printing callback function pointer - very simple, if you want your
// app to do the printing then implement the function and pass the
// pointer to it into the init() function. The funnction is expected
// to return TRUE on success and FALSE on error.

typedef gboolean (*pfTwtrCompPrintCallback) (gchar* pgcDat);


// NB: All of the functions return TRUE on success and FALSE on failure

// init() gets all of the systems instantiated and confirms that the
// component instance is authorized.

gboolean
TWTR_init(pfTwtrCompPrintCallback pfPrt);


// get() will grab the current tweets up to MAX_RECV_ITEMS, unless
// their is fewer tweets than MAX_RECV_ITEMS, in which case it will
// wait (*BLOCK!!!*) until MAX_RECV_ITEMS have been recieved.
//
// This behavior can be defeated by passing FALSE into gbWat - this is the
// suggested use paradigm.
//
// The resultant data will be printed out via the printing callback
// passed in as the argument to init()
//
// Any service side errors that mojito reports* to us will also cause the
// function to return FALSE with an error message available from
// lastError()

gboolean
TWTR_get(gboolean gbWat);


// put() pushes a tweet out to twitter, passing a NULL is an error. Any
// service side errors that mojito reports* to us will also cause the
// function to return FALSE with an error message available from
// lastError()

gboolean
TWTR_put(gchar* pgcTwt);

// shutdown() frees up all of the things that where created in init()
// it does return a value to be checked because that's useful for
// debugging - if it returns FALSE, then something went wrong and
// there should be a message in lastError()

gboolean
TWTR_shutdown(void);


// provides error data for the proceeding functions, should be called
// whenever a FALSE is returned.
//
// *its possible that some errors may not be returned by mojito

gboolean
TWTR_lastError(gchar* pgcErr);




