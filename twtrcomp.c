// Twitter Demo with Component using Intel Atom Developer Program authorization
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <mojito-client/mojito-client.h>
#include <mojito-client/mojito-client-service.h>

#include "twtrcomp.h"

#define SERVICE_NAME "twitter"

char g_acMsgErr[MAX_STR_CHARS];  // Err Msg returned in TWTR_lastError()

gboolean g_gbTimeOut = FALSE;    // Flag to set timeout error message
                                 // for calls to mojito

gboolean g_gbWat     = FALSE;    // tells item_add_cb() callback if we won't
		                 // wait for max items to be filled




// This structure contains the data that we need to have available in
// our callbacks that allow us to do some important things:
//
// 1. Throvide mojito with tweet we would like it to send out
// 2. The program loop instance that mojito runs on our behalf
// 3. The handle by which mojito-server is able to keep track of our client
//
// TODO Learn more about this

typedef struct
{
  gchar *pgcMsg;
  GMainLoop *pgmLp;
  MojitoClient *pMC;
} sDatCli;

sDatCli *g_psDatCli; // global pointer to the data struct, will new it
		     // in init()

// the caller passes us one of these if they want to do the printing
// instead of letting us just print to stdout

pfTwtrCompPrintCallback g_pfPrt = NULL;


// hide the printing implementation from the component. We expect that
// a client will want to take responsibility for printing out any
// strings. But if that isn't the case because the caller chose to pass
// a NULL function pointer to us as the argument to TWTR_init(), we
// can print to stdout ourselves.

void
present_string(gchar* pgcDat)
{
  g_pfPrt ? g_pfPrt(pgcDat) : g_print(pgcDat);
}


// Service update callback.
// Provides status data via glib's GError reporting mechanism
// http://library.gnome.org/devel/glib/stable/glib-Error-Reporting.html#GError

static void
update_cb(MojitoClientService  *pMCSrv, const GError *pGErr,
          gpointer gpDatCli)
{
  gchar agcRes[MAX_STR_CHARS];

  // pGErr is !NULL on failure and has specific error data available
  // that may help the user resolve the problem that has created the
  // error. 

  if(pGErr)
    sprintf(agcRes, "Service Update Result: Failure, %s, Code=%d\n",
	    pGErr->message, pGErr->code);
  else
    sprintf(agcRes, "Service Update Result: Success\n");

  present_string(agcRes);
 
  g_main_loop_quit(((sDatCli*)gpDatCli)->pgmLp);
}


// Mojito comes with different service backends and we are using the
// twitter one in this example. Currently there are ones for digg,
// flickr, myspace, lastfm. There is also a dummy and a skeleton that
// can be used as a basis for writing other mojito service backends. 
//
// Each of these online services provide different things, thus each
// service backend has to be able to inform the client of what it is
// capable of providing, Note that not all functionality from an
// online service may be provided by the mojito service backend.

static void
capabilities_cb(MojitoClientService *pMCSrv, guint32 uiCanUpd,
		gpointer gpDatCli)
{
  // cast gpointer (aka void*) back to our client data structure

  sDatCli *g_psDatCli = (sDatCli*) gpDatCli;

  if(uiCanUpd)
    mojito_client_service_update_status(pMCSrv, update_cb, g_psDatCli->pgcMsg,
					g_psDatCli);

  else // updating twitter is not possible for some reason, let user know
  {
    sprintf(g_acMsgErr, "Error: capabilities_cb(): Twitter Service does not support update operation \n");

    g_main_loop_quit(((sDatCli*)gpDatCli)->pgmLp);
  }
}


// mechanism for abstracting how data returned from the service is
// displayed. we give this function to mojito-core and we get to
// decide how the function will actually print the data

static void
service_item_print(gpointer gpKey, gpointer gpVal, gpointer gpDatCli)
{
  gchar agcRes[MAX_STR_CHARS];

  // assume both key and value are simple string. in the case of a
  // different service, these could be complex data structures

  // if we are not using the print callback we use a format string
  // that is appropriate for using with stdout, complete with a
  // carriage return, if we have the print function callback pointer,
  // then we just use a space separated pair with no decoration so
  // that it is easier to parse.

  sprintf(agcRes,g_pfPrt ? "%s %s":"key: %s, value: %s \n", (gchar*) gpKey,
	  (gchar*) gpVal);

  present_string(agcRes);
}


