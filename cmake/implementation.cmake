####
# implementation.cmake:
#
# This file handles enables deployments (and other executables) to choose implementations that resolve
# at link time. This enables projects to choose different underlying implementations for specific concepts
# (e.g. OS, Memory, etc.) at link time.
####
include_guard()
include(config_assembler)

####
# Function `fprime_target_implementations`:
#
# This function will calculate and add the implementations required for a target. It will use the default
# implementations if no OVERRIDES are provided. Default implementations are the final set of chosen
# implementations as specified in `register_fprime_config` using the CHOOSES_IMPLEMENTATIONS call.
#
# Choosing implementations ensures that there is some implementation of all globally required
# implementations specified by `register_fprime_*` via the REQUIRED_IMPLEMENTATIONS argument.
#
# > [!WARNING]
# > This function may only be called on targets that already exist
#
# > [!NOTE]
# > Target will be updated with FPRIME_CHOSEN_IMPLEMENTATIONS property and FPRIME_CHOSEN_<IMPLEMENTATION>
# > properties for each implementation.  `target_link_libraries` will be updated.
#
# Args:
#  **BUILD_SYSTEM_TARGET**: the target in the build system to add implementations dependencies to
#  **ARGN**: a list of implementations to override the default implementations with
####
function(fprime_target_implementations BUILD_SYSTEM_TARGET)
    # Recorded for fprime__internal_flush_implementation_configuration: implementations supplying
    # CONFIGURATION_HEADERS may only be chosen globally, and are not all registered yet
    foreach (TARGET_CHOICE IN LISTS ARGN)
        append_list_property("${BUILD_SYSTEM_TARGET}=${TARGET_CHOICE}" GLOBAL PROPERTY FPRIME_TARGET_CHOSEN_IMPLEMENTATIONS)
    endforeach()
    append_list_property("${ARGN}" TARGET "${BUILD_SYSTEM_TARGET}" PROPERTY FPRIME_CHOSEN_IMPLEMENTATIONS)
    fprime__internal_choose_implementations("${BUILD_SYSTEM_TARGET}" INTERNAL_ALL_IMPLEMENTATIONS)

    set(LINK_SCOPE)
    if (NOT FPRIME_USE_PLAIN_LINK_SIGNATURE)
        set(LINK_SCOPE PRIVATE)
    endif()
    fprime_target_dependencies("${BUILD_SYSTEM_TARGET}" "${LINK_SCOPE}" "${INTERNAL_ALL_IMPLEMENTATIONS}")
endfunction()

####
# Function `fprime__internal_implementation_detect_implementations`:
#
# This function will scan the list of global default implementations (detected via the config modules and set in
# the sub-build) and the implementations chosen by the current module. It will then choose one implementation for
# each "IMPLEMENTS" type.
#
# If IMPLEMENTS cannot be determined it is added to the list of unknown implementations.
#
# Args:
#  CURRENT_MODULE: the module to check for implementations
####
function(fprime__internal_implementation_detect_implementations CURRENT_MODULE)
    get_target_property(MODULE_CHOSEN "${CURRENT_MODULE}" FPRIME_CHOSEN_IMPLEMENTATIONS)
    get_property(CONFIG_CHOSEN GLOBAL PROPERTY FPRIME_BASE_CHOSEN_IMPLEMENTATIONS)
    fprime_cmake_debug_message("[implementation] ${CURRENT_MODULE} has chosen implementations: ${MODULE_CHOSEN}")   
    fprime_cmake_debug_message("[implementation]     and base implementations: ${CONFIG_CHOSEN}")   

    foreach (IMPLEMENTATION IN LISTS CONFIG_CHOSEN MODULE_CHOSEN)
        # Break our from empty property
        if (NOT IMPLEMENTATION)
            continue()
        endif()
        # If we know about this implementation, add it as a chosen property
        get_property(IMPLEMENTS GLOBAL PROPERTY "FPRIME_${IMPLEMENTATION}_IMPLEMENTS")
        if (IMPLEMENTS)
            set_target_properties("${CURRENT_MODULE}" PROPERTIES "FPRIME_CHOSEN_${IMPLEMENTS}" "${IMPLEMENTATION}")
        # # None implementation is a special case, the caller is responsible for 
        # elseif(IMPLEMENTATION MATCHES ".*None")
        # Otherwise add it as an unknown implementation
        elseif(NOT FPRIME_IS_SUB_BUILD)
            message(WARNING "[implementation] ${CURRENT_MODULE} has chosen unknown implementation: ${IMPLEMENTATION}")
            append_list_property("${IMPLEMENTATION}" TARGET "${CURRENT_MODULE}" PROPERTY FPRIME_UNKNOWN_IMPLEMENTATIONS)
        endif()
    endforeach()
