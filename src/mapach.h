/// @file:  mapach.h
///
/// Map generator declarations

typedef double         height_type;
typedef unsigned long  index_type;
typedef double         volume_type;
typedef unsigned long  group_type;

typedef struct {
  index_type x;
  index_type y;
} coord_type;

typedef struct {
  height_type    elevation;
  volume_type    water;
  group_type     group;
} datum_type;

typedef struct {
  coord_type  dim;
  index_type  size;
  coord_type  dir_offset[8];
  datum_type  *data;
} mapdata_type;

typedef enum {
  NO_ERROR = 0,
  MD_MEMORY_ERROR,
  BUF_ALLOC_ERROR,
  BUF_RESIZE_ERROR,
} error_type;

// Clockwise around.
// 7 0 1
// 6 C 2
// 5 4 3

typedef enum {
  DIR_NN = 0,
  DIR_NE,
  DIR_EE,
  DIR_SE,
  DIR_SS,
  DIR_SW,
  DIR_WW,
  DIR_NW,
  DIR_ENUM_SIZE,
} direction_type;

extern const char *map_error_to_str(error_type e);
extern void        map_perror(error_type e);
extern void        map_exit_on_error(error_type e);

extern error_type mapdata_init(mapdata_type **mdh, index_type dim_x, index_type dim_y);
extern void       mapdata_free(mapdata_type **mdh);

extern index_type mapdata_coord_to_idx(mapdata_type *md, coord_type coord);
extern index_type mapdata_xy_to_idx(mapdata_type *md, index_type x, index_type y);
extern coord_type mapdata_idx_to_coord(mapdata_type *md, index_type idx);

extern index_type mapdata_surround(mapdata_type *md, index_type center, direction_type d);
