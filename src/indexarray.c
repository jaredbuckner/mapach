/// @file:  indexarray.c
///
/// Arrays to hold map indexes.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "maptypes.h"

error_type array_init(array_type **array, size_t capacity) {
  array_type *ad = (array_type *) malloc(2 * sizeof(size_t)
                                           + capacity * sizeof(size_t));

  if(NULL == ad) return BUF_ALLOC_ERROR;

  ad->size = 0;
  ad->capacity = capacity;

  *array = ad;

  return NO_ERROR;
}

error_type array_resize(array_type **array, size_t capacity) {
  array_type *ad = (array_type *) realloc(*array,
                                          2 * sizeof(size_t)
                                          + capacity * sizeof(size_t));

  if(NULL == ad) return BUF_RESIZE_ERROR;

  ad->capacity = capacity;
  *array = ad;

  return NO_ERROR;
}

void array_free(array_type **array) {
  free(*array);
  *array = NULL;
}


error_type array_insert(array_type **array, size_t idx, size_t value) {
  array_type *ad = *array;
  error_type err = NO_ERROR;

  assert(ad != NULL);
  assert(idx <= ad->size);
  
  if(ad->size == ad->capacity) {
    size_t new_capacity = ad->capacity ? ad->capacity * 2 : 1;
    if(NO_ERROR != (err = array_resize(array, new_capacity * 2))) return err;
    ad = *array;
  }

  memmove(ad->data + idx + 1, ad->data + idx, (ad->size - idx) * sizeof(size_t));
  ad->size += 1;
  ad->data[idx] = value;

  return NO_ERROR;
}

error_type array_swap_elem(array_type **array, size_t idx0, size_t idx1) {
  array_type *ad = *array;

  assert(ad != NULL);
  assert(idx0 < ad->size);
  assert(idx1 < ad->size);

  size_t vtmp = ad->data[idx0];
  ad->data[idx0] = ad->data[idx1];
  ad->data[idx1] = vtmp;

  return NO_ERROR;
}

// The 'dst_idx' argument refers to the index of the element the source element
// will be inserted before.  Since elements may be moved -- and their indexes
// changed -- the actual location of the inserted element is returned.
size_t array_move_elem(array_type **array,
                       size_t src_idx, size_t dst_idx) {
  array_type *ad = *array;

  assert(ad != NULL);
  assert(src_idx < ad->size);
  assert(dst_idx <= ad->size);

  if(src_idx + 1 < dst_idx) {
    size_t vtmp = ad->data[src_idx];
    memmove(ad->data + src_idx,
            ad->data + src_idx + 1,
            (dst_idx - src_idx - 1) * sizeof(size_t));
    ad->data[dst_idx - 1] = vtmp;
    return dst_idx - 1;
  } else if(dst_idx < src_idx) {
    size_t vtmp = ad->data[src_idx];
    memmove(ad->data + dst_idx + 1,
            ad->data + dst_idx,
            (src_idx - dst_idx) * sizeof(size_t));
    ad->data[dst_idx] = vtmp;
    return dst_idx;
  } else {
    return src_idx;
  }  
}

error_type array_delete(array_type **array, size_t idx) {
  array_type *ad = *array;

  assert(ad != NULL);
  assert(idx < ad->size);
  
  memmove(ad->data + idx, ad->data + idx + 1, (ad->size - idx - 1) * sizeof(size_t));
  ad->size -= 1;

  return NO_ERROR;
}


// If an array is sorted with respect to a predicate such that all values which
// pass the predicate come before all values which fail the predicate, find the
// first value which fails the predicate
size_t array_bisect(array_type **array, predicate_fn_type pred,
                    void *predData) {
  array_type *ad = *array;
  assert(ad != NULL);
  size_t lb = 0;
  size_t ub = ad->size;

  while(lb < ub) {
    size_t aidx = (lb + ub) / 2;
    size_t didx = ad->data[aidx];
    if(pred(didx, predData)) {
      lb = aidx + 1;
    } else {
      ub = aidx;
    }
  }

  return(ub);
}



int idx_lt_bound(size_t value, void *data) {
  size_t *idx = data;
  return value < *idx;
}