endfunction()

####
# Function `fprime__internal_choose_implementations`:
#
# This function will scan the list of global default implementations module choices via
# `fprime__internal_implementation_detect_implementations`. It will then ensure that each REQUIRED implementation
# has a chosen implementation.
#
# It is an error to not have a chosen implementation for required unless there is a set of unknown implementations
# and in that case it is a warning.
#
# > [!WARNING]
# > If UNKNOWN implementations are set, it is assumed that one of them fills any missing REQUIRED implementations
# > but users should update those modules to explicitly set IMPLEMENTS flag to avoid this warning.
# >
# > If the UNKNOWN implementations do not provided the required implementations a linker error will occur.
#
# Args:
#  CURRENT_MODULE: the module to check for implementations
#  OUTPUT_VARIABLE: the variable to set with the list of chosen implementations
####
function(fprime__internal_choose_implementations CURRENT_MODULE OUTPUT_VARIABLE)
    set(IMPLEMENTATION_DEPENDENCIES)
    fprime__internal_implementation_detect_implementations("${CURRENT_MODULE}")
    
    # Need to know the modules that were chosen but not known yet
    get_target_property(UNKNOWNS "${CURRENT_MODULE}" FPRIME_UNKNOWN_IMPLEMENTATIONS)
    if (UNKNOWNS)
        list(APPEND IMPLEMENTATION_DEPENDENCIES "${UNKNOWNS}")
    endif()

    # Get all required implementations
    get_property(REQUIRED_IMPLEMENTATIONS GLOBAL PROPERTY "FPRIME_REQUIRED_IMPLEMENTATIONS")
    foreach(REQUIRED IN LISTS REQUIRED_IMPLEMENTATIONS)
        get_target_property(IMPLEMENTOR "${CURRENT_MODULE}" "FPRIME_CHOSEN_${REQUIRED}")
        if (IMPLEMENTOR)
            fprime_cmake_debug_message("[implementation] ${CURRENT_MODULE} has chosen implementation: ${IMPLEMENTOR} for ${REQUIRED}")
            list(APPEND IMPLEMENTATION_DEPENDENCIES "${IMPLEMENTOR}")
        elseif(UNKNOWNS AND NOT FPRIME_IS_SUB_BUILD)
            fprime_cmake_warning("[implementation] ${CURRENT_MODULE} requires implementation of ${REQUIRED} assuming one of ${UNKNOWNS} is correct")
        elseif(NOT FPRIME_IS_SUB_BUILD)
            fprime_cmake_fatal_error("[implementation] ${CURRENT_MODULE} requires implementation of ${REQUIRED} but none was chosen")
        endif()
    endforeach()

    append_list_property("${IMPLEMENTATION_DEPENDENCIES}" TARGET "${CURRENT_MODULE}" PROPERTY FPRIME_CHOSEN_IMPLEMENTATIONS)
    set("${OUTPUT_VARIABLE}" "${IMPLEMENTATION_DEPENDENCIES}" PARENT_SCOPE)
endfunction()

