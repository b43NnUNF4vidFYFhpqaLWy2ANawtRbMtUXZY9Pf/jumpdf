#define _GNU_SOURCE
#include <stdio.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <poppler.h>
#include <stdbool.h>

#include "app.h"
#include "config.h"
#include "keys.h"
#include "win.h"
#include "viewer.h"
#include "input_FSM.h"

static void window_redraw(Window *win);
static void window_update_page_label(Window *win);
static void window_render_page(Window *win, cairo_t *cr, PopplerPage *page);
static void window_highlight_search(Window *win, cairo_t *cr, PopplerPage *page);

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

struct _Window {
  GtkApplicationWindow parent;

  GtkEventController *event_controller;
  GtkEventController *scroll_controller;

  GtkWidget *page_label;
  GtkWidget *header_bar;
  GtkWidget *view;

  Viewer* viewer;
  InputState current_input_state;
};

G_DEFINE_TYPE(Window, window, GTK_TYPE_APPLICATION_WINDOW)

static void window_init(Window *win) {
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

  win->page_label = gtk_label_new(NULL);
  win->header_bar = gtk_header_bar_new();
  gtk_header_bar_pack_start(GTK_HEADER_BAR(win->header_bar), win->page_label);
  gtk_window_set_titlebar(GTK_WINDOW(win), win->header_bar);

  win->view = gtk_drawing_area_new();
  gtk_widget_set_hexpand(win->view, TRUE);
  gtk_widget_set_vexpand(win->view, TRUE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(win->view), draw_function,
                                 win, NULL);
  gtk_window_set_child(GTK_WINDOW(win), win->view);
  g_signal_connect(win->view, "resize", G_CALLBACK(on_resize), win);

  gtk_window_set_title(GTK_WINDOW(win), "Jumpdf");
}

static void window_finalize(GObject *object) {
  Window *win;
  win = (Window *)object;

  if (win->viewer) {
    viewer_destroy(win->viewer);
  }
  
  G_OBJECT_CLASS(window_parent_class)->finalize(object);
}

static void window_class_init(WindowClass *class) {
  G_OBJECT_CLASS(class)->finalize = window_finalize;
}

Window *window_new(App *app) {
  return g_object_new(WINDOW_TYPE, "application", app, NULL);
}

void window_open(Window *win, GFile *file) {
  GError *err;
  PopplerDocument* doc;
  double default_width, default_height;

  err = NULL;

  doc = poppler_document_new_from_gfile(file, NULL, NULL, &err);

  if (!doc) {
    g_printerr("Poppler: %s\n", err->message);
    g_error_free(err);
  } else {
    win->viewer = viewer_new(doc);

    window_update_page_label(win);
    gtk_window_set_title(GTK_WINDOW(win), g_file_get_basename(file));
    poppler_page_get_size(win->viewer->pages[0], &default_width, &default_height);
    gtk_window_set_default_size(GTK_WINDOW(win), (int)default_width,
                                (int)default_width);
  }
}

void window_show_search_dialog(Window *win) {
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *entry;

  dialog = gtk_dialog_new_with_buttons("Search", GTK_WINDOW(win),
                                       GTK_DIALOG_MODAL,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Search", GTK_RESPONSE_ACCEPT,
                                       NULL);
  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  entry = gtk_entry_new();
  gtk_box_append(GTK_BOX(content_area), entry);

  g_signal_connect(entry, "activate", G_CALLBACK(on_search_entry_activate), dialog);
  g_signal_connect(dialog, "response", G_CALLBACK(on_search_dialog_response), win);

  gtk_widget_show(dialog);
}

Viewer *window_get_viewer(Window *win) {
  return win->viewer;
}

static void window_redraw(Window *win) {
  window_update_page_label(win);
  gtk_widget_queue_draw(win->view);
}
static void window_update_page_label(Window *win) {
  char *page_str;

  if (asprintf(&page_str, "%d/%d", win->viewer->current_page + 1, win->viewer->n_pages) != -1) {
    gtk_label_set_text(GTK_LABEL(win->page_label), page_str);
    free(page_str);
  } else {
    g_printerr("Error: Failed to update page label");
  }
}

static void window_render_page(Window *win, cairo_t *cr, PopplerPage *page) {
  poppler_page_render(page, cr);
  window_highlight_search(win, cr, page);
}

static void window_highlight_search(Window *win, cairo_t *cr, PopplerPage *page) {
  PopplerRectangle *highlight_rect;
  double highlight_rect_x, highlight_rect_y, highlight_rect_width,
      highlight_rect_height;
  GList *matches;

  if (!win->viewer->search_text) {
    return;
  }

  matches = poppler_page_find_text(page, win->viewer->search_text);
  for (GList *elem = matches; elem; elem = elem->next) {
    highlight_rect = elem->data;
    highlight_rect_x = highlight_rect->x1;
    highlight_rect_y = win->viewer->pdf_height - highlight_rect->y1;
    highlight_rect_width = highlight_rect->x2 - highlight_rect->x1;
    highlight_rect_height = highlight_rect->y1 - highlight_rect->y2;

    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.5);
    cairo_rectangle(cr, highlight_rect_x, highlight_rect_y, highlight_rect_width,
                    highlight_rect_height);
    cairo_fill(cr);
  }

  g_list_free(matches);
}

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller) {
  g_print("keyval = %d, keycode = %d, state = %d\n", keyval, keycode, state);

  Window *win;
  win = (Window *)user_data;

  win->current_input_state = execute_state(win->current_input_state, win, keyval);
  g_print("%d\n", win->current_input_state);
  viewer_handle_offset_update(win->viewer);
  window_redraw(win);

  return TRUE;
}

