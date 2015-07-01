// Twitter Demo with Component using Marketplace Authorization

#include "twtrcomp.h"

#include <gtk/gtk.h>

gboolean g_bGUI = TRUE;    //set to FALSE to use stdout

// toplevel window handle need it to be global so that show_error()
// can use it to present an error dialog if we are operating in a UI
// context, which is the case when g_bGUI is set to true.

GtkWidget *g_pGtkW = NULL,

// array of label handles for presenting tweets. 

          *g_apGtkWL[TIMELINE_SIZE];


// print_cb implements the way the component can provide data to the
// user without the component having to know anything about the ui
// toolset chosen for the application. if we choose not to implement
// this, then the component knows that it will need to print out data
// and error messages itself and it will do so using stdout

// index of tweet label array. we reset it when we do a get.

int g_siCnt = 0;

gboolean
print_cb(gchar* pgcDat)
{
  // variables that tell us we have the parts we want

  static gboolean sbID = FALSE, sbCon = FALSE, sbDat = FALSE;

  // variables that contain the parts we want

  static gchar sagcID[MAX_STR_CHARS],sagcDat[MAX_STR_CHARS],
               sagcCon[MAX_STR_CHARS];

  // string that will be used to put together the label

  static gchar sagcLbl[MAX_STR_CHARS];

  if(!pgcDat) return FALSE;

  if(FALSE==g_bGUI)
  {
    g_print("Using Print Callback: %s\n", pgcDat);
    return TRUE;
  }

  // look for the things we want to present in our ui, we choose to
  // only use a small subset

  g_print("%s\n", pgcDat); // developer fyi: allows you to see all that came over

  if(!strncmp("date", pgcDat, 4))
  {
    sbDat = TRUE;
    sprintf(sagcDat, "%s ", strchr(pgcDat, ' ') + 1);
  }

  if(!strncmp("author ", pgcDat, 7)) // space here so we dont get 'authorid'
  {
    sbID = TRUE;
    sprintf(sagcID, "%s ", strchr(pgcDat, ' ') + 1);
  }

  if(!strncmp("content", pgcDat, 7))
  {
    sbCon = TRUE;
    sprintf(sagcCon,"%s ", strchr(pgcDat, ' ') + 1);
  }

  // we have what we need to write the label format it and attach it
  // to the control

  if(sbCon && sbID && sbDat)
  {
    sprintf(sagcLbl, "%s:\t%s\n\t%s", sagcID, sagcDat, sagcCon);
    gtk_label_set_text(GTK_LABEL(g_apGtkWL[g_siCnt++]), sagcLbl);

    // label is written, clean up for use on  the next label

    sbCon=sbID=sbDat=FALSE;
    bzero(sagcLbl, MAX_STR_CHARS);
    bzero(sagcID,  MAX_STR_CHARS);
    bzero(sagcCon, MAX_STR_CHARS);
    bzero(sagcDat, MAX_STR_CHARS);
  }

  return TRUE;
}


// Generates the error dialog we used for all errors in this application

void
show_error(gchar* pgcMsg)
{
  GtkWidget *pGtkD;

  if(FALSE==g_bGUI) g_print(pgcMsg); // use stdout if GUI isnt available
  else
  {
    pGtkD = gtk_message_dialog_new(GTK_WINDOW(g_pGtkW), 
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				   pgcMsg, "title");

    gtk_window_set_title(GTK_WINDOW(pGtkD), "Error");
    gtk_dialog_run(GTK_DIALOG(pGtkD));
    gtk_widget_destroy(pGtkD);
  }
}


// callbacks for interacting with twitter component from UI buttons

// If the Get Status button is pushed, the following callback is
// executed. Notice that there is no ui component to this, all the
// display is performed in print_cb() above

void
get_status(GtkWidget *pGtkW, gpointer gpWin)
{
  gchar agcErr[MAX_STR_CHARS];

 // we start this over for each get, used in print_cb to present tweets
  
  g_siCnt = 0;

  if(TRUE==TWTR_get(FALSE)) return;

  // if we did not succeed in getting the tweets, then collect the
  // error message from the component and report the error message via
  // the error dialog  

  TWTR_lastError(agcErr);
  show_error(agcErr);
}


// If the Post Tweet button is pushed, the following callback is
// executed. A dialog box is created that contains a gtk entry control
// and an OK button, the user enters the text for the tweet and if the
// user pushes the OK button then the tweet is sent out to twitter via
// the twitter component and mojito

#define TWITTER_MAX_CHARS 140

