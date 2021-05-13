/// @file:  mapach.c
///
/// Map generator

#include <assert.h>
#include <math.h>
#include <netinet/in.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "maptypes.h"
#include "indexarray.h"
#include "mapach.h"


const char *_map_errors[] = {
  "No error occurred.",
  "While initializing map data, unable to allocate enough memory.",
  "Memory exhaustion while allocating an internal buffer.",
  "Memory exhaustion while resizing an internal buffer.",
  "Unable to generate PNG information.",
};



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



void _water_ellipse(double* a, double* b, double slope, double water) {
  // For now we're going to create an ellipse with the given half-volume, for which the ratio of a to b is the slope (m)
  // A = 0.5 * pi * a * b
  //   = 0.5 * pi * m * a * a
  //
  *a = sqrt(water / M_PI_2 / slope);
  *b = slope * *a;
}

double _ellipse_height(double a, double b, double x, double y, double slope) {
  double rsq = x*x + y*y;
  double r = sqrt(rsq);
  if(r > a) {
    return 2 * b + slope * (r - a);
  } else {
    double asq = a * a;
    double alfa = b * (2 - sqrt(1 - rsq / asq));
    if(r < a / 2) {
      double bravo = 2 * b * (1 - sqrt(1 - 4 * rsq / asq));
      return alfa < bravo ? alfa : bravo;
    } else {
      return alfa;
    }
  }
}


