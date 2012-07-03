/* Copyright (C) 2012 Papavasileiou Dimitris                             
 *                                                                      
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or    
 * (at your option) any later version.                                  
 *                                                                      
 * This program is distributed in the hope that it will be useful,      
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
 * GNU General Public License for more details.                         
 *                                                                      
 * You should have received a copy of the GNU General Public License    
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static GtkWidget *window, *inspector_window;
static GtkWidget *scrolled;
static GtkWidget *paned;

static int attached, closed = 1;

static gboolean show_inspector (WebKitWebInspector *inspector)
{
    WebKitWebView *inspector_view;
    
    inspector_view = webkit_web_inspector_get_web_view(inspector);
	
    if (attached) {
	gtk_container_remove(GTK_CONTAINER(window), GTK_WIDGET(scrolled));

	gtk_paned_add1(GTK_PANED(paned), GTK_WIDGET(inspector_view));
	gtk_paned_add2(GTK_PANED(paned), GTK_WIDGET(scrolled));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(paned));

	gtk_widget_show(paned);    
    } else {
	gtk_container_add(GTK_CONTAINER(inspector_window),
			  GTK_WIDGET(inspector_view));
	gtk_widget_show(GTK_WIDGET(inspector_window));
    }

    closed = 0;

    return TRUE;
}

static gboolean close_inspector (WebKitWebInspector *inspector)
{
    WebKitWebView *inspector_view;

    if(closed) {
	return TRUE;
    }
    
    inspector_view = webkit_web_inspector_get_web_view(inspector);
	
    if (attached) {
	gtk_widget_hide(paned);
    
	gtk_container_remove(GTK_CONTAINER(paned), scrolled);
	gtk_container_remove(GTK_CONTAINER(paned),
			     GTK_WIDGET(inspector_view));

	gtk_container_remove(GTK_CONTAINER(window), paned);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(scrolled));
    } else {
	gtk_container_remove(GTK_CONTAINER(inspector_window),
			     GTK_WIDGET(inspector_view));
	gtk_widget_hide(GTK_WIDGET(inspector_window));
    }

    attached = 0;
    closed = 1;
    
    return TRUE;
}

static WebKitWebView *create_inspector (WebKitWebInspector *inspector,
					WebKitWebView *content_view,
					gpointer data)
{
    WebKitWebView *inspector_view;
    
    inspector_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    paned = gtk_vpaned_new();    

    g_object_ref (inspector_view);
    g_object_ref (paned);

    gtk_widget_show(GTK_WIDGET(inspector_view));

    inspector_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW(inspector_window), "Web inspector");
    gtk_widget_set_name (inspector_window, "Web inspector");
    gtk_window_set_default_size (GTK_WINDOW (inspector_window), 800, 300);
    g_signal_connect_swapped(G_OBJECT(inspector_window),
			      "delete-event",
			      G_CALLBACK(close_inspector),
			      inspector);

    return inspector_view;
}

static gboolean attach_inspector (WebKitWebInspector *inspector)
{
    assert (!attached);

    close_inspector(inspector);
    attached = 1;
    show_inspector(inspector);
    
    return FALSE;
}

static gboolean detach_inspector (WebKitWebInspector *inspector)
{
    assert (attached);

    close_inspector(inspector);
    attached = 0;
    show_inspector(inspector);
    
    return FALSE;
}

static gboolean finish_inspector (WebKitWebInspector *inspector)
{
    WebKitWebView *inspector_view;
    
    inspector_view = webkit_web_inspector_get_web_view(inspector);
	
    g_object_unref(inspector_view);
    g_object_unref(paned);
    gtk_widget_destroy(GTK_WIDGET(inspector_view));
    gtk_widget_destroy(paned);

    return FALSE;
}

static gboolean close_view(WebKitWebView *view, gpointer data)
{
    GtkWidget *parent;

    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));
    gtk_widget_destroy (parent);
    
    return TRUE;
}

static void notify_title (WebKitWebView *view,
			  GParamSpec *pspec,
			  gpointer data)
{
    GtkWidget *parent;
    const gchar  *title;

    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));
    
    title = webkit_web_view_get_title(view);
    gtk_window_set_title (GTK_WINDOW(parent), title ? title : "Techne");
}

static void notify_progress (WebKitWebView *view,
			     GParamSpec *pspec,
			     gpointer data)
{
    GtkWidget *parent;
    GString *string;
    gchar *title;
    gint progress;

    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));

    title = (gchar *)webkit_web_view_get_title(view);
    progress = webkit_web_view_get_progress (view) * 100;
    string = g_string_new (title ? title : "Techne");

    if (progress < 100) {
        g_string_append_printf (string, " (%d%%)", progress);
    }
    
    title = g_string_free (string, FALSE);
    gtk_window_set_title (GTK_WINDOW(parent), title);
    g_free (title);
}

static void configure_window (WebKitWebWindowFeatures *features,
			      GParamSpec *pspec,
			      WebKitWebView *view)
{
    GtkWidget *parent;
    int width, height, x, y, fullscreen;
    
    g_object_get (G_OBJECT (features),
    		  "width", &width,
    		  "height", &height,
    		  "x", &x,
    		  "y", &y,
    		  "fullscreen", &fullscreen,
    		  NULL);

    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));

    gtk_window_resize (GTK_WINDOW (parent), width, height);
    gtk_window_move (GTK_WINDOW (parent), x, y);

    if (fullscreen) {
	gtk_window_fullscreen (GTK_WINDOW (parent));
    } else {
	gtk_window_unfullscreen (GTK_WINDOW (parent));
    }

    /* printf ("%d, %d, %d, %d\n", width, height, x, y); */
}