int idx_ngt_bound(size_t value, void *data) {
  size_t *idx = data;
  return value <= *idx;
}

int rhgt_lt_bound(size_t value, void *data) {
  curry_type *cd = data;

  return cd->height < cd->md->data[value].elevation;
}

int rhgt_ngt_bound(size_t value, void *data) {
  curry_type *cd = data;
  
  return cd->height <= cd->md->data[value].elevation;
}

void _test_indexarray(void) {
  char statebuf[256];
  struct random_data rbuf;
  signed int randresult;
  array_type  *myarray;
  error_type err = NO_ERROR;
  size_t maxsize = 0;
  size_t maxcapacity = 0;
  
  rbuf.state = NULL;
  initstate_r(time(NULL), statebuf, 256, &rbuf);

  if(NO_ERROR != (err = array_init(&myarray, 16))) exit(err);

  for(unsigned int i = 0; i < 20; ++i) {
    size_t idx;
    
    random_r(&rbuf, &randresult);
    idx = randresult % ( myarray->size + 1 );
    if(NO_ERROR != (err = array_insert(&myarray, idx, i))) exit(err);

  }

  for(size_t idx = 0; idx < myarray->size; ++idx) {
    printf(" %ld" + !(idx), myarray->data[idx]);
  }

  printf("\nFinal size:  %ld/%ld\n", myarray->size, myarray->capacity);
  array_free(&myarray);
  
  
  if(NO_ERROR != (err = array_init(&myarray, 16))) exit(err);  
  
  for(unsigned int i = 0; i < 1000000; ++i) {
    if(myarray->size == 0) {
      if(NO_ERROR != (err = array_insert(&myarray, 0, i))) exit(err);
      
    } else {
      size_t op, idx, idx2;
      random_r(&rbuf, &randresult);
      op = randresult % 6;
      random_r(&rbuf, &randresult);
      if(op < 2) {
        idx = randresult % myarray->size;
        if(NO_ERROR != (err = array_delete(&myarray, idx))) exit(err);
        
      } else if (op < 5) {
        idx = randresult % ( myarray->size + 1 );
        if(NO_ERROR != (err = array_insert(&myarray, idx, i))) exit(err);
        
      } else {
        idx = randresult % myarray->size;
        random_r(&rbuf, &randresult);
        idx2 = randresult % ( myarray->size + 1 );
        array_move_elem(&myarray, idx, idx2);
      }
    }
    if(myarray->size > maxsize) maxsize = myarray->size;
    if(myarray->capacity > maxcapacity) maxcapacity = myarray->capacity;
  }

  printf("Current size:  %ld/%ld\n", myarray->size, myarray->capacity);
  printf("Maximum size:  %ld/%ld\n", maxsize, maxcapacity);
  
  array_free(&myarray);
  maxsize = 0;
  maxcapacity = 0;
  
  if(NO_ERROR != (err = array_init(&myarray, 16))) exit(err);
  
  for(unsigned int i = 0; i < 1000; ++i) {
    size_t op, vidx, idx;
    random_r(&rbuf, &randresult);
    op = randresult % 2;
    random_r(&rbuf, &randresult);
    vidx = randresult % (i + 1);
    idx = array_bisect(&myarray, idx_lt_bound, &vidx);
    switch(op) {
    case 0: {
      array_insert(&myarray, idx, vidx);
      break;
    }
    case 1: {
      if(myarray->data[idx] == vidx) array_delete(&myarray, idx);
      break;
    }
    }
    
    printf("[");
    for(size_t pidx = 0; pidx < myarray->size; ++pidx) {
      if(pidx) printf(", ");
      if(idx == pidx) printf("(");
      printf("%ld", myarray->data[pidx]);
      if(idx == pidx) printf(")");
    }
    printf("]\n");
    
    if(myarray->size > maxsize) maxsize = myarray->size;
    if(myarray->capacity > maxcapacity) maxcapacity = myarray->capacity;
  }

  printf("Current size:  %ld/%ld\n", myarray->size, myarray->capacity);
  printf("Maximum size:  %ld/%ld\n", maxsize, maxcapacity);
  
  array_free(&myarray);
  
  exit(NO_ERROR);
}
