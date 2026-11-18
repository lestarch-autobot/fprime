// ======================================================================
// \title Os/Directory.hpp
// \brief public Os::Directory interface and alias
//
// This header aggregates all definitions needed to use Os::Directory:
// the interface, the configured alias, and the concrete delegate type.
//
// The includes below must appear in the order given: each depends on names
// defined by the one before it, so reordering them will fail to compile.
// The ordering constraints are:
//
//   1. config/OsSelection.hpp (CFG) defines the Os::Directory type alias by
//      forward-declaring a link-time delegate (e.g. DelegateDirectory) or directly
//      aliasing a concrete implementation.
//      Must not include Os OSAL headers (they aren't yet defined).
//      Included by Os/DirectoryInterface.hpp, which this header includes first.
//
//   2. Os/DirectoryInterface.hpp (IF) includes CFG first (so the Os::Directory alias is
//      available), then defines DirectoryHandle and DirectoryInterface.
//
//   3. OS_DIRECTORY_HEADER (IMPL) is defined by CFG and points to the concrete
//      implementation header. If using delegation, this points to
//      Os/DelegateDirectory.hpp. If using compile-time selection, it points
//      directly to a platform-specific implementation. If CFG defines neither
//      the alias nor OS_DIRECTORY_HEADER, the link-time delegate is used.
//
// DirectoryInterface.hpp must precede OS_DIRECTORY_HEADER here,
// and CFG must never include either of them (that would form a cycle).
// ======================================================================
#ifndef Os_Directory_hpp
#define Os_Directory_hpp

#include "Os/DirectoryInterface.hpp"

#ifdef OS_DIRECTORY_HEADER
#include OS_DIRECTORY_HEADER
#else
// A project config/OsSelection.hpp that predates the Directory selection defines neither the Os::Directory
// alias nor OS_DIRECTORY_HEADER: keep the link-time delegate so such projects build unchanged.
#include "Os/DelegateDirectory.hpp"
namespace Os {
using Directory = DelegateDirectory;
}  // namespace Os
#endif

#endif  // Os_Directory_hpp
