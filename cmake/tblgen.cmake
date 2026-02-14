# add_tblgen(<target> <tblgen_tool>
#     GENERATOR <gen_name> INPUT <td_file> [OUTPUT <inc_file>]
#     ...
# )
#
# For each GENERATOR/INPUT group, runs <tblgen_tool> with --gen=<gen_name>
# to produce a .inc file alongside the INPUT .td file in the source tree.
# If OUTPUT is omitted, the .inc filename is derived from the INPUT (.td -> .inc).
# OUTPUT is resolved relative to the directory containing the INPUT file.
# Wires up rebuild dependencies so the .inc is regenerated when the .td or tool changes.
function(add_tblgen target tblgen_tool)
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

        # Resolve input to absolute path and get its directory
        cmake_path(ABSOLUTE_PATH td_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE td_abs)
        cmake_path(GET td_abs PARENT_PATH td_dir)

        # Resolve output path relative to the input file's directory
        if(output_file STREQUAL "")
            cmake_path(GET td_abs FILENAME inc_name)
            cmake_path(REMOVE_EXTENSION inc_name)
            set(inc_name "${inc_name}.inc")
        else()
            set(inc_name "${output_file}")
        endif()

        cmake_path(ABSOLUTE_PATH inc_name BASE_DIRECTORY "${td_dir}" OUTPUT_VARIABLE inc_abs)
        cmake_path(RELATIVE_PATH td_abs BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE td_rel)
        cmake_path(RELATIVE_PATH inc_abs BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE inc_rel)

        add_custom_command(
            OUTPUT "${inc_abs}"
            COMMAND ${tblgen_tool} "${td_abs}" --gen="${gen_name}" --write-if-changed -o "${inc_abs}" -I "${CMAKE_CURRENT_SOURCE_DIR}"
            DEPENDS ${tblgen_tool} "${td_abs}"
            COMMENT "Generating ${inc_rel} (--gen=${gen_name})"
        )
        list(APPEND all_inc_files "${inc_abs}")

        math(EXPR i "${i} + 1")
    endwhile()

    add_custom_target(${target}_tblgen DEPENDS ${all_inc_files})
    add_dependencies(${target} ${target}_tblgen)
endfunction()
