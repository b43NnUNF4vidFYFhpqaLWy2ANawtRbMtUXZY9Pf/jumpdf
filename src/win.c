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
#include "viewer.h"
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
static void window_render_page(Window *win, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar);
static void window_highlight_search(Window *win, cairo_t *cr, PopplerPage *page);
static void window_draw_links(Window *win, cairo_t *cr, unsigned int from, unsigned int to);
static void window_populate_toc(Window *win);
static void window_add_toc_entries(Window *win, PopplerIndexIter *iter, int level);

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller);
static void on_scroll(GtkEventControllerScroll *controller, double dx,
                      double dy, gpointer user_data);
static void on_resize(GtkDrawingArea *area, int width, int height,
                      gpointer user_data);
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data);
static void on_search_dialog_response(GtkDialog *dialog, int response_id, Window *win);
static void on_search_entry_activate(GtkEntry *entry, GtkDialog *dialog);
static void on_toc_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
static void on_toc_search_changed(GtkSearchEntry *entry, gpointer user_data);
static void on_toc_search_stopped(GtkSearchEntry *entry, gpointer user_data);

struct _Window {
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

    GtkWidget *search_dialog;
    GtkWidget *search_content_area;
    GtkWidget *search_entry;

    GtkWidget *toc_scroll_window;
    GtkWidget *toc_box;
    GtkWidget *toc_search_entry;
    GtkWidget *toc_container;

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

