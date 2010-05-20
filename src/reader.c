#include <stddef.h>
#include "reader.h"
#include "smalloc.h"


void
page_free(page_t *page)
{
    if(page == NULL)
        return;
    page_head_t *head = (page_head_t *)page;
    sfree(head->keyword);
    sfree(head->ig);
    sfree(head->param);

    sfree(page->body);
    sfree(page);
}
