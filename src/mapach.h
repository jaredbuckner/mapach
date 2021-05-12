/// @file:  mapach.h
///
/// Map generator declarations
extern const char *map_error_to_str(error_type e);
extern void        map_perror(error_type e);
extern void        map_exit_on_error(error_type e);

extern error_type mapdata_init(mapdata_type **mdh, size_t dim_x, size_t dim_y);
extern void       mapdata_free(mapdata_type **mdh);

extern size_t     mapdata_coord_to_idx(mapdata_type *md, coord_type coord);
extern size_t     mapdata_xy_to_idx(mapdata_type *md, size_t x, size_t y);
extern coord_type mapdata_idx_to_coord(mapdata_type *md, size_t idx);

extern size_t     mapdata_surround(mapdata_type *md, size_t center, direction_type d);

extern error_type mapdata_rough_gen(mapdata_type *md, struct random_data *rbuf,
                                    double max_slope);

extern error_type mapdata_write_png(FILE *fp, mapdata_type *md,
                                    double black_elev, double white_elev);