// Each time mojito-core detects an item change it will emit a signal.
// Mojito will check between adjacent refreshes, Since each twitter
// update will return most recent TIMELINE_SIZE items, this means each
// time a new item comes into the timeline, there is also an old item
// disappeared. 

// each newly removed item will trigger this callback

static void
item_removed_cb(MojitoClientView *pMCV, MojitoItem *pMI,
		gpointer gpDatCli)
{
  gchar agcRes[MAX_STR_CHARS];

  if(!g_strcmp0(pMI->service, SERVICE_NAME)) return;

  if(!g_pfPrt) // extra info if we are using stdout
    sprintf(agcRes,"---\nService %s item removed : \n", pMI->service);

  present_string(agcRes);

  g_hash_table_foreach (pMI->props, (GHFunc) service_item_print , NULL);
}


// each newly added item will trigger this callback

static void
item_added_cb(MojitoClientView *pMCV, MojitoItem *pMI,
	      gpointer gpDatCli)
{
  int iCnt = 0; // was static for some reason in orig sample

  gchar agcRes[MAX_STR_CHARS];

  gulong  gulDum    = 0;   // dummy argument for gtimer
  gdouble gdTimeMax = 5.0; // how long to wait before giving up

  GTimer* pGTWat;

  // make sure we are talking to the service that we expect, otherwise bail

  if(g_strcmp0(pMI->service, SERVICE_NAME)) return;

  pGTWat = g_timer_new();

  if(!g_pfPrt) // extra info if we are using stdout
  {
    sprintf(agcRes,"---\nService %s item added : \n", pMI->service);
    present_string(agcRes);
  }

  g_hash_table_foreach(pMI->props, (GHFunc) service_item_print, NULL);

  // if we have max count of items or we have timed out waiting and have
  // not been asked to wait for max items then we will bail

  if(++iCnt >= MAX_RECV_ITEMS ||
     (gdTimeMax > g_timer_elapsed(pGTWat, &gulDum ) && !g_gbWat))
    g_main_loop_quit(((sDatCli*)gpDatCli)->pgmLp);

  g_timer_destroy(pGTWat);
}


// open view callback initiates relationship with mojito twitter
// service by telling it which of our functions are used to tell us
// about twitter status queue changes and initiating our view.

static void
open_view_cb(MojitoClient *pMC, MojitoClientView *pMCV,
              gpointer gpDatCli)
{
  g_signal_connect(pMCV, "item-added",   G_CALLBACK(item_added_cb),   gpDatCli);
  g_signal_connect(pMCV, "item-removed", G_CALLBACK(item_removed_cb), gpDatCli);

  mojito_client_view_start(pMCV);
}


// This callback is invoked when the client online query is returned by
// mojito_client_is_online() TODO it appears that we get hungup on
// occasion somewhere in mojito, ideally, getting a timer involved in
// this that we could then use to trigger g_main_loop_quit() might
// alleviate that, but i think that the hang occurs before this
// callback ever gets invoked. TODO since i have a loop handle, what
// about what using g_timeout_*() stuff?

static void
client_online_cb(MojitoClient *pMC, gboolean gbOnline, gpointer gpDatCli)
{
  gboolean gbOK = FALSE;

  MojitoClientService *pMCSrv;

  sDatCli *g_psDatCli = (sDatCli*)gpDatCli;

  if(!gbOnline) goto fin; // cant do anything without The Internet
 
  if(g_psDatCli->pgcMsg)    // we have a msg to send
  {
    if(!(pMCSrv=mojito_client_get_service(pMC, SERVICE_NAME))) goto fin;

    g_signal_connect(pMCSrv, "capabilities-changed",
		     G_CALLBACK(capabilities_cb), gpDatCli);
  }
  else // no msg to send, so we look for ones coming in, perhaps waiting
    mojito_client_open_view_for_service(pMC, SERVICE_NAME, TIMELINE_SIZE,
					 open_view_cb, g_psDatCli);
  gbOK = TRUE;

 fin:

  if(FALSE==gbOK)
  {
    sprintf(g_acMsgErr, "Error: client_online_cb(): %s",!gbOnline ?
	    "mojito client is not online !\n" :
	    "can not get service twitter client \n");

    g_main_loop_quit(g_psDatCli->pgmLp);
  }
}