####
# Function `fprime__internal_implementation_configuration_headers`:
#
# Records the CONFIGURATION_HEADERS supplied by an implementation. These headers are placed in the implementation
# configuration directory (included as `config/<header name>`) if, and only if, the implementation is the one chosen
# for what it implements by the configuration modules of the build (`register_fprime_config` CHOOSES_IMPLEMENTATIONS).
# This keeps a header-level (compile-time) selection in step with the implementation that is linked.
#
# The headers are placed by `fprime__internal_flush_implementation_configuration`, deferred to the end of the top-level
# directory: all implementations, configuration modules, and targets are known by then.
#
# Args:
#  IMPLEMENTATION: name of the implementation supplying the headers
#  HEADERS: list of headers supplied
####
function(fprime__internal_implementation_configuration_headers IMPLEMENTATION HEADERS)
    if (FPRIME_IS_SUB_BUILD)
        return()
    endif()
    set(RESOLVED_HEADERS)
    set(HEADER_NAMES)
    foreach (HEADER IN LISTS HEADERS)
        get_filename_component(HEADER "${HEADER}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
        get_filename_component(HEADER_NAME "${HEADER}" NAME)
        if (NOT EXISTS "${HEADER}" OR IS_DIRECTORY "${HEADER}")
            fprime_cmake_fatal_error("${IMPLEMENTATION} supplies CONFIGURATION_HEADERS file that does not exist: ${HEADER}")
        elseif (HEADER_NAME IN_LIST HEADER_NAMES)
            fprime_cmake_fatal_error("${IMPLEMENTATION} supplies multiple CONFIGURATION_HEADERS named: ${HEADER_NAME}")
        endif()
        list(APPEND RESOLVED_HEADERS "${HEADER}")
        list(APPEND HEADER_NAMES "${HEADER_NAME}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${HEADER}")
    endforeach()
    set_property(GLOBAL PROPERTY "FPRIME_${IMPLEMENTATION}_CONFIGURATION_HEADERS" "${RESOLVED_HEADERS}")

    get_property(FLUSH_SCHEDULED GLOBAL PROPERTY FPRIME_IMPLEMENTATION_CONFIG_FLUSH_SCHEDULED)
    if (NOT FLUSH_SCHEDULED)
        set_property(GLOBAL PROPERTY FPRIME_IMPLEMENTATION_CONFIG_FLUSH_SCHEDULED TRUE)
        # Headers are included as config/<header name>
        set(IMPLEMENTATION_CONFIG_ROOT "${CMAKE_BINARY_DIR}/fprime-implementation-config")
        set_property(GLOBAL PROPERTY FPRIME_IMPLEMENTATION_CONFIG_DIRECTORY "${IMPLEMENTATION_CONFIG_ROOT}/config")
        file(MAKE_DIRECTORY "${IMPLEMENTATION_CONFIG_ROOT}/config")
        target_include_directories("${FPRIME_GLOBAL_INTERFACE_TARGET}" INTERFACE "${IMPLEMENTATION_CONFIG_ROOT}")
        cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
                       CALL fprime__internal_flush_implementation_configuration)
    endif()
endfunction()

