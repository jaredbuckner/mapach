/// @file:  maptypes.h
///
/// Common location for typedefs

typedef int group_type;

typedef struct {
  size_t x;
  size_t y;
} coord_type;

typedef struct {
  double elevation;
  double water;
  long   group;
} datum_type;

typedef struct {
  coord_type dim;
  size_t     size;
  coord_type dir_offset[8];
  datum_type *data;
} mapdata_type;

typedef struct {
  size_t size;
  size_t capacity;
  size_t data[];
} array_type;

typedef enum {
  NO_ERROR = 0,
  MD_MEMORY_ERROR,
  BUF_ALLOC_ERROR,
  BUF_RESIZE_ERROR,
  PNG_GEN_ERROR,
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

typedef int(*predicate_fn_type)(size_t, void*);

typedef struct {
  double       height;
  mapdata_type *md;
} curry_type;

