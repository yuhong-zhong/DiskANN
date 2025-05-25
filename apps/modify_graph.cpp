#include <cstring>
#include <iomanip>
#include <algorithm>
#include <map>
#include <numeric>
#include <omp.h>
#include <set>
#include <string.h>
#include <boost/program_options.hpp>

#ifndef _WINDOWS
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#endif

#include "index.h"
#include "memory_mapper.h"
#include "utils.h"
#include "program_options_utils.hpp"
#include "index_factory.h"

namespace po = boost::program_options;

template <typename T, typename LabelT = uint32_t>
int modify_graph(diskann::Metric &metric, const std::string &index_path, const uint32_t num_threads, std::vector<uint32_t> &Lvec, const bool dynamic, const bool tags, size_t dimension, const std::string &to_delete, const std::string& to_add){
    using TagT = uint32_t;

    const size_t num_frozen_pts = diskann::get_graph_num_frozen_points(index_path);
    auto config = diskann::IndexConfigBuilder()
                    .with_metric(metric)
                    .with_dimension(dimension)
                    .with_max_points(0)
                    .with_data_load_store_strategy(diskann::DataStoreStrategy::MEMORY)
                    .with_graph_load_store_strategy(diskann::GraphStoreStrategy::MEMORY)
                    .with_data_type(diskann_type_to_name<T>())
                    .with_label_type(diskann_type_to_name<LabelT>())
                    .with_tag_type(diskann_type_to_name<TagT>())
                    .is_dynamic_index(dynamic)
                    .is_enable_tags(tags)
                    .is_concurrent_consolidate(false)
                    .is_pq_dist_build(false)
                    .is_use_opq(false)
                    .with_num_pq_chunks(0)
                    .with_num_frozen_pts(num_frozen_pts)
                    .build();

    auto index_factory = diskann::IndexFactory(config);
    auto index = index_factory.create_instance();
    index->load(index_path.c_str(), num_threads, *(std::max_element(Lvec.begin(), Lvec.end())));
    std::cout << "Index loaded" << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    std::string data_type, dist_fn, index_path_prefix, query_file, filter_label, label_type,
    query_filters_file, to_delete, to_add;
    uint32_t num_threads, L, R, K;
    size_t dimension;
    std::vector<uint32_t> Lvec;
    bool dynamic, tags;
    float fail_if_recall_below = 0.0f;

    po::options_description desc{
        program_options_utils::make_program_description("search_memory_index", "Searches in-memory DiskANN indexes")};
    try
    {
        desc.add_options()("help,h", "Print this information on arguments");

        // Required parameters
        po::options_description required_configs("Required");
        required_configs.add_options()("data_type", po::value<std::string>(&data_type)->required(),
                                       program_options_utils::DATA_TYPE_DESCRIPTION);
        required_configs.add_options()("dist_fn", po::value<std::string>(&dist_fn)->required(),
                                       program_options_utils::DISTANCE_FUNCTION_DESCRIPTION);
        required_configs.add_options()("index_path_prefix", po::value<std::string>(&index_path_prefix)->required(),
                                       program_options_utils::INDEX_PATH_PREFIX_DESCRIPTION);
        required_configs.add_options()("search_list,L",
                                       po::value<std::vector<uint32_t>>(&Lvec)->multitoken()->required(),
                                       program_options_utils::SEARCH_LIST_DESCRIPTION);
        required_configs.add_options()("dimension", po::value<std::string>(&to_add)->required(),
                                       program_options_utils::VECTOR_DIMENSION);
        required_configs.add_options()("points_to_delete", po::value<std::string>(&to_delete)->required(),
                                       program_options_utils::POINTS_TO_DELETE);
        required_configs.add_options()("points_to_add", po::value<std::string>(&to_add)->required(),
                                       program_options_utils::POINTS_TO_ADD);

        // Optional parameters
        po::options_description optional_configs("Optional");
        optional_configs.add_options()("filter_label",
                                       po::value<std::string>(&filter_label)->default_value(std::string("")),
                                       program_options_utils::FILTER_LABEL_DESCRIPTION);
        optional_configs.add_options()("query_filters_file",
                                       po::value<std::string>(&query_filters_file)->default_value(std::string("")),
                                       program_options_utils::FILTERS_FILE_DESCRIPTION);
        optional_configs.add_options()("label_type", po::value<std::string>(&label_type)->default_value("uint"),
                                       program_options_utils::LABEL_TYPE_DESCRIPTION);
        optional_configs.add_options()("num_threads,T",
                                       po::value<uint32_t>(&num_threads)->default_value(omp_get_num_procs()),
                                       program_options_utils::NUMBER_THREADS_DESCRIPTION);
        optional_configs.add_options()(
            "dynamic", po::value<bool>(&dynamic)->default_value(false),
            "Whether the index is dynamic. Dynamic indices must have associated tags.  Default false.");
        optional_configs.add_options()("tags", po::value<bool>(&tags)->default_value(false),
                                       "Whether to search with external identifiers (tags). Default false.");

        // Merge required and optional parameters
        desc.add(required_configs).add(optional_configs);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            std::cout << desc;
            return 0;
        }
        po::notify(vm);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }

    diskann::Metric metric;
    if ((dist_fn == std::string("mips")) && (data_type == std::string("float")))
    {
        metric = diskann::Metric::INNER_PRODUCT;
    }
    else if (dist_fn == std::string("l2"))
    {
        metric = diskann::Metric::L2;
    }
    else if (dist_fn == std::string("cosine"))
    {
        metric = diskann::Metric::COSINE;
    }
    else if ((dist_fn == std::string("fast_l2")) && (data_type == std::string("float")))
    {
        metric = diskann::Metric::FAST_L2;
    }
    else
    {
        std::cout << "Unsupported distance function. Currently only l2/ cosine are "
                     "supported in general, and mips/fast_l2 only for floating "
                     "point data."
                  << std::endl;
        return -1;
    }

    if (dynamic && not tags)
    {
        std::cerr << "Tags must be enabled while searching dynamically built indices" << std::endl;
        return -1;
    }

    if (filter_label != "" && query_filters_file != "")
    {
        std::cerr << "Only one of filter_label and query_filters_file should be provided" << std::endl;
        return -1;
    }

    std::vector<std::string> query_filters;
    if (filter_label != "")
    {
        query_filters.push_back(filter_label);
    }
    else if (query_filters_file != "")
    {
        query_filters = read_file_to_vector_of_strings(query_filters_file);
    }

    if (data_type == std::string("int8"))
    {
        modify_graph<int8_t>(metric, index_path_prefix, num_threads, Lvec, dynamic, tags, dimension, to_delete, to_add);
    }
    else if (data_type == std::string("uint8"))
    {
        modify_graph<uint8_t>(metric, index_path_prefix, num_threads, Lvec, dynamic, tags, dimension, to_delete, to_add);
    }
    else if (data_type == std::string("float"))
    {
        modify_graph<float>(metric, index_path_prefix, num_threads, Lvec, dynamic, tags, dimension, to_delete, to_add);
    }
    else
    {
        std::cout << "Unsupported type. Use float/int8/uint8" << std::endl;
        return -1;
    }
    
    return 0;
}