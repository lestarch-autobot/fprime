// ======================================================================
// \title  SeqSchedulerTester.hpp
// \brief  hpp file for SeqScheduler component test harness implementation class
// ======================================================================

#ifndef Svc_SeqSchedulerTester_HPP
#define Svc_SeqSchedulerTester_HPP

#include "Fw/Types/MallocAllocator.hpp"
#include "Svc/SeqScheduler/SeqScheduler.hpp"
#include "Svc/SeqScheduler/SeqSchedulerGTestBase.hpp"

namespace Svc {

class SeqSchedulerTester final : public SeqSchedulerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 200;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;
    static const U32 TICK_USEC = 1000000;  //!< Tick period used by the tests
    static const U32 T0 = 1000;            //!< System time, in seconds, of the first tick
    static const FwSizeType DP_BUFFER_SIZE = 16384;

    using State = SeqScheduler_RowState;
    using Mode = SeqScheduler_TimeMode;
    using Policy = SeqScheduler_BusyPolicy;

    //! A row as written to a table file
    struct FileRow {
        TimeBase::T base;
        U32 seconds;
        U32 useconds;
        U8 mode;
        U8 policy;
        U8 enabled;
        const char* fileName;
    };

  public:
    //! Construct a tester with a table of the given row count
    explicit SeqSchedulerTester(FwSizeType rows = 8);
    ~SeqSchedulerTester();

  public:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Name of the default table file of this tester
    const char* defaultFile() const;
    //! Set system time
    void setTime(U32 seconds, U32 useconds = 0, TimeBase::T base = TimeBase::TB_WORKSTATION_TIME);
    //! Send count ticks, each handled, advancing system time by one tick period before each
    void tick(U32 count = 1);
    //! Send count ticks handled as one, advancing system time by count tick periods
    void tickBatch(U32 count);
    //! Report the running sequence complete
    void done(Fw::CmdResponse::T response = Fw::CmdResponse::OK);
    //! Write a table file; fileRows overrides the row count field, corrupt inverts a row byte
    void writeFile(const char* name, const FileRow* rows, FwSizeType count);
    //! Build table file bytes
    FwSizeType buildFile(U8* bytes, FwSizeType capacity, const FileRow* rows, FwSizeType count);
    //! Write raw bytes to a file
    void writeBytes(const char* name, const U8* bytes, FwSizeType size);
    //! Read up to capacity bytes of a file; the count read, 0 on error
    FwSizeType readBytes(const char* name, U8* bytes, FwSizeType capacity);
    //! Remove a file if present
    void removeFile(const char* name);
    //! Commands, each dispatched
    void cmdLoad();
    void cmdSave();
    void cmdSetRow(FwSizeType row,
                   Mode::T mode,
                   U32 seconds,
                   const char* fileName,
                   Policy::T policy = Policy::WAIT_FOR_RUNNING,
                   U32 useconds = 0,
                   TimeBase::T base = TimeBase::TB_WORKSTATION_TIME);
    void cmdRowEnabled(FwSizeType row, bool enabled);
    void cmdSeqEnabled(const char* fileName, bool enabled);
    void cmdDump();
    void cmdReport();
    void cmdEnabled(bool enabled);
    //! Set and enable an ABSOLUTE row due at T0 + offset seconds
    void absRow(FwSizeType row, U32 offset, const char* fileName, Policy::T policy = Policy::WAIT_FOR_RUNNING);
    //! Start from an empty table: missing default file, first tick at T0, history cleared
    void startEmpty();
    //! Row state
    State::T state(FwSizeType row) const;
    //! Last command response was OK
    void assertLastCmd(FwOpcodeType opCode, Fw::CmdResponse::T response);
    //! Query helper
    bool query(FwSizeType i, Fw::Time& timeDue, Fw::String& fileName);
    //! Count of events of all kinds since clearHistory
    void assertRun(U32 index, FwSizeType row, const char* fileName);

    // ----------------------------------------------------------------------
    // Tests, named as the unit-test list of the SDD
    // ----------------------------------------------------------------------

    void StartupLoad();
    void RowFields();
    void Configure();
    void DefaultTableFile();
    void ShortTableFile();
    void LongTableFile();
    void MissingTableFile();
    void LoadFailures();
    void SetRowAfterFailedLoad();
    void CommandedLoad();
    void LoadKeepsOrigins();
    void AbsoluteDue();
    void AbsoluteOtherBase();
    void StartRelativeDue();
    void TimeBaseRelativeDue();
    void TickRounding();
    void TicksWhileBusy();
    void LoadBeforeFirstTick();
    void TimeBaseRecordFull();
    void WaitForRunning();
    void CancelRunning();
    void SkipWhenRunning();
    void SameTickOrder();
    void DueOrderEarlyTicks();
    void WaitingOrder();
    void WaitingNotLate();
    void NoCompletion();
    void WaitEndedRejudged();
    void CancelSameTick();
    void SecondCancelWaits();
    void CancelSkipsAhead();
    void CancelLastOfSeveralDue();
    void CancelBetweenDue();
    void TwoCancelsDue();
    void FailedLoadKeepsTable();
    void DisableThenEnableSkipsWaiting();
    void FirstTickStandsForSeveral();
    void PingOnFullQueue();
    void SaveWriteFailureKeepsFile();
    void ConfigureAllocation();
    void TimeBaseWarningThrottle();
    void UnsolicitedDone();
    void RunRefused();
    void ExecutingReplaced();
    void LateRow();
    void ToleranceFloor();
    void DisabledRowSkipped();
    void ComponentDisabled();
    void ReportActiveRow();
    void LoadedRowStates();
    void ReloadWithinTolerance();
    void EnableByIndex();
    void EnableByName();
    void SetRow();
    void SetWaitingRow();
    void EditsDuringSequence();
    void SaveTable();
    void SaveFailures();
    void ArgumentErrors();
    void DumpTable();
    void DumpTruncated();
    void DumpNoBuffer();
    void QueryByPosition();
    void QueryLargeTable();
    void QueryAfterEdit();
    void QueryWaitingAfterBaseChange();
    void QueryFromSecondThread();
    void SkipEventsBounded();
    void ToleranceParameterInvalid();
    void QueryRelative();
    void QueryNotUpcoming();
    void QueryInvalid();
    void Telemetry();
    void TwoInstances();
    void ParamReread();
    void Ping();
    void RandomOperations();

  private:
    // ----------------------------------------------------------------------
    // Handlers for data products
    // ----------------------------------------------------------------------

    Fw::Success::T productGet_handler(FwDpIdType id, FwSizeType dataSize, Fw::Buffer& buffer) override;

    void connectPorts();
    void initComponents();

  private:
    SeqScheduler component;                              //!< The component under test
    Fw::MallocAllocator m_allocator;                     //!< Source of the table memory
    FwSizeType m_rows;                                   //!< Row count given to configure
    U32 m_seconds = T0 - 1;                              //!< Current test time, seconds
    U32 m_useconds = 0;                                  //!< Current test time, microseconds
    TimeBase::T m_base = TimeBase::TB_WORKSTATION_TIME;  //!< Current test time base
    FwSizeType m_dpAsked = 0;                            //!< Data size of the last product request
    bool m_dpGrant = true;                               //!< Grant product buffers
    FwSizeType m_dpLimit = DP_BUFFER_SIZE;               //!< Largest product buffer granted
    U8 m_dpStorage[DP_BUFFER_SIZE];                      //!< Product buffer storage
    U32 m_cmdSeq = 0;                                    //!< Command sequence number
    Fw::String m_defaultFile;                            //!< Default table file name
};

}  // namespace Svc

#endif
