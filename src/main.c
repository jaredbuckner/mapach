/// @file:  main.c
///
/// Map generator main entry point

#include "mapach.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
  mapdata_type *md;

  printf("Testing arrays...\n");
  map_exit_on_error(test_array());
  
  printf("Initializing map data...\n");
  map_exit_on_error(mapdata_init(&md, 10, 10));
  printf("MD: %p, MD->D: %p, MD->X: %ld, MD->Y: %ld, MD->S: %ld\n",
         md, md->data, md->dim.x, md->dim.y, md->size);

  for(index_type cidx=0; cidx < md->size; cidx += md->dim.x + 1) {
    coord_type centercoord = mapdata_idx_to_coord(md, cidx);
    printf("CENTER: %ld (%ld, %ld)\n", cidx, centercoord.x, centercoord.y);    
    
    for(int nidx=0; nidx < 8; ++nidx) {
      index_type locidx = mapdata_surround(md, cidx, nidx);
      coord_type loccoord = mapdata_idx_to_coord(md, locidx);
      printf("  ROT: %d: %ld (%ld, %ld)\n", nidx, locidx, loccoord.x, loccoord.y);
    }
  }
  
  printf("Freeing map data...\n");
  mapdata_free(&md);
  printf("Exiting...\n");
  return(0);
}
