// ======================================================================
// \title  SeqScheduler.cpp
// \brief  cpp file for SeqScheduler component implementation class
// ======================================================================

#include "Svc/SeqScheduler/SeqScheduler.hpp"

#include <cstring>
#include <new>

#include "Fw/Types/Assert.hpp"
#include "Os/FileSystem.hpp"

namespace Svc {

namespace {

constexpr U64 USEC_PER_SEC = 1000000;

//! Largest tick count, in microseconds: a quarter of the signed 64-bit range
constexpr U64 MAX_TICK_USEC = 0x1FFFFFFFFFFFFFFFULL;
constexpr I64 MAX_SIGNED_USEC = 0x1FFFFFFFFFFFFFFFLL;

//! Round a size up to a multiple of an alignment
FwSizeType alignUp(FwSizeType size, FwSizeType alignment) {
    FW_ASSERT(alignment > 0);
    return ((size + alignment - 1) / alignment) * alignment;
}

//! Write an unsigned value big-endian
void putBigEndian(U8* buffer, U64 value, FwSizeType bytes) {
    FW_ASSERT(buffer != nullptr);
    for (FwSizeType i = 0; i < bytes; i++) {
        buffer[bytes - 1 - i] = static_cast<U8>(value & 0xFF);
        value >>= 8;
    }
}

//! Read an unsigned value big-endian
U64 getBigEndian(const U8* buffer, FwSizeType bytes) {
    FW_ASSERT(buffer != nullptr);
    U64 value = 0;
    for (FwSizeType i = 0; i < bytes; i++) {
        value = (value << 8) | buffer[i];
    }
    return value;
}

//! A time value as microseconds
U64 toUsec(U32 seconds, U32 useconds) {
    return static_cast<U64>(seconds) * USEC_PER_SEC + useconds;
}

//! A system time plus microseconds, in the base of the system time; seconds saturate
Fw::Time addUsec(const Fw::Time& time, U64 usec) {
    const U64 total = toUsec(time.getSeconds(), time.getUSeconds()) + usec;
    const U64 seconds = total / USEC_PER_SEC;
    const U32 clamped = (seconds > 0xFFFFFFFFU) ? 0xFFFFFFFFU : static_cast<U32>(seconds);
    return Fw::Time(time.getTimeBase(), time.getContext(), clamped, static_cast<U32>(total % USEC_PER_SEC));
}

}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SeqScheduler::SeqScheduler(const char* const compName) : SeqSchedulerComponentBase(compName) {}

SeqScheduler::~SeqScheduler() {}

void SeqScheduler::configure(const Fw::StringBase& defaultTableFile,
                             FwSizeType rows,
                             U32 tickPeriodUsec,
                             Fw::MemAllocator& allocator,
                             FwEnumStoreType memId) {
    FW_ASSERT(this->m_memory == nullptr);
    FW_ASSERT((rows >= 1) && (rows <= MAX_ROWS), static_cast<FwAssertArgType>(rows));
    FW_ASSERT(tickPeriodUsec > 0);
    FW_ASSERT(defaultTableFile.length() > 0);
    FW_ASSERT(defaultTableFile.length() <= FileNameStringSize, static_cast<FwAssertArgType>(defaultTableFile.length()));

    const FwSizeType alignment = alignof(std::max_align_t);
    const FwSizeType tableSize = alignUp(rows * sizeof(SeqScheduler_Row), alignment);
    const FwSizeType rowDueSize = alignUp(rows * sizeof(RowDue), alignment);
    const FwSizeType treeSize = Fw::ExternalRedBlackTreeSet<DueRow>::getByteArraySize(rows);
    static_assert(Fw::ExternalRedBlackTreeSet<DueRow>::getByteArrayAlignment() <= alignof(std::max_align_t),
                  "The due-row tree must fit the alignment of the allocation");
    const FwSizeType wanted = 2 * tableSize + rowDueSize + treeSize;

    FwSizeType granted = wanted;
    bool recoverable = false;
    void* const memory = allocator.allocate(memId, granted, recoverable, alignment);
    FW_ASSERT(memory != nullptr);
    FW_ASSERT(granted >= wanted, static_cast<FwAssertArgType>(granted), static_cast<FwAssertArgType>(wanted));

    U8* const bytes = static_cast<U8*>(memory);
    this->m_table = new (bytes) SeqScheduler_Row[rows];
    this->m_loadRows = new (bytes + tableSize) SeqScheduler_Row[rows];
    this->m_rowDue = new (bytes + 2 * tableSize) RowDue[rows];
    this->m_due.setStorage(Fw::ByteArray(bytes + 2 * tableSize + rowDueSize, treeSize), rows);

    this->m_memory = memory;
    this->m_allocator = &allocator;
    this->m_memId = memId;
    this->m_rows = rows;
    this->m_tickPeriodUsec = tickPeriodUsec;
    this->m_defaultTableFile = defaultTableFile;
}

void SeqScheduler::cleanup() {
    if (this->m_memory != nullptr) {
        FW_ASSERT(this->m_allocator != nullptr);
        for (FwSizeType i = 0; i < this->m_rows; i++) {
            this->m_table[i].~SeqScheduler_Row();
            this->m_loadRows[i].~SeqScheduler_Row();
        }
        this->m_due = Fw::ExternalRedBlackTreeSet<DueRow>();
        this->m_allocator->deallocate(this->m_memId, this->m_memory);
        this->m_memory = nullptr;
        this->m_table = nullptr;
        this->m_loadRows = nullptr;
        this->m_rowDue = nullptr;
        this->m_rows = 0;
    }
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SeqScheduler::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

bool SeqScheduler::queryIn_handler(FwIndexType portNum, FwSizeType i, Fw::Time& timeDue, Fw::StringBase& fileName) {
    FW_ASSERT(this->m_memory != nullptr);
    FwSizeType row = 0;
    if (!this->findUpcoming(i, row, timeDue)) {
        return false;
    }
    fileName = this->m_table[row].get_fileName();
    return true;
}

void SeqScheduler::run_handler(FwIndexType portNum, U32 context) {
    if (this->m_ticksPending.fetch_add(1) == 0) {
        this->runInternal_internalInterfaceInvoke();
    }
}

void SeqScheduler::seqDoneIn_handler(FwIndexType portNum,
                                     FwOpcodeType opCode,
                                     U32 cmdSeq,
                                     const Fw::CmdResponse& response) {
    FW_ASSERT(this->m_memory != nullptr);
    if (this->m_seq.state == SeqState::IDLE) {
        this->log_WARNING_LO_SeqDoneUnexpected(response);
        return;
    }
    const FwSizeType row = this->m_seq.row;
    this->log_ACTIVITY_HI_SeqCompleted(row, response);
    if (this->m_table[row].get_state() == SeqScheduler_RowState::EXECUTING) {
        this->setRowState(row, SeqScheduler_RowState::EXECUTED);
        this->tlmWrite_LastExecutedRow(row);
    }
    this->m_seq.state = SeqState::IDLE;
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void SeqScheduler::LOAD_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->m_memory != nullptr);
    // A LOAD_TABLE handled before any load is itself the start-up load
    const bool ok = this->loadTable(this->m_loadState == LoadState::NOT_TRIED);
    this->cmdResponse_out(opCode, cmdSeq, ok ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
}

void SeqScheduler::SAVE_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    const FwSizeType rows = this->usedRows();
    if (rows == 0) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::TABLE_EMPTY);
        return;
    }
    Fw::FileNameString name;
    this->tableFileName(name);
    SeqScheduler_SaveFailure reason;
    if (this->writeTableFile(name, rows, reason)) {
        this->log_ACTIVITY_HI_TableSaved(name);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->log_WARNING_HI_TableSaveFailed(name, reason);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void SeqScheduler::SET_ROW_ENABLED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, FwSizeType row, bool enabled) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    if (row >= this->m_rows) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::ROW_INDEX);
        return;
    }
    if (enabled && this->isEmpty(row)) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::FILE_NAME);
        return;
    }
    this->lock();
    this->m_table[row].set_enabled(enabled);
    if (!enabled) {
        this->removeDue(row);
    }
    this->unLock();
    this->log_ACTIVITY_HI_RowEnabledSet(row, enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SeqScheduler::SET_SEQ_ENABLED_cmdHandler(FwOpcodeType opCode,
                                              U32 cmdSeq,
                                              const Fw::CmdStringArg& fileName,
                                              bool enabled) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    if (fileName.length() == 0) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::FILE_NAME);
        return;
    }
    FwSizeType count = 0;
    this->lock();
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (this->m_table[row].get_fileName() == fileName) {
            this->m_table[row].set_enabled(enabled);
            if (!enabled) {
                this->removeDue(row);
            }
            count++;
        }
    }
    this->unLock();
    if (count == 0) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::NO_MATCH);
        return;
    }
    this->log_ACTIVITY_HI_SeqEnabledSet(fileName, enabled, count);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SeqScheduler::SET_ROW_cmdHandler(FwOpcodeType opCode,
                                      U32 cmdSeq,
                                      FwSizeType row,
                                      const Fw::TimeValue& time,
                                      const Svc::SeqScheduler_TimeMode& mode,
                                      const Fw::CmdStringArg& fileName,
                                      const Svc::SeqScheduler_BusyPolicy& busyPolicy) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    if (row >= this->m_rows) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::ROW_INDEX);
        return;
    }
    if (time.get_useconds() >= USEC_PER_SEC) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::TIME);
        return;
    }
    if (fileName.length() == 0) {
        this->reject(opCode, cmdSeq, SeqScheduler_CmdRejection::FILE_NAME);
        return;
    }
    this->lock();
    SeqScheduler_Row& entry = this->m_table[row];
    entry.set_time(time);
    entry.set_mode(mode);
    entry.set_fileName(fileName);
    entry.set_busyPolicy(busyPolicy);
    entry.set_state(SeqScheduler_RowState::PENDING);
    this->removeDue(row);
    this->unLock();
    this->log_ACTIVITY_HI_RowSet(row, time, mode, fileName, busyPolicy);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SeqScheduler::DUMP_TABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    const FwSizeType wanted = this->m_rows * SIZE_OF_RowRecord_RECORD;
    // The container asked for, header and hashes included, is at most SeqSchedulerDpMaxBytes
    const FwSizeType most = static_cast<FwSizeType>(SeqSchedulerDpMaxBytes) - Fw::DpContainer::MIN_PACKET_SIZE;
    const FwSizeType size = (wanted < most) ? wanted : most;
    if (this->dpGet_TableContainer(size, this->m_dpContainer) != Fw::Success::SUCCESS) {
        this->log_WARNING_HI_TableDumpFailed();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    FwSizeType sent = 0;
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        const Fw::SerializeStatus status = this->m_dpContainer.serializeRecord_RowRecord(this->m_table[row]);
        if (status != Fw::FW_SERIALIZE_OK) {
            break;
        }
        sent++;
    }
    this->dpSend(this->m_dpContainer);
    if (sent < this->m_rows) {
        this->log_WARNING_LO_TableDumpTruncated(sent, this->m_rows);
    } else {
        this->log_ACTIVITY_HI_TableDumped();
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SeqScheduler::REPORT_ACTIVE_ROW_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    FwSizeType row = 0;
    Fw::Time timeDue;
    this->lock();
    const bool found = this->findUpcoming(0, row, timeDue);
    this->unLock();
    if (found) {
        this->log_ACTIVITY_HI_ActiveRow(row, timeDue.asTimeValue(), this->m_table[row].get_fileName());
    } else {
        this->log_ACTIVITY_HI_NoActiveRow();
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SeqScheduler::SET_ENABLED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enabled) {
    FW_ASSERT(this->m_memory != nullptr);
    this->loadIfNotTried();
    this->lock();
    this->m_enabled = enabled;
    this->unLock();
    this->log_ACTIVITY_HI_EnabledSet(enabled);
    this->tlmWrite_Enabled(enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void SeqScheduler::runInternal_internalInterfaceHandler() {
    FW_ASSERT(this->m_memory != nullptr);
    const U32 ticks = this->m_ticksPending.exchange(0);
    FW_ASSERT(ticks > 0);
    const Fw::Time now = this->getTime();
    this->lock();
    this->m_tick += ticks;
    this->m_tickTime = now;
    this->unLock();
    this->recordBase(ticks);

    this->loadIfNotTried();
    const bool quiet = (this->m_loadState == LoadState::LOADED_NEW);

    if (!this->m_enabled) {
        this->skipDueRowsWhileDisabled();
    }
    this->evaluatePendingRows(quiet);
    this->applyCancelRule();
    this->runFirstDueRow();
    this->skipBusyRows();
    if (quiet) {
        this->m_loadState = LoadState::LOADED;
    }
    this->writeTelemetry();
}

// ----------------------------------------------------------------------
// Tick steps
// ----------------------------------------------------------------------

void SeqScheduler::skipDueRowsWhileDisabled() {
    FW_ASSERT(this->m_rowDue != nullptr);
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (this->m_rowDue[row].due) {
            this->setRowState(row, SeqScheduler_RowState::SKIPPED);
            this->log_ACTIVITY_LO_RowSkippedDisabled(row);
        }
    }
    this->lock();
    this->clearDue();
    this->unLock();
}

void SeqScheduler::evaluatePendingRows(bool quiet) {
    const U64 tolerance = this->lateToleranceUsec();
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        const SeqScheduler_Row& entry = this->m_table[row];
        if ((entry.get_state() != SeqScheduler_RowState::PENDING) || this->isEmpty(row) || this->m_rowDue[row].due) {
            continue;
        }
        const DueInfo info = this->dueInfo(row);
        if (!info.due) {
            continue;
        }
        if ((!this->m_enabled) || (!entry.get_enabled())) {
            this->setRowState(row, SeqScheduler_RowState::SKIPPED);
            if (!quiet) {
                this->log_ACTIVITY_LO_RowSkippedDisabled(row);
            }
        } else if (info.lateUsec > tolerance) {
            this->setRowState(row, SeqScheduler_RowState::SKIPPED);
            if (!quiet) {
                this->log_WARNING_LO_RowLate(row);
            }
        } else {
            DueRow element;
            element.dueUsec = info.dueUsec;
            element.row = static_cast<U16>(row);
            this->lock();
            const Fw::Success status = this->m_due.insert(element);
            FW_ASSERT(status == Fw::Success::SUCCESS, static_cast<FwAssertArgType>(row));
            this->m_rowDue[row].due = true;
            this->m_rowDue[row].dueUsec = info.dueUsec;
            this->unLock();
        }
    }
}

void SeqScheduler::applyCancelRule() {
    // Forward walk to the last CANCEL_RUNNING row among the due rows
    bool found = false;
    DueRow last;
    auto it = this->m_due.begin();
    // The tree holds at most one entry per row
    for (FwSizeType walked = 0; (walked < this->m_rows) && (it != this->m_due.end()); walked++) {
        if (this->m_table[(*it).row].get_busyPolicy() == SeqScheduler_BusyPolicy::CANCEL_RUNNING) {
            found = true;
            last = *it;
        }
        ++it;
    }
    if (!found) {
        return;
    }
    // Remove the first due row until the cancel row is the first; bounded by the due row count
    const FwSizeType size = this->m_due.getSize();
    for (FwSizeType i = 0; i < size; i++) {
        const DueRow first = *this->m_due.begin();
        if (first == last) {
            break;
        }
        this->setRowState(first.row, SeqScheduler_RowState::SKIPPED);
        this->lock();
        this->removeDue(first.row);
        this->unLock();
        this->log_WARNING_LO_RowSkippedByCancel(first.row, last.row);
    }
    if (this->m_seq.state == SeqState::RUNNING) {
        this->seqCancelOut_out(0);
        this->m_seq.state = SeqState::CANCELLING;
        this->log_ACTIVITY_HI_SeqCancelled(this->m_seq.row, last.row);
    }
}

void SeqScheduler::runFirstDueRow() {
    if ((this->m_seq.state != SeqState::IDLE) || (this->m_due.getSize() == 0)) {
        return;
    }
    const DueRow first = *this->m_due.begin();
    this->lock();
    this->removeDue(first.row);
    this->unLock();
    this->runRow(first.row);
}

void SeqScheduler::skipBusyRows() {
    if (this->m_due.getSize() == 0) {
        return;
    }
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (this->m_rowDue[row].due &&
            (this->m_table[row].get_busyPolicy() == SeqScheduler_BusyPolicy::SKIP_WHEN_RUNNING)) {
            this->setRowState(row, SeqScheduler_RowState::SKIPPED);
            this->lock();
            this->removeDue(row);
            this->unLock();
            this->log_WARNING_LO_RowSkippedBusy(row, this->m_seq.row);
        }
    }
}

