#ifndef STATE_MACHINE_HH
#define STATE_MACHINE_HH

#include <iostream>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstring>
#include <cstdlib>
#include "utilities.hh"

namespace gcc_reorder
{

// Property state machine
//
// Gets a map: pass id -> it's properties
// Then, one by one applies passes and changes the property state correspondingly
// If a pass is met, which required properties are not met, reports a failure to std::cerr
//
// Also, has static functions to compress two set of properties/pass_properties into one
struct PropertyStateMachine
{
    std::unordered_map<int, pass_prop> num_to_prop_;
    std::vector<int> passes_;

    unsigned long original_property_state = 1; // in gcc dumps, the first pass already required an existing property 0x1
    unsigned long custom_property_state = 0;

    PropertyStateMachine() = default;

    PropertyStateMachine(const std::unordered_map<int, pass_prop>& num_to_prop) :
    num_to_prop_(num_to_prop)
    {}

    int apply_pass(int pass)
    {
        passes_.push_back(pass);
        pass_prop pass_prop = num_to_prop_.at(pass);

        if (((original_property_state & pass_prop.original.required) != pass_prop.original.required) ||
            ((custom_property_state & pass_prop.custom.required) != pass_prop.custom.required))
        {
            return -1;
        }

        custom_property_state |= pass_prop.custom.provided;
        custom_property_state &= ~pass_prop.custom.destroyed;

        original_property_state |= pass_prop.original.provided;
        original_property_state &= ~pass_prop.original.destroyed;

        return 0;
    }

};


class PassListGenerator
{
    std::vector<pass_info> info_vec_;
    std::unordered_map<std::string, int> name_to_id_map_; // we give each pass an id and work with ids to avoid needless heap indirection
    std::unordered_map<int, std::string> id_to_name;

    std::unordered_map<int, pass_prop> pass_to_properties_; // hash map: pass id -> it's properties

    std::unordered_map<std::string, int> pass_to_list_num;

    std::vector<std::pair<unsigned long, unsigned long>> start_properties = { {76079 | 130760, 0}, {76079, 0}, {76079, 0}, {130760, 0} };
    // std::vector<std::pair<unsigned long, unsigned long>> end_properties = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };

    PropertyStateMachine state;
    std::vector<char*> action_space;

    bool in_loop = false;

    std::vector<char*> list1;
    std::vector<char*> list2;
    std::vector<char*> list3;
    std::vector<char*> all_lists;
    std::vector<char*> loop_action_space;

public:
    int starting_list = -1;
    int current_list = -1;

    static constexpr int MAX_PASS_AMOUNT = 400;
    static constexpr int MAX_PASS_LENGTH = 40;

    PassListGenerator() : action_space(MAX_PASS_AMOUNT, 0)
    {
        auto&& allocate = [](char* ptr){ return new char[MAX_PASS_LENGTH]{0}; };
        std::transform(action_space.begin(), action_space.end(), action_space.begin(), allocate);
    }

    ~PassListGenerator()
    {
        auto&& delete_lambda = [](char* ptr){ delete ptr; return nullptr; };
        std::transform(action_space.begin(), action_space.end(), action_space.begin(), delete_lambda);

        std::transform(list1.begin(), list1.end(), list1.begin(), delete_lambda);
        std::transform(list2.begin(), list2.end(), list2.begin(), delete_lambda);
        std::transform(list3.begin(), list3.end(), list3.begin(), delete_lambda);
        std::transform(loop_action_space.begin(), loop_action_space.end(), loop_action_space.begin(), delete_lambda);
    }

    template <typename iter_info>
    PassListGenerator(iter_info begin_info, iter_info end_info) :
    info_vec_{begin_info, end_info}
    {
        setup_structures();
    }

    template <typename iter>
    void set_info_vec(iter begin, iter end)
    {
        info_vec_ = {begin, end};
    }

    template <typename iter>
    void set_list1(iter begin, iter end)
    {
        init_internal_vec(list1, begin, end);
    }

    template <typename iter>
    void set_list2(iter begin, iter end)
    {
        init_internal_vec(list2, begin, end);
    }

    template <typename iter>
    void set_list3(iter begin, iter end)
    {
        init_internal_vec(list3, begin, end);
    }

    template <typename iter>
    void set_list4_subpasses(iter begin, iter end)
    {
        init_internal_vec(loop_action_space, begin, end);
    }

    // map passes' names onto ids and batches of passes onto ids
    void setup_structures();

    void change_list(const std::vector<std::pair<unsigned long, unsigned long>>& vec);

    char** get_new_action_space(const char** full_action_space, const char** applied_passes, int size_full,
                                int size_applied, int list_num, size_t* size_ptr);

    int valid_pass_seq(char** pass_seq, int size, int list_num);

    int get_pass_list(char* pass_name);

private:

    template<typename iter>
    char** get_action_space_helper(iter begin, iter end, unsigned long original_state, unsigned long custom_state, size_t* size_ptr)
    {
        int new_size = 0;
        // std::cout << std::endl << "State: " << ' ' << original_state << ' ' << custom_state << std::endl;
        for (; begin != end; begin++)
        {
            auto&& both_props = pass_to_properties_[name_to_id_map_.at(std::string(*begin))];

            auto&& original_required = both_props.original.required;
            auto&& custom_required = both_props.custom.required;

            // std::cout << "Checking " << std::string(*begin) << " with " << original_required << ' ' << custom_required << std::endl;

            if (((original_required & original_state) == original_required) && ((custom_required & custom_state) == custom_required))
            {
                std::copy(*begin, *begin + strlen(*begin) + 1, action_space[new_size++]);
            }
            // else
            // {
            //     std::cout << std::string(*begin) << " no good: " << (original_required & original_state) << ' ' << (custom_required & custom_state) << std::endl;
            // }

        }
        *size_ptr = new_size;

        return action_space.data();
    }

    template <typename iter>
    void init_internal_vec(std::vector<char*>& to_init, iter begin, iter end)
    {
        to_init.resize(end - begin);

        for (int i = 0; begin != end; begin++, i++)
        {
            to_init[i] = new char[MAX_PASS_LENGTH]{0};
            std::copy(begin->begin(), begin->end(), to_init[i]);
        }
    }
};

} // namespace gcc_reorder

#endif