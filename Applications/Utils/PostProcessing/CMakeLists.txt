
if(OGS_BUILD_PROCESS_LIE)
    add_executable(postLIE postLIE.cpp)
    target_link_libraries(postLIE LIECommon)
    set_target_properties(postLIE PROPERTIES FOLDER Utilities)

    ####################
    ### Installation ###
    ####################
    install(TARGETS
        postLIE
        RUNTIME DESTINATION bin
        COMPONENT Utilities
    )
endif()
