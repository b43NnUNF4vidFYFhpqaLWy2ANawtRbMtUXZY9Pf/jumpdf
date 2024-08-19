#include "toc.h"

typedef struct {
    GObjectClass parent_class;
} TOCItemClass;

G_DEFINE_TYPE(TOCItem, toc_item, G_TYPE_OBJECT)

static void add_toc_entries(GListStore *store, TOCItem *parent, PopplerDocument *doc, PopplerIndexIter *iter);
static PopplerDest *get_actual_dest(PopplerDocument *doc, PopplerDest *dest);

static void toc_item_init(TOCItem *self) {
    self->title = NULL;
    self->page = -1;
    self->dest = NULL;
    self->children = NULL;
}

static void toc_item_class_init(TOCItemClass *klass)
{
}

TOCItem *toc_item_new(const gchar *title, int page, PopplerDest *dest, GListStore *children)
{
    TOCItem *item = g_object_new(toc_item_get_type(), NULL);

    item->title = g_strdup(title);
    item->children = children;
    item->page = page;
    item->dest = dest;

    return item;
}

GListModel *get_toc_tree(PopplerDocument *doc)
{
    GListStore *store = g_list_store_new(toc_item_get_type());
    PopplerIndexIter *iter = poppler_index_iter_new(doc);

    if (iter) {
        add_toc_entries(store, NULL, doc, iter);
        poppler_index_iter_free(iter);
    }

    return G_LIST_MODEL(store);
}

static void add_toc_entries(GListStore *store, TOCItem *parent, PopplerDocument *doc, PopplerIndexIter *iter)
{
    PopplerAction *action = NULL;
    PopplerDest *dest = NULL;
    TOCItem *item = NULL;
    PopplerIndexIter *child = NULL;

    while (iter) {
        action = poppler_index_iter_get_action(iter);
        if (action && action->type == POPPLER_ACTION_GOTO_DEST) {
            dest = get_actual_dest(doc, action->goto_dest.dest);

            item = toc_item_new(action->any.title, dest->page_num, dest, NULL);

            if (parent) {
                if (!parent->children) {
                    parent->children = g_list_store_new(toc_item_get_type());
                }
                g_list_store_append(parent->children, item);
            } else {
                g_list_store_append(store, item);
            }

            child = poppler_index_iter_get_child(iter);
            if (child) {
                add_toc_entries(store, item, doc, child);
                poppler_index_iter_free(child);
            }
        }

        poppler_action_free(action);

        if (!poppler_index_iter_next(iter)) {
            break;
        }
    }
}

static PopplerDest *get_actual_dest(PopplerDocument *doc, PopplerDest *dest) {
    PopplerDest *actual_dest = NULL;

    if (dest->type == POPPLER_DEST_NAMED) {
        actual_dest = poppler_document_find_dest(doc, dest->named_dest);
    } else {
        actual_dest = dest;
    }

    return actual_dest;
}