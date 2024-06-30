#pragma once

#include <gtk/gtk.h>
#include <poppler.h>

typedef struct Viewer {
  PopplerDocument *doc;
  PopplerPage **pages;
  int n_pages;
  int current_page;

  // View dimensions not known until drawn, so use draw_function to update
  int view_width, view_height;
  double pdf_width, pdf_height;
  double x_offset, y_offset, scale;

  bool center_mode;
} Viewer;

Viewer* viewer_new(PopplerDocument* doc);
void viewer_destroy(Viewer* viewer);

void viewer_fit_scale(Viewer* viewer);
void viewer_toggle_center_mode(Viewer* viewer);
void viewer_center(Viewer* viewer);
void viewer_set_scale(Viewer* viewer, double new_scale);
void viewer_handle_offset_update(Viewer* viewer);
void viewer_handle_key(Viewer* viewer, guint keyval);
