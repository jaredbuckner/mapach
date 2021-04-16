/// @file:  mapach.c
///
/// Map generator

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mapach.h"


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

  assert(ad != NULL);
  assert(idx <= ad->size);
  
  if(ad->size == ad->capacity) {
    size_t new_capacity = ad->capacity ? ad->capacity * 2 : 1;
    if(NO_ERROR != (err = _array_resize(array, new_capacity * 2))) return err;
    ad = *array;
  }

  memmove(ad->data + idx + 1, ad->data + idx, (ad->size - idx) * sizeof(index_type));
  ad->size += 1;
  ad->data[idx] = value;

  return NO_ERROR;
}


error_type _array_delete(_array_type **array, size_t idx) {
  _array_type *ad = *array;

  assert(ad != NULL);
  assert(idx < ad->size);
  
  memmove(ad->data + idx, ad->data + idx + 1, (ad->size - idx - 1) * sizeof(index_type));
  ad->size -= 1;

  return NO_ERROR;
}


error_type test_array(){
  char statebuf[256];
  struct random_data rbuf;
  signed int randresult;
  _array_type  *myarray;
  error_type err = NO_ERROR;
  size_t maxsize = 0;
  size_t maxcapacity = 0;
  
  rbuf.state = NULL;
  initstate_r(time(NULL), statebuf, 256, &rbuf);

  if(NO_ERROR != (err = _array_init(&myarray, 16))) return err;

  for(unsigned int i = 0; i < 20; ++i) {
    size_t idx;
    
    random_r(&rbuf, &randresult);
    idx = randresult % ( myarray->size + 1 );
    if(NO_ERROR != (err = _array_insert(&myarray, idx, i))) return err;

  }

  for(size_t idx = 0; idx < myarray->size; ++idx) {
    printf(" %ld" + !(idx), myarray->data[idx]);
  }

  printf("\nFinal size:  %ld/%ld\n", myarray->size, myarray->capacity);
  _array_free(&myarray);
  
  
  if(NO_ERROR != (err = _array_init(&myarray, 16))) return err;  
  
  for(unsigned int i = 0; i < 1000000; ++i) {
    if(myarray->size == 0) {
      if(NO_ERROR != (err = _array_insert(&myarray, 0, i))) return err;
      
    } else {
      size_t op, idx;
      random_r(&rbuf, &randresult);
      op = randresult % 5;
      random_r(&rbuf, &randresult);
      if(op < 2) {
        idx = randresult % myarray->size;
        if(NO_ERROR != (err = _array_delete(&myarray, idx))) return err;
        
      } else {
        idx = randresult % ( myarray->size + 1 );
        if(NO_ERROR != (err = _array_insert(&myarray, idx, i))) return err;
        
      }
    }
    if(myarray->size > maxsize) maxsize = myarray->size;
    if(myarray->capacity > maxcapacity) maxcapacity = myarray->capacity;
  }

  printf("Current size:  %ld/%ld\n", myarray->size, myarray->capacity);
  printf("Maximum size:  %ld/%ld\n", maxsize, maxcapacity);
  
  _array_free(&myarray);
  
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

int _can_place_here(mapdata_type *md, index_type hereIdx) {
  int        flips = 0;
  group_type lastGroup = md->data[mapdata_surround(md, hereIdx, 7)].group;

  for(size_t sidx = 0; sidx < 8; ++sidx) {
    group_type group = md->data[mapdata_surround(md, hereIdx, sidx)].group;
    if(group != lastGroup) {
      if(++flips == 3) {
        return 0;
      }
      lastGroup = group;
    }
  }

  return 1;
}

void _scan_environ(mapdata_type *md, index_type hereIdx,
                   height_type *min_elev, volume_type *ground_water) {
  *min_elev = 0;
  *ground_water = 0;
  for(size_t sidx = 0; sidx < 8; sidx += 2) {
    size_t thereIdx = mapdata_surround(md, hereIdx, sidx);
    int drains = 1;
    
    if(md->data[thereIdx].group == 0) continue;
    if(md->data[thereIdx].elevation < *min_elev) *min_elev = md->data[thereIdx].elevation;

    for(size_t tidx = 0; tidx < 8; tidx += 2) {
      size_t dortIdx = mapdata_surround(md, thereIdx, tidx);
      if(dortIdx == hereIdx) continue;
      if(md->data[dortIdx].group == 0) {
        drains = 0;
        break;
      }
    }
    if(drains) *ground_water+= md->data[thereIdx].water;
  }
}

void _rough_place(mapdata_type *md, height_type max_slope, index_type working_index) {
  height_type min_surround;
  volume_type ground_water;
  
  _scan_environ(md, working_index, &min_surround, &ground_water);
  md->data[working_index].elevation = min_surround - max_slope;
  md->data[working_index].water = ground_water + 1;
  md->data[working_index].group = 1;
  
}
  
error_type mapdata_rough_gen(mapdata_type *md, struct random_data *rbuf,
                             height_type max_slope) {
  _array_type *pending_indices;
  index_type   working_index = mapdata_xy_to_idx(md, md->dim.x / 2, md->dim.y / 2);  
  signed int   randresult;
  
  md->data[working_index].elevation = 0;
  md->data[working_index].water = 1;
  md->data[working_index].group = 1;
  
  map_exit_on_error(_array_init(&pending_indices, 1024));
  for(size_t sidx = DIR_NN; sidx < DIR_ENUM_SIZE; sidx += 2) {    
    map_exit_on_error(_array_insert(&pending_indices, pending_indices->size,
                                    mapdata_surround(md, working_index, sidx)));
  }
  
  while(pending_indices->size != 0) {
    size_t arrIdx;
    
    random_r(rbuf, &randresult);
    arrIdx = randresult % pending_indices->size;
    working_index = pending_indices->data[arrIdx];
    // Move the last entry here instead of a giant memmove
    pending_indices->data[arrIdx] = pending_indices->data[pending_indices->size - 1];
    pending_indices->size -= 1;  // And manually decrease the size, bypassing some... stuff

    if(md->data[working_index].group != 0) continue;  //  Don't recalculate an already-handled entry
    if(!_can_place_here(md, working_index)) continue;  //  Don't place blocking entries

    _rough_place(md, max_slope, working_index);
    
    for(size_t sidx = DIR_NN; sidx < DIR_ENUM_SIZE; sidx += 2) {
      index_type newIdx = mapdata_surround(md, working_index, sidx);
      if(md->data[newIdx].group == 0) {
        map_exit_on_error(_array_insert(&pending_indices, pending_indices->size,
                                        newIdx));
      }
    }
  }

  for(size_t arrIdx = 0; arrIdx < md->size; ++arrIdx) {
    if(md->data[arrIdx].group == 0) {
      map_exit_on_error(_array_insert(&pending_indices, pending_indices->size, arrIdx));
    }
  }

  while(pending_indices->size != 0) {
    size_t arrIdx;
    
    random_r(rbuf, &randresult);
    arrIdx = randresult % pending_indices->size;
    working_index = pending_indices->data[arrIdx];
    // Move the last entry here instead of a giant memmove
    pending_indices->data[arrIdx] = pending_indices->data[pending_indices->size - 1];
    pending_indices->size -= 1;  // And manually decrease the size, bypassing some... stuff

    _rough_place(md, max_slope, working_index);
  }
  
  return NO_ERROR;
}