####
# Function `fprime__internal_flush_implementation_configuration`:
#
# Places the CONFIGURATION_HEADERS of the globally chosen implementations into the implementation configuration
# directory, and removes any other file from it. The globally chosen implementations are resolved exactly as
# `fprime__internal_implementation_detect_implementations` resolves them for linking: the last implementation chosen
# for each implemented package wins.
#
# The following are fatal errors:
#  1. An executable, deployment, or unit test choosing (CHOOSES_IMPLEMENTATIONS) an implementation that supplies
#     CONFIGURATION_HEADERS: the headers are global to the build, so the choice must be made in a configuration module
#  2. A globally chosen implementation that supplies no CONFIGURATION_HEADERS when another implementation of the same
#     package does: the code using the package would not find its header
#  3. Two globally chosen implementations supplying a header of the same name with differing content
#  4. A supplied header named as a file of a configuration module (`register_fprime_config`): both are included as
#     `config/<header name>`. Consequently such headers cannot be replaced with CONFIGURATION_OVERRIDES.
####
function(fprime__internal_flush_implementation_configuration)
    get_property(CONFIG_DIRECTORY GLOBAL PROPERTY FPRIME_IMPLEMENTATION_CONFIG_DIRECTORY)

    # 1. Per-target choices may not have configuration headers
    get_property(TARGET_CHOICES GLOBAL PROPERTY FPRIME_TARGET_CHOSEN_IMPLEMENTATIONS)
    foreach (TARGET_CHOICE IN LISTS TARGET_CHOICES)
        string(REGEX MATCH "^([^=]*)=(.*)$" MATCHED "${TARGET_CHOICE}")
        get_property(CHOICE_HEADERS GLOBAL PROPERTY "FPRIME_${CMAKE_MATCH_2}_CONFIGURATION_HEADERS")
        if (CHOICE_HEADERS)
            fprime_cmake_fatal_error(
                "[implementation] ${CMAKE_MATCH_1} chooses ${CMAKE_MATCH_2}, which supplies CONFIGURATION_HEADERS."
                "Such implementations are selected at compile time for the whole build, and may only be chosen via"
                "CHOOSES_IMPLEMENTATIONS in register_fprime_config."
            )
        endif()
    endforeach()

    # Resolve the global choices: last chosen for each implemented package wins
    get_property(GLOBAL_CHOSEN GLOBAL PROPERTY FPRIME_BASE_CHOSEN_IMPLEMENTATIONS)
    set(IMPLEMENTED_PACKAGES)
    foreach (IMPLEMENTATION IN LISTS GLOBAL_CHOSEN)
        if (NOT IMPLEMENTATION)
            continue()
        endif()
        get_property(IMPLEMENTS GLOBAL PROPERTY "FPRIME_${IMPLEMENTATION}_IMPLEMENTS")
        if (IMPLEMENTS)
            set("CHOSEN_${IMPLEMENTS}" "${IMPLEMENTATION}")
            list(APPEND IMPLEMENTED_PACKAGES "${IMPLEMENTS}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES IMPLEMENTED_PACKAGES)

    set(PLACED_NAMES)
    set(PLACED_SOURCES)
    set(PLACED_IMPLEMENTATIONS)
    foreach (IMPLEMENTS IN LISTS IMPLEMENTED_PACKAGES)
        set(IMPLEMENTATION "${CHOSEN_${IMPLEMENTS}}")
        get_property(HEADERS GLOBAL PROPERTY "FPRIME_${IMPLEMENTATION}_CONFIGURATION_HEADERS")
        # 2. The chosen implementation must supply headers when its alternatives do
        if (NOT HEADERS)
            get_property(IMPLEMENTORS GLOBAL PROPERTY "FPRIME_${IMPLEMENTS}_IMPLEMENTORS")
            foreach (IMPLEMENTOR IN LISTS IMPLEMENTORS)
                get_property(IMPLEMENTOR_HEADERS GLOBAL PROPERTY "FPRIME_${IMPLEMENTOR}_CONFIGURATION_HEADERS")
                if (IMPLEMENTOR_HEADERS)
                    fprime_cmake_fatal_error(
                        "[implementation] ${IMPLEMENTATION} is chosen for ${IMPLEMENTS} but supplies no"
                        "CONFIGURATION_HEADERS, whereas ${IMPLEMENTOR} does. Supply them in the"
                        "register_fprime_implementation call of ${IMPLEMENTATION}."
                    )
                endif()
            endforeach()
            continue()
        endif()
        foreach (HEADER IN LISTS HEADERS)
            get_filename_component(HEADER_NAME "${HEADER}" NAME)
            list(FIND PLACED_NAMES "${HEADER_NAME}" PLACED_INDEX)
            # 3. Chosen implementations must agree on the content of a header
            if (NOT PLACED_INDEX EQUAL -1)
                list(GET PLACED_SOURCES ${PLACED_INDEX} PLACED_SOURCE)
                list(GET PLACED_IMPLEMENTATIONS ${PLACED_INDEX} PLACED_IMPLEMENTATION)
                execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${PLACED_SOURCE}" "${HEADER}"
                                RESULT_VARIABLE HEADERS_DIFFER OUTPUT_QUIET ERROR_QUIET)
                if (HEADERS_DIFFER)
                    fprime_cmake_fatal_error(
                        "[implementation] chosen implementations ${PLACED_IMPLEMENTATION} and ${IMPLEMENTATION}"
                        "supply differing CONFIGURATION_HEADERS named ${HEADER_NAME}:"
                        "${PLACED_SOURCE} and ${HEADER}"
                    )
                endif()
                continue()
            endif()
            list(APPEND PLACED_NAMES "${HEADER_NAME}")
            list(APPEND PLACED_SOURCES "${HEADER}")
            list(APPEND PLACED_IMPLEMENTATIONS "${IMPLEMENTATION}")
        endforeach()
    endforeach()

    # 4. Headers may not share a name with a configuration module file
    set(CONFIG_MODULE_DIRECTORIES)
    get_property(CONFIG_MODULES GLOBAL PROPERTY FPRIME_CONFIG_MODULES)
    foreach (CONFIG_MODULE IN LISTS CONFIG_MODULES)
        get_target_property(CONFIG_SOURCES "${CONFIG_MODULE}" SUPPLIED_SOURCES)
        get_target_property(CONFIG_HEADERS "${CONFIG_MODULE}" SUPPLIED_HEADERS)
        get_target_property(CONFIG_AUTOCODER_INPUTS "${CONFIG_MODULE}" SUPPLIED_AUTOCODER_INPUTS)
        foreach (CONFIG_FILE IN LISTS CONFIG_SOURCES CONFIG_HEADERS CONFIG_AUTOCODER_INPUTS)
            get_filename_component(CONFIG_NAME "${CONFIG_FILE}" NAME)
            get_filename_component(CONFIG_MODULE_DIRECTORY "${CONFIG_FILE}" DIRECTORY)
            list(APPEND CONFIG_MODULE_DIRECTORIES "${CONFIG_MODULE_DIRECTORY}")
            list(FIND PLACED_NAMES "${CONFIG_NAME}" PLACED_INDEX)
            if (NOT PLACED_INDEX EQUAL -1)
                list(GET PLACED_IMPLEMENTATIONS ${PLACED_INDEX} PLACED_IMPLEMENTATION)
                fprime_cmake_fatal_error(
                    "[implementation] configuration module ${CONFIG_MODULE} supplies ${CONFIG_NAME}, which is"
                    "supplied as CONFIGURATION_HEADERS by the chosen implementation ${PLACED_IMPLEMENTATION}."
                    "Choose another implementation (CHOOSES_IMPLEMENTATIONS) rather than supplying the file."
                )
            endif()
        endforeach()
    endforeach()

    # A build cache generated when a configuration module supplied such a file still holds it, where it is found as
    # config/<header name> as well. No configuration module supplies it now (4.), so it is removed. Only files in the
    # build cache are removed: source trees are never touched.
    list(REMOVE_DUPLICATES CONFIG_MODULE_DIRECTORIES)
    foreach (CONFIG_MODULE_DIRECTORY IN LISTS CONFIG_MODULE_DIRECTORIES)
        cmake_path(IS_PREFIX CMAKE_BINARY_DIR "${CONFIG_MODULE_DIRECTORY}" NORMALIZE IN_BUILD_CACHE)
        if (NOT IN_BUILD_CACHE)
            continue()
        endif()
        foreach (HEADER_NAME IN LISTS PLACED_NAMES)
            if (EXISTS "${CONFIG_MODULE_DIRECTORY}/${HEADER_NAME}")
                fprime_cmake_status("[implementation] Removing stale ${CONFIG_MODULE_DIRECTORY}/${HEADER_NAME}")
                file(REMOVE "${CONFIG_MODULE_DIRECTORY}/${HEADER_NAME}")
            endif()
        endforeach()
    endforeach()

    # Place the headers, and remove anything left by a previous choice
    foreach (HEADER_NAME HEADER IN ZIP_LISTS PLACED_NAMES PLACED_SOURCES)
        file(COPY_FILE "${HEADER}" "${CONFIG_DIRECTORY}/${HEADER_NAME}" ONLY_IF_DIFFERENT)
    endforeach()
    file(GLOB EXISTING_FILES LIST_DIRECTORIES TRUE RELATIVE "${CONFIG_DIRECTORY}" "${CONFIG_DIRECTORY}/*")
    foreach (EXISTING_FILE IN LISTS EXISTING_FILES)
        if (NOT EXISTING_FILE IN_LIST PLACED_NAMES)
            file(REMOVE_RECURSE "${CONFIG_DIRECTORY}/${EXISTING_FILE}")
        endif()
    endforeach()
endfunction()
