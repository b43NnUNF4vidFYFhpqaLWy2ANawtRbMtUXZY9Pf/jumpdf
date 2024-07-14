#pragma once

#include <gtk/gtk.h>
#include <poppler.h>

// TODO: Consider splitting Viewer into ViewerPosition, ViewerSearch, ViewerLinks
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

  unsigned int input_number;

  const char *search_text;
  int last_goto_page;

  GPtrArray *visible_links;
  bool follow_links_mode;
} Viewer;

Viewer* viewer_new(PopplerDocument* doc);
void viewer_destroy(Viewer* viewer);

void viewer_fit_horizontal(Viewer* viewer);
void viewer_fit_vertical(Viewer* viewer);
void viewer_toggle_center_mode(Viewer* viewer);
void viewer_center(Viewer* viewer);
void viewer_set_scale(Viewer* viewer, double new_scale);
void viewer_handle_offset_update(Viewer* viewer);

void viewer_goto_next_search(Viewer* viewer);
void viewer_goto_prev_search(Viewer* viewer);

unsigned int viewer_get_links(Viewer* viewer, PopplerPage* page);
void viewer_clear_links(Viewer* viewer);