void SeqScheduler::writeTelemetry() {
    FW_ASSERT(this->m_table != nullptr);
    FwSizeType pending = 0;
    FwSizeType executed = 0;
    FwSizeType skipped = 0;
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (this->isEmpty(row)) {
            continue;
        }
        switch (this->m_table[row].get_state()) {
            case SeqScheduler_RowState::PENDING:
                pending++;
                break;
            case SeqScheduler_RowState::EXECUTED:
                executed++;
                break;
            case SeqScheduler_RowState::SKIPPED:
                skipped++;
                break;
            default:
                break;
        }
    }
    FwSizeType nextRow = this->m_rows;
    Fw::Time nextTime = Fw::Time::zero(TimeBase::TB_NONE);
    this->lock();
    FwSizeType row = 0;
    Fw::Time timeDue;
    if (this->findUpcoming(0, row, timeDue)) {
        nextRow = row;
        nextTime = timeDue;
    }
    this->unLock();
    this->tlmWrite_TableLoaded((this->m_loadState == LoadState::LOADED_NEW) ||
                               (this->m_loadState == LoadState::LOADED));
    this->tlmWrite_Enabled(this->m_enabled);
    this->tlmWrite_NextRow(nextRow);
    this->tlmWrite_NextTimeDue(nextTime.asTimeValue());
    this->tlmWrite_RunningRow((this->m_seq.state == SeqState::IDLE) ? this->m_rows : this->m_seq.row);
    this->tlmWrite_RowsWaiting(this->m_due.getSize());
    this->tlmWrite_RowsPending(pending);
    this->tlmWrite_RowsExecuted(executed);
    this->tlmWrite_RowsSkipped(skipped);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void SeqScheduler::reject(FwOpcodeType opCode, U32 cmdSeq, SeqScheduler_CmdRejection::T reason) {
    this->log_WARNING_HI_CmdRejected(opCode, reason);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
}

