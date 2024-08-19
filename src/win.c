#define _GNU_SOURCE
#include <stdio.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <poppler.h>
#include <stdbool.h>

#include "win.h"
#include "project_config.h"
#include "app.h"
#include "config.h"
#include "toc.h"
#include "viewer.h"
#include "render.h"
#include "viewer_mark_manager.h"
#include "viewer_info.h"
#include "viewer_cursor.h"
#include "viewer_search.h"
#include "viewer_links.h"
#include "input_FSM.h"

// TODO: Load from file or resource
static const char *css = 
    ".statusline {"
    "    background-color: black;"
    "}";

static void window_update_cursors(Window *win);
static void window_redraw_all_windows(Window *win);
static void window_update_statusline(Window *win);
static void window_init_toc(Window *win);

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller);
static void on_scroll(GtkEventControllerScroll *controller, double dx,
                      double dy, gpointer user_data);
static void on_resize(GtkDrawingArea *area, int width, int height,
                      gpointer user_data);
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data);
static void on_search_entry_activate(GtkEntry *entry, gpointer user_data);
static gboolean on_search_window_key_press(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void on_toc_row_activated(GtkListView *list_view, guint position, gpointer user_data);
static void on_toc_search_changed(GtkSearchEntry *entry, gpointer user_data);
static void on_toc_search_stopped(GtkSearchEntry *entry, gpointer user_data);
static GListModel* get_children(GObject *item, gpointer user_data);
static void on_setup(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data);
static void on_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data);

struct _Window {
    // TODO: Breakdown into separate structs
    GtkApplicationWindow parent;

    GtkEventController *event_controller;
    GtkEventController *scroll_controller;

    GtkWidget *main_container;

    GtkWidget *view_box;
    GtkWidget *view;

    GtkWidget *statusline;
    GtkWidget *statusline_left;
    GtkWidget *statusline_middle;
    GtkWidget *statusline_right;
    GtkWidget *left_label;
    GtkWidget *middle_label;
    GtkWidget *right_label;

    GtkWidget *toc_scroll_window;
    GtkWidget *toc_box;
    GtkWidget *toc_search_entry;

    GListModel *toc_list_model;
    GtkTreeListModel *toc_tree_list_model;
    GtkWidget *toc_list_view;

    GtkWidget *search_window;
    GtkWidget *search_box;
    GtkWidget *search_entry;
    GtkEventController *search_event_controller;

    /*
    This reference to App is necessary, as the Window is not associated
    with a GtkApplication in the destruction phase, making
    gtk_window_get_application not viable.
    */
    App *app;
    ViewerMarkManager *mark_manager;
    Viewer *viewer;
    InputState current_input_state;
};

G_DEFINE_TYPE(Window, window, GTK_TYPE_APPLICATION_WINDOW)

