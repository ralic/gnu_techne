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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>

struct context {
    GtkWidget *window, *inspector_window, *scrolled, *paned;
    int attached, open;
};

static GtkWindowGroup *group;
static int hidden;

static JSValueRef hide_browser(JSContextRef context,
                               JSObjectRef function,
                               JSObjectRef thisObject,
                               size_t argumentCount,
                               const JSValueRef arguments[],
                               JSValueRef *exception)
{
    GList *toplevel = gtk_window_group_list_windows (group);

    for (; toplevel ; toplevel = toplevel->next) {
        gtk_widget_hide_all(GTK_WIDGET(toplevel->data));
    }

    hidden = 1;

    return JSValueMakeUndefined(context);
}

static JSValueRef show_browser(JSContextRef context,
                               JSObjectRef function,
                               JSObjectRef thisObject,
                               size_t argumentCount,
                               const JSValueRef arguments[],
                               JSValueRef *exception)
{
    GList *toplevel = gtk_window_group_list_windows (group);

    for (; toplevel ; toplevel = toplevel->next) {
        gtk_widget_show_all(GTK_WIDGET(toplevel->data));
    }

    hidden = 0;

    return JSValueMakeUndefined(context);
}

static void window_object_cleared(WebKitWebView *web_view,
                                  WebKitWebFrame *frame,
                                  gpointer context,
                                  gpointer window_object,
                                  gpointer user_data)

{
    const JSStaticFunction browser_staticfuncs[] = {
        {"hide", hide_browser, kJSPropertyAttributeReadOnly},
        {"show", show_browser, kJSPropertyAttributeReadOnly},
        {NULL, NULL, 0}
    };

    const JSClassDefinition class_def = {
        0, kJSClassAttributeNone, "Browser",
        NULL, NULL, browser_staticfuncs,
        NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL
    };

    /* Add Browser class to JavaScriptCore */

    JSClassRef classDef = JSClassCreate(&class_def);
    JSObjectRef classObj = JSObjectMake(context, classDef, context);
    JSObjectRef globalObj = JSContextGetGlobalObject(context);
    JSStringRef str = JSStringCreateWithUTF8CString("Browser");
    JSObjectSetProperty(context, globalObj, str, classObj,
                        kJSPropertyAttributeNone, NULL);
}

static gboolean show_inspector (WebKitWebInspector *inspector,
                                struct context *context)
{
    WebKitWebView *inspector_view;

    inspector_view = webkit_web_inspector_get_web_view(inspector);

    if (context->attached) {
        gtk_container_remove(GTK_CONTAINER(context->window),
                             GTK_WIDGET(context->scrolled));

        gtk_paned_add1(GTK_PANED(context->paned),
                       GTK_WIDGET(inspector_view));
        gtk_paned_add2(GTK_PANED(context->paned),
                       GTK_WIDGET(context->scrolled));
        gtk_container_add(GTK_CONTAINER(context->window),
                          GTK_WIDGET(context->paned));
    } else {
        gtk_container_add(GTK_CONTAINER(context->inspector_window),
                          GTK_WIDGET(inspector_view));

        if (!hidden) {
            gtk_widget_show_all(GTK_WIDGET(context->inspector_window));
        }
    }

    context->open = 1;

    return TRUE;
}

static gboolean close_inspector (WebKitWebInspector *inspector,
                                 struct context *context)
{
    GtkWidget *inspector_view;

    if(!context->open) {
        return TRUE;
    }

    if (context->attached) {
        inspector_view = gtk_paned_get_child1(GTK_PANED(context->paned));

        if (inspector_view) {
            gtk_container_remove(GTK_CONTAINER(context->paned),
                                 inspector_view);
        }

        gtk_container_remove(GTK_CONTAINER(context->paned),
                             context->scrolled);

        if (gtk_bin_get_child(GTK_BIN(context->window))) {
            gtk_container_remove(GTK_CONTAINER(context->window),
                                 context->paned);
        }

        gtk_container_add(GTK_CONTAINER(context->window),
                          GTK_WIDGET(context->scrolled));
    } else {
        inspector_view = gtk_bin_get_child(GTK_BIN(context->inspector_window));
        gtk_container_remove(GTK_CONTAINER(context->inspector_window),
                             inspector_view);
        gtk_widget_hide_all(GTK_WIDGET(context->inspector_window));
    }

    context->attached = 0;
    context->open = 0;

    return TRUE;
}

gboolean delete_inspector_window (GtkWidget *widget,
                                  GdkEvent *event,
                                  struct context *context)
{
    GtkWidget *content_view;
    WebKitWebInspector *inspector;

    content_view = gtk_bin_get_child (GTK_BIN(widget));
    inspector = webkit_web_view_get_inspector (WEBKIT_WEB_VIEW(content_view));
    close_inspector (inspector, context);

    return TRUE;
}

