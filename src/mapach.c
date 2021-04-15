/// @file:  mapach.c
///
/// Map generator

#include "mapach.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  size_t size;
  size_t capacity;
  index_type data[];
} _array_type;

const char *_map_errors[] = {
  "No error occurred.",
  "While initializing map data, unable to allocate enough memory.",
  "Memory exhaustion while allocating an internal buffer.",
  "Memory exhaustion while resizing an internal buffer.",
};



error_type _array_init(_array_type **array, size_t capacity) {
  _array_type *ad = (_array_type *) malloc(2 * sizeof(size_t)
                                           + capacity * sizeof(index_type));

  if(NULL == ad) return BUF_ALLOC_ERROR;

  ad->size = 0;
  ad->capacity = capacity;

  *array = ad;

  return NO_ERROR;
}


error_type _array_resize(_array_type **array, size_t capacity) {
  _array_type *ad = (_array_type *) realloc(*array,
                                            2 * sizeof(size_t)
                                            + capacity * sizeof(index_type));

  if(NULL == ad) return BUF_RESIZE_ERROR;

  ad->capacity = capacity;
  *array = ad;

  return NO_ERROR;
}

void _array_free(_array_type **array) {
  free(*array);
  *array = NULL;
}


error_type _array_insert(_array_type **array, size_t idx, index_type value) {
  _array_type *ad = *array;
  error_type err = NO_ERROR;
  
  if(ad->size == ad->capacity) {
    size_t new_capacity = ad->capacity ? ad->capacity * 2 : 1;
    if(NO_ERROR != (err = _array_resize(array, new_capacity * 2))) return err;
    ad = *array;
  }

  memmove(ad->data + idx, ad->data + idx + 1, (ad->size - idx) * sizeof(index_type));
  ad->size += 1;
  ad->data[idx] = value;

  return NO_ERROR;
}



const char *map_error_to_str(error_type e) {
  return(_map_errors[e]);
}

void map_perror(error_type e) {  
  printf("%s\n", map_error_to_str(e));
}

void map_exit_on_error(error_type e) {
  if(e) {
    map_perror(e);
    exit(2);
  }
}

error_type mapdata_init(mapdata_type **mdh, index_type dim_x, index_type dim_y) {
  mapdata_type *md = (mapdata_type *) malloc(sizeof(mapdata_type));
  
  if(NULL == md) return(MD_MEMORY_ERROR);
  
  md->dim.x = dim_x;
  md->dim.y = dim_y;
  md->size = dim_x * dim_y;
  
  md->dir_offset[0].x = 0;         md->dir_offset[0].y = dim_y - 1;
  md->dir_offset[1].x = 1;         md->dir_offset[1].y = dim_y - 1;
  md->dir_offset[2].x = 1;         md->dir_offset[2].y = 0;
  md->dir_offset[3].x = 1;         md->dir_offset[3].y = 1;
  md->dir_offset[4].x = 0;         md->dir_offset[4].y = 1;
  md->dir_offset[5].x = dim_x - 1; md->dir_offset[5].y = 1;
  md->dir_offset[6].x = dim_x - 1; md->dir_offset[6].y = 0;
  md->dir_offset[7].x = dim_x - 1; md->dir_offset[7].y = dim_y - 1;
  
  md->data = (datum_type *) calloc(md->size, sizeof(datum_type));
  if(NULL == md->data) {
    free(md);
    md = NULL;
    return(MD_MEMORY_ERROR);
  }

  *mdh = md;
  return(NO_ERROR);
}


void mapdata_free(mapdata_type **mdh) {
  free((*mdh)->data);
  free((*mdh));
  *mdh = NULL;
}

index_type mapdata_coord_to_idx(mapdata_type *md, coord_type coord) {
  return(mapdata_xy_to_idx(md, coord.x, coord.y));
}

index_type mapdata_xy_to_idx(mapdata_type *md, index_type x, index_type y) {
  return(y * md->dim.x + x);  
}

coord_type mapdata_idx_to_coord(mapdata_type *md, index_type idx) {
  coord_type result;
  result.x = idx % md->dim.x;
  result.y = idx / md->dim.x;
  return(result);
}

index_type mapdata_surround(mapdata_type *md, index_type center, direction_type d) {
  index_type x = (center + md->dir_offset[d].x) % md->dim.x;
  index_type y = (center / md->dim.x + md->dir_offset[d].y) % md->dim.y;
  return(y * md->dim.x + x);
}
  
