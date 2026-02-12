find_package(LLVM REQUIRED CONFIG)

function(configure_llvm project_name)
    llvm_map_components_to_libnames(
        llvm_libs
        core
        support
        bitwriter
        transformUtils
        native
        AllTargetsCodeGens
        AllTargetsDescs
        AllTargetsInfos
        TargetParser
    )
    target_include_directories(${project_name} SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(${project_name} PUBLIC ${LLVM_DEFINITIONS})
    target_link_libraries(${project_name} PRIVATE ${llvm_libs})
endfunction()

function(configure_tblgen project_name)
    llvm_map_components_to_libnames(
        llvm_libs
        core
        support
        TableGen
    )
    target_include_directories(${project_name} SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(${project_name} PUBLIC ${LLVM_DEFINITIONS})
    target_link_libraries(${project_name} PRIVATE ${llvm_libs})
endfunction()
