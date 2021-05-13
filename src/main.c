/// @file:  main.c
///
/// Map generator main entry point


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "maptypes.h"
#include "mapach.h"


int main(int argc, char* argv[]) {
  char statebuf[256];
  struct random_data rbuf;
  mapdata_type *md;
  const size_t dim = 1081;
  const size_t dimx = 1 * dim, dimy = 1 * dim;
  
  const double pixelheight = 1024.0 / 65535.0;
  const double pixelres = 16.65;
  const double max_grade = 1.0;
  
  const double max_slope = max_grade * pixelres / pixelheight;
  const double gen_slope = max_slope * 0.04;
  const double rainwater = 0.1;
  
  //const size_t dimx = 18000, dimy = dimx;
  
  rbuf.state = NULL;
  initstate_r(time(NULL), statebuf, 256, &rbuf);

  printf("Initializing map data...\n");
  map_exit_on_error(mapdata_init(&md, dimx, dimy));

  printf("Map generation...\n");
  map_exit_on_error(mapdata_rough_gen(md, &rbuf, gen_slope, rainwater));

  printf("Map erosion...\n");
  map_exit_on_error(mapdata_erode(md, gen_slope, max_slope));
  
  printf("\nELEVATION:\n");
  double min_elev = md->data[0].elevation;
  double max_elev = min_elev;
  for(size_t idx = 0; idx < md->size; ++idx) {
    double elev = md->data[idx].elevation;
    char isZenith = 1;
    char isNadir = 1;
    for(size_t sidx = 0; sidx < 8; sidx += 2) {
      size_t eidx = mapdata_surround(md, idx, sidx);
      double eElev = md->data[eidx].elevation;
      if(eElev > elev) isZenith = 0;
      else if(eElev < elev) isNadir = 0;
    }
    
    if(elev < min_elev) {
      min_elev = elev;
    } else if (elev > max_elev) {
      max_elev = elev;
    }

    if(isNadir || isZenith) {
      coord_type xy = mapdata_idx_to_coord(md, idx);
      printf("%c%c %5ld,%-5ld %03g\n", isNadir ? 'N' : ' ',
             isZenith ? 'Z' : ' ', xy.x, xy.y,
             md->data[idx].elevation);
    }
  }


  
  printf("\nRange:  %g-%g\n\nWATER:\n", min_elev, max_elev);
  double max_vol = 0;
  for(size_t idx = 0; idx < md->size; ++idx) {
    if(md->data[idx].water > max_vol) {
      max_vol = md->data[idx].water;
    }
    
    // printf(" %03g", md->data[idx].water);
    // if((idx + 1) % md->dim.x == 0) printf("\n");
  }

  printf("\nMax:  %g\n\n", max_vol);

  {
    double scale_elev = max_elev - min_elev > 65535 ? max_elev : min_elev + 65535;
    FILE *fp = fopen("sample.png", "wb");
    if(fp) {
      mapdata_write_png(fp, md, min_elev, scale_elev);
      fclose(fp);
    }
  }
  
  printf("Exiting...\n");
  return(0);
}