void
post_tweet(GtkWidget *pGtkW, gpointer gpWin)
{
  gint giRet = 0;

  // 141 = length of tweet + null terminator

  gchar agcTwt[TWITTER_MAX_CHARS+1], agcErr[MAX_STR_CHARS];
 
  GtkWidget *pGtkDA, *pGtkE, *pGtkD;

  // create the posting dialog

  pGtkD = gtk_dialog_new_with_buttons("Post Tweet", GTK_WINDOW(gpWin),
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      //GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OK, GTK_RESPONSE_OK,
				      NULL);

  // make it large enough to see a good portion of the tweet, but not
  // so large that it consumes too much UI area

  gtk_window_set_default_size(GTK_WINDOW(pGtkD), 300, 100);

  // obtain a handle to the upper half of the dialog box (referred to
  // as the 'content area' in gtk parlance), then create a text entry
  // control and add it into the upper half of the dialog box.

  pGtkDA = gtk_dialog_get_content_area(GTK_DIALOG(pGtkD));

  pGtkE = gtk_entry_new(); 

  gtk_entry_set_max_length(GTK_ENTRY(pGtkE), TWITTER_MAX_CHARS);

  gtk_container_add(GTK_CONTAINER(pGtkDA), pGtkE);

  gtk_widget_show_all(pGtkD); // things wont show up unless this is done.

  // dialog is now ready for use by the user. note that we only
  // attempt to post if the user has hit the OK button

  if(GTK_RESPONSE_OK!= (giRet = gtk_dialog_run(GTK_DIALOG(pGtkD)))) return;
  
  // at this point, the user has hit OK, and the dialog has closed.
  // copy tweet string into new buffer since the text entry buffer in
  // the dialog will be free'd when the  dialog box is free'd in
  // gtk_widget_destroy() 

  if(!gtk_entry_get_text(GTK_ENTRY(pGtkE))) return;

  snprintf(agcTwt, sizeof(agcTwt), "%s", gtk_entry_get_text(GTK_ENTRY(pGtkE)));

  gtk_widget_destroy(pGtkD);

  if(TRUE==TWTR_put(agcTwt)) return;

  // if we did not succeed in posting the tweet, then collect the
  // error message from the component and report the error message via
  // the error dialog 

  TWTR_lastError(agcErr);
  show_error(agcErr);
}


// create our main window it contains our main display area for
// presenting recieved tweets and the the buttons used to post new
// tweets and refresh the current set of presented tweets.
//
// if this function fails to complete then it will return FALSE

gboolean
initGUI(int *pCntArg, char ***pppcArgs) // really has triple indirection
{
  int i;
  GtkWidget *pGtkBxV, *pGtkFBxV, *pGtkBbH, *pGtkBGS, *pGtkBPT, *pGtkSpH;

  // try and initialize the gtk global context, we use the 'check'
  // version because it will fail gracefully if it finds its
  // circumstances to be unsatisfactory

  if(FALSE== gtk_init_check(pCntArg, pppcArgs)) return FALSE;

  // create our toplevel window, this is where we will place our
  // buttons and present our collected tweets. we size it large enough
  // to be useful, but not so large that it would dwarf the screen of
  // a MID

  if(!(g_pGtkW = gtk_window_new(GTK_WINDOW_TOPLEVEL))) return FALSE;

  // this permits the user to kill us from the UI controls

  g_signal_connect(G_OBJECT(g_pGtkW), "destroy",
		   G_CALLBACK(gtk_main_quit),
		   NULL);

  gtk_window_set_position(GTK_WINDOW(g_pGtkW), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(g_pGtkW), 440, 300);
  gtk_window_set_title(GTK_WINDOW(g_pGtkW), "Twitter Component Demo");

  pGtkFBxV = gtk_frame_new("Status");
  gtk_container_add(GTK_CONTAINER(g_pGtkW), pGtkFBxV);

  pGtkBxV = gtk_vbox_new(TRUE, 15);
  gtk_container_add(GTK_CONTAINER(pGtkFBxV),pGtkBxV);

  for(i=0; i<TIMELINE_SIZE; i++)
  {
    g_apGtkWL[i] = gtk_label_new(i ? "":"Click 'Get Status' Button for Updates");
    gtk_box_pack_start(GTK_BOX(pGtkBxV),  g_apGtkWL[i], FALSE, FALSE, 0);
  }

  pGtkSpH = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(pGtkBxV), pGtkSpH, FALSE, FALSE, 0);

  pGtkBbH = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(pGtkBxV), pGtkBbH, FALSE, TRUE, 0);

  gtk_button_box_set_layout(GTK_BUTTON_BOX(pGtkBbH), GTK_BUTTONBOX_SPREAD);
  // create the button widgets and attach them to the button hbox.

  pGtkBGS = gtk_button_new_with_label("Get Status");
  g_signal_connect(G_OBJECT(pGtkBGS), "clicked",
		   G_CALLBACK(get_status), (gpointer) g_pGtkW);

  gtk_container_add(GTK_CONTAINER(pGtkBbH), pGtkBGS);

  pGtkBPT = gtk_button_new_with_label("Post Tweet");
  g_signal_connect(G_OBJECT(pGtkBPT), "clicked",
		   G_CALLBACK(post_tweet), (gpointer) g_pGtkW);

  gtk_container_add(GTK_CONTAINER(pGtkBbH), pGtkBPT);

  gtk_widget_show_all(g_pGtkW); // make our added widgets visible

  return TRUE;
}


