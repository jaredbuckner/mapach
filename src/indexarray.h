/// @file:  indexarray.h
///
/// Map index array declarations

extern error_type array_init     (array_type **array, size_t capacity);
extern error_type array_resize   (array_type **array, size_t capacity);
extern void       array_free     (array_type **array);
extern error_type array_insert   (array_type **array, size_t idx, size_t value);
extern error_type array_swap_elem(array_type **array, size_t idx0, size_t idx1);
extern size_t     array_move_elem(array_type **array, size_t src_idx, size_t dst_idx);
extern error_type array_delete   (array_type **array, size_t idx);
extern size_t     array_bisect   (array_type **array, predicate_fn_type pred, void *predData);

// lt:   Is value less than the curried value?
// ngt:  Is value not greater than the curried value?
extern int        idx_lt_bound   (size_t value, void *data);
extern int        idx_ngt_bound  (size_t value, void *data);
extern int        rhgt_lt_bound  (size_t value, void *data);
extern int        rhgt_ngt_bound (size_t value, void *data);