void SeqScheduler::tableFileName(Fw::StringBase& name) {
    Fw::ParamValid valid = Fw::ParamValid::INVALID;
    const Fw::ParamString param = this->paramGet_TABLE_FILE(valid);
    if (((valid == Fw::ParamValid::VALID) || (valid == Fw::ParamValid::DEFAULT)) && (param.length() > 0)) {
        name = param;
    } else {
        name = this->m_defaultTableFile;
    }
}

void SeqScheduler::loadIfNotTried() {
    if (this->m_loadState == LoadState::NOT_TRIED) {
        (void)this->loadTable(true);
    }
}

bool SeqScheduler::loadTable(bool startUp) {
    Fw::FileNameString name;
    this->tableFileName(name);
    bool missing = false;
    SeqScheduler_LoadFailure reason;
    FwSizeType fileRows = 0;
    FwSizeType loadedRows = 0;
    bool ok = this->readTableFile(name, missing, reason, fileRows, loadedRows);
    if ((!ok) && missing && startUp) {
        // A missing file at the start-up load gives an empty table
        ok = true;
        loadedRows = 0;
    } else {
        missing = false;
    }
    if (!ok) {
        this->log_WARNING_HI_TableLoadFailed(name, reason);
        if (startUp) {
            this->m_loadState = LoadState::FAILED;
        }
        return false;
    }
    this->lock();
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (row < loadedRows) {
            this->m_table[row] = this->m_loadRows[row];
        } else {
            this->m_table[row] = SeqScheduler_Row();
        }
        this->m_table[row].set_state(SeqScheduler_RowState::PENDING);
    }
    this->clearDue();
    this->unLock();
    this->m_loadState = LoadState::LOADED_NEW;
    this->log_WARNING_HI_TimeBaseNotRecorded_ThrottleClear();
    if (missing) {
        this->log_WARNING_LO_TableFileMissing(name);
    } else {
        this->log_ACTIVITY_HI_TableLoaded(name, loadedRows);
        if (fileRows > this->m_rows) {
            this->log_WARNING_LO_TableFileRowsDropped(name, fileRows, this->m_rows);
        }
    }
    return true;
}

