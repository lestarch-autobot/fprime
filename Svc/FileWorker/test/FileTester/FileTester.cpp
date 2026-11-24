#include "FileTester.hpp"

namespace Svc {
Os::FileInterface::Status FileTester::m_statOpen = Os::FileInterface::Status();
Os::FileInterface::Status FileTester::m_statRead = Os::FileInterface::Status();
Os::FileInterface::Status FileTester::m_statWrite = Os::FileInterface::Status();
Os::FileInterface::Status FileTester::m_statSize = Os::FileInterface::Status();
FwSizeType FileTester::m_size = 0;
}  // namespace Svc