static WebKitWebView *create_inspector (WebKitWebInspector *inspector,
                                        WebKitWebView *content_view,
                                        struct context *context)
{
    WebKitWebView *inspector_view;

    inspector_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    context->paned = gtk_vpaned_new();

    g_object_ref (inspector_view);
    g_object_ref (context->paned);

    context->inspector_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW(context->inspector_window),
                          "Web inspector");
    gtk_widget_set_name (context->inspector_window, "Web inspector");
    gtk_window_set_default_size (GTK_WINDOW (context->inspector_window),
                                 800, 300);
    gtk_window_group_add_window (group,
                                 GTK_WINDOW (context->inspector_window));

    g_signal_connect(G_OBJECT(context->inspector_window),
                     "delete-event",
                     G_CALLBACK(delete_inspector_window),
                     context);

    return inspector_view;
}

static gboolean attach_inspector (WebKitWebInspector *inspector,
                                  struct context *context)
{
    assert (!context->attached);

    close_inspector(inspector, context);
    context->attached = 1;
    show_inspector(inspector, context);

    return FALSE;
}

static gboolean detach_inspector (WebKitWebInspector *inspector,
                                  struct context *context)
{
    assert (context->attached);

    close_inspector(inspector, context);
    context->attached = 0;
    show_inspector(inspector, context);

    return FALSE;
}

static gboolean finish_inspector (WebKitWebInspector *inspector,
                                  struct context *context)
{
    WebKitWebView *inspector_view;

    inspector_view = webkit_web_inspector_get_web_view(inspector);

    g_object_unref(inspector_view);
    g_object_unref(context->paned);
    gtk_widget_destroy(GTK_WIDGET(inspector_view));
    gtk_widget_destroy(context->paned);

    return FALSE;
}

static gboolean close_view(WebKitWebView *view,
                           struct context *context)
{
    GtkWidget *parent;

    parent = gtk_widget_get_toplevel(GTK_WIDGET(view));
    assert (context->window == parent);
    gtk_widget_destroy (parent);
    free (context);

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

    if (!hidden) {
        gtk_widget_show_all (parent);
    }

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

static gboolean decide_policy (WebKitWebView *webView,
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
    struct context *context;
    WebKitWebView *view;
    WebKitWebInspector *inspector;
    WebKitWebSettings *settings = webkit_web_settings_new();

    context = (struct context *)calloc (1, sizeof(struct context));

    context->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW(context->window), "Techne");
    gtk_widget_set_name (context->window, "Techne");
    gtk_window_set_default_size (GTK_WINDOW (context->window), 800, 600);
    gtk_window_group_add_window (group, GTK_WINDOW (context->window));
    /* gtk_window_set_deletable(GTK_WINDOW(context->window), FALSE); */

    context->scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (context->scrolled),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    g_object_ref(context->scrolled);

    view = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    /* Enable and setup the inspector. */
    g_object_set (G_OBJECT(settings),
                  "enable-developer-extras", TRUE, NULL);
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(view), settings);

    inspector = webkit_web_view_get_inspector(view);
    g_signal_connect (G_OBJECT (inspector), "inspect-web-view",
                      G_CALLBACK (create_inspector), context);
    g_signal_connect (G_OBJECT (inspector), "show_window",
                      G_CALLBACK (show_inspector), context);
    g_signal_connect (G_OBJECT (inspector), "close_window",
                      G_CALLBACK (close_inspector), context);
    g_signal_connect (G_OBJECT (inspector), "attach_window",
                      G_CALLBACK (attach_inspector), context);
    g_signal_connect (G_OBJECT (inspector), "detach_window",
                      G_CALLBACK (detach_inspector), context);
    g_signal_connect (G_OBJECT (inspector), "finished",
                      G_CALLBACK (finish_inspector), context);

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
                      G_CALLBACK (close_view), context);
    g_signal_connect (view, "download-requested",
                      G_CALLBACK (start_download), NULL);
    g_signal_connect (view, "mime-type-policy-decision-requested",
                      G_CALLBACK (decide_policy), NULL);

    g_signal_connect (view, "window-object-cleared",
                      G_CALLBACK(window_object_cleared), view);

    /* Place the WebKitWebView in the GtkScrolledWindow */
    gtk_container_add (GTK_CONTAINER (context->scrolled),
                       GTK_WIDGET (view));
    gtk_container_add (GTK_CONTAINER (context->window),
                       context->scrolled);

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
        } else if (!strcmp (argv[i], "--hidden")) {
            hidden = 1;
        } else if (!uri && (argv[i][0] != '-')) {
            uri = argv[i];
        } else {
            fprintf (stderr, "usage: %s [--geometry specification] [url]\n",
                     argv[0]);

            return 1;
        }
    }

    group = gtk_window_group_new();
    view = create_view (NULL, NULL, NULL);

    window = gtk_widget_get_toplevel (GTK_WIDGET(view));

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
