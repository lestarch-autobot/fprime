// ======================================================================
// \title  SeqScheduler.hpp
// \brief  hpp file for SeqScheduler component implementation class
// ======================================================================

#ifndef Svc_SeqScheduler_HPP
#define Svc_SeqScheduler_HPP

#include <atomic>

#include "Fw/DataStructures/ExternalRedBlackTreeSet.hpp"
#include "Fw/Types/FileNameString.hpp"
#include "Fw/Types/MemAllocator.hpp"
#include "Os/File.hpp"
#include "Svc/SeqScheduler/SeqSchedulerComponentAc.hpp"
#include "Utils/Hash/Hash.hpp"

namespace Svc {

//! A due row, ordered by when it came due, then row index
struct SeqSchedulerDueRow {
    I64 dueUsec = 0;  //!< When the row came due, in microseconds of tick count
    U16 row = 0;      //!< Row index
    bool operator==(const SeqSchedulerDueRow& other) const { return (dueUsec == other.dueUsec) && (row == other.row); }
    bool operator<(const SeqSchedulerDueRow& other) const {
        return (dueUsec < other.dueUsec) || ((dueUsec == other.dueUsec) && (row < other.row));
    }
};

class SeqScheduler final : public SeqSchedulerComponentBase {
    friend class SeqSchedulerTester;

  public:
    //! Largest table, in rows: the table file stores its row count in 16 bits
    static constexpr FwSizeType MAX_ROWS = 65535;

    //! Version of the table file layout
    static constexpr U8 TABLE_FILE_VERSION = 1;

    //! Size of the table file header: version and row count
    static constexpr FwSizeType TABLE_FILE_HEADER_SIZE = 3;

    //! Size of one row in the table file
    static constexpr FwSizeType TABLE_FILE_ROW_SIZE =
        sizeof(FwTimeBaseStoreType) + sizeof(FwTimeContextStoreType) + 2 * sizeof(U32) + 3 + FileNameStringSize + 1;

    //! Construct SeqScheduler object
    SeqScheduler(const char* const compName  //!< The component name
    );

    //! Destroy SeqScheduler object
    ~SeqScheduler();

    SeqScheduler(const SeqScheduler&) = delete;
    SeqScheduler& operator=(const SeqScheduler&) = delete;

    //! Configure the instance; call once, before the component thread starts
    void configure(const Fw::StringBase& defaultTableFile,  //!< Table file used when TABLE_FILE is not set or empty
                   FwSizeType rows,                         //!< Rows of the table; 1 to MAX_ROWS
                   U32 tickPeriodUsec,                      //!< Period of the attached rate group; above zero
                   Fw::MemAllocator& allocator,             //!< Source of the table memory
                   FwEnumStoreType memId = 0                //!< Identifier passed to the allocator
    );

    //! Return the table memory to the allocator; call at teardown
    void cleanup();

  private:
    // ----------------------------------------------------------------------
    // Types
    // ----------------------------------------------------------------------

    //! A due row
    using DueRow = SeqSchedulerDueRow;

    //! Load state
    enum class LoadState : U8 { NOT_TRIED, LOADED_NEW, LOADED, FAILED };

    //! Sequence state
    enum class SeqState : U8 { IDLE, RUNNING, CANCELLING };

    //! Per-row due record
    struct RowDue {
        bool due = false;  //!< The row is among the due rows
        I64 dueUsec = 0;   //!< The value the row was inserted with
    };

    //! A recorded time base
    struct BaseRecord {
        TimeBase base = TimeBase::TB_NONE;  //!< The time base
        U64 firstTick = 0;                  //!< Tick on which it was first reported
        Fw::Time firstTime;                 //!< System time of that tick
    };

    //! The sequence run by this component
    struct Seq {
        SeqState state = SeqState::IDLE;  //!< Sequence state
        FwSizeType row = 0;               //!< Row whose sequence is running or being cancelled
    };

    //! When a row is due
    struct DueInfo {
        bool known = false;  //!< The origin of the row is known
        bool due = false;    //!< The row has come due
        I64 dueUsec = 0;     //!< When the row comes due, in microseconds of tick count
        U64 lateUsec = 0;    //!< Time past due, when due
        Fw::Time timeDue;    //!< Time due as a system time
    };

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    void pingIn_handler(FwIndexType portNum, U32 key) override;

    bool queryIn_handler(FwIndexType portNum, FwSizeType i, Fw::Time& timeDue, Fw::StringBase& fileName) override;

    void run_handler(FwIndexType portNum, U32 context) override;

    void seqDoneIn_handler(FwIndexType portNum,
                           FwOpcodeType opCode,
                           U32 cmdSeq,
                           const Fw::CmdResponse& response) override;

    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    void LOAD_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void SAVE_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void SET_ROW_ENABLED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, FwSizeType row, bool enabled) override;

    void SET_SEQ_ENABLED_cmdHandler(FwOpcodeType opCode,
                                    U32 cmdSeq,
                                    const Fw::CmdStringArg& fileName,
                                    bool enabled) override;