bool SeqScheduler::readTableFile(const Fw::StringBase& name,
                                 bool& missing,
                                 SeqScheduler_LoadFailure& reason,
                                 FwSizeType& fileRows,
                                 FwSizeType& loadedRows) {
    missing = false;
    Os::File file;
    const Os::File::Status openStatus = file.open(name.toChar(), Os::File::OPEN_READ);
    if (openStatus != Os::File::OP_OK) {
        missing = (openStatus == Os::File::DOESNT_EXIST);
        reason = SeqScheduler_LoadFailure::OPEN;
        return false;
    }
    FwSizeType fileSize = 0;
    if (file.size(fileSize) != Os::File::OP_OK) {
        reason = SeqScheduler_LoadFailure::READ;
        file.close();
        return false;
    }
    if (fileSize < TABLE_FILE_HEADER_SIZE + HASH_DIGEST_LENGTH) {
        reason = SeqScheduler_LoadFailure::SIZE;
        file.close();
        return false;
    }
    Utils::Hash hash;
    hash.init();
    U8 buffer[TABLE_FILE_ROW_SIZE];
    FwSizeType size = TABLE_FILE_HEADER_SIZE;
    Os::File::Status status = file.read(buffer, size);
    if ((status != Os::File::OP_OK) || (size != TABLE_FILE_HEADER_SIZE)) {
        reason = SeqScheduler_LoadFailure::READ;
        file.close();
        return false;
    }
    hash.update(buffer, TABLE_FILE_HEADER_SIZE);
    if (buffer[0] != TABLE_FILE_VERSION) {
        reason = SeqScheduler_LoadFailure::VERSION;
        file.close();
        return false;
    }
    fileRows = static_cast<FwSizeType>(getBigEndian(&buffer[1], 2));
    if (fileSize != TABLE_FILE_HEADER_SIZE + fileRows * TABLE_FILE_ROW_SIZE + HASH_DIGEST_LENGTH) {
        reason = SeqScheduler_LoadFailure::SIZE;
        file.close();
        return false;
    }
    loadedRows = (fileRows < this->m_rows) ? fileRows : this->m_rows;
    bool contentValid = true;
    // Bounded by the 16-bit row count of the file
    for (FwSizeType row = 0; row < fileRows; row++) {
        size = TABLE_FILE_ROW_SIZE;
        status = file.read(buffer, size);
        if ((status != Os::File::OP_OK) || (size != TABLE_FILE_ROW_SIZE)) {
            reason = SeqScheduler_LoadFailure::READ;
            file.close();
            return false;
        }
        hash.update(buffer, TABLE_FILE_ROW_SIZE);
        if (row < loadedRows) {
            const bool decoded = decodeRow(buffer, this->m_loadRows[row]);
            contentValid = contentValid && decoded;
        }
    }
    U8 stored[HASH_DIGEST_LENGTH];
    size = HASH_DIGEST_LENGTH;
    const Os::File::Status readStatus = file.read(stored, size);
    file.close();
    if ((readStatus != Os::File::OP_OK) || (size != HASH_DIGEST_LENGTH)) {
        reason = SeqScheduler_LoadFailure::READ;
        return false;
    }
    Utils::HashBuffer computed;
    hash.finalize(computed);
    if (memcmp(stored, computed.getBuffAddr(), HASH_DIGEST_LENGTH) != 0) {
        reason = SeqScheduler_LoadFailure::CHECKSUM;
        return false;
    }
    if (!contentValid) {
        reason = SeqScheduler_LoadFailure::ROW_CONTENT;
        return false;
    }
    return true;
}