static void window_init(Window *win)
{
    GtkCssProvider *css_provider;

    win->viewer = NULL;
    win->current_input_state = STATE_NORMAL;

    win->event_controller = gtk_event_controller_key_new();
    g_signal_connect_object(win->event_controller, "key-pressed",
        G_CALLBACK(on_key_pressed), win, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(GTK_WIDGET(win), win->event_controller);

    win->scroll_controller =
        gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(win->scroll_controller, "scroll", G_CALLBACK(on_scroll),
        win);
    gtk_widget_add_controller(GTK_WIDGET(win),
        GTK_EVENT_CONTROLLER(win->scroll_controller));

    win->view = gtk_drawing_area_new();
    gtk_widget_set_hexpand(win->view, TRUE);
    gtk_widget_set_vexpand(win->view, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(win->view), draw_function,
        win, NULL);
    g_signal_connect(win->view, "resize", G_CALLBACK(on_resize), win);

    win->statusline = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(win->statusline, "statusline");

    win->statusline_left = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(win->statusline_left, FALSE);
    gtk_widget_set_halign(win->statusline_left, GTK_ALIGN_START);
    win->left_label = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(win->statusline_left), win->left_label);

    win->statusline_middle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(win->statusline_middle, TRUE);
    gtk_widget_set_halign(win->statusline_middle, GTK_ALIGN_CENTER);
    win->middle_label = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(win->statusline_middle), win->middle_label);

    win->statusline_right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(win->statusline_right, FALSE);
    gtk_widget_set_halign(win->statusline_right, GTK_ALIGN_END);
    win->right_label = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(win->statusline_right), win->right_label);

    gtk_box_append(GTK_BOX(win->statusline), win->statusline_left);
    gtk_box_append(GTK_BOX(win->statusline), win->statusline_middle);
    gtk_box_append(GTK_BOX(win->statusline), win->statusline_right);

    win->view_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(win->view_box, TRUE);
    gtk_widget_set_vexpand(win->view_box, TRUE);
    gtk_box_append(GTK_BOX(win->view_box), win->view);
    gtk_box_append(GTK_BOX(win->view_box), win->statusline);

    win->toc_search_entry = gtk_search_entry_new();
    g_signal_connect(win->toc_search_entry, "search-changed", G_CALLBACK(on_toc_search_changed), win);
    g_signal_connect(win->toc_search_entry, "stop-search", G_CALLBACK(on_toc_search_stopped), win);

    win->toc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(win->toc_box, TRUE);
    gtk_box_append(GTK_BOX(win->toc_box), win->toc_search_entry);

    win->toc_scroll_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(win->toc_scroll_window), win->toc_box);
    gtk_widget_set_visible(win->toc_scroll_window, FALSE);
    /*
    * in window_toggle_toc, gtk_box_prepend only increases the ref count
    * the first time it is called, so we need to manually increase it here
    */
    g_object_ref(win->toc_scroll_window);

    win->search_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win->search_window), "Search");
    gtk_window_set_modal(GTK_WINDOW(win->search_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(win->search_window), GTK_WINDOW(win));
    gtk_window_set_hide_on_close(GTK_WINDOW(win->search_window), TRUE);

    win->search_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(win->search_window), win->search_box);

    win->search_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(win->search_box), win->search_entry);
    g_signal_connect(win->search_entry, "activate", G_CALLBACK(on_search_entry_activate), win);

    win->search_event_controller = gtk_event_controller_key_new();
    g_signal_connect(win->search_event_controller, "key-pressed",
        G_CALLBACK(on_search_window_key_press), win);
    gtk_widget_add_controller(win->search_window, win->search_event_controller);

    win->main_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(win), win->main_container);
    gtk_box_append(GTK_BOX(win->main_container), win->view_box);

    gtk_window_set_title(GTK_WINDOW(win), APP_NAME_STR);

    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

static void window_finalize(GObject *object)
{
    Window *win = (Window *)object;

    if (win->viewer) {
        // Cursor already destroyed by mark_manager destruction
        viewer_info_destroy(win->viewer->info);
        free(win->viewer->info);

        viewer_search_destroy(win->viewer->search);
        free(win->viewer->search);

        viewer_links_destroy(win->viewer->links);
        free(win->viewer->links);

        free(win->viewer);
    }

    app_remove_window(win->app, win);

    G_OBJECT_CLASS(window_parent_class)->finalize(object);
}

static void window_class_init(WindowClass *class)
{
    G_OBJECT_CLASS(class)->finalize = window_finalize;
}

Window *window_new(App *app)
{
    Window *win = g_object_new(WINDOW_TYPE, "application", app, NULL);
    win->app = app;

    return win;
}

void window_open(Window *win, GFile *file, ViewerMarkManager *mark_manager)
{
    GError *err = NULL;
    ViewerCursor *cursor;
    ViewerSearch *search;
    ViewerLinks *links;
    GFileInfo *file_info;
    double default_width, default_height;

    cursor = viewer_mark_manager_get_current_cursor(mark_manager);
    search = viewer_search_new();
    links = viewer_links_new();

    win->mark_manager = mark_manager;
    win->viewer = viewer_new(cursor->info, cursor, search, links);

    window_init_toc(win);
    window_update_statusline(win);

    file_info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, G_FILE_QUERY_INFO_NONE, NULL, &err);
    if (file_info == NULL) {
        g_printerr("GFile: %s\n", err->message);
        g_error_free(err);
    } else {
        gtk_window_set_title(GTK_WINDOW(win), g_file_info_get_display_name(file_info));
        g_object_unref(file_info);
    }

    poppler_page_get_size(win->viewer->info->pages[0], &default_width, &default_height);
    gtk_window_set_default_size(GTK_WINDOW(win), (int)default_width,
        (int)default_width);
}

