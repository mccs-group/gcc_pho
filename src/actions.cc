#include "action_space.hh"

gcc_reorder::PassListGenAdapter adapter{};
bool initialized;

extern "C" char** get_new_action_space(const char** full_action_space,const char** applied_passes, int size_full,
                                       int size_applied, int list_num, size_t* size_ptr)
{
    if (!initialized)
        adapter.setup();
    return adapter.get_new_action_space(applied_passes, size_applied, list_num, size_ptr);
}

extern "C" int get_pass_list(char* pass_name)
{
    if (!initialized)
        adapter.setup();
    return adapter.get_pass_list(pass_name);
}

extern "C" int valid_pass_seq(char** pass_seq, int size, int list_num)
{
    if (!initialized)
        adapter.setup();
    return adapter.valid_pass_seq(pass_seq, size, list_num);
}

extern "C" char** make_valid_pass_seq(char** pass_seq, int size, int list_num, size_t* size_ptr)
{
    if (!initialized)
        adapter.setup();
    return adapter.make_valid_pass_seq(pass_seq, size, list_num, size_ptr);

}

extern "C" void set_path(char* path)
{
    adapter.set_path_to_dir(std::filesystem::path{path}.parent_path());
}
