/*
 * PrmDbTester.hpp
 *
 *  Created on: Mar 18, 2015
 *      Author: tcanham
 */

#ifndef PRMDB_TEST_UT_PRMDBTESTER_HPP_
#define PRMDB_TEST_UT_PRMDBTESTER_HPP_

#include <Os/test/ut/interceptors/FileTestInterceptor.hpp>
#include <PrmDbGTestBase.hpp>
#include <Svc/PrmDb/PrmDbImpl.hpp>
#include <config/PrmDbImplTesterCfg.hpp>

namespace Svc {

class PrmDbTester : public PrmDbGTestBase {
  public:
    PrmDbTester(Svc::PrmDbImpl& inst);
    virtual ~PrmDbTester();

    void runNominalPopulate();
    void runNominalSaveFile();
    void runNominalLoadFile();
    void runMissingExtraParams();
    void runFileReadError();
    void runFileWriteError();
    void runDbEqualTest();
    void runDbCopyTest();
    void runDbCommitTest();
    void runPrmFileLoadNominal();
    void runPrmFileLoadWithErrors();
    void runPrmFileLoadIllegal();
    void runPrmFileLoadEmptyFileName();
    void runPrmFileLoadSandboxViolation();
    void runShorterSaveDoesNotCorrupt();

    void runRefPrmFile();

  private:
    bool dbEqual();

    //! Handler for from_pingOut
    //!
    void from_pingOut_handler(const FwIndexType portNum, /*!< The port number*/
                              U32 key                    /*!< Value to return to pinger*/
    );
    Svc::PrmDbImpl& m_impl;
    void resetEvents();

    // enumeration to tell what kind of error to inject
    enum ErrorType {
        FILE_STATUS_ERROR,   // return a bad read status
        FILE_SIZE_ERROR,     // return a bad size
        FILE_DATA_ERROR,     // return unexpected data
        FILE_READ_NO_ERROR,  // No error
    };
    Os::File::Status m_status;
    FwSizeType m_waits = 0;
    ErrorType m_errorType = FILE_READ_NO_ERROR;

    BYTE m_io_data[PRMDB_IMPL_TESTER_MAX_READ_BUFFER];

    // write call modifiers

    Os::File::Status WriteInterceptor();
    Os::File::Status m_testWriteStatus;

  public:
    //! Replaces every Os::File call with an in-memory file over the buffers set below; never calls `real`
    class PrmDbTestFile : public Os::Test::FileTestInterceptor {
      public:
        Status open(Os::FileInterface& real, const char* path, Mode mode, OverwriteType overwrite) override;
        void close(Os::FileInterface& real) override;
        Status size(Os::FileInterface& real, FwSizeType& size_result) override;
        Status position(Os::FileInterface& real, FwSizeType& position_result) override;
        Status preallocate(Os::FileInterface& real, FwSizeType offset, FwSizeType length) override;
        Status seek(Os::FileInterface& real, FwSignedSizeType offset, SeekType seekType) override;
        Status flush(Os::FileInterface& real) override;
        Status read(Os::FileInterface& real, U8* buffer, FwSizeType& size, WaitType wait) override;
        Status write(Os::FileInterface& real, const U8* buffer, FwSizeType& size, WaitType wait) override;

        //! Set the status returned by every following call
        void setNextStatus(Os::File::Status status);
        //! Set the data returned by following read calls
        void setReadResult(U8* buffer, FwSizeType size);
        //! Set the buffer filled by following write calls
        void setWriteResult(U8* buffer, FwSizeType size);

        // Tracks the current tester
        static void setTester(PrmDbTester* tester);
        static PrmDbTester* s_tester;

        //! File pointer
        FwSizeType pointer = 0;

      private:
        Status fakeRead(U8* buffer, FwSizeType& size);
        Status fakeWrite(const U8* buffer, FwSizeType& size);

        Os::File::Status m_nextStatus = Os::File::OP_OK;
        U8* m_readResult = nullptr;
        FwSizeType m_readResultSize = 0;
        U8* m_writeResult = nullptr;
        FwSizeType m_writeResultSize = 0;
    };

  private:
    //! Intercepts every Os::File call while this tester exists
    PrmDbTestFile m_fileInterceptor;

    //! Expose PrmDbImpl::computeCrc for hardcoded CRC value testing
};

}  // namespace Svc

#endif /* PRMDB_TEST_UT_PRMDBTESTER_HPP_ */
