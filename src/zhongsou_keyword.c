#include <string.h>
#include <stdlib.h>
#include "zhongsou_keyword.h"
#include "util.h"
#include "smalloc.h"
#include "log.h"
#include "settings.h"

static str_map_t *domains = NULL;
static str_map_t *synonyms = NULL;//default: zhongsou
#define MAX_DOMAIN 1024
struct dm_s {
  char *domain;
  str_map_t *map;
} domain_keywords[MAX_DOMAIN];

int
cmp_str_map_node(str_map_node_t *n1, str_map_node_t *n2)
{
  return strcmp(n1->key, n2->key);
}

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
{tlog(INFO, "read_strmap(%s)", file);
  str_map_t *ret = NULL;
  mmap_array_t mt;
  if (mmap_read(&mt, file)) {
    size_t len = count_lines(&mt), i, n = 0;
    size_t pln = sizeof(size_t)
               + sizeof(str_map_node_t *)
               + len*sizeof(str_map_node_t);
    char *str;
    ret = smalloc(pln + mt.len + 1);
    memset(ret, '\n', pln + mt.len + 1);// ensure last '\n'
    ret->len = len;
    ret->base= (str_map_node_t *)((char *)ret + sizeof(size_t) + sizeof(str_map_node_t *));
    memcpy((char*)ret+pln, (char*)mt.data, mt.len);

    str = (char *)ret+pln;
    char *k=NULL, *v=NULL;
    for (i = 0; i<mt.len+1 && n<len; i++) {
      switch(*(str+i)) {
      case '\n':
        *(str+i) = '\0';
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
        *(str+i) = '\0';
        break;
      default:
        if (k == NULL) {
          k = str+i;
        } else if (v == NULL && *(str+i-1)=='\0') {
          v = str+i;
        }
      }
    }
    ret->len=n;
    //sort
    qsort(ret->base, ret->len, sizeof(str_map_node_t),
          (int(*)(const void*,const void*))cmp_str_map_node);

    mmap_close(&mt);
  }
  return ret;
}

void
read_domain(const char *file)
{tlog(DEBUG, "read_domain(%s)", file);
  domains = read_strmap(file);
}

void
read_domain_synonyms(char *domain, char *file)
{tlog(DEBUG, "read_domain_synonyms(%s, %s)", domain, file);
  int i;
  struct dm_s *current = NULL;
  for(i=0; i<MAX_DOMAIN; i++) {
    if (domain_keywords[i].domain == NULL) {
      current = &(domain_keywords[i]);
      current->domain = domain;
      break;
    }
  }
  if(current != NULL) current->map = read_strmap(file);
}

void
read_synonyms(const char *file)
{tlog(DEBUG, "read_synonyms(%s)", file);
  mmap_array_t mt;
  char *files, *f1=NULL, *f2=NULL;
  memset(domain_keywords, 0, MAX_DOMAIN*sizeof(struct dm_s));
  if (mmap_read(&mt, file)) {
    int i;
    files = smalloc(mt.len+1);
    memset(files, '\n', mt.len+1);
    memcpy(files, mt.data, mt.len);

    for (i=0; i<=mt.len; i++) {
      if(*(files+i)=='\n'||
         *(files+i)=='\r'||
         *(files+i)=='\t'||
         *(files+i)==' ') {
        char c = *(files+i);
        *(files+i) = '\0';
        if (f1!=NULL && f2!=NULL && c=='\n') {
          if (strcmp(f1, "default")==0) {
            synonyms = read_strmap(f2);
          } else {
            read_domain_synonyms(f1, f2);
          }
          f1 = NULL;
          f2 = NULL;
        }
      } else {
        if (f1==NULL) {
          f1=files+i;
        } else if(f2==NULL && *(files+i-1)=='\0') {
          f2=files+i;
        }
      }
    }
  }
}

char * search(str_map_t *map, const char *key, size_t s, size_t e);

const char *
domain2kw(const char *s)
{
  const char *ret = NULL;
  if (domains!=NULL) ret = search(domains, s, 0, domains->len);
  return ret == NULL ? NULL : ret;
}

const char *
synonyms2kw(const char *domain, const char *s)
{
  int i;
  const char *ret = NULL;
  for (i=0; i<MAX_DOMAIN; i++) {
    if (domain_keywords[i].domain == NULL) break;
    if (strcmp(domain, domain_keywords[i].domain)==0) {
      ret = search(domain_keywords[i].map, s, 0, domain_keywords[i].map->len);
      tlog(DEBUG, "synonyms2kw(%s, %s)->%s", domain, s, ret);
      break;
    }
  }
  if (ret==NULL && synonyms!=NULL) {
    ret = search(synonyms, s, 0, synonyms->len);
    tlog(DEBUG, "synonyms2kw(%s)->%s", s, ret);
  }
  return ret == NULL ? s : ret;
}

/**
  transform keyword
  */
const char *
find_keyword(const char *domain, const char *keyword)
{
  const char *kw = keyword;

  // try domain's default kwyword
  if (kw==NULL || strcmp(kw,"/")==0) {
    kw=domain2kw(domain);
  } else {
    // not "www.zhongsou.net", "g.zhonsou.net" ...
    // must find by domain
    if (cfg.multi_keyword_domains != NULL) {
      bool mkd = false;
      int i;
      for (i=0; i<cfg.multi_keyword_domains_len; i++) {
        if (strcmp(domain, cfg.multi_keyword_domains[i])==0)
          mkd = true;
        break;
      }
      if (!mkd) kw=domain2kw(domain);
    }
  }

  if (kw!=NULL) kw=synonyms2kw(domain, kw);
  tlog(DEBUG, "find_keyword(domain=%s, keyword=%s)->%s", domain, keyword, kw);
  return kw==NULL?keyword:kw;
}

char *
search(str_map_t *map, const char *key, size_t s, size_t e)
{//[s, e)
  if (map == NULL || e < s || e-s < 1) {
    tlog(DEBUG, "search(%p, %s,%d,%d)->NULL", map, key, s, e);
    return NULL;
  } else {
    size_t m = (s+e)/2;
    str_map_node_t *node = (str_map_node_t*)(map->base)+m;
    int cr = strcmp(key, node->key);
    if (cr == 0) {
      tlog(DEBUG, "search(%p, %s,%d,%d)->%s", map, key, s, e, node->value);
      return node->value;
    } else if (cr < 0) {
      return search(map, key, s, m);
    } else /* cr > 0 */{
      return search(map, key, m+1, e);
    }
  }
}