void window_update_cursor(Window *win)
{
    win->viewer->cursor = viewer_mark_manager_get_current_cursor(win->mark_manager);
    viewer_cursor_handle_offset_update(win->viewer->cursor);
}

void window_redraw(Window *win)
{
    window_update_statusline(win);
    gtk_widget_queue_draw(win->view);
}

void window_show_search_dialog(Window *win)
{
    gtk_window_present(GTK_WINDOW(win->search_window));
}

void window_toggle_toc(Window *win)
{
    gboolean is_visible = gtk_widget_get_visible(win->toc_scroll_window);

    gtk_widget_set_visible(win->toc_scroll_window, !is_visible);

    // It is necessary to remove from main_container in addition,
    // otherwise it will still occupy space
    if (is_visible) {
        g_object_ref(win->toc_scroll_window);
        gtk_box_remove(GTK_BOX(win->main_container), win->toc_scroll_window);
    } else {
        gtk_box_prepend(GTK_BOX(win->main_container), win->toc_scroll_window);
        g_object_unref(win->toc_scroll_window);
    }
}

void window_focus_toc_search(Window *win)
{
    gtk_widget_grab_focus(win->toc_search_entry);
}

void window_execute_toc_row(Window *win, GtkListBoxRow *row)
{
    GtkWidget *toc_entry_box;
    PopplerDest *dest;

    if (row != NULL) {
        toc_entry_box = gtk_list_box_row_get_child(row);
        dest = g_object_get_data(G_OBJECT(toc_entry_box), "dest");

        if (dest) {
            viewer_cursor_goto_poppler_dest(win->viewer->cursor, dest);
            window_redraw_all_windows(win);
        } else {
            g_printerr("Error: TOC entry has no destination\n");
        }
    }
}

ViewerMarkManager *window_get_mark_manager(Window *win)
{
    return win->mark_manager;
}

Viewer *window_get_viewer(Window *win)
{
    return win->viewer;
}

static void window_update_cursors(Window *win)
{
    app_update_cursors(win->app);
}

static void window_redraw_all_windows(Window *win)
{
    app_redraw_windows(win->app);
}

static void window_update_statusline(Window *win)
{
    gchar *statusline_left_str = statusline_section_to_str(global_config->statusline_left, win);
    gchar *statusline_middle_str = statusline_section_to_str(global_config->statusline_middle, win);
    gchar *statusline_right_str = statusline_section_to_str(global_config->statusline_right, win);

    gtk_label_set_text(GTK_LABEL(win->left_label), statusline_left_str);
    gtk_label_set_text(GTK_LABEL(win->middle_label), statusline_middle_str);
    gtk_label_set_text(GTK_LABEL(win->right_label), statusline_right_str);

    g_free(statusline_left_str);
    g_free(statusline_middle_str);
    g_free(statusline_right_str);
}

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
    guint keycode, GdkModifierType state,
    GtkEventControllerKey *event_controller)
{
    Window *win;
    win = (Window *)user_data;

    win->current_input_state = execute_state(win->current_input_state, win, keyval);
    window_update_cursors(win);
    window_redraw_all_windows(win);

    return TRUE;
}

static void on_scroll(GtkEventControllerScroll *controller, double dx,
    double dy, gpointer user_data)
{
    Window *win;
    GdkEvent *event;
    GdkModifierType state;

    win = (Window *)user_data;

    event =
        gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(controller));
    if (event) {
        state = gdk_event_get_modifier_state(event);
        switch (state) {
        case GDK_CONTROL_MASK:
            viewer_cursor_set_scale(win->viewer->cursor, win->viewer->cursor->scale - dy * global_config->scale_step);
            break;
        default:
            win->viewer->cursor->x_offset -= dx;
            win->viewer->cursor->y_offset += dy;

            viewer_cursor_handle_offset_update(win->viewer->cursor);
        }
    }

    window_redraw_all_windows(win);
}

