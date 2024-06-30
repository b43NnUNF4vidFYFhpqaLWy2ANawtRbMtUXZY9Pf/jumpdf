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

static void window_redraw(Window *win);
static void window_update_page_label(Window *win);

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller);
static void on_scroll(GtkEventControllerScroll *controller, double dx,
                      double dy, gpointer user_data);
static void on_resize(GtkDrawingArea *area, int width, int height,
                      gpointer user_data);
static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data);

struct _Window {
  GtkApplicationWindow parent;

  GtkEventController *event_controller;
  GtkEventController *scroll_controller;

  GtkWidget *page_label;
  GtkWidget *header_bar;
  GtkWidget *view;

  Viewer* viewer;
};

G_DEFINE_TYPE(Window, window, GTK_TYPE_APPLICATION_WINDOW)

static void window_init(Window *win) {
  win->viewer = NULL;

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

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller) {
  g_print("keyval = %d, keycode = %d, state = %d\n", keyval, keycode, state);

  Window *win;
  win = (Window *)user_data;

  viewer_handle_key(win->viewer, keyval);
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
  double center_x, background_x, background_width, background_y,
      background_height;
  int i;

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
  center_x = ((win->viewer->view_width / 2.0) - (win->viewer->pdf_width / 2.0)) /
             (win->viewer->pdf_width / STEPS);
  // First term gets you x-coordinate of left side of PDF as if it was centered,
  // then second term moves it by the offset from center,
  // i.e. win->x_offset - center_x
  background_x =
      (win->viewer->view_width - win->viewer->scale * win->viewer->pdf_width) / 2 +
      ((win->viewer->x_offset - center_x) / STEPS) * win->viewer->scale * win->viewer->pdf_width;
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

  poppler_page_render(page, cr_pdf);

  // for (int j = 0; j < win->n_pages; j++) {
  //   PopplerPage *search_page = win->pages[j];
  //   GList *matches = poppler_page_find_text(search_page, "the");
  //   PopplerRectangle *match_rect;
  //   double match_rect_x, match_rect_y, match_rect_width, match_rect_height;
  //
  //   for (GList * elem = matches; elem; elem = elem->next) {
  //     match_rect = elem->data;
  //     match_rect_x = background_x + match_rect->x1;
  //     match_rect_y = win->pdf_width - match_rect->y1;
  //     match_rect_width = match_rect->x2 - match_rect->x1;
  //     match_rect_height = match_rect->y2 - match_rect->y1;
  //
  //     cairo_set_source_rgba(cr_pdf, 0.0, 1.0, 0.0, 0.5);
  //     cairo_rectangle(cr_pdf, match_rect_x, match_rect_y, match_rect_width,
  //     match_rect_height);
  //     cairo_fill(cr_pdf);
  //   }
  //
  //   g_list_free(matches);
  // }

  double real_y = win->viewer->y_offset * win->viewer->pdf_width / STEPS * win->viewer->scale;

  cairo_save(cr_pdf);

  // Render pages before current that can be seen.
  i = 1;
  do {
    if (win->viewer->current_page - i < 0)
      break;
    cairo_translate(cr_pdf, 0, -win->viewer->pdf_height);
    poppler_page_render(win->viewer->pages[win->viewer->current_page - i], cr_pdf);
    i++;
  } while (real_y - i * win->viewer->pdf_height * win->viewer->scale > 0);

  cairo_restore(cr_pdf);
  cairo_save(cr_pdf);

  // Render pages after current that can be seen.
  i = 1;
  do {
    if (win->viewer->current_page + i >= win->viewer->n_pages)
      break;
    cairo_translate(cr_pdf, 0, win->viewer->pdf_height);
    poppler_page_render(win->viewer->pages[win->viewer->current_page + i], cr_pdf);
    i++;
  } while (real_y + i * win->viewer->pdf_height * win->viewer->scale < win->viewer->view_height);

  cairo_restore(cr_pdf);

  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr_pdf);
  cairo_surface_destroy(surface);
}
