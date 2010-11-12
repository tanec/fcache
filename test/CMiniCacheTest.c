#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <glib.h>

#include "CMiniCache.h"

static int run = 0, ret = 0;
static CMiniCache *cache;

typedef struct {
  md5_digest_t key;
  int num;
  time_t tm;
} val_t;

static void
test(gpointer data, gpointer user_data)
{
  int tn = *((int *)data), dn = *((int *)user_data);

  printf("===> test start: %d\n", tn);
  val_t vals[dn];
  int i, mul=1;
  while (mul < dn) mul *= 10;
  for (i=0; i<dn; i++) {
    vals[i].num = tn*mul + i;
    vals[i].tm  = time(NULL);
    md5_digest(&vals[i].num, sizeof(int), vals[i].key.digest);

    DelData(cache, &vals[i].key);
  }

  for (i=0; i<dn; i++) {
    sleep(time(NULL)%2);
    int result;
    size_t data_size=sizeof(val_t);
    result = AddData(cache, &vals[i].key, (void *)&vals[i], data_size);
    if (result != 0) {
      ret = 1;
      val_t *v = &vals[i];
      printf("+>set error: v(%p).num = %d\n", v, ((val_t *)v)->num);
    }
  }

  for (i=0; i<dn; i++) {
    val_t *v;
    size_t data_size;
    GetData(cache, &vals[i].key, (void *)&v, &data_size);
    if (v == NULL) {
      ret = 2;
      printf("-> get wrong: v=%p\n", v);
    } else {
      if (v != &vals[i]) {
        ret = 2;
        printf("-> get wrong: v(%p) != vals[%d](%p)\n",
               v, i, &vals[i]);
      }
      if (v->num != vals[i].num) {
        ret = 2;
        printf("-> get wrong: v->num(%d) != vals[%d].num(%d)\n",
               v->num, i, vals[i].num);
      }
      if (v->tm  != vals[i].tm) {
        ret = 2;
        printf("-> get wrong: v->tm (%d) != vals[%d].tm (%d)\n",
               v->tm,  i, vals[i].tm);
      }
      if (md5_compare(((map_key_t)v)->digest,
                      ((map_key_t)&vals[i].key)->digest)!=0) {
        ret = 2;
        printf("-> get wrong: md5\n");
      }
    }
  }

  for (i=0; i<dn; i++) {
    int result;
    result = DelData(cache, &vals[i].key);
    if (result != 0) {
      ret = 3;
      printf("-> remove wrong: v=%p\n", &vals[i]);
    }
  }

  printf("===> test stop : %d\n", tn);
  run--;
}

int
dfree(void *data)
{
  if (data) {
    //free(data);
    return 0;
  }
  return 1;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s threadnum datanum\n", argv[0]);
  } else {
    int tn = 0, dn = 0, i;

    tn = atoi(argv[1]);
    dn = atoi(argv[2]);

    if (tn > 0) {
      cache = CMiniCache_alloc(400000, 500000, 400000, 5000, dfree);

      g_thread_init(NULL);
      GThreadPool *tp = g_thread_pool_new(
          test, &dn, tn, false, NULL);
      if (tp == NULL) {
        perror("can not initialize thread pool!");
        exit(1);
      }

      int iarr[tn];
      for (i=0; i<tn; i++) {
        iarr[i] = i+1;
        g_thread_pool_push(tp, &(iarr[i]), NULL);
        run ++;
      }

      while (run > 0);
      printf("done.\n");
    }
  }
  return ret;
}
