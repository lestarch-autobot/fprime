# Configuration
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FaultConfig/")

# Interfaces, ports and types
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Interfaces/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Ports/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Types/")

# Components
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FatalToFault/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FaultManager/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/RebootResponder/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SequenceResponder/")

# Subtopologies
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Subtopology/")