static void connect_features (WebKitWebView *view,
			      GParamSpec *pspec,
			      gpointer data)
{
    WebKitWebWindowFeatures *features;

    features = webkit_web_view_get_window_features (view);

    g_signal_connect (G_OBJECT(features), "notify::width",
    		      G_CALLBACK(configure_window), view);
    g_signal_connect (G_OBJECT(features), "notify::height",
    		      G_CALLBACK(configure_window), view);
    g_signal_connect (G_OBJECT(features), "notify::x",
    		      G_CALLBACK(configure_window), view);
    g_signal_connect (G_OBJECT(features), "notify::y",
    		      G_CALLBACK(configure_window), view);
    g_signal_connect (G_OBJECT(features), "notify::fullscreen",
    		      G_CALLBACK(configure_window), view);
}

static gboolean show_window (WebKitWebView *view, gpointer data)
{
    GtkWidget *parent;
    
    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));

    gtk_widget_grab_focus (GTK_WIDGET (view));
    gtk_widget_show_all (parent);
    
    return FALSE;
}

static gboolean start_download (WebKitWebView *view,
				WebKitDownload *download,
				gpointer data)
{
    GtkWidget *dialog, *parent;
    gchar *filename, *uri;
    
    filename = (gchar *)webkit_download_get_suggested_filename(download);
    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));
    dialog = gtk_file_chooser_dialog_new ("Save File",
					  GTK_WINDOW(parent),
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
    
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
						    TRUE);
    
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	uri = g_strconcat ("file://", filename, NULL);

	webkit_download_set_destination_uri (download, uri);
	webkit_download_start (download);

	g_free (filename);
	g_free (uri);
    }
    
    gtk_widget_destroy (dialog);
    
    return TRUE;
}

gboolean decide_policy (WebKitWebView *webView,
			WebKitWebFrame *frame,
			WebKitNetworkRequest *request,
			gchar *mimetype,
			WebKitWebPolicyDecision *policy_decision,
			gpointer user_data)
{
    if (webkit_web_view_can_show_mime_type(webView, mimetype)) {
	webkit_web_policy_decision_use (policy_decision);
    } else {
	webkit_web_policy_decision_download (policy_decision);
    }
    
    return TRUE;
}

static WebKitWebView *create_view (WebKitWebView *parent,
				   WebKitWebFrame *frame,
				   gpointer data)
{
    WebKitWebView *view;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW(window), "Techne");
    gtk_widget_set_name (window, "Techne");
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
    /* gtk_window_set_deletable(GTK_WINDOW(window), FALSE); */

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);
    g_object_ref(scrolled);

    view = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    {
        /* Create a new websettings and disable java script */
        WebKitWebInspector *inspector;
        WebKitWebSettings *settings = webkit_web_settings_new();
        g_object_set (G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);
        webkit_web_view_set_settings(WEBKIT_WEB_VIEW(view), settings);
        
        inspector = webkit_web_view_get_inspector(view);
        g_signal_connect (G_OBJECT (inspector), "inspect-web-view",
			  G_CALLBACK (create_inspector), NULL);
        g_signal_connect (G_OBJECT (inspector), "show_window",
			  G_CALLBACK (show_inspector), NULL);
        g_signal_connect (G_OBJECT (inspector), "close_window",
			  G_CALLBACK (close_inspector), NULL);
        g_signal_connect (G_OBJECT (inspector), "attach_window",
			  G_CALLBACK (attach_inspector), NULL);
        g_signal_connect (G_OBJECT (inspector), "detach_window",
			  G_CALLBACK (detach_inspector), NULL);
        g_signal_connect (G_OBJECT (inspector), "finished",
			  G_CALLBACK (finish_inspector), NULL);
    }

    connect_features (view, NULL, NULL);

    g_signal_connect (view, "notify::title",
    		      G_CALLBACK (notify_title), NULL);
    g_signal_connect (view, "notify::progress",
    		      G_CALLBACK (notify_progress), NULL);
    g_signal_connect (view, "notify::window-features",
    		      G_CALLBACK (connect_features), NULL);
    g_signal_connect (view, "create-web-view",
		      G_CALLBACK (create_view), NULL);
    g_signal_connect (view, "web-view-ready",
    		      G_CALLBACK (show_window), NULL);
    g_signal_connect (view, "close-web-view",
    		      G_CALLBACK (close_view), NULL);
    g_signal_connect (view, "download-requested",
    		      G_CALLBACK (start_download), NULL);
    g_signal_connect (view, "mime-type-policy-decision-requested",
    		      G_CALLBACK (decide_policy), NULL);

    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (window), scrolled);

    return view;
}

int main (int argc, char *argv[])
{
    WebKitWebView *view;
    GtkWidget *window;
    char *uri = NULL, *geometry = NULL;
    int i;
    
    gtk_init (&argc, &argv);

    for (i = 1 ; i < argc ; i += 1) {
	if (!strcmp (argv[i], "--geometry") && i < argc - 1) {
	    geometry = argv[i + 1];
	    i += 1;
	} else if (!uri && (argv[i][0] != '-')) {
	    uri = argv[i];
	} else {
	    fprintf (stderr, "usage: %s [--geometry specification] [url]\n",
		     argv[0]);

	    return 1;
	}
    }

    view = create_view (NULL, NULL, NULL);
    
    window = gtk_widget_get_toplevel (GTK_WIDGET (view));

    g_signal_connect (window, "delete-event",
		      G_CALLBACK (gtk_main_quit), NULL);
    
    if (geometry &&
	!gtk_window_parse_geometry (GTK_WINDOW (window), geometry)) {
        fprintf (stderr,
		 "%s: failed to parse geometry specification.\n",
		 argv[0]);
    }
    
    webkit_web_view_load_uri (view, uri ? uri : "http://localhost:29176");

    show_window (view, NULL);
    
    gtk_main ();

    return 0;
}