bool SeqScheduler::decodeRow(const U8* buffer, SeqScheduler_Row& row) {
    FW_ASSERT(buffer != nullptr);
    FwSizeType offset = 0;
    const U64 base = getBigEndian(&buffer[offset], sizeof(FwTimeBaseStoreType));
    offset += sizeof(FwTimeBaseStoreType);
    const U64 context = getBigEndian(&buffer[offset], sizeof(FwTimeContextStoreType));
    offset += sizeof(FwTimeContextStoreType);
    const U32 seconds = static_cast<U32>(getBigEndian(&buffer[offset], sizeof(U32)));
    offset += sizeof(U32);
    const U32 useconds = static_cast<U32>(getBigEndian(&buffer[offset], sizeof(U32)));
    offset += sizeof(U32);
    const U8 mode = buffer[offset++];
    const U8 busyPolicy = buffer[offset++];
    const U8 enabled = buffer[offset++];
    const char* const fileName = reinterpret_cast<const char*>(&buffer[offset]);
    const bool terminated = (memchr(fileName, 0, FileNameStringSize + 1) != nullptr);
    const TimeBase::T baseValue = static_cast<TimeBase::T>(base);
    const SeqScheduler_TimeMode::T modeValue = static_cast<SeqScheduler_TimeMode::T>(mode);
    const SeqScheduler_BusyPolicy::T policyValue = static_cast<SeqScheduler_BusyPolicy::T>(busyPolicy);
    if ((!terminated) || (!TimeBase::isValid(baseValue)) || (!SeqScheduler_TimeMode::isValid(modeValue)) ||
        (!SeqScheduler_BusyPolicy::isValid(policyValue)) || (enabled > 1) || (useconds >= USEC_PER_SEC) ||
        ((enabled == 1) && (fileName[0] == 0))) {
        return false;
    }
    const Fw::TimeValue time(baseValue, static_cast<FwTimeContextStoreType>(context), seconds, useconds);
    row.set(time, modeValue, Fw::String(fileName), policyValue, enabled == 1, SeqScheduler_RowState::PENDING);
    return true;
}

