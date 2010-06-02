#include <string.h>

#include "zhongsou_keyword.h"
#include "util.h"
#include "smalloc.h"
#include "log.h"

static str_map_t *domains = NULL;
static str_map_t *synonyms = NULL;

size_t
count_lines(mmap_array_t *mt)
{
  size_t sz = 0;
  off_t i;
  for(i=0; i<mt->len; i++)
    if (*(mt->data+i) == '\n')
      sz ++;
  if (*(mt->data+mt->len-1) != '\n')
    sz ++;
  return sz;
}

str_map_t *
read_strmap(const char *file)
{
  str_map_t *ret = NULL;
  mmap_array_t mt;
  if (mmap_read(&mt, file)) {
    size_t len = count_lines(&mt), i, n = 0;
    size_t pln = sizeof(size_t) + len*sizeof(str_map_node_t);
    char *str;
    ret = smalloc(pln + mt.len + 1);
    ret->len = len;
    memset(ret, 0, pln);
    memset(ret+pln+mt.len, '\n', 1); // ensure last '\n'
    memcpy((char*)ret+pln, (char*)mt.data, mt.len);

    str = (char *)ret+pln;
    for (i = 0; i<mt.len && n<len; i++) {
      char *k=NULL, *v=NULL;

      switch(*(str+i)) {
      case '\n':
        *(str+i) = 0;
        if (k != NULL && v != NULL) {
          (ret->base+n)->key = k;
          (ret->base+n)->value = v;
          n++;
        }
        k = NULL;
        v = NULL;
        break;
      case ' ':
      case '\r':
      case '\t':
        *(str+i) = 0;
        break;
      default:
        if (k == NULL) {
          k = str+i;
        } else if (v == NULL) {
          v = str + i;
        }
      }
    }
    //sort
    for (i = 0; i<len; i++) {
      int j;
      for (j = i+1; j < len; j++) {
        if (strcmp((ret->base+i)->key, (ret->base+j)->key) > 0) {
          char *k = (ret->base+i)->key, *v = (ret->base+i)->value;
          (ret->base+i)->key = (ret->base+j)->key;
          (ret->base+i)->value = (ret->base+j)->value;
          (ret->base+j)->key = k;
          (ret->base+j)->value = v;
        }
      }
    }
    mmap_close(&mt);
  }
  return ret;
}

void
read_domain(const char *file)
{
  domains = read_strmap(file);
}

void
read_synonyms(const char *file)
{
  synonyms = read_strmap(file);
}

char * search(str_map_t *map, char *key, size_t s, size_t e);

char *
domain2kw(char *s)
{
  char *ret = search(domains, s, 0, domains->len);
  return ret == NULL ? NULL : ret;
}

char *
synonyms2kw(char *s)
{
  char *ret = search(synonyms, s, 0, synonyms->len);
  return ret == NULL ? s : ret;
}

char *
find_keyword(const char*domain, const char *uri, char *keyword)
{
  char *kw = domain2kw((char *)domain);
  if (kw==NULL && uri!=NULL) {
    int i;
    size_t len = strlen(uri);
    memset(keyword, 0, len);
    memcpy(keyword, uri, len);
    kw = keyword+1;
    for (i=0; i<len-2; i++)
      if(*(kw+i)=='/') {*(kw+i)='\0';break;}
  }

  if (kw != NULL) kw = synonyms2kw(kw);
  tlog(DEBUG, "(domain:%s, url%s)->keyword:%s", domain, uri, kw);
  return kw;
}

char *
search(str_map_t *map, char *key, size_t s, size_t e)
{//[s, e)
  if (map == NULL || e < s || e-s < 1) {
    return NULL;
  } else {
    size_t m = (s+e)/2;
    str_map_node_t *node = (str_map_node_t*)(map->base)+m;
    int cr = strcmp(key, node->key);
    if (cr == 0) {
      return node->value;
    } else if (cr < 0) {
      return search(map, key, s, m);
    } else /* cr > 0 */{
      return search(map, key, m+1, e);
    }
  }
}
