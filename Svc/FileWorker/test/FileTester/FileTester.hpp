// ======================================================================
// \title FileTester
// \brief Testing implementation of Os::File
// ======================================================================
#ifndef SVC_FILEWORKER_TEST_UT_FILETESTER_HPP
#define SVC_FILEWORKER_TEST_UT_FILETESTER_HPP

#include "Os/Interfaces/Test/FileTestInterceptor.hpp"

namespace Svc {

//! \brief TEST ONLY: interceptor answering every Os::File call with a preset status
//!
//! While an instance exists no Os::File call reaches the real implementation.
//!
class FileTester : public Os::Test::FileTestInterceptor {
  public:
    Status open(Os::FileInterface& real, const char* path, Mode mode, OverwriteType overwrite) override {
        return m_statOpen;
    }

    void close(Os::FileInterface& real) override { return; }

    Status size(Os::FileInterface& real, FwSizeType& size_result) override {
        size_result = m_size;
        return m_statSize;
    }

    Status position(Os::FileInterface& real, FwSizeType& position_result) override { return OP_OK; }

    Status preallocate(Os::FileInterface& real, FwSizeType offset, FwSizeType length) override { return OP_OK; }

    Status seek(Os::FileInterface& real, FwSignedSizeType offset, SeekType seekType) override { return OP_OK; }

    Status flush(Os::FileInterface& real) override { return OP_OK; }

    Status read(Os::FileInterface& real, U8* buffer, FwSizeType& size, WaitType wait) override { return m_statRead; }

    Status write(Os::FileInterface& real, const U8* buffer, FwSizeType& size, WaitType wait) override {
        return m_statWrite;
    }

    static void setOpen(const Status s) { m_statOpen = s; }
    static void setRead(const Status s) { m_statRead = s; }
    static void setWrite(const Status s) { m_statWrite = s; }
    static void setSize(const Status s, const FwSizeType sz) {
        m_statSize = s;
        m_size = sz;
    }

  private:
    static Status m_statOpen;
    static Status m_statRead;
    static Status m_statWrite;
    static Status m_statSize;
    static FwSizeType m_size;
};

}  // namespace Svc

#endif  // SVC_FILEWORKER_TEST_UT_FILETESTER_HPP
