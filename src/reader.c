#include "reader.h"
#include "smalloc.h"

void
page_free(page_t *page)
{
    page_head_t head = page->head;
    sfree(head.keyword);
    sfree(head.ig);
    sfree(head.param);

    sfree(page->body);
    sfree(page);
}