// Validate with ADP the calling app should have already initialized it

gboolean
AppAuthOK(void)
{
 ADP_RET_CODE arc = ADP_FAILURE;

 if(ADP_AUTHORIZED == (arc = ADP_IsAppAuthorized(ADP_DEBUG_COMPONENTID)))
   return TRUE;

 sprintf(g_acMsgErr,"Error: Component is Not Authorized Err=%d", arc);

 return FALSE;
}


// This function get's called from glib's main event loop because we
// tell glib about it via the g_source_set_callback() function. If we
// get here then we will set the timeout flag to true, write an error
// message for TWTR_lastError() and quit the loop

gboolean
timeout_cb(gpointer pgdat)
{
  g_gbTimeOut = TRUE;
  sprintf(g_acMsgErr, "Error: invoke_mojito_twtr() timed out");
  g_main_loop_quit((GMainLoop *)pgdat);
  return TRUE;
}


// setup all the 

gboolean
TWTR_init(pfTwtrCompPrintCallback pfPrt)
{
  if(FALSE==AppAuthOK()) return FALSE;

  g_pfPrt = pfPrt; // set the copy of the print callback so we can use it.

  g_type_init();   // initiate glib type implementations and signals.

  g_psDatCli = g_new0(sDatCli, 1);     // initiate global identity struct
                                       // fill out the parts thereof
  g_psDatCli->pgcMsg = NULL; 
  g_psDatCli->pMC    = mojito_client_new();
  g_psDatCli->pgmLp  = g_main_loop_new(NULL, FALSE);

  return TRUE;
}


// TWTR_put() and TWTR_get() use this to get work done.
// we tell mojito that we want to come online, then we set the
// timeout function so that we can return if mojito hangs whilst
// polling for something and the start the loop. when we drop out of
// the loop, we will have either finished successfully (so g_gbTimeOut
// will remain FALSE) or we will have timed out and g_gbTimeOut will
// have been set to TRUE, thus our inversion of it for our return code. 

gboolean
invoke_mojito_twtr(void)
{
  GSource* pGscTO;
  g_gbTimeOut = FALSE;

  mojito_client_is_online(g_psDatCli->pMC, client_online_cb, g_psDatCli);

  pGscTO = g_timeout_source_new_seconds(30); // how long we will wait
  g_source_set_callback(pGscTO, &timeout_cb, g_psDatCli->pgmLp,NULL);
  g_source_attach(pGscTO, NULL);

  g_main_loop_run(g_psDatCli->pgmLp);   // run until done or time out

  // if the value is true, then we timed out and that is an error
  return !g_gbTimeOut;
}


// request status messages from twitter - the result will be shown via
// the print callback function that we where handed a pointer to when
// TWTR_init() was called. The argument gbWat allow the user to
// specify if they want to wait until the MAX_RECV_ITEMS have
// arrived. Waiting for that to happen could block indefinitely.

gboolean 
TWTR_get(gboolean gbWat)
{
  if(FALSE==AppAuthOK()) return FALSE;

  g_gbWat     = gbWat; 

  return invoke_mojito_twtr();
}


// post the passed in status message to twitter

gboolean
TWTR_put(gchar* pgcTwt)
{
  if(FALSE==AppAuthOK()) return FALSE;

  if(!pgcTwt)
  {
    sprintf(g_acMsgErr, "Error: put(): tweet argument was NULL");
    return FALSE;
  }

  g_psDatCli->pgcMsg = pgcTwt;

  return invoke_mojito_twtr();
}


// clean up prior to exit.

gboolean
TWTR_shutdown(void)
{
  gboolean bSuc = TRUE;
  
  if(g_psDatCli) // if we have it, free the global identity struct
  {
    if(g_psDatCli->pMC)  g_object_unref( g_psDatCli->pMC); // free client inst
    if(g_psDatCli->pgmLp) g_main_loop_unref(g_psDatCli->pgmLp); // free  loop
    g_free(g_psDatCli);                   
  }

  return bSuc;
}


gboolean
TWTR_lastError(gchar* pgcErr)
{
  if(!pgcErr) return FALSE;

  snprintf(pgcErr, MAX_STR_CHARS, "%s", g_acMsgErr);

  return TRUE;
}
