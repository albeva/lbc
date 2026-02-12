# add_tblgen(<target> <tblgen_tool> <td_files>...)
#
# For each .td file (path relative to CMAKE_CURRENT_SOURCE_DIR), runs
# <tblgen_tool> to produce a .inc file under ${CMAKE_BINARY_DIR}/generated/,
# preserving the path relative to PROJECT_SOURCE_DIR. Adds the generated
# include directory to <target> and wires up rebuild dependencies.
function(add_tblgen target tblgen_tool)
    set(generated_dir "${CMAKE_BINARY_DIR}/generated")
    set(all_inc_files "")

    foreach(td_file ${ARGN})
        cmake_path(ABSOLUTE_PATH td_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE td_abs)
        cmake_path(RELATIVE_PATH td_abs BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE inc_rel)
        cmake_path(REMOVE_EXTENSION inc_rel)
        set(inc_rel "${inc_rel}.inc")
        set(inc_abs "${generated_dir}/${inc_rel}")
        cmake_path(GET inc_abs PARENT_PATH inc_dir)

        add_custom_command(
            OUTPUT "${inc_abs}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${inc_dir}"
            COMMAND ${tblgen_tool} "${td_abs}" -o "${inc_abs}"
            COMMAND clang-format -i --style=file:"${PROJECT_SOURCE_DIR}/.clang-format" "${inc_abs}"
            DEPENDS ${tblgen_tool} "${td_abs}"
            COMMENT "Generating ${inc_rel}"
        )
        list(APPEND all_inc_files "${inc_abs}")
    endforeach()

    add_custom_target(${target}_tblgen DEPENDS ${all_inc_files})
    add_dependencies(${target} ${target}_tblgen)
    target_include_directories(${target} PRIVATE "${generated_dir}")
endfunction()
