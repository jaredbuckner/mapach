/// @file:  main.c
///
/// Map generator main entry point


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mapach.h"

void test() {
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
}

int main(int argc, char* argv[]) {
  // test();

  char statebuf[256];
  struct random_data rbuf;
  mapdata_type *md;
  const index_type dim = 1081;
  const index_type dimx = 1 * dim, dimy = 1 * dim;
  //const index_type dimx = 18000, dimy = dimx;
  
  rbuf.state = NULL;
  initstate_r(time(NULL), statebuf, 256, &rbuf);

  printf("Initializing map data...\n");
  map_exit_on_error(mapdata_init(&md, dimx, dimy));

  printf("Map generation...\n");
  map_exit_on_error(mapdata_rough_gen(md, &rbuf, 1));

  printf("GROUP:\n");
  for(size_t idx = 0; idx < md->size; ++idx) {
    // printf(" %ld", md->data[idx].group);
    // if((idx + 1) % md->dim.x == 0) printf("\n");
  }

  printf("\nELEVATION:\n");
  height_type min_elev = 0;
  for(size_t idx = 0; idx < md->size; ++idx) {
    height_type elev = md->data[idx].elevation;
    char isZenith = 1;
    char isNadir = 1;
    for(size_t sidx = 0; sidx < 8; sidx += 2) {
      index_type eidx = mapdata_surround(md, idx, sidx);
      height_type eElev = md->data[eidx].elevation;
      if(eElev > elev) isZenith = 0;
      else if(eElev < elev) isNadir = 0;
    }

    
    if(elev < min_elev) {
      min_elev = elev;
    }

    if(isNadir || isZenith) {
      coord_type xy = mapdata_idx_to_coord(md, idx);
      printf("%c%c %5ld,%-5ld %03g\n", isNadir ? 'N' : ' ',
             isZenith ? 'Z' : ' ', xy.x, xy.y,
             md->data[idx].elevation);
    }
  }
  
  printf("\nMin:  %g\n\nWATER:\n", min_elev);
  volume_type max_vol = 0;
  for(size_t idx = 0; idx < md->size; ++idx) {
    if(md->data[idx].water > max_vol) {
      max_vol = md->data[idx].water;
    }
    
    // printf(" %03g", md->data[idx].water);
    // if((idx + 1) % md->dim.x == 0) printf("\n");
  }

  printf("\nMax:  %g\n\n", max_vol);

  {
    FILE *fp = fopen("sample.png", "wb");
    if(fp) {
      mapdata_write_png(fp, md, min_elev, 0);
      fclose(fp);
    }
  }
  
  printf("Exiting...\n");
  return(0);
}
