####
# dictionary_check.cmake:
#
# Dictionary check stage. Runs the checkers registered with `register_fprime_dictionary_check` on the topology
# dictionaries of a deployment and compiles the source file each checker generates into the deployment. A checker
# typically generates `static_assert`s, so that a dictionary that violates a constraint fails the deployment build.
#
# A checker runs only for the deployments that depend on the module that registered it. It is invoked as:
#
#   <python> <script> <dictionary> -o <output source file> [-D <variable>=<value>]...
#
# and must exit non-zero, writing no output file, when the dictionary cannot be checked.
####
include_guard()
include(target/dictionary)

####
# Function `dictionary_check_add_global_target`:
#
# The dictionary check has no global target.
#
# - **TARGET:** target name
####
function(dictionary_check_add_global_target TARGET)
endfunction(dictionary_check_add_global_target)

####
# Function `dictionary_check_add_module_target`:
#
# The dictionary check does nothing for modules: dictionaries belong to deployments.
#
# - **MODULE:** name of the module
# - **TARGET:** name of target to produce
# - **SOURCES:** list of source file inputs
# - **DEPENDENCIES:** MOD_DEPS input from CMakeLists.txt
####
function(dictionary_check_add_module_target MODULE TARGET SOURCES DEPENDENCIES)
endfunction(dictionary_check_add_module_target)

####
# Function `dictionary_check_output_name`:
#
# Names the source file a checker generates for a dictionary: `<Deployment><CheckerName>.cpp`, where `<Deployment>` is
# the dictionary file name without `TopologyDictionary.json` and `<CheckerName>` is the script's file name without
# extension converted from snake_case to CamelCase (`tlm_packetizer_sizing.py` gives `TlmPacketizerSizing`).
#
# - **DICTIONARY:** path to the topology dictionary
# - **SCRIPT:** path to the checker script
# - **OUTPUT_VAR:** variable to set with the file name
####
function(dictionary_check_output_name DICTIONARY SCRIPT OUTPUT_VAR)
    get_filename_component(DEPLOYMENT_NAME "${DICTIONARY}" NAME)
    string(REGEX REPLACE "TopologyDictionary\\.json$" "" DEPLOYMENT_NAME "${DEPLOYMENT_NAME}")
    get_filename_component(SCRIPT_NAME "${SCRIPT}" NAME_WE)
    string(REPLACE "_" ";" SCRIPT_WORDS "${SCRIPT_NAME}")
    set(CHECKER_NAME "")
    foreach(WORD IN LISTS SCRIPT_WORDS)
        string(SUBSTRING "${WORD}" 0 1 FIRST_LETTER)
        string(SUBSTRING "${WORD}" 1 -1 REMAINDER)
        string(TOUPPER "${FIRST_LETTER}" FIRST_LETTER)
        string(APPEND CHECKER_NAME "${FIRST_LETTER}${REMAINDER}")
    endforeach()
    set(${OUTPUT_VAR} "${DEPLOYMENT_NAME}${CHECKER_NAME}.cpp" PARENT_SCOPE)
endfunction(dictionary_check_output_name)

####
# Function `dictionary_check_add_deployment_target`:
#
# Runs every registered checker whose owning module the deployment depends on, once per topology dictionary of the
# deployment, and adds the generated source files to the deployment.
#
# - **MODULE:** name of the deployment module
# - **TARGET:** name of this target
# - **SOURCES:** list of source file inputs
# - **DEPENDENCIES:** MOD_DEPS input from CMakeLists.txt
# - **FULL_DEPENDENCIES:** recursive dependencies of the deployment
####
function(dictionary_check_add_deployment_target MODULE TARGET SOURCES DEPENDENCIES FULL_DEPENDENCIES)
    get_property(CHECK_MODULES GLOBAL PROPERTY FPRIME_DICTIONARY_CHECK_MODULES)
    get_property(CHECK_SCRIPTS GLOBAL PROPERTY FPRIME_DICTIONARY_CHECK_SCRIPTS)
    get_property(CHECK_VARIABLES GLOBAL PROPERTY FPRIME_DICTIONARY_CHECK_VARIABLES)
    list(LENGTH CHECK_MODULES CHECK_COUNT)
    if (CHECK_COUNT EQUAL 0)
        return()
    endif()

    # Dictionaries come from the deployment itself or from a dependency (e.g. its Top module)
    set(DICTIONARY_MODULES "${FULL_DEPENDENCIES}")
    list(REMOVE_ITEM DICTIONARY_MODULES "${MODULE}")
    dictionary_dependencies_with_dictionaries("${DICTIONARY_MODULES}" dictionary DICTIONARY_MODULES)
    if (TARGET "${MODULE}_dictionary")
        list(APPEND DICTIONARY_MODULES "${MODULE}")
    endif()
    set(DICTIONARIES)
    foreach(DICTIONARY_MODULE IN LISTS DICTIONARY_MODULES)
        get_target_property(DICTIONARY_FILES "${DICTIONARY_MODULE}" FPRIME_DICTIONARIES)
        if (DICTIONARY_FILES)
            list(APPEND DICTIONARIES ${DICTIONARY_FILES})
            # The dictionary may be generated in another directory: order the deployment after its producer
            add_dependencies("${MODULE}" "${DICTIONARY_MODULE}_dictionary")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES DICTIONARIES)

    math(EXPR CHECK_LAST "${CHECK_COUNT} - 1")
    foreach(CHECK_INDEX RANGE ${CHECK_LAST})
        list(GET CHECK_MODULES ${CHECK_INDEX} CHECK_MODULE)
        list(GET CHECK_SCRIPTS ${CHECK_INDEX} CHECK_SCRIPT)
        list(GET CHECK_VARIABLES ${CHECK_INDEX} CHECK_VARIABLE_NAMES)
        if (NOT CHECK_MODULE IN_LIST FULL_DEPENDENCIES)
            continue()
        endif()
        # Variables are read here, in the scope of the deployment's registration
        set(CHECK_DEFINITIONS)
        string(REPLACE "," ";" CHECK_VARIABLE_NAMES "${CHECK_VARIABLE_NAMES}")
        foreach(CHECK_VARIABLE IN LISTS CHECK_VARIABLE_NAMES)
            if (DEFINED ${CHECK_VARIABLE})
                list(APPEND CHECK_DEFINITIONS -D "${CHECK_VARIABLE}=${${CHECK_VARIABLE}}")
            endif()
        endforeach()
        foreach(DICTIONARY IN LISTS DICTIONARIES)
            dictionary_check_output_name("${DICTIONARY}" "${CHECK_SCRIPT}" CHECK_OUTPUT)
            set(CHECK_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CHECK_OUTPUT}")
            add_custom_command(OUTPUT "${CHECK_OUTPUT}"
                COMMAND "${PYTHON}" "${CHECK_SCRIPT}" "${DICTIONARY}" -o "${CHECK_OUTPUT}" ${CHECK_DEFINITIONS}
                DEPENDS "${DICTIONARY}" "${CHECK_SCRIPT}"
                COMMENT "Checking ${DICTIONARY} with ${CHECK_SCRIPT}"
                VERBATIM
            )
            target_sources("${MODULE}" PRIVATE "${CHECK_OUTPUT}")
        endforeach()
    endforeach()
endfunction(dictionary_check_add_deployment_target)