    void SET_ROW_cmdHandler(FwOpcodeType opCode,
                            U32 cmdSeq,
                            FwSizeType row,
                            const Fw::TimeValue& time,
                            const Svc::SeqScheduler_TimeMode& mode,
                            const Fw::CmdStringArg& fileName,
                            const Svc::SeqScheduler_BusyPolicy& busyPolicy) override;

    void DUMP_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void REPORT_ACTIVE_ROW_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void SET_ENABLED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enabled) override;

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined internal interfaces
    // ----------------------------------------------------------------------

    void runInternal_internalInterfaceHandler() override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Resolve the table file name: TABLE_FILE when set and not empty, else the configured default
    void tableFileName(Fw::StringBase& name);

    //! Perform the start-up load if no load has been attempted
    void loadIfNotTried();

    //! Load the table file; startUp selects the start-up rules
    //! \return true when the load succeeded
    bool loadTable(bool startUp);

    //! Read and check the table file into the load copy
    //! \return true when the file was read and checked
    bool readTableFile(const Fw::StringBase& name,        //!< File name
                       bool& missing,                     //!< Set when the file does not exist
                       SeqScheduler_LoadFailure& reason,  //!< Set on failure
                       FwSizeType& fileRows,              //!< Rows the file holds
                       FwSizeType& loadedRows             //!< Rows read into the load copy
    );

    //! Decode one table file row into a row; \return true when the content is valid
    static bool decodeRow(const U8* buffer, SeqScheduler_Row& row);

    //! Encode one row for the table file
    static void encodeRow(const SeqScheduler_Row& row, U8* buffer);

    //! Write the table file; \return true on success
    bool writeTableFile(const Fw::StringBase& name, FwSizeType rows, SeqScheduler_SaveFailure& reason);

    //! Whether a row is empty
    bool isEmpty(FwSizeType row) const;

    //! Rows up to the last one that is not empty
    FwSizeType usedRows() const;

    //! Set a row state under the component lock
    void setRowState(FwSizeType row, SeqScheduler_RowState::T state);

    //! Remove a row from the due rows, if it is among them
    void removeDue(FwSizeType row);

    //! Clear the due rows
    void clearDue();

    //! Record the time base reported on this tick
    void recordBase(U32 ticks);

    //! Find the record of a time base; \return nullptr when not recorded
    const BaseRecord* findBase(TimeBase base) const;

    //! Compute when a row is due
    DueInfo dueInfo(FwSizeType row) const;

    //! Late tolerance in microseconds, not below the tick period
    U64 lateToleranceUsec();

    //! Find the position-th upcoming row; \return true when there is one
    bool findUpcoming(FwSizeType position, FwSizeType& row, Fw::Time& timeDue) const;

    //! Whether a row is upcoming, and its place in the order
    bool upcomingKey(FwSizeType row, DueRow& key, Fw::Time& timeDue) const;

    //! Run the sequence of a row
    void runRow(FwSizeType row);

    //! Tick steps
    void skipDueRowsWhileDisabled();
    void evaluatePendingRows(bool quiet);
    void applyCancelRule();
    void runFirstDueRow();
    void skipBusyRows();
    void writeTelemetry();

    //! Emit CmdRejected and answer the command
    void reject(FwOpcodeType opCode, U32 cmdSeq, SeqScheduler_CmdRejection::T reason);

    // ----------------------------------------------------------------------
    // State
    // ----------------------------------------------------------------------

    SeqScheduler_Row* m_table = nullptr;                //!< The rows
    SeqScheduler_Row* m_loadRows = nullptr;             //!< Rows of the file being loaded
    RowDue* m_rowDue = nullptr;                         //!< Per-row due records
    FwSizeType m_rows = 0;                              //!< Row count given to configure
    Fw::ExternalRedBlackTreeSet<DueRow> m_due;          //!< The due rows
    Fw::MemAllocator* m_allocator = nullptr;            //!< Allocator given to configure
    FwEnumStoreType m_memId = 0;                        //!< Allocator identifier
    void* m_memory = nullptr;                           //!< The allocation
    Fw::FileNameString m_defaultTableFile;              //!< Table file name given to configure
    U32 m_tickPeriodUsec = 0;                           //!< Tick period in microseconds
    bool m_enabled = true;                              //!< The component is enabled
    LoadState m_loadState = LoadState::NOT_TRIED;       //!< Load state
    U64 m_tick = 0;                                     //!< Schedule ticks that have arrived
    std::atomic<U32> m_ticksPending{0};                 //!< Ticks counted by run and not yet handled
    Fw::Time m_tickTime;                                //!< System time read on the last handled tick
    BaseRecord m_bases[SeqSchedulerTimeBases];          //!< Recorded time bases
    FwSizeType m_baseCount = 0;                         //!< Number of recorded time bases
    TimeBase m_lastUnrecordedBase = TimeBase::TB_NONE;  //!< Last base that found the record full
    Seq m_seq;                                          //!< The sequence run by this component
    DpContainer m_dpContainer;                          //!< Table data product container
};

}  // namespace Svc

#endif
