// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <vector>
#include <string>

#include "types.h"
#include "windows_customizations.h"
#include "distance.h"

namespace diskann
{

class FilterLabelSet
{
};

template <typename data_t> class AbstractDataStore
{
  public:
    AbstractDataStore(const location_t capacity, const size_t max_search_filters);

    // virtual ~AbstractDataStore() = default;

    // Return number of points returned
    virtual location_t load(const std::string &filename) = 0;

    // Why does store take num_pts? Since store only has capacity, but we allow
    // resizing we can end up in a situation where the store has spare capacity.
    // To optimize disk utilization, we pass the number of points that are "true"
    // points, so that the store can discard the empty locations before saving.
    virtual size_t save(const std::string &filename, const location_t num_pts) = 0;

    DISKANN_DLLEXPORT virtual location_t capacity() const;

    DISKANN_DLLEXPORT virtual size_t get_max_search_filters() const;

    // load filters from file
    virtual void populate_labels(const std::string &filename) = 0;

    // Returns the updated capacity of the datastore. Clients should check
    // if resize actually changed the capacity to new_num_points before
    // proceeding with operations. See the code below:
    //  auto new_capcity = data_store->resize(new_num_points);
    //  if ( new_capacity >= new_num_points) {
    //   //PROCEED
    //  else
    //    //ERROR.
    virtual location_t resize(const location_t new_num_points);

    // operations on vectors
    // like populate_data function, but over one vector at a time useful for
    // streaming setting
    virtual void get_labels(const location_t i, FilterLabelSet *dest) const = 0;
    virtual void set_labels(const location_t i, const FilterLabelSet *const vector) = 0;
    virtual void prefetch_labels(const location_t loc) = 0;

    // internal shuffle operations to move around vectors
    // will bulk-move all the vectors in [old_start_loc, old_start_loc +
    // num_points) to [new_start_loc, new_start_loc + num_points) and set the old
    // positions to zero vectors.
    virtual void move_labels(const location_t old_start_loc, const location_t new_start_loc,
                             const location_t num_points) = 0;

    // same as above, without resetting the vectors in [from_loc, from_loc +
    // num_points) to zero
    virtual void copy_labels(const location_t from_loc, const location_t to_loc, const location_t num_points) = 0;

    // A DISTANCE COMPUTATION ON EMPTY SETS SHOULD RETURN 0
    virtual float get_distance(const FilterLabelSet *query, const location_t loc) const = 0;
    virtual void get_distance(const FilterLabelSet *query, const location_t *locations, const uint32_t location_count,
                              float *distances) const = 0;
    virtual float get_distance(const location_t loc1, const location_t loc2) const = 0;

    virtual Distance<FilterLabelSet> *get_dist_fn() = 0;

  protected:
    // Expand the datastore to new_num_points. Returns the new capacity created,
    // which should be == new_num_points in the normal case. Implementers can also
    // return _capacity to indicate that there are not implementing this method.
    virtual location_t expand(const location_t new_num_points) = 0;

    // Shrink the datastore to new_num_points. It is NOT an error if shrink
    // doesn't reduce the capacity so callers need to check this correctly. See
    // also for "default" implementation
    virtual location_t shrink(const location_t new_num_points) = 0;

    location_t _capacity;
    size_t _max_labels;
    size_t _max_search_filters;
};

} // namespace diskann
