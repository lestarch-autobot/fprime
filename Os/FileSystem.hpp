// ======================================================================
// \title Os/FileSystem.hpp
// \brief public Os::FileSystem interface and alias
//
// This header aggregates all definitions needed to use Os::FileSystem:
// the interface, the configured alias, and the selected concrete type.
//
// WARNING — include order is load-bearing. Do not reorder.
//
//   1. config/OsSelection.hpp (CFG) defines the Os::FileSystem type alias by
//      forward-declaring the link-time delegate (DelegateFileSystem) or an
//      implementation selected at compile time. It must not include Os OSAL
//      headers. It is included by Os/FileSystemInterface.hpp.
//
//   2. Os/FileSystemInterface.hpp (IF) includes CFG, then defines
//      FileSystemHandle and FileSystemInterface.
//
//   3. OS_FILE_SYSTEM_HEADER (IMPL) is defined by CFG and names the header
//      defining the type selected as Os::FileSystem: Os/DelegateFileSystem.hpp
//      for the link-time delegate, or the implementation header.
//
// FileSystemInterface.hpp must precede OS_FILE_SYSTEM_HEADER here, and CFG
// must never include either of them (that would form a cycle).
// ======================================================================
#ifndef _OS_FILESYSTEM_HPP_
#define _OS_FILESYSTEM_HPP_

#include "Os/FileSystemInterface.hpp"

#include OS_FILE_SYSTEM_HEADER

#endif