static void on_scroll(GtkEventControllerScroll *controller, double dx,
                      double dy, gpointer user_data) {
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
      viewer_set_scale(win->viewer, MAX(MIN_SCALE, win->viewer->scale - dy * SCALE_STEP));
      break;
    default:
      win->viewer->x_offset -= dx;
      win->viewer->y_offset += dy;

      viewer_handle_offset_update(win->viewer);
    }
  }

  window_redraw(win);
}

static void on_resize(GtkDrawingArea *area, int width, int height,
                      gpointer user_data) {
  Window *win;

  win = (Window *)user_data;

  win->viewer->view_width = width;
  win->viewer->view_height = height;
}

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data) {
  Window *win;
  PopplerPage *page;
  cairo_surface_t *surface;
  cairo_t *cr_pdf;
  double center_x_offset, background_x, background_width, background_y,
      background_height;
  double visible_pages;
  unsigned int visible_pages_before, visible_pages_after;

  win = (Window *)user_data;
  if (!win->viewer->doc || !win->viewer->pages) {
    return;
  }

  page = win->viewer->pages[win->viewer->current_page];
  poppler_page_get_size(page, &win->viewer->pdf_width, &win->viewer->pdf_height);

  if (win->viewer->center_mode) {
    viewer_center(win->viewer);
  }

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, win->viewer->view_width,
                                       win->viewer->view_height);
  cr_pdf = cairo_create(surface);

  // Clear to white background (for PDFs with missing background)
  center_x_offset = ((win->viewer->view_width / 2.0) - (win->viewer->pdf_width / 2.0)) /
             (win->viewer->pdf_width / STEPS);
  // First term gets you x-coordinate of left side of PDF as if it was centered
  // (2*margin + real_pdf_width = view_width, where margin = center_x_offset),
  // then second term moves it by the offset from center,
  // i.e. x_offset - center_x_offset
  background_x =
      (win->viewer->view_width - win->viewer->scale * win->viewer->pdf_width) / 2 +
      ((win->viewer->x_offset - center_x_offset) / STEPS) * win->viewer->scale * win->viewer->pdf_width;
  background_y = 0;
  background_width = win->viewer->scale * win->viewer->pdf_width;
  background_height = win->viewer->view_height;
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_rectangle(cr, background_x, background_y, background_width,
                  background_height);
  cairo_fill(cr);

  cairo_translate(cr_pdf, win->viewer->view_width / 2.0, win->viewer->view_height / 2.0);
  cairo_scale(cr_pdf, win->viewer->scale, win->viewer->scale);
  cairo_translate(cr_pdf, -win->viewer->view_width / 2.0, -win->viewer->view_height / 2.0);

  cairo_translate(cr_pdf, win->viewer->x_offset * win->viewer->pdf_width / STEPS,
                          -win->viewer->y_offset * win->viewer->pdf_height / STEPS);

  window_render_page(win, cr_pdf, page);

  visible_pages = win->viewer->view_height / (win->viewer->scale * win->viewer->pdf_height) - 1;
  visible_pages_before = ceil(visible_pages / 2);
  visible_pages_after = ceil(visible_pages / 2) + 1;

  cairo_save(cr_pdf);
  for (int i = 1; i <= visible_pages_before; i++) {
    if (win->viewer->current_page - i >= 0) {
      cairo_translate(cr_pdf, 0, -win->viewer->pdf_height);
      window_render_page(win, cr_pdf, win->viewer->pages[win->viewer->current_page - i]);
    }
  }
  cairo_restore(cr_pdf);
  cairo_save(cr_pdf);
  for (int i = 1; i <= visible_pages_after; i++) {
    if (win->viewer->current_page + i < win->viewer->n_pages) {
      cairo_translate(cr_pdf, 0, win->viewer->pdf_height);
      window_render_page(win, cr_pdf, win->viewer->pages[win->viewer->current_page + i]);
    }
  }
  cairo_restore(cr_pdf);

  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr_pdf);
  cairo_surface_destroy(surface);
}

static void on_search_dialog_response(GtkDialog *dialog, int response_id, Window *win) {
  GtkWidget *content_area;
  GtkWidget *entry;
  GtkEntryBuffer *buffer;
  const char* search_text;

  if (response_id == GTK_RESPONSE_ACCEPT) {
    content_area = gtk_dialog_get_content_area(dialog);
    entry = gtk_widget_get_first_child(content_area);
    buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
    search_text = gtk_entry_buffer_get_text(buffer);

    win->viewer->search_text = strdup(search_text);
    window_redraw(win);
  }

  gtk_window_destroy(GTK_WINDOW(dialog)); // Destroy the dialog
}

static void on_search_entry_activate(GtkEntry *entry, GtkDialog *dialog) {
  gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
}