    win->search_dialog = gtk_dialog_new_with_buttons("Search", GTK_WINDOW(win),
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Search", GTK_RESPONSE_ACCEPT,
        NULL);
    win->search_content_area = gtk_dialog_get_content_area(GTK_DIALOG(win->search_dialog));
    win->search_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(win->search_content_area), win->search_entry);

    g_signal_connect(win->search_entry, "activate", G_CALLBACK(on_search_entry_activate), win->search_dialog);
    g_signal_connect(win->search_dialog, "response", G_CALLBACK(on_search_dialog_response), win);

    win->toc_search_entry = gtk_search_entry_new();
    g_signal_connect(win->toc_search_entry, "search-changed", G_CALLBACK(on_toc_search_changed), win);
    g_signal_connect(win->toc_search_entry, "stop-search", G_CALLBACK(on_toc_search_stopped), win);

    win->toc_container = gtk_list_box_new();
    gtk_widget_set_hexpand(win->toc_container, TRUE);
    g_signal_connect(win->toc_container, "row-activated", G_CALLBACK(on_toc_row_activated), win);

    win->toc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(win->toc_box), win->toc_search_entry);
    gtk_box_append(GTK_BOX(win->toc_box), win->toc_container);

    win->toc_scroll_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(win->toc_scroll_window), win->toc_box);
    gtk_widget_set_visible(win->toc_scroll_window, FALSE);
    /*
    * in window_toggle_toc, gtk_box_prepend only increases the ref count
    * the first time it is called, so we need to manually increase it here
    */
    g_object_ref(win->toc_scroll_window);

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

    window_populate_toc(win);
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
    gtk_widget_show(win->search_dialog);
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
    GtkWidget *label;
    PopplerDest *dest;

    if (row != NULL) {
        label = gtk_list_box_row_get_child(row);
        dest = g_object_get_data(G_OBJECT(label), "dest");

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

GtkListBox *window_get_toc_listbox(Window *win)
{
    return GTK_LIST_BOX(win->toc_container);
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

static void window_render_page(Window *win, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar)
{
    unsigned int links_to_draw;

    // Clear to white background (for PDFs with missing background)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, win->viewer->info->pdf_width, win->viewer->info->pdf_height);
    cairo_fill(cr);

    poppler_page_render(page, cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_move_to(cr, 0, 0);
    cairo_rel_line_to(cr, win->viewer->info->pdf_width, 0);
    cairo_stroke(cr);

    window_highlight_search(win, cr, page);

    if (win->viewer->links->follow_links_mode) {
        links_to_draw = viewer_links_get_links(win->viewer->links, page);
        window_draw_links(win, cr, *links_drawn_sofar, *links_drawn_sofar + links_to_draw);
        *links_drawn_sofar += links_to_draw;
    }
}

static void window_highlight_search(Window *win, cairo_t *cr, PopplerPage *page)
{
    PopplerRectangle *highlight_rect;
    double highlight_rect_x, highlight_rect_y, highlight_rect_width,
        highlight_rect_height;
    GList *matches;

    if (!win->viewer->search->search_text) {
        return;
    }

    matches = poppler_page_find_text(page, win->viewer->search->search_text);
    for (GList *elem = matches; elem; elem = elem->next) {
        highlight_rect = elem->data;
        highlight_rect_x = highlight_rect->x1;
        highlight_rect_y = win->viewer->info->pdf_height - highlight_rect->y1;
        highlight_rect_width = highlight_rect->x2 - highlight_rect->x1;
        highlight_rect_height = highlight_rect->y1 - highlight_rect->y2;

        cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.5);
        cairo_rectangle(cr, highlight_rect_x, highlight_rect_y, highlight_rect_width,
            highlight_rect_height);
        cairo_fill(cr);
    }

    g_list_free(matches);
}

static void window_draw_links(Window *win, cairo_t *cr, unsigned int from, unsigned int to)
{
    PopplerLinkMapping *link_mapping;
    char *link_text;

    for (int i = from; i < to; i++) {
        link_mapping = g_ptr_array_index(win->viewer->links->visible_links, i);
        link_text = g_strdup_printf("%d", i + 1);

        // Outline
        cairo_move_to(cr, link_mapping->area.x1, win->viewer->info->pdf_height - link_mapping->area.y1);
        cairo_text_path(cr, link_text);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke_preserve(cr);

        // Actual text
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_fill(cr);

        g_free(link_text);
    }
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
        GdkModifierType state = gdk_event_get_modifier_state(event);
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
    Window *win;
    PopplerPage *page;
    cairo_surface_t *surface;
    cairo_t *cr_pdf;
    double visible_pages;
    unsigned int visible_pages_before, visible_pages_after;
    unsigned int links_drawn_sofar = 0;

    win = (Window *)user_data;
    if (!win->viewer->info->doc || !win->viewer->info->pages) {
        return;
    }

    page = win->viewer->info->pages[win->viewer->cursor->current_page];
    poppler_page_get_size(page, &win->viewer->info->pdf_width, &win->viewer->info->pdf_height);

    if (win->viewer->cursor->center_mode) {
        viewer_cursor_center(win->viewer->cursor);
    }

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, win->viewer->info->view_width,
        win->viewer->info->view_height);
    cr_pdf = cairo_create(surface);

    cairo_translate(cr_pdf, win->viewer->info->view_width / 2.0, win->viewer->info->view_height / 2.0);
    cairo_scale(cr_pdf, win->viewer->cursor->scale, win->viewer->cursor->scale);
    cairo_translate(cr_pdf, -win->viewer->info->view_width / 2.0, -win->viewer->info->view_height / 2.0);

    cairo_translate(cr_pdf, win->viewer->cursor->x_offset * win->viewer->info->pdf_width / global_config->steps,
        -win->viewer->cursor->y_offset * win->viewer->info->pdf_height / global_config->steps);

    viewer_links_clear_links(win->viewer->links);
    window_render_page(win, cr_pdf, page, &links_drawn_sofar);

    visible_pages = win->viewer->info->view_height / (win->viewer->cursor->scale * win->viewer->info->pdf_height) - 1;
    visible_pages_before = ceil(visible_pages / 2);
    visible_pages_after = ceil(visible_pages / 2) + 1;

    cairo_save(cr_pdf);
    for (int i = 1; i <= visible_pages_before; i++) {
        if (win->viewer->cursor->current_page - i >= 0) {
            cairo_translate(cr_pdf, 0, -win->viewer->info->pdf_height);
            window_render_page(win, cr_pdf, win->viewer->info->pages[win->viewer->cursor->current_page - i], &links_drawn_sofar);
        }
    }
    cairo_restore(cr_pdf);
    cairo_save(cr_pdf);
    for (int i = 1; i <= visible_pages_after; i++) {
        if (win->viewer->cursor->current_page + i < win->viewer->info->n_pages) {
            cairo_translate(cr_pdf, 0, win->viewer->info->pdf_height);
            window_render_page(win, cr_pdf, win->viewer->info->pages[win->viewer->cursor->current_page + i], &links_drawn_sofar);
        }
    }
    cairo_restore(cr_pdf);

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr_pdf);
    cairo_surface_destroy(surface);
}