void SeqScheduler::encodeRow(const SeqScheduler_Row& row, U8* buffer) {
    FW_ASSERT(buffer != nullptr);
    (void)memset(buffer, 0, TABLE_FILE_ROW_SIZE);
    const Fw::TimeValue& time = row.get_time();
    FwSizeType offset = 0;
    putBigEndian(&buffer[offset], static_cast<U64>(time.get_timeBase()), sizeof(FwTimeBaseStoreType));
    offset += sizeof(FwTimeBaseStoreType);
    putBigEndian(&buffer[offset], time.get_timeContext(), sizeof(FwTimeContextStoreType));
    offset += sizeof(FwTimeContextStoreType);
    putBigEndian(&buffer[offset], time.get_seconds(), sizeof(U32));
    offset += sizeof(U32);
    putBigEndian(&buffer[offset], time.get_useconds(), sizeof(U32));
    offset += sizeof(U32);
    buffer[offset++] = static_cast<U8>(row.get_mode());
    buffer[offset++] = static_cast<U8>(row.get_busyPolicy());
    buffer[offset++] = row.get_enabled() ? 1 : 0;
    const FwSizeType length = row.get_fileName().length();
    FW_ASSERT(length <= FileNameStringSize, static_cast<FwAssertArgType>(length));
    (void)memcpy(&buffer[offset], row.get_fileName().toChar(), length);
}

