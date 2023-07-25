import ctypes
import pathlib
import random
import time

# prepares python list of strings for c function
def make_c_array(lst):
    return (ctypes.c_char_p * len(lst))(*[x.encode() for x in lst])

# makes a python list from c-like array of strings
def make_list(arr, size):
    return [elem.decode() for elem in arr[:size.value]]

# returns a lib object, which is necessary for the rest of the functions
def setuplib(name : pathlib.Path):
    lib = ctypes.CDLL(str(name))
    lib.get_new_action_space.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p), ctypes.c_int32,
                                            ctypes.c_int32, ctypes.c_int32, ctypes.POINTER(ctypes.c_size_t)]

    lib.get_new_action_space.restype = ctypes.POINTER(ctypes.c_char_p)

    lib.get_pass_list.argtypes = [ctypes.c_char_p]
    lib.get_pass_list.restype = ctypes.c_int32

    lib.valid_pass_seq.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_int32, ctypes.c_int32]
    lib.valid_pass_seq.restype = ctypes.c_int32

    return lib

def get_pass_list(name : str, lib):
    return int(lib.get_pass_list(name.encode()))


# receives lib object, action list from previous step (could be empty), used list(could be empty), and number of list from which to take passes
# returns a list of available passes
def get_action_list(libactions, old_action_list, used_actions, list_num):
    size = ctypes.c_size_t()
    new_action_byte_array = libactions.get_new_action_space(make_c_array(old_action_list), make_c_array(used_actions), len(old_action_list), len(used_actions),
                                                             list_num, ctypes.byref(size))
    return make_list(new_action_byte_array, size)


# receives a lib object, passes sequence to check, and number of list from which these passes are
# return 0 if ok, a number of failed pass
def valid_pass_seq(lib, lst, list_num):
    return int(lib.valid_pass_seq(make_c_array(lst), len(lst), list_num))

if __name__ == "__main__":

    # Example

    libname = pathlib.Path().absolute() / "actions.so"
    lib = setuplib(libname)

    file = open("lists/to_shuffle2.txt")
    action_space = []
    used = []

    for line in file:
        action_space.append(line.strip())

    # print("Starting:")
    # print(action_space)
    list_num = 2

    action_space = get_action_list(lib, action_space, used, list_num)

    used_vec_needed_length = 100

    for i in range (used_vec_needed_length):
        action_space = get_action_list(lib, action_space, used, list_num)
        # print("Action space:")
        # print(action_space)

        index = random.randrange(0, len(action_space))
        # print(f"Taken: {action_space[index]}, Index: {index}, len: {len(action_space) - 1}")

        # print("Used vec")
        used.append(action_space[index])
        # print(used)

    # print("Used vec")
    # print(used)

    # print("Validation")
    # print(len(used))
    # print(valid_pass_seq(lib, used, list_num))