static void on_search_dialog_response(GtkDialog *dialog, int response_id, Window *win)
{
    GtkWidget *content_area;
    GtkWidget *entry;
    GtkEntryBuffer *buffer;
    const char *search_text;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        content_area = gtk_dialog_get_content_area(dialog);
        entry = gtk_widget_get_first_child(content_area);
        buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
        search_text = gtk_entry_buffer_get_text(buffer);

        win->viewer->search->search_text = strdup(search_text);
        window_redraw_all_windows(win);
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

static void on_search_entry_activate(GtkEntry *entry, GtkDialog *dialog)
{
    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
}

static void window_populate_toc(Window *win)
{
    PopplerIndexIter *iter = poppler_index_iter_new(win->viewer->info->doc);
    if (iter) {
        window_add_toc_entries(win, iter, 0);
        poppler_index_iter_free(iter);
    }
}

static void window_add_toc_entries(Window *win, PopplerIndexIter *iter, int level)
{
    PopplerAction *action;
    gchar *markup;
    GtkWidget *label;
    PopplerIndexIter *child;
    PopplerDest *dest;
    GtkListBoxRow *first_row;

    while (iter) {
        action = poppler_index_iter_get_action(iter);
        if (action && action->type == POPPLER_ACTION_GOTO_DEST) {
            markup = g_markup_printf_escaped("%*s%s", level * 2, " ", action->any.title);
            label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(label), markup);
            g_free(markup);
            gtk_label_set_xalign(GTK_LABEL(label), 0.0); // Align to the left to maintain indentation
            gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

            dest = viewer_info_get_dest(win->viewer->info, action->goto_dest.dest);
            g_object_set_data(G_OBJECT(label), "dest", poppler_dest_copy(dest));
            if (action->goto_dest.dest->type == POPPLER_DEST_NAMED) {
                // Won't be freed by poppler_action_free
                poppler_dest_free(dest);
            }

            gtk_list_box_insert(GTK_LIST_BOX(win->toc_container), label, -1);

            child = poppler_index_iter_get_child(iter);
            if (child) {
                window_add_toc_entries(win, child, level + 1);
                poppler_index_iter_free(child);
            }
        }

        poppler_action_free(action);

        if (!poppler_index_iter_next(iter)) {
            break;
        }
    }

    first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(win->toc_container), 0);
    if (first_row != NULL) {
        gtk_list_box_select_row(GTK_LIST_BOX(win->toc_container), first_row);
    }
}

static void on_toc_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    Window *win = (Window *)user_data;
    GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(box);

    window_execute_toc_row(win, selected_row);
}

static void on_toc_search_changed(GtkSearchEntry *entry, gpointer user_data)
{
    Window *win = (Window *)user_data;
    const gchar *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(win->toc_container));
    GtkWidget *label;
    const gchar *child_text;
    gboolean visible;
    GtkListBoxRow *first_visible_row;

    while (child) {
        label = gtk_widget_get_first_child(child);
        if (GTK_IS_LABEL(label)) {
            child_text = gtk_label_get_text(GTK_LABEL(label));

            visible = strcasestr(child_text, text) != NULL;
            gtk_widget_set_visible(child, visible);
        }

        child = gtk_widget_get_next_sibling(child);
    }

    first_visible_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(win->toc_container), 0);
    while (first_visible_row != NULL && !gtk_widget_get_visible(GTK_WIDGET(first_visible_row))) {
        first_visible_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(win->toc_container), gtk_list_box_row_get_index(first_visible_row) + 1);
    }
    if (first_visible_row != NULL) {
        gtk_list_box_select_row(GTK_LIST_BOX(win->toc_container), first_visible_row);
    }
}

static void on_toc_search_stopped(GtkSearchEntry *entry, gpointer user_data)
{
    Window *win = (Window *)user_data;

    gtk_widget_grab_focus(win->toc_scroll_window);
}