bool SeqScheduler::writeTableFile(const Fw::StringBase& name, FwSizeType rows, SeqScheduler_SaveFailure& reason) {
    // The temporary name is the table file name plus ".tmp", in a string long enough to hold it
    Fw::StringTemplate<FileNameStringSize + 4> tmpName;
    const Fw::FormatStatus formatted = tmpName.format("%s.tmp", name.toChar());
    FW_ASSERT(formatted == Fw::FormatStatus::SUCCESS, static_cast<FwAssertArgType>(formatted));
    Os::File file;
    if (file.open(tmpName.toChar(), Os::File::OPEN_CREATE, Os::File::OVERWRITE) != Os::File::OP_OK) {
        reason = SeqScheduler_SaveFailure::OPEN;
        return false;
    }
    Utils::Hash hash;
    hash.init();
    U8 buffer[TABLE_FILE_ROW_SIZE];
    buffer[0] = TABLE_FILE_VERSION;
    putBigEndian(&buffer[1], rows, 2);
    FwSizeType size = TABLE_FILE_HEADER_SIZE;
    bool ok = (file.write(buffer, size) == Os::File::OP_OK) && (size == TABLE_FILE_HEADER_SIZE);
    hash.update(buffer, TABLE_FILE_HEADER_SIZE);
    for (FwSizeType row = 0; ok && (row < rows); row++) {
        encodeRow(this->m_table[row], buffer);
        size = TABLE_FILE_ROW_SIZE;
        ok = (file.write(buffer, size) == Os::File::OP_OK) && (size == TABLE_FILE_ROW_SIZE);
        hash.update(buffer, TABLE_FILE_ROW_SIZE);
    }
    if (ok) {
        Utils::HashBuffer computed;
        hash.finalize(computed);
        size = HASH_DIGEST_LENGTH;
        ok = (file.write(computed.getBuffAddr(), size) == Os::File::OP_OK) && (size == HASH_DIGEST_LENGTH);
    }
    file.close();
    if (!ok) {
        reason = SeqScheduler_SaveFailure::WRITE;
        return false;
    }
    if (Os::FileSystem::rename(tmpName.toChar(), name.toChar()) != Os::FileSystem::OP_OK) {
        reason = SeqScheduler_SaveFailure::RENAME;
        return false;
    }
    return true;
}

bool SeqScheduler::isEmpty(FwSizeType row) const {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    return this->m_table[row].get_fileName().length() == 0;
}

FwSizeType SeqScheduler::usedRows() const {
    FwSizeType rows = 0;
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        if (!this->isEmpty(row)) {
            rows = row + 1;
        }
    }
    return rows;
}

void SeqScheduler::setRowState(FwSizeType row, SeqScheduler_RowState::T state) {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    this->lock();
    this->m_table[row].set_state(state);
    this->unLock();
}

void SeqScheduler::removeDue(FwSizeType row) {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    if (this->m_rowDue[row].due) {
        DueRow element;
        element.dueUsec = this->m_rowDue[row].dueUsec;
        element.row = static_cast<U16>(row);
        const Fw::Success status = this->m_due.remove(element);
        FW_ASSERT(status == Fw::Success::SUCCESS, static_cast<FwAssertArgType>(row));
        this->m_rowDue[row].due = false;
    }
}

void SeqScheduler::clearDue() {
    this->m_due.clear();
    for (FwSizeType row = 0; row < this->m_rows; row++) {
        this->m_rowDue[row].due = false;
    }
}

void SeqScheduler::recordBase(U32 ticks) {
    const TimeBase base = this->m_tickTime.getTimeBase();
    if (this->findBase(base) != nullptr) {
        return;
    }
    if (this->m_baseCount < FW_NUM_ARRAY_ELEMENTS(this->m_bases)) {
        BaseRecord record;
        record.base = base;
        record.firstTick = this->m_tick;
        record.firstTime = this->m_tickTime;
        if (this->m_baseCount == 0) {
            // The first handled tick may stand for several: the first entry is the base of tick 1
            record.firstTick = 1;
            const U64 now = toUsec(this->m_tickTime.getSeconds(), this->m_tickTime.getUSeconds());
            const U64 back = static_cast<U64>(ticks - 1) * this->m_tickPeriodUsec;
            const U64 first = (now > back) ? (now - back) : 0;
            record.firstTime = Fw::Time(base, this->m_tickTime.getContext(), static_cast<U32>(first / USEC_PER_SEC),
                                        static_cast<U32>(first % USEC_PER_SEC));
            this->m_lastUnrecordedBase = base;
        }
        this->lock();
        this->m_bases[this->m_baseCount] = record;
        this->m_baseCount++;
        this->unLock();
    } else if (base != this->m_lastUnrecordedBase) {
        this->m_lastUnrecordedBase = base;
        this->log_WARNING_HI_TimeBaseNotRecorded(base);
    }
}

const SeqScheduler::BaseRecord* SeqScheduler::findBase(TimeBase base) const {
    for (FwSizeType i = 0; i < this->m_baseCount; i++) {
        if (this->m_bases[i].base == base) {
            return &this->m_bases[i];
        }
    }
    return nullptr;
}

