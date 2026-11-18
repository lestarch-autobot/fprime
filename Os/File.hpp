// ======================================================================
// \title Os/File.hpp
// \brief public Os::File interface and alias
//
// This header aggregates all definitions needed to use Os::File:
// the interface, the configured alias, and the concrete implementation type.
//
// The includes below must appear in the order given:
//
//   1. config/OsSelection.hpp (CFG) defines the Os::File type alias by
//      forward-declaring a link-time delegate (DelegateFile) or a concrete
//      implementation (e.g. Os::Posix::File::PosixFile). It must not include
//      Os OSAL headers and should only be included by Os/FileInterface.hpp.
//
//   2. Os/FileInterface.hpp (IF) includes CFG first, then defines FileHandle
//      and FileInterface.
//
//   3. OS_FILE_HEADER (IMPL) is defined by CFG and points to the header of the
//      class Os::File aliases: Os/DelegateFile.hpp for link-time selection, or
//      an implementation header (e.g. Os/Posix/File.hpp) for compile-time
//      selection.
// ======================================================================
#ifndef Os_File_hpp_
#define Os_File_hpp_

#include "Os/FileInterface.hpp"

// Validate that OS_FILE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_FILE_HEADER
#error "OS_FILE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_FILE_HEADER

#endif  // Os_File_hpp_
