#include <cairo.h>
#include <gtk/gtk.h>
#include <poppler.h>
#include <stdbool.h>

#include "app.h"
#include "config.h"
#include "keys.h"
#include "win.h"

static void window_handle_offset_update(Window *win);
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

  GtkWidget *view;

  PopplerDocument *doc;
  PopplerPage **pages; // TODO: free on object destruction
  int n_pages;
  int current_page;

  // View dimensions not known until drawn, so use draw_function to update
  int view_width, view_height;
  double pdf_width, pdf_height;
  double x_offset, y_offset, scale;

  bool center_mode;
};

G_DEFINE_TYPE(Window, window, GTK_TYPE_APPLICATION_WINDOW)

static void window_init(Window *win) {
  win->doc = NULL;
  win->pages = NULL;
  win->current_page = 0;

  win->view_width = 0;
  win->view_height = 0;
  win->pdf_width = 0;
  win->pdf_height = 0;
  win->x_offset = 0.0;
  win->y_offset = 0.0;
  win->scale = 1.0;

  win->center_mode = true;

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
  gtk_window_set_child(GTK_WINDOW(win), win->view);
  g_signal_connect(win->view, "resize", G_CALLBACK(on_resize), win);

  gtk_window_set_title(GTK_WINDOW(win), "Manypdf");
}

static void window_dispose(GObject *object) {
  G_OBJECT_CLASS(window_parent_class)->dispose(object);
}

static void window_class_init(WindowClass *class) {
  G_OBJECT_CLASS(class)->dispose = window_dispose;
}

Window *window_new(App *app) {
  return g_object_new(WINDOW_TYPE, "application", app, NULL);
}

void window_open(Window *win, GFile *file) {
  GError *err;
  double default_width, default_height;

  err = NULL;

  win->doc = poppler_document_new_from_gfile(
      file, NULL, NULL, &err); // TODO: Destroy on object destruction

  if (!win->doc) {
    g_printerr("Poppler: %s\n", err->message);
    g_error_free(err);
  } else {
    win->n_pages = poppler_document_get_n_pages(win->doc);

    win->pages = malloc(sizeof(PopplerPage *) * win->n_pages);
    if (!win->pages) {
      return;
    }

    for (int i = 0; i < win->n_pages; i++) {
      win->pages[i] = poppler_document_get_page(win->doc, i);

      if (!win->pages[i]) {
        g_printerr("Could not open %i'th page of document\n", i);
        g_object_unref(win->pages[i]);
      }
    }

    gtk_window_set_title(GTK_WINDOW(win), g_file_get_basename(file));
    poppler_page_get_size(win->pages[0], &default_width, &default_height);
    gtk_window_set_default_size(GTK_WINDOW(win), (int)default_width,
                                (int)default_width);
  }
}

static void window_handle_offset_update(Window *win) {
  if (win->y_offset < 0) {
    win->y_offset = STEPS - 1;
    if (win->current_page < win->n_pages - 1) {
      win->current_page++;
    }
  } else if (win->y_offset >= STEPS) {
    win->y_offset = 0;
    if (win->current_page > 0) {
      win->current_page--;
    }
  }
}

static gboolean on_key_pressed(GtkWidget *user_data, guint keyval,
                               guint keycode, GdkModifierType state,
                               GtkEventControllerKey *event_controller) {
  g_print("keyval = %d, keycode = %d, state = %d\n", keyval, keycode, state);

  Window *win;
  win = (Window *)user_data;

  handle_key(win, keyval);
  window_handle_offset_update(win);

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
      window_set_scale(win, MAX(MIN_SCALE, win->scale - dy * SCALE_STEP));
      break;
    default:
      win->x_offset -= dx;
      win->y_offset -= dy;

      window_handle_offset_update(win);
    }
  }

  window_redraw(win);
}

static void on_resize(GtkDrawingArea *area, int width, int height,
                      gpointer user_data) {
  Window *win;

  win = (Window *)user_data;

  win->view_width = width;
  win->view_height = height;
}

static void draw_function(GtkDrawingArea *area, cairo_t *cr, int width,
                          int height, gpointer user_data) {
  Window *win;
  PopplerPage *page;
  cairo_surface_t *surface;
  cairo_t *cr_pdf;
  cairo_matrix_t start_matrix, transformed_matrix;
  int i;

  win = (Window *)user_data;
  if (!win->doc || !win->pages) {
    return;
  }

  page = win->pages[win->current_page];
  poppler_page_get_size(page, &win->pdf_width, &win->pdf_height);

  if (win->center_mode) {
    window_center(win);
  }

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, win->view_width,
                                       win->view_height);
  cr_pdf = cairo_create(surface);

  cairo_get_matrix(cr_pdf, &start_matrix);

  cairo_translate(cr_pdf, win->view_width / 2.0, win->view_height / 2.0);
  cairo_scale(cr_pdf, win->scale, win->scale);
  cairo_translate(cr_pdf, -win->view_width / 2.0, -win->view_height / 2.0);

  cairo_translate(cr_pdf, win->x_offset * win->pdf_width / STEPS,
                  win->y_offset * win->pdf_height / STEPS);

  cairo_get_matrix(cr_pdf, &transformed_matrix);

  poppler_page_render(page, cr_pdf);

  double real_y = win->y_offset * win->pdf_width / STEPS * win->scale;

  // Render pages before current that can be seen.
  i = 1;
  do {
    if (win->current_page - i < 0)
      break;
    cairo_translate(cr_pdf, 0, -win->pdf_height);
    poppler_page_render(win->pages[win->current_page - i], cr_pdf);
    i++;
  } while (real_y - i * win->pdf_height * win->scale > 0);

  cairo_set_matrix(cr_pdf, &transformed_matrix);

  // Render pages after current that can be seen.
  i = 1;
  do {
    if (win->current_page + i >= win->n_pages)
      break;
    cairo_translate(cr_pdf, 0, win->pdf_height);
    poppler_page_render(win->pages[win->current_page + i], cr_pdf);
    i++;
  } while (real_y + i * win->pdf_height * win->scale < win->view_height);

  cairo_set_matrix(cr_pdf, &transformed_matrix);

  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr_pdf);
  cairo_surface_destroy(surface);
}

void window_toggle_center_mode(Window *win) {
  win->center_mode = !win->center_mode;
}

void window_center(Window *win) {
  win->x_offset =
      ((win->view_width / 2.0) - (win->pdf_width / 2.0)) /
      (win->pdf_width / STEPS);
}

void window_fit_scale(Window *win) {
  win->scale = win->view_width / win->pdf_width;
  window_center(win);
}

void window_redraw(Window *win) { gtk_widget_queue_draw(win->view); }

double window_get_x_offset(Window *win) { return win->x_offset; }

void window_set_x_offset(Window *win, double new) { win->x_offset = new; }

double window_get_y_offset(Window *win) { return win->y_offset; }

void window_set_y_offset(Window *win, double new) { win->y_offset = new; }

double window_get_scale(Window *win) { return win->scale; }

void window_set_scale(Window *win, double new) {
  win->scale = MAX(MIN_SCALE, new);
}