SeqScheduler::DueInfo SeqScheduler::dueInfo(FwSizeType row) const {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    FW_ASSERT(this->m_tickPeriodUsec > 0);
    // Tick counts stay far below the range of the signed microsecond values computed from them
    FW_ASSERT(this->m_tick <= MAX_TICK_USEC / this->m_tickPeriodUsec);
    DueInfo info;
    const SeqScheduler_Row& entry = this->m_table[row];
    const Fw::TimeValue& time = entry.get_time();
    const U64 rowUsec = toUsec(time.get_seconds(), time.get_useconds());
    const I64 nowTickUsec = static_cast<I64>(this->m_tick * this->m_tickPeriodUsec);
    if (entry.get_mode() == SeqScheduler_TimeMode::ABSOLUTE) {
        if ((this->m_tick == 0) || (this->m_tickTime.getTimeBase() != time.get_timeBase())) {
            return info;
        }
        const U64 nowUsec = toUsec(this->m_tickTime.getSeconds(), this->m_tickTime.getUSeconds());
        info.known = true;
        info.timeDue = Fw::Time(time.get_timeBase(), time.get_timeContext(), time.get_seconds(), time.get_useconds());
        // Both differences are bounded by 32 bits of seconds, in microseconds
        if (nowUsec >= rowUsec) {
            info.due = true;
            info.lateUsec = nowUsec - rowUsec;
            FW_ASSERT(info.lateUsec <= MAX_TICK_USEC);
            const I64 late = static_cast<I64>(info.lateUsec);
            FW_ASSERT((nowTickUsec >= 0) && (late >= 0) && (late <= MAX_SIGNED_USEC));
            info.dueUsec = nowTickUsec - late;
        } else {
            const U64 aheadUsec = rowUsec - nowUsec;
            FW_ASSERT(aheadUsec <= MAX_TICK_USEC);
            const I64 ahead = static_cast<I64>(aheadUsec);
            FW_ASSERT((nowTickUsec >= 0) && (nowTickUsec <= MAX_SIGNED_USEC) && (ahead <= MAX_SIGNED_USEC));
            info.dueUsec = nowTickUsec + ahead;
        }
        return info;
    }
    const BaseRecord* origin = nullptr;
    if (entry.get_mode() == SeqScheduler_TimeMode::START_RELATIVE) {
        origin = (this->m_baseCount > 0) ? &this->m_bases[0] : nullptr;
    } else {
        origin = this->findBase(time.get_timeBase());
    }
    if (origin == nullptr) {
        return info;
    }
    const U64 delayTicks = (rowUsec + this->m_tickPeriodUsec - 1) / this->m_tickPeriodUsec;
    const U64 dueTick = origin->firstTick + delayTicks;
    info.known = true;
    info.dueUsec = static_cast<I64>(dueTick * this->m_tickPeriodUsec);
    info.timeDue = addUsec(origin->firstTime, rowUsec);
    if (this->m_tick >= dueTick) {
        info.due = true;
        info.lateUsec = (this->m_tick - dueTick) * this->m_tickPeriodUsec;
    }
    return info;
}

U64 SeqScheduler::lateToleranceUsec() {
    Fw::ParamValid valid = Fw::ParamValid::INVALID;
    const U64 tolerance = static_cast<U64>(this->paramGet_LATE_TOLERANCE_SEC(valid)) * USEC_PER_SEC;
    return (tolerance < this->m_tickPeriodUsec) ? this->m_tickPeriodUsec : tolerance;
}

bool SeqScheduler::upcomingKey(FwSizeType row, DueRow& key, Fw::Time& timeDue) const {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    const SeqScheduler_Row& entry = this->m_table[row];
    if ((!this->m_enabled) || (!entry.get_enabled()) || this->isEmpty(row) ||
        (entry.get_state() != SeqScheduler_RowState::PENDING)) {
        return false;
    }
    const DueInfo info = this->dueInfo(row);
    if (!info.known) {
        return false;
    }
    key.row = static_cast<U16>(row);
    key.dueUsec = this->m_rowDue[row].due ? this->m_rowDue[row].dueUsec : info.dueUsec;
    timeDue = info.timeDue;
    return true;
}

bool SeqScheduler::findUpcoming(FwSizeType position, FwSizeType& row, Fw::Time& timeDue) const {
    if (position >= this->m_rows) {
        return false;
    }
    bool havePrevious = false;
    DueRow previous;
    // Select the smallest key above the previous one, position + 1 times
    for (FwSizeType n = 0; n <= position; n++) {
        bool found = false;
        DueRow best;
        Fw::Time bestTime;
        for (FwSizeType candidate = 0; candidate < this->m_rows; candidate++) {
            DueRow key;
            Fw::Time candidateTime;
            if (!this->upcomingKey(candidate, key, candidateTime)) {
                continue;
            }
            if (havePrevious && (!(previous < key))) {
                continue;
            }
            if ((!found) || (key < best)) {
                found = true;
                best = key;
                bestTime = candidateTime;
            }
        }
        if (!found) {
            return false;
        }
        previous = best;
        havePrevious = true;
        timeDue = bestTime;
    }
    row = previous.row;
    return true;
}

void SeqScheduler::runRow(FwSizeType row) {
    FW_ASSERT(row < this->m_rows, static_cast<FwAssertArgType>(row));
    const Svc::SeqArgs args;
    this->seqRunOut_out(0, this->m_table[row].get_fileName(), args);
    this->setRowState(row, SeqScheduler_RowState::EXECUTING);
    this->m_seq.state = SeqState::RUNNING;
    this->m_seq.row = row;
    this->log_ACTIVITY_HI_SeqRun(row, this->m_table[row].get_fileName());
}

}  // namespace Svc