// initialize our relationship with the authorization system and
// confirm that we are an appropriately validated application

ADP_RET_CODE
initAuthApp()
{
  gchar agcLog[MAX_STR_CHARS];

  ADP_RET_CODE arc = ADP_FAILURE;

  if(ADP_SUCCESS != (arc = ADP_Initialize()))
    switch( arc )
    {
    case ADP_INCOMPATIBLE_VERSION:
      strcpy(agcLog,"Error: Incompatible version");


    // TODO List version expected and version available
    // via approriate API

    goto fin;


    case ADP_NOT_AVAILABLE:
      strcpy(agcLog,"Error: Client Auth Agent not running");


    // TODO Inform user of how to start it, or start it ourselves?

    goto fin;

    
    case ADP_FAILURE:
    
    default:
      strcpy(agcLog, "Error: unknown error");

    goto fin;
    }

  // now that we know that the Auth system is live, we can now check
  // that we are authorized

  if(ADP_AUTHORIZED != (arc = ADP_IsAuthorized(ADP_DEBUG_APPLICATIONID)))
  {
    sprintf(agcLog, "Error: Application is Not Authorized Err=%d", arc);
    goto fin;
  }

  // Finally, we register with the application monitoring service,
  // but if ADP_ApplicationBeginEvent() fails then we report it and bail

  if(ADP_SUCCESS != (arc = ADP_ApplicationBeginEvent()))
  {
    sprintf(agcLog,"Application Monitoring Error: %d", arc);
    goto fin;
  }

 fin:

  if(ADP_SUCCESS != arc) show_error(agcLog);
  return arc;
}


int
main (int argc, char **argv)
{
  gchar agcLog[MAX_STR_CHARS]; agcLog[0] = '\0';

  ADP_RET_CODE arc = ADP_FAILURE;
  gboolean bSuc = TRUE;

  if(argc > 2 || ((argc == 2) && !strcmp(argv[1], "--help")))
  {
      g_print ("\
               usage: (if not compiled to use GTK UI)    \n\
               type %s to get items or   \n\
               type %s msg to upload msg to server \n", argv[0], argv[0]);
      return -1;
  }

  // if we are set to use the GUI, initialize it, if it doesn't work,
  // we will set the global b_GUI to false so that the rest of the app
  // will expect to use stdout to present things to the user

  if(TRUE==g_bGUI) g_bGUI = initGUI(&argc, &argv);
 
  // make sure that we are permitted to operate else bail

  if(ADP_SUCCESS != (arc = initAuthApp())) goto fin;

  // init the twitter component, this will check that the component is
  // authorized and set up the connection with twitter via mojito

  if(FALSE == (bSuc = TWTR_init(g_bGUI ? &print_cb : NULL)))
  {
    gchar agcErr[MAX_STR_CHARS];
    TWTR_lastError(agcErr);
    sprintf(agcLog,"Component Error: %s", agcErr);
    show_error(agcLog);
    goto fin;
  }

  // if all is well and we are running the GUI, execute the event loop.

  if(TRUE == g_bGUI) gtk_main();

  else // stdout impl: sends cl arg as tweet or prints status(es) to stdout

    bSuc = 2==argc ? TWTR_put(argv[1]) : TWTR_get(FALSE);   
 
 fin:

  // if we made it thru the ADP inits and the component init, then we
  // will need to shut them all down. If any of those shutdowns fail we
  // will inform the user.

  if(TRUE == bSuc && FALSE == TWTR_shutdown()) // component shutdown
  {
    TWTR_lastError(agcLog);
    show_error(agcLog);
  }

  if(ADP_SUCCESS == arc) // ADP Shutdown
  {
    if(ADP_SUCCESS != (arc = ADP_ApplicationEndEvent()))
    {
      sprintf(agcLog, "Error: ADP_ApplicationEndEvent: Err=%d", arc);
      show_error(agcLog);
    }

    if(ADP_SUCCESS != ADP_Close())
      show_error("ADP_Close() returned ADP_FAILURE");
  }
  return bSuc;
}
