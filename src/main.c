/// @file:  main.c
///
/// Map generator main entry point


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "maptypes.h"
#include "mapach.h"


void _minax_elev_xy(double *min, double *max, mapdata_type *md, size_t x, size_t y) {
  size_t idx = mapdata_xy_to_idx(md, x, y);
  double elev = md->data[idx].elevation;
  if(min && !(*min <= elev)) *min=elev;
  if(max && !(*max <= elev)) *max=elev;
}

void _minax_elev_x(double *min, double *max, mapdata_type *md, size_t x, size_t y0, size_t y1) {
  size_t y = y0;
  do {
    _minax_elev_xy(min, max, md, x, y);
    y = (y + 1) % md->dim.y;
  } while(y != y1);
}

void _minax_elev_y(double *min, double *max, mapdata_type*md, size_t x0, size_t x1, size_t y) {
  size_t x = x0;
  do {
    _minax_elev_xy(min, max, md, x, y);
    x = (x + 1) % md->dim.x;
  } while(x != x1);
}

void _minax_elev_bound(double *min, double *max, mapdata_type*md,
                       size_t x0, size_t x1,
                       size_t y0, size_t y1) {
  _minax_elev_x(min, max, md, x0, y0, (y1+1)%md->dim.y);
  _minax_elev_x(min, max, md, x1, y0, (y1+1)%md->dim.y);
  _minax_elev_y(min, max, md, x0, (x1+1)%md->dim.x, y0);
  _minax_elev_y(min, max, md, x0, (x1+1)%md->dim.x, y1);
}

void _minax_elev(double *min, double *max, mapdata_type *md, size_t x0, size_t x1, size_t y0, size_t y1) {
  size_t y = y0;
  do {
    _minax_elev_y(min, max, md, x0, x1, y);
    y = (y+1) % md->dim.y;
  } while(y != y1);
}


int main(int argc, char* argv[]) {
  char statebuf[256];
  struct random_data rbuf;
  mapdata_type *mdr, *md;
  const size_t picdim = 1081 * 1.5;
  const size_t dimmul = 2;
  const size_t dimx = dimmul * picdim, dimy = dimmul * picdim;
  
  const double pixelheight = 1024.0 / 65535.0;
  const double pixelres = 16.65 / (double) dimmul;
  const double max_grade = 0.71;
  
  const double max_slope = max_grade * pixelres / pixelheight;
  const double gen_slope = max_slope * 0.04;
  const double rainwater = 0.23;
  const double omicron = 2;
  
  //const size_t dimx = 18000, dimy = dimx;
  
  rbuf.state = NULL;
  initstate_r(time(NULL), statebuf, 256, &rbuf);

  printf("Initializing map data...\n");
  map_exit_on_error(mapdata_init(&mdr, dimx, dimy));
  map_exit_on_error(mapdata_init(&md, picdim, picdim));

  printf("Map generation...\n");
  map_exit_on_error(mapdata_rough_gen(mdr, &rbuf, gen_slope, rainwater));

  printf("Map erosion...\n");
  map_exit_on_error(mapdata_erode(mdr, gen_slope, max_slope, omicron));

  {
    FILE *fp = fopen("precopy.png", "wb");
    if(fp) {
      double rmin=NAN, rmax=NAN; _minax_elev(&rmin, &rmax, mdr, 0, 0, 0, 0);
      double scale_elev = rmax - rmin > 65535 ? rmax : rmin + 65535;
      mapdata_write_png(fp, mdr, 0, 0, mdr->dim.x, mdr->dim.y, rmin, scale_elev);
      fclose(fp);
    }
  }

  printf("Map implosion...\n");
  mapdata_copy(mdr, md);

  mapdata_free(&mdr);
  
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

  // **
  double special_min = 0;
  _minax_elev_bound(&special_min, 0, md,
                    2 * md->dim.x / 9,  7 * md->dim.y / 9,
                    2 * md->dim.y / 9,  7 * md->dim.y / 9);
  
  double special_min2 = 0;
  _minax_elev_bound(&special_min2, 0, md,
                    0, md->dim.x-1,
                    0, md->dim.y-1);

  if(special_min2 > special_min) {
    special_min = special_min2;
  }
  
  special_min2 = 0;
  _minax_elev_bound(&special_min2, 0, md,
                    3 * md->dim.x / 9,  6 * md->dim.y / 9,
                    3 * md->dim.y / 9,  6 * md->dim.y / 9);
  
  if(special_min2 > special_min) {
    special_min = special_min2;
  }
  
  printf("\nMax:  %g\n\n", max_vol);

  {
    double scale_elev = max_elev - special_min > 65535 ? max_elev : special_min + 65535;
    FILE *fp = fopen("sample.png", "wb");
    if(fp) {
      size_t xb = (md->dim.x - 1081) / 2;
      size_t yb = (md->dim.y - 1081) / 2;
      mapdata_write_png(fp, md, xb, yb, xb + 1081, yb + 1081, special_min, scale_elev);
      fclose(fp);
    }
  }
  
  printf("Exiting...\n");
  return(0);
}
