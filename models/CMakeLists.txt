# Automatically search and add model subdirectories
CMAKE_POLICY(SET CMP0009 NEW) # follow sym-links
FILE(GLOB_RECURSE my_list . FOLLOW_SYMLINKS */CMakeLists.txt)
FOREACH(cmf ${my_list})
    GET_FILENAME_COMPONENT(dir_path ${cmf} PATH)
    ADD_SUBDIRECTORY(${dir_path})
#    MESSAGE(${dir_path})
ENDFOREACH(cmf)