static void on_resize(GtkDrawingArea *area, int width, int height,
    gpointer user_data)
{
    Window *win;

    win = (Window *)user_data;

    win->viewer->info->view_width = width;
    win->viewer->info->view_height = height;
}

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
    int height, gpointer user_data)
{
    Window *win = (Window *)user_data;
    cairo_surface_t *surface = viewer_render(win->viewer);

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_surface_destroy(surface);
}

static void window_init_toc(Window *win) {
    GtkSingleSelection *selection_model;
    GtkListItemFactory *factory;

    win->toc_list_model = get_toc_tree(win->viewer->info->doc);

    win->toc_tree_list_model = gtk_tree_list_model_new(
        win->toc_list_model,
        FALSE,
        TRUE,
        (GtkTreeListModelCreateModelFunc)get_children,
        NULL,
        NULL
    );

    selection_model = gtk_single_selection_new(win->toc_list_model);

    factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(on_setup), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(on_bind), NULL);

    win->toc_list_view = gtk_list_view_new(GTK_SELECTION_MODEL(selection_model), factory);
    g_signal_connect(win->toc_list_view, "activate", G_CALLBACK(on_toc_row_activated), win);

    gtk_box_append(GTK_BOX(win->toc_box), win->toc_list_view);
}

static void on_search_entry_activate(GtkEntry *entry, gpointer user_data) {
    Window *win = (Window *)user_data;
    const gchar *text = gtk_editable_get_text(GTK_EDITABLE(entry));

    win->viewer->search->search_text = g_strdup(text);

    gtk_window_close(GTK_WINDOW(win->search_window));
    window_redraw(win);
}

static gboolean on_search_window_key_press(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    Window *win = (Window *)user_data;

    switch (keyval) {
    case GDK_KEY_Escape:
        gtk_window_close(GTK_WINDOW(win->search_window));
        window_redraw(win);
        return TRUE;
    default:
        return FALSE;
    }
}

static void on_toc_row_activated(GtkListView *list_view, guint position, gpointer user_data)
{
}

static void on_toc_search_changed(GtkSearchEntry *entry, gpointer user_data)
{
    Window *win = (Window *)user_data;
    const gchar *text = gtk_editable_get_text(GTK_EDITABLE(entry));
}

static void on_toc_search_stopped(GtkSearchEntry *entry, gpointer user_data)
{
    Window *win = (Window *)user_data;

    gtk_widget_grab_focus(win->toc_scroll_window);
}

static GListModel* get_children(GObject *item, gpointer user_data) {
    TOCItem *toc_item = (TOCItem *)item;

    if (!toc_item->children) {
        return NULL;
    } else {
        return G_LIST_MODEL(toc_item->children);
    }
}

static void on_setup(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkWidget *expander = gtk_tree_expander_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *title_label, *page_label;

    title_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_widget_set_hexpand(title_label, TRUE);

    page_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(page_label), 1.0);

    gtk_box_append(GTK_BOX(box), title_label);
    gtk_box_append(GTK_BOX(box), page_label);

    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), box);
    gtk_list_item_set_child(list_item, expander);
}

static void on_bind(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    GtkTreeExpander *expander = GTK_TREE_EXPANDER(gtk_list_item_get_child(list_item));
    GtkWidget *box = gtk_tree_expander_get_child(expander);
    GtkWidget *title_label = gtk_widget_get_first_child(box);
    GtkWidget *page_label = gtk_widget_get_last_child(box);
    TOCItem *toc_item = (TOCItem *)gtk_list_item_get_item(list_item);
    gchar *page_number = g_strdup_printf("%d", toc_item->page + 1);
    GtkTreeListRow *list_row = GTK_TREE_LIST_ROW(gtk_list_item_get_item(list_item));

    gtk_tree_expander_set_list_row(expander, list_row);

    gtk_label_set_text(GTK_LABEL(title_label), toc_item->title);
    gtk_label_set_text(GTK_LABEL(page_label), page_number);

    g_free(page_number);
}