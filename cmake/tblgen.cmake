# add_tblgen(<target> <tblgen_tool>
#     GENERATOR <gen_name> INPUT <td_file> [OUTPUT <inc_file>]
#     ...
# )
#
# For each GENERATOR/INPUT group, runs <tblgen_tool> with --gen=<gen_name>
# to produce a .inc file under ${CMAKE_BINARY_DIR}/generated/.
# If OUTPUT is omitted, the .inc filename is derived from the INPUT (.td -> .inc).
# Adds the generated include directory to <target> and wires up rebuild dependencies.
function(add_tblgen target tblgen_tool)
    set(generated_dir "${CMAKE_BINARY_DIR}/generated")
    set(all_inc_files "")

    # Parse GENERATOR ... INPUT ... [OUTPUT ...] groups from ARGN
    set(args ${ARGN})
    list(LENGTH args argc)
    set(i 0)

    while(i LESS argc)
        list(GET args ${i} keyword)
        if(NOT keyword STREQUAL "GENERATOR")
            message(FATAL_ERROR "add_tblgen: expected GENERATOR, got '${keyword}'")
        endif()

        # Read generator name
        math(EXPR i "${i} + 1")
        list(GET args ${i} gen_name)

        # Read INPUT keyword
        math(EXPR i "${i} + 1")
        list(GET args ${i} input_kw)
        if(NOT input_kw STREQUAL "INPUT")
            message(FATAL_ERROR "add_tblgen: expected INPUT after generator name, got '${input_kw}'")
        endif()

        # Read td file
        math(EXPR i "${i} + 1")
        list(GET args ${i} td_file)

        # Check for optional OUTPUT
        set(output_file "")
        math(EXPR next "${i} + 1")
        if(next LESS argc)
            list(GET args ${next} maybe_output)
            if(maybe_output STREQUAL "OUTPUT")
                math(EXPR i "${next} + 1")
                list(GET args ${i} output_file)
            endif()
        endif()

        # Resolve paths
        cmake_path(ABSOLUTE_PATH td_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE td_abs)

        if(output_file STREQUAL "")
            cmake_path(RELATIVE_PATH td_abs BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE inc_rel)
            cmake_path(REMOVE_EXTENSION inc_rel)
            set(inc_rel "${inc_rel}.inc")
        else()
            set(inc_rel "${output_file}")
        endif()

        set(inc_abs "${generated_dir}/${inc_rel}")
        cmake_path(GET inc_abs PARENT_PATH inc_dir)

        add_custom_command(
            OUTPUT "${inc_abs}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${inc_dir}"
            COMMAND ${tblgen_tool} "${td_abs}" --gen=${gen_name} --write-if-changed -o "${inc_abs}" -I "${CMAKE_CURRENT_SOURCE_DIR}"
            DEPENDS ${tblgen_tool} "${td_abs}"
            COMMENT "Generating ${inc_rel} (--gen=${gen_name})"
        )
        list(APPEND all_inc_files "${inc_abs}")

        math(EXPR i "${i} + 1")
    endwhile()

    add_custom_target(${target}_tblgen DEPENDS ${all_inc_files})
    add_dependencies(${target} ${target}_tblgen)
    target_include_directories(${target} PUBLIC "${generated_dir}")
endfunction()
