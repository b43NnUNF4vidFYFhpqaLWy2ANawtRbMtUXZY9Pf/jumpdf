#include "viewer_links.h"

ViewerLinks *viewer_links_new(void)
{
    ViewerLinks *links = malloc(sizeof(ViewerLinks));
    if (links == NULL) {
        return NULL;
    }

    viewer_links_init(links);

    return links;
}

void viewer_links_init(ViewerLinks *links)
{
    links->visible_links = g_ptr_array_new();
    links->follow_links_mode = false;
}

void viewer_links_destroy(ViewerLinks *links)
{
    viewer_links_clear_links(links);
    g_ptr_array_free(links->visible_links, FALSE);
}

unsigned int viewer_links_get_links(ViewerLinks *links, PopplerPage *page)
{
    GList *link_mappings = poppler_page_get_link_mapping(page);
    unsigned int link_count = g_list_length(link_mappings);
    PopplerLinkMapping *link_mapping;

    for (GList *l = link_mappings; l; l = l->next) {
        link_mapping = poppler_link_mapping_copy(l->data);
        g_ptr_array_add(links->visible_links, link_mapping);
    }

    poppler_page_free_link_mapping(link_mappings);

    return link_count;
}

void viewer_links_clear_links(ViewerLinks *links)
{
    g_ptr_array_foreach(links->visible_links, (GFunc)poppler_link_mapping_free, NULL);
    g_ptr_array_set_size(links->visible_links, 0);
}