error_type mapdata_init(mapdata_type **mdh, size_t dim_x, size_t dim_y) {
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

size_t mapdata_coord_to_idx(mapdata_type *md, coord_type coord) {
  return(mapdata_xy_to_idx(md, coord.x, coord.y));
}

size_t mapdata_xy_to_idx(mapdata_type *md, size_t x, size_t y) {
  return(y * md->dim.x + x);  
}

coord_type mapdata_idx_to_coord(mapdata_type *md, size_t idx) {
  coord_type result;
  result.x = idx % md->dim.x;
  result.y = idx / md->dim.x;
  return(result);
}

size_t mapdata_surround(mapdata_type *md, size_t center, direction_type d) {
  size_t x = (center + md->dir_offset[d].x) % md->dim.x;
  size_t y = (center / md->dim.x + md->dir_offset[d].y) % md->dim.y;
  return(y * md->dim.x + x);
}

int _can_place_here(mapdata_type *md, size_t hereIdx, group_type nextGroup) {
  int        flips = 0;
  group_type groupAlfa = 0;
  group_type groupBravo = 0;
  group_type lastGroup = md->data[mapdata_surround(md, hereIdx, 7)].group;
  
  for(size_t sidx = 0; sidx < 8; ++sidx) {    
    group_type group = md->data[mapdata_surround(md, hereIdx, sidx)].group;
    if(group && group != groupAlfa && group != groupBravo) {
      if(groupAlfa == 0) {
        groupAlfa = group;
      } else if(groupBravo == 0) {
        groupBravo = group;
      } else {
        return 0;
      }
    }
    if(group != lastGroup) {
      flips += 1;
      if(flips == 5 || ( groupBravo == 0 && flips == 3)) {
        return 0;
      }
      lastGroup = group;
    }
  }

  if(groupAlfa && groupBravo) {
    for(size_t aIdx = 0; aIdx < md->size; ++aIdx) {
      if(md->data[aIdx].group == groupBravo) {
        md->data[aIdx].group = groupAlfa;
      }
    }
  }
  
  return(groupAlfa || nextGroup);
}

void _scan_environ(mapdata_type *md, size_t hereIdx,
                   double *min_elev, double *ground_water) {
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

void _rough_place(mapdata_type *md, double max_slope, double rainwater, size_t working_index, group_type group) {
  double min_surround;
  double ground_water;
  
  _scan_environ(md, working_index, &min_surround, &ground_water);
  md->data[working_index].elevation = min_surround - max_slope;
  md->data[working_index].water = ground_water + rainwater;
  md->data[working_index].group = group;
  
}
  
error_type mapdata_rough_gen(mapdata_type *md, struct random_data *rbuf,
                             double max_slope, double rainwater) {
  array_type *pending_indices;
  size_t   working_index = mapdata_xy_to_idx(md, md->dim.x / 2, md->dim.y / 2);  
  signed int   randresult;
  size_t remaining = md->size;
  size_t peaks;
  group_type group;
  
  map_exit_on_error(array_init(&pending_indices, 1024));

  random_r(rbuf, &randresult);
  peaks = (randresult % 81) + 1;
  for(size_t peak = 0; peak < peaks; ++peak) {
    do {
      size_t x;
      size_t y;
      random_r(rbuf, &randresult); x = randresult % (md->dim.x / 2) + (md->dim.x / 4);
      random_r(rbuf, &randresult); y = randresult % (md->dim.y / 4) + (md->dim.x * 3 / 8);
      working_index = mapdata_xy_to_idx(md, x, y);
    } while(!(group = _can_place_here(md, working_index, peak+1)));
    
    _rough_place(md, max_slope, rainwater, working_index, group);
    remaining -= 1;

    for(size_t sidx = DIR_NN; sidx < DIR_ENUM_SIZE; sidx += 2) {
      size_t newIdx = mapdata_surround(md, working_index, sidx);
      if(md->data[newIdx].group == 0) {
        map_exit_on_error(array_insert(&pending_indices, pending_indices->size,
                                       newIdx));
      }
    }
  }
  
  while(remaining) {
    if(pending_indices->size) {
      size_t arrIdx;
      
      random_r(rbuf, &randresult);
      arrIdx = randresult % pending_indices->size;
      working_index = pending_indices->data[arrIdx];
      // Move the last entry here instead of a giant memmove
      pending_indices->data[arrIdx] = pending_indices->data[pending_indices->size - 1];
      pending_indices->size -= 1;  // And manually decrease the size, bypassing some... stuff
      
      if(md->data[working_index].group != 0) continue;  //  Don't recalculate an already-handled entry
      if(!(group = _can_place_here(md, working_index, 1))) continue;  //  Don't place blocking entries
      
      _rough_place(md, max_slope, rainwater, working_index, group);
      remaining -= 1;
      
      for(size_t sidx = DIR_NN; sidx < DIR_ENUM_SIZE; sidx += 2) {
        size_t newIdx = mapdata_surround(md, working_index, sidx);
        if(md->data[newIdx].group == 0) {
          map_exit_on_error(array_insert(&pending_indices, pending_indices->size,
                                         newIdx));
        }
      }
    } else {
      size_t arrIdx;
      for(size_t arrIdx = 0; arrIdx < md->size; ++arrIdx) {
        if(md->data[arrIdx].group == 0) {
          map_exit_on_error(array_insert(&pending_indices, pending_indices->size, arrIdx));
        }
      }

      assert(pending_indices->size);
      random_r(rbuf, &randresult);
      arrIdx = randresult % pending_indices->size;
      working_index = pending_indices->data[arrIdx];
      pending_indices->size = 0;  // Manually clear the array... sort of
      
      _rough_place(md, max_slope, rainwater, working_index, 1);
      remaining -= 1;
      
      for(size_t sidx = DIR_NN; sidx < DIR_ENUM_SIZE; sidx += 2) {
        size_t newIdx = mapdata_surround(md, working_index, sidx);
        if(md->data[newIdx].group == 0) {
          map_exit_on_error(array_insert(&pending_indices, pending_indices->size,
                                         newIdx));
        }
      }
    }
  }
  
  array_free(&pending_indices);
  
  return NO_ERROR;
}


error_type mapdata_transform(mapdata_type *md,
                             double scale, double translate) {
  for(size_t idx = 0; idx < md->size; ++idx) {
    md->data[idx].elevation *= scale;
    md->data[idx].elevation += translate;
  }

  return NO_ERROR;
}

int _is_nadir(mapdata_type *md, size_t idx) {
  double elev = md->data[idx].elevation;
  return( md->data[mapdata_surround(md, idx, 0)].elevation > elev &&
          md->data[mapdata_surround(md, idx, 2)].elevation > elev &&
          md->data[mapdata_surround(md, idx, 4)].elevation > elev &&
          md->data[mapdata_surround(md, idx, 6)].elevation > elev);
}

void _insert_unique(array_type **pending_by_index,
                    array_type **pending_by_height,
                    mapdata_type *md, size_t idx) {
  
  size_t index_where = array_bisect(pending_by_index, idx_lt_bound, &idx);
  if(index_where == (*pending_by_index)->size || (*pending_by_index)->data[index_where] != idx) {
    curry_type cd;
    cd.height = md->data[idx].elevation;
    cd.md = md;

    size_t height_where = array_bisect(pending_by_height, rhgt_lt_bound, &cd);
    array_insert(pending_by_index, index_where, idx);
    array_insert(pending_by_height, height_where, idx);
    
    // assert((*pending_by_height)->size == (*pending_by_index)->size);
    // for(size_t cidx = 1; cidx < (*pending_by_height)->size; ++cidx) {
    //   assert(md->data[(*pending_by_height)->data[cidx]].elevation
    //          <= md->data[(*pending_by_height)->data[cidx - 1]].elevation);
    //   assert((*pending_by_index)->data[cidx-1] < (*pending_by_index)->data[cidx]);
    // }
  }
}

size_t _pop_next(array_type **pending_by_index,
                 array_type **pending_by_height,
                 mapdata_type *md) {
  assert((*pending_by_height)->size != 0);
  size_t idx = (*pending_by_height)->data[--((*pending_by_height)->size)];
  size_t index_where = array_bisect(pending_by_index, idx_lt_bound, &idx);
  assert(index_where != (*pending_by_index)->size);
  assert((*pending_by_index)->data[index_where] == idx);
  array_delete(pending_by_index, index_where);
  return idx;  
}

void _safe_update_elev(array_type **pending_by_height,
                       mapdata_type *md,
                       size_t idx,
                       double new_elev) {
  curry_type cd;
  cd.height = md->data[idx].elevation;
  cd.md = md;
  
  size_t lb = array_bisect(pending_by_height, rhgt_lt_bound, &cd);
  size_t ub = array_bisect(pending_by_height, rhgt_ngt_bound, &cd);

  for(size_t aidx = lb; aidx < ub; ++aidx) {
    if((*pending_by_height)->data[aidx] == idx) {
      cd.height = new_elev;
      size_t naidx = array_bisect(pending_by_height, rhgt_lt_bound, &cd);
      array_move_elem(pending_by_height, aidx, naidx);
      break;
    }
  }
  md->data[idx].elevation = new_elev;
}  

error_type mapdata_erode(mapdata_type *md, double river_slope,
                         double max_slope) {
  array_type *pending_by_index;
  array_type *pending_by_height;
  
  map_exit_on_error(array_init(&pending_by_index, 1024));
  map_exit_on_error(array_init(&pending_by_height, 1024));

  for(size_t idx = 0; idx < md->size; ++idx) {
    md->data[idx].group = 1;
    if(_is_nadir(md, idx)) {
      _insert_unique(&pending_by_index, &pending_by_height, md, idx);
    }
  }

  while(pending_by_index->size != 0) {
    double a, b;
    size_t hspan;
    size_t idx = _pop_next(&pending_by_index, &pending_by_height, md);
    double elev = md->data[idx].elevation;
    coord_type coord = mapdata_idx_to_coord(md, idx);
    md->data[idx].group = 0;
    if(pending_by_index->size % 1000 == 0) {
      printf("... %ld (%ld @ %g)\n", pending_by_index->size, idx, md->data[idx].elevation);
    }
    _water_ellipse(&a, &b, river_slope, md->data[idx].water);
    hspan = a;
    if(hspan < 4) hspan = 4;
    if(hspan > md->dim.x / 4) hspan = md->dim.x / 4;
    if(hspan > md->dim.y / 4) hspan = md->dim.y / 4;

    for(size_t yoff = md->dim.y - hspan; yoff <= md->dim.y + hspan; ++yoff) {
      size_t ymag = yoff < md->dim.y ? md->dim.y - yoff : yoff - md->dim.y;
      size_t y = (coord.y + yoff) % md->dim.y;
      for(size_t xoff = md->dim.x - hspan; xoff <= md->dim.x + hspan; ++xoff) {
        size_t xmag = xoff < md->dim.x ? md->dim.x - xoff : xoff - md->dim.x;
        if(xmag == 0 && ymag == 0) continue;
        
        size_t x = (coord.x + xoff) % md->dim.x;
        size_t widx = mapdata_xy_to_idx(md, x, y);
        double welev = md->data[widx].elevation;
        if(welev < elev) continue;
        
        double limitheight = _ellipse_height(a, b, xmag, ymag, max_slope);
        double lelev = elev + limitheight;
        if(lelev < welev) {
          _safe_update_elev(&pending_by_height, md, widx, lelev);
        }


        if(md->data[widx].group) {
          _insert_unique(&pending_by_index, &pending_by_height, md, widx);
        }
      }
    }    
  }

  array_free(&pending_by_index);
  array_free(&pending_by_height);
  
  return NO_ERROR;
}
                         


error_type mapdata_write_png(FILE *fp, mapdata_type *md,
                             double black_elev, double white_elev) {
  png_structp png_ptr = 0;
  png_infop info_ptr = 0;
  error_type e = PNG_GEN_ERROR;
  double full_span = white_elev - black_elev;
  
  if(!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) goto cleanup;
  if(!(info_ptr = png_create_info_struct(png_ptr)))                        goto cleanup;
  if(setjmp(png_jmpbuf(png_ptr)))                                          goto cleanup;

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, md->dim.x, md->dim.y, 16,
               PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);

  png_byte* row = malloc(2 * md->dim.x);
  for(size_t y = 0; y < md->dim.y; ++y) {
    for(size_t x = 0; x < md->dim.x; ++x) {
      size_t idx = mapdata_xy_to_idx(md, x, y);
      double elev = md->data[idx].elevation;
      double elev_span = elev - black_elev;
      double color = 65535.0 * elev_span / full_span;
      if(color > 65535) color = 65535;
      else if(color < 0) color = 0;
      
      ((unsigned short*)row)[x] = htons((unsigned short)color);
    }
    png_write_row(png_ptr, row);
  }
  png_write_end(png_ptr, info_ptr);
  free(row);
  e = NO_ERROR;
  
 cleanup:
  if(png_ptr != 0) {
    if(info_ptr == 0) png_destroy_write_struct(&png_ptr, 0);
    else              png_destroy_write_struct(&png_ptr, &info_ptr);
  }

  return e;
}

