// ======================================================================
// \title  SeqSchedulerTester.cpp
// \brief  cpp file for SeqScheduler component test harness implementation class
// ======================================================================

#include "SeqSchedulerTester.hpp"

#include <unistd.h>
#include <cstdio>
#include <cstring>

#include <limits>
#include "Os/File.hpp"
#include "Os/FileSystem.hpp"

#include "STest/Pick/Pick.hpp"
#include "STest/Random/Random.hpp"
#include "Utils/Hash/Hash.hpp"

namespace Svc {

namespace {
U32 testerCount = 0;  //!< Gives each tester its own default table file
}

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

SeqSchedulerTester::SeqSchedulerTester(FwSizeType rows)
    : SeqSchedulerGTestBase("SeqSchedulerTester", SeqSchedulerTester::MAX_HISTORY_SIZE),
      component("SeqScheduler"),
      m_rows(rows) {
    this->initComponents();
    this->connectPorts();
    this->m_defaultFile.format("seqsched_ut_%u.tbl", testerCount++);
    this->removeFile(this->m_defaultFile.toChar());
    this->component.configure(this->m_defaultFile, rows, TICK_USEC, this->m_allocator, 0);
    this->setTime(this->m_seconds, 0);
}

SeqSchedulerTester::~SeqSchedulerTester() {
    this->component.cleanup();
    this->component.deinit();
    this->removeFile(this->m_defaultFile.toChar());
}

// ----------------------------------------------------------------------
// Handlers for data products
// ----------------------------------------------------------------------

Fw::Success::T SeqSchedulerTester::productGet_handler(FwDpIdType id, FwSizeType dataSize, Fw::Buffer& buffer) {
    this->pushProductGetEntry(id, dataSize);
    if (!this->m_dpGrant) {
        return Fw::Success::FAILURE;
    }
    const FwSizeType wanted = Fw::DpContainer::getPacketSizeForDataSize(dataSize);
    this->m_dpAsked = dataSize;
    const FwSizeType size = (wanted < this->m_dpLimit) ? wanted : this->m_dpLimit;
    buffer.set(this->m_dpStorage, size);
    return Fw::Success::SUCCESS;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

const char* SeqSchedulerTester::defaultFile() const {
    return this->m_defaultFile.toChar();
}

void SeqSchedulerTester::setTime(U32 seconds, U32 useconds, TimeBase::T base) {
    this->m_seconds = seconds;
    this->m_useconds = useconds;
    this->m_base = base;
    this->setTestTime(Fw::Time(base, seconds, useconds));
}

void SeqSchedulerTester::tick(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->setTime(this->m_seconds + 1, this->m_useconds, this->m_base);
        this->invoke_to_run(0, 0);
        this->component.doDispatch();
    }
}

void SeqSchedulerTester::tickBatch(U32 count) {
    this->setTime(this->m_seconds + count, this->m_useconds, this->m_base);
    for (U32 i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
    }
    this->component.doDispatch();
}

FwSizeType SeqSchedulerTester::readBytes(const char* name, U8* bytes, FwSizeType capacity) {
    Os::File file;
    if (file.open(name, Os::File::OPEN_READ) != Os::File::OP_OK) {
        return 0;
    }
    FwSizeType size = capacity;
    const Os::File::Status status = file.read(bytes, size);
    file.close();
    return (status == Os::File::OP_OK) ? size : 0;
}

void SeqSchedulerTester::done(Fw::CmdResponse::T response) {
    this->invoke_to_seqDoneIn(0, 0, 0, Fw::CmdResponse(response));
    this->component.doDispatch();
}

FwSizeType SeqSchedulerTester::buildFile(U8* bytes, FwSizeType capacity, const FileRow* rows, FwSizeType count) {
    const FwSizeType rowSize = SeqScheduler::TABLE_FILE_ROW_SIZE;
    const FwSizeType size = SeqScheduler::TABLE_FILE_HEADER_SIZE + count * rowSize + HASH_DIGEST_LENGTH;
    FW_ASSERT(size <= capacity);
    (void)memset(bytes, 0, size);
    bytes[0] = 1;
    bytes[1] = static_cast<U8>(count >> 8);
    bytes[2] = static_cast<U8>(count & 0xFF);
    for (FwSizeType i = 0; i < count; i++) {
        U8* const r = &bytes[SeqScheduler::TABLE_FILE_HEADER_SIZE + i * rowSize];
        const FileRow& row = rows[i];
        // Layout of SDD section 4.2, written here independently of the component
        r[0] = static_cast<U8>(static_cast<U16>(row.base) >> 8);
        r[1] = static_cast<U8>(static_cast<U16>(row.base) & 0xFF);
        r[2] = 0;
        r[3] = static_cast<U8>(row.seconds >> 24);
        r[4] = static_cast<U8>(row.seconds >> 16);
        r[5] = static_cast<U8>(row.seconds >> 8);
        r[6] = static_cast<U8>(row.seconds);
        r[7] = static_cast<U8>(row.useconds >> 24);
        r[8] = static_cast<U8>(row.useconds >> 16);
        r[9] = static_cast<U8>(row.useconds >> 8);
        r[10] = static_cast<U8>(row.useconds);
        r[11] = row.mode;
        r[12] = row.policy;
        r[13] = row.enabled;
        (void)strncpy(reinterpret_cast<char*>(&r[14]), row.fileName, FileNameStringSize);
    }
    Utils::HashBuffer digest;
    Utils::Hash::hash(bytes, size - HASH_DIGEST_LENGTH, digest);
    (void)memcpy(&bytes[size - HASH_DIGEST_LENGTH], digest.getBuffAddr(), HASH_DIGEST_LENGTH);
    return size;
}

void SeqSchedulerTester::writeBytes(const char* name, const U8* bytes, FwSizeType size) {
    Os::File file;
    ASSERT_EQ(file.open(name, Os::File::OPEN_CREATE, Os::File::OVERWRITE), Os::File::OP_OK);
    FwSizeType written = size;
    ASSERT_EQ(file.write(bytes, written), Os::File::OP_OK);
    ASSERT_EQ(written, size);
    file.close();
}

void SeqSchedulerTester::writeFile(const char* name, const FileRow* rows, FwSizeType count) {
    static U8 bytes[64 * 1024];
    const FwSizeType size = this->buildFile(bytes, sizeof(bytes), rows, count);
    this->writeBytes(name, bytes, size);
}

void SeqSchedulerTester::removeFile(const char* name) {
    (void)Os::FileSystem::removeFile(name);
}

void SeqSchedulerTester::cmdLoad() {
    this->sendCmd_LOAD_TABLE(0, ++this->m_cmdSeq);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdSave() {
    this->sendCmd_SAVE_TABLE(0, ++this->m_cmdSeq);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdSetRow(FwSizeType row,
                                   Mode::T mode,
                                   U32 seconds,
                                   const char* fileName,
                                   Policy::T policy,
                                   U32 useconds,
                                   TimeBase::T base) {
    this->sendCmd_SET_ROW(0, ++this->m_cmdSeq, row, Fw::TimeValue(base, 0, seconds, useconds), Mode(mode),
                          Fw::String(fileName), Policy(policy));
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdRowEnabled(FwSizeType row, bool enabled) {
    this->sendCmd_SET_ROW_ENABLED(0, ++this->m_cmdSeq, row, enabled);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdSeqEnabled(const char* fileName, bool enabled) {
    this->sendCmd_SET_SEQ_ENABLED(0, ++this->m_cmdSeq, Fw::String(fileName), enabled);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdDump() {
    this->sendCmd_DUMP_TABLE(0, ++this->m_cmdSeq);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdReport() {
    this->sendCmd_REPORT_ACTIVE_ROW(0, ++this->m_cmdSeq);
    this->component.doDispatch();
}

void SeqSchedulerTester::cmdEnabled(bool enabled) {
    this->sendCmd_SET_ENABLED(0, ++this->m_cmdSeq, enabled);
    this->component.doDispatch();
}

void SeqSchedulerTester::absRow(FwSizeType row, U32 offset, const char* fileName, Policy::T policy) {
    this->cmdSetRow(row, Mode::ABSOLUTE, T0 + offset, fileName, policy);
    this->cmdRowEnabled(row, true);
}

void SeqSchedulerTester::startEmpty() {
    this->tick();
    ASSERT_EVENTS_TableFileMissing_SIZE(1);
    this->clearHistory();
}

SeqSchedulerTester::State::T SeqSchedulerTester::state(FwSizeType row) const {
    return this->component.m_table[row].get_state();
}

void SeqSchedulerTester::assertLastCmd(FwOpcodeType opCode, Fw::CmdResponse::T response) {
    ASSERT_GT(this->cmdResponseHistory->size(), 0U);
    const CmdResponse& last = this->cmdResponseHistory->at(this->cmdResponseHistory->size() - 1);
    ASSERT_EQ(last.opCode, opCode);
    ASSERT_EQ(last.cmdSeq, this->m_cmdSeq);
    ASSERT_EQ(last.response, Fw::CmdResponse(response));
}

bool SeqSchedulerTester::query(FwSizeType i, Fw::Time& timeDue, Fw::String& fileName) {
    return this->invoke_to_queryIn(0, i, timeDue, fileName);
}

void SeqSchedulerTester::assertRun(U32 index, FwSizeType row, const char* fileName) {
    ASSERT_GT(this->eventHistory_SeqRun->size(), index);
    ASSERT_EQ(this->eventHistory_SeqRun->at(index).row, row);
    ASSERT_STREQ(this->eventHistory_SeqRun->at(index).fileName.toChar(), fileName);
    ASSERT_GT(this->fromPortHistory_seqRunOut->size(), index);
    ASSERT_STREQ(this->fromPortHistory_seqRunOut->at(index).filename.toChar(), fileName);
}

// TESTS-BEGIN

void SeqSchedulerTester::StartupLoad() {
    // REQ-SeqScheduler-001: loaded by the first tick, before a row is evaluated
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"}};
    Fw::String other("seqsched_ut_param.tbl");
    this->writeFile(other.toChar(), rows, 1);
    this->paramSet_TABLE_FILE(other, Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->clearHistory();
    this->tick();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_EVENTS_TableLoaded(0, other.toChar(), 1);
    this->assertRun(0, 0, "a.bin");
    // Loaded once: a later tick does not load again
    this->tick();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    this->removeFile(other.toChar());

    // By the first command, before it is handled; a first LOAD_TABLE reads the file once
    SeqSchedulerTester second;
    second.writeFile(second.defaultFile(), rows, 1);
    second.cmdRowEnabled(0, false);
    second.assertLastCmd(SeqScheduler::OPCODE_SET_ROW_ENABLED, Fw::CmdResponse::OK);
    ASSERT_EQ(second.eventHistory_TableLoaded->size(), 1U);
    SeqSchedulerTester third;
    third.writeFile(third.defaultFile(), rows, 1);
    third.cmdLoad();
    third.assertLastCmd(SeqScheduler::OPCODE_LOAD_TABLE, Fw::CmdResponse::OK);
    ASSERT_EQ(third.eventHistory_TableLoaded->size(), 1U);
}

void SeqSchedulerTester::MissingTableFile() {
    // REQ-SeqScheduler-053: missing file at the start-up load gives an empty table
    this->tick();
    ASSERT_EVENTS_TableFileMissing_SIZE(1);
    ASSERT_EVENTS_TableFileMissing(0, this->defaultFile());
    ASSERT_EVENTS_TableLoaded_SIZE(0);
    ASSERT_EVENTS_TableLoadFailed_SIZE(0);
    ASSERT_TLM_TableLoaded(0, true);
    // REQ-SeqScheduler-055: an empty table is not saved
    this->clearHistory();
    this->cmdSave();
    this->assertLastCmd(SeqScheduler::OPCODE_SAVE_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_CmdRejected_SIZE(1);
    ASSERT_EVENTS_CmdRejected(0, SeqScheduler::OPCODE_SAVE_TABLE, SeqScheduler_CmdRejection::TABLE_EMPTY);
    // It can be edited, and then saved
    this->absRow(0, 100, "a.bin");
    this->cmdSave();
    this->assertLastCmd(SeqScheduler::OPCODE_SAVE_TABLE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TableSaved_SIZE(1);
    // REQ-SeqScheduler-003: on a later LOAD_TABLE a missing file is a load failure, table unchanged
    this->removeFile(this->defaultFile());
    this->clearHistory();
    this->cmdLoad();
    this->assertLastCmd(SeqScheduler::OPCODE_LOAD_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_TableLoadFailed_SIZE(1);
    ASSERT_EVENTS_TableLoadFailed(0, this->defaultFile(), SeqScheduler_LoadFailure::OPEN);
    ASSERT_EVENTS_TableFileMissing_SIZE(0);
    ASSERT_STREQ(this->component.m_table[0].get_fileName().toChar(), "a.bin");

    // A first LOAD_TABLE is the start-up load: a missing file gives an empty table
    SeqSchedulerTester second;
    second.cmdLoad();
    second.assertLastCmd(SeqScheduler::OPCODE_LOAD_TABLE, Fw::CmdResponse::OK);
    ASSERT_EQ(second.eventHistory_TableFileMissing->size(), 1U);
    ASSERT_EQ(second.eventHistory_TableLoadFailed->size(), 0U);
}

void SeqSchedulerTester::AbsoluteDue() {
    // REQ-SeqScheduler-006, 012, 020, 040
    this->startEmpty();
    this->absRow(0, 3, "a.bin");
    this->clearHistory();
    this->tick(2);
    ASSERT_from_seqRunOut_SIZE(0);
    ASSERT_EQ(this->state(0), State::PENDING);
    this->tick();
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_EVENTS_SeqRun_SIZE(1);
    this->assertRun(0, 0, "a.bin");
    ASSERT_EQ(this->state(0), State::EXECUTING);
    this->tick();
    ASSERT_EQ(this->state(0), State::EXECUTING);
    ASSERT_TLM_RunningRow(this->tlmHistory_RunningRow->size() - 1, 0);
    this->done();
    ASSERT_EVENTS_SeqCompleted_SIZE(1);
    ASSERT_EVENTS_SeqCompleted(0, 0, Fw::CmdResponse::OK);
    ASSERT_EQ(this->state(0), State::EXECUTED);
    ASSERT_TLM_LastExecutedRow_SIZE(1);
    ASSERT_TLM_LastExecutedRow(0, 0);
    this->tick(3);
    ASSERT_from_seqRunOut_SIZE(1);
}

void SeqSchedulerTester::AbsoluteOtherBase() {
    // REQ-SeqScheduler-007
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 2, "a.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_SC_TIME);
    this->cmdRowEnabled(0, true);
    this->tick(5);
    ASSERT_from_seqRunOut_SIZE(0);
    ASSERT_EQ(this->state(0), State::PENDING);
    ASSERT_EVENTS_RowLate_SIZE(0);
    // Judged once its base is reported: now long past, so late
    this->setTime(this->m_seconds, 0, TimeBase::TB_SC_TIME);
    this->tick();
    ASSERT_EVENTS_RowLate_SIZE(1);
    ASSERT_EQ(this->state(0), State::SKIPPED);
}

void SeqSchedulerTester::CancelBetweenDue() {
    // REQ-SeqScheduler-060, 015, 045, 017: a CANCEL_RUNNING row due between other due rows
    this->startEmpty();
    this->absRow(0, 1, "long.bin");
    this->absRow(1, 2, "w1.bin");
    this->absRow(3, 3, "w3.bin");
    this->absRow(2, 4, "c.bin", Policy::CANCEL_RUNNING);
    this->absRow(5, 5, "after5.bin");
    this->absRow(4, 6, "after4.bin");
    this->clearHistory();
    this->tick(3);  // row 0 runs, rows 1 and 3 wait
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_TLM_RowsWaiting(this->tlmHistory_RowsWaiting->size() - 1, 2);
    this->tick();  // cancel row comes due
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_SeqCancelled_SIZE(1);
    ASSERT_EVENTS_SeqCancelled(0, 0, 2);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(2);
    ASSERT_EVENTS_RowSkippedByCancel(0, 1, 2);
    ASSERT_EVENTS_RowSkippedByCancel(1, 3, 2);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_EQ(this->state(3), State::SKIPPED);
    this->tick(2);  // rows 5 and 4 come due after the cancel was sent: kept, no second cancel
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(2);
    ASSERT_from_seqRunOut_SIZE(1);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 2, "c.bin");
    this->done();
    this->tick();
    this->assertRun(2, 5, "after5.bin");
    this->done();
    this->tick();
    this->assertRun(3, 4, "after4.bin");
    ASSERT_from_seqRunOut_SIZE(4);
    ASSERT_EVENTS_RowLate_SIZE(0);
}

void SeqSchedulerTester::CancelLastOfSeveralDue() {
    // REQ-SeqScheduler-060, 015, 017: several rows wait and a CANCEL_RUNNING row comes due after all of them
    this->startEmpty();
    this->absRow(0, 1, "long.bin");
    this->absRow(3, 2, "w3.bin");
    this->absRow(1, 2, "w1.bin");
    this->absRow(2, 3, "w2.bin");
    this->absRow(4, 4, "c.bin", Policy::CANCEL_RUNNING);
    this->clearHistory();
    this->tick(4);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(3);
    ASSERT_EVENTS_RowSkippedByCancel(0, 1, 4);
    ASSERT_EVENTS_RowSkippedByCancel(1, 3, 4);
    ASSERT_EVENTS_RowSkippedByCancel(2, 2, 4);
    ASSERT_from_seqRunOut_SIZE(1);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 4, "c.bin");
    ASSERT_from_seqRunOut_SIZE(2);
    ASSERT_TLM_RowsWaiting(this->tlmHistory_RowsWaiting->size() - 1, 0);
}

void SeqSchedulerTester::TwoCancelsDue() {
    // REQ-SeqScheduler-060, 015: every row ahead of the later cancel row is skipped; one cancel is sent
    this->startEmpty();
    this->absRow(0, 1, "long.bin");
    this->absRow(1, 2, "w1.bin");
    this->absRow(2, 3, "c2.bin", Policy::CANCEL_RUNNING);
    this->absRow(3, 4, "w3.bin");
    this->absRow(4, 5, "c4.bin", Policy::CANCEL_RUNNING);
    this->absRow(5, 6, "w5.bin");
    this->clearHistory();
    this->tick(6);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_SeqCancelled_SIZE(1);
    ASSERT_EVENTS_SeqCancelled(0, 0, 2);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(3);
    ASSERT_EVENTS_RowSkippedByCancel(0, 1, 2);
    ASSERT_EVENTS_RowSkippedByCancel(1, 2, 4);
    ASSERT_EVENTS_RowSkippedByCancel(2, 3, 4);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 4, "c4.bin");
    this->done();
    this->tick();
    this->assertRun(2, 5, "w5.bin");
    ASSERT_from_seqRunOut_SIZE(3);
}

void SeqSchedulerTester::Ping() {
    const U32 key = STest::Pick::any();
    this->invoke_to_pingIn(0, key);
    this->component.doDispatch();
    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, key);
}

void SeqSchedulerTester::StartRelativeDue() {
    // REQ-SeqScheduler-008, 010: due its delay after the first tick (tick 1)
    this->startEmpty();
    this->cmdSetRow(0, Mode::START_RELATIVE, 3, "a.bin");
    this->cmdRowEnabled(0, true);
    this->tick(2);
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::TimeBaseRelativeDue() {
    // REQ-SeqScheduler-009, 010, 011
    this->startEmpty();
    this->cmdSetRow(0, Mode::TIME_BASE_RELATIVE, 3, "a.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_SC_TIME);
    this->cmdRowEnabled(0, true);
    this->tick(5);
    ASSERT_from_seqRunOut_SIZE(0);
    ASSERT_EQ(this->state(0), State::PENDING);
    // The base is first reported on this tick; it goes and returns without moving the origin
    this->setTime(this->m_seconds, 0, TimeBase::TB_SC_TIME);
    this->tick();
    this->setTime(this->m_seconds, 0, TimeBase::TB_WORKSTATION_TIME);
    this->tick();
    this->setTime(this->m_seconds, 0, TimeBase::TB_SC_TIME);
    this->tick();
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::TickRounding() {
    // REQ-SeqScheduler-010: delays round up to whole ticks and ignore system-time jumps
    this->startEmpty();
    this->cmdSetRow(0, Mode::START_RELATIVE, 1, "a.bin", Policy::WAIT_FOR_RUNNING, 1);
    this->cmdRowEnabled(0, true);
    this->tick();
    ASSERT_from_seqRunOut_SIZE(0);
    this->setTime(this->m_seconds + 5000, 0);
    this->tick();
    this->assertRun(0, 0, "a.bin");
    ASSERT_EVENTS_RowLate_SIZE(0);
}

void SeqSchedulerTester::TicksWhileBusy() {
    // REQ-SeqScheduler-010: ticks arriving while one is queued queue no second message and are all counted
    this->startEmpty();
    this->cmdSetRow(0, Mode::START_RELATIVE, 4, "a.bin");
    this->cmdRowEnabled(0, true);
    this->tickBatch(3);
    ASSERT_EQ(this->component.m_tick, 4U);
    ASSERT_EQ(this->component.m_queue.getMessagesAvailable(), 0U);
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::LoadBeforeFirstTick() {
    // REQ-SeqScheduler-008, 010: a load command before the first tick leaves the first tick as tick 1
    const FileRow rows[] = {{TimeBase::TB_NONE, 2, 0, Mode::START_RELATIVE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"}};
    this->writeFile(this->defaultFile(), rows, 1);
    this->cmdLoad();
    ASSERT_EQ(this->component.m_tick, 0U);
    this->tick(2);
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::WaitForRunning() {
    // REQ-SeqScheduler-013, 014: runs after either completion status
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->absRow(2, 3, "c.bin");
    this->tick(4);
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_from_seqCancelOut_SIZE(0);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 1, "b.bin");
    this->done(Fw::CmdResponse::OK);
    this->tick();
    this->assertRun(2, 2, "c.bin");
}

void SeqSchedulerTester::CancelRunning() {
    // REQ-SeqScheduler-013, 015: one cancel, none while one is pending, runs once reported complete
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin", Policy::CANCEL_RUNNING);
    this->tick(2);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_SeqCancelled(0, 0, 1);
    this->tick(3);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_from_seqRunOut_SIZE(1);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 1, "b.bin");
    ASSERT_EQ(this->state(0), State::EXECUTED);
}

void SeqSchedulerTester::SkipWhenRunning() {
    // REQ-SeqScheduler-013, 016, 012
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin", Policy::SKIP_WHEN_RUNNING);
    this->absRow(2, 5, "c.bin", Policy::SKIP_WHEN_RUNNING);
    this->tick(2);
    ASSERT_EVENTS_RowSkippedBusy_SIZE(1);
    ASSERT_EVENTS_RowSkippedBusy(0, 1, 0);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    this->done();
    this->tick(3);
    this->assertRun(1, 2, "c.bin");
    ASSERT_from_seqRunOut_SIZE(2);
}

void SeqSchedulerTester::SameTickOrder() {
    // REQ-SeqScheduler-017: rows found due on one tick are handled in the order they came due, then index
    this->startEmpty();
    this->cmdSetRow(3, Mode::ABSOLUTE, T0 + 1, "abs_late.bin", Policy::WAIT_FOR_RUNNING, 700000);
    this->cmdSetRow(2, Mode::ABSOLUTE, T0 + 1, "abs_early.bin", Policy::WAIT_FOR_RUNNING, 200000);
    this->cmdSetRow(1, Mode::START_RELATIVE, 2, "rel.bin");
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 1, "abs_late0.bin", Policy::WAIT_FOR_RUNNING, 700000);
    for (FwSizeType row = 0; row < 4; row++) {
        this->cmdRowEnabled(row, true);
    }
    this->tick(2);
    this->assertRun(0, 2, "abs_early.bin");
    this->done();
    this->tick();
    this->assertRun(1, 0, "abs_late0.bin");
    this->done();
    this->tick();
    this->assertRun(2, 3, "abs_late.bin");
    this->done();
    this->tick();
    this->assertRun(3, 1, "rel.bin");
}

void SeqSchedulerTester::DueOrderEarlyTicks() {
    // REQ-SeqScheduler-017: an ABSOLUTE row past its time on tick 1 has a place in the order below zero
    const FileRow rows[] = {
        {TimeBase::TB_NONE, 0, 0, Mode::START_RELATIVE, Policy::WAIT_FOR_RUNNING, 1, "rel.bin"},
        {TimeBase::TB_WORKSTATION_TIME, T0 - 3, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "abs.bin"}};
    this->writeFile(this->defaultFile(), rows, 2);
    this->paramSet_LATE_TOLERANCE_SEC(10, Fw::ParamValid::VALID);
    this->paramSend_LATE_TOLERANCE_SEC(0, 0);
    this->tick();
    this->assertRun(0, 1, "abs.bin");
    ASSERT_LT(this->component.m_rowDue[0].dueUsec, static_cast<I64>(2 * TICK_USEC));
    this->done();
    this->tick();
    this->assertRun(1, 0, "rel.bin");
}

void SeqSchedulerTester::WaitingOrder() {
    // REQ-SeqScheduler-045, 017: one per completion report, in the order they came due, then index
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 4, "late.bin");
    this->absRow(4, 2, "early4.bin");
    this->absRow(2, 2, "early2.bin");
    this->tick(4);
    ASSERT_from_seqRunOut_SIZE(1);
    this->done();
    this->tick();
    this->assertRun(1, 2, "early2.bin");
    this->tick();
    ASSERT_from_seqRunOut_SIZE(2);
    this->done();
    this->tick();
    this->assertRun(2, 4, "early4.bin");
    this->done();
    this->tick();
    this->assertRun(3, 1, "late.bin");
}

void SeqSchedulerTester::WaitingNotLate() {
    // REQ-SeqScheduler-014, 021: a row that waits beyond the late tolerance still runs
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->tick(20);
    ASSERT_EVENTS_RowLate_SIZE(0);
    this->done();
    this->tick();
    this->assertRun(1, 1, "b.bin");
}

void SeqSchedulerTester::LateRow() {
    // REQ-SeqScheduler-021, 056
    this->startEmpty();
    this->absRow(0, 0, "abs.bin");  // 2 s past on the next tick: tolerance is 1 s
    this->setTime(this->m_seconds + 1);
    this->cmdSetRow(1, Mode::START_RELATIVE, 0, "rel.bin");
    this->cmdRowEnabled(1, true);
    this->clearHistory();
    this->tickBatch(2);  // the relative row is found two ticks past due
    ASSERT_EVENTS_RowLate_SIZE(2);
    ASSERT_EVENTS_RowLate(0, 0);
    ASSERT_EVENTS_RowLate(1, 1);
    ASSERT_EQ(this->state(0), State::SKIPPED);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_from_seqRunOut_SIZE(0);
    // No event on the first tick handled after a load
    this->cmdSave();
    this->cmdLoad();
    this->clearHistory();
    this->tick();
    ASSERT_EVENTS_RowLate_SIZE(0);
    ASSERT_EQ(this->state(0), State::SKIPPED);
    ASSERT_EQ(this->state(1), State::SKIPPED);
}

void SeqSchedulerTester::DisableThenEnableSkipsWaiting() {
    // SDD 7: a waiting row at a component disable is SKIPPED for good, also when the enable precedes the next tick
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 1, "b.bin");
    this->tick();
    ASSERT_EQ(this->state(0), State::EXECUTING);
    ASSERT_EQ(this->component.m_due.getSize(), 1U);  // row 1 waits
    this->clearHistory();
    this->cmdEnabled(false);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_EQ(this->component.m_due.getSize(), 0U);
    ASSERT_EVENTS_RowSkippedDisabled_SIZE(1);
    ASSERT_EVENTS_RowSkippedDisabled(0, 1);
    this->cmdEnabled(true);
    this->done();
    this->tick(2);
    ASSERT_EQ(this->state(0), State::EXECUTED);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_from_seqRunOut_SIZE(0);  // nothing run since the history was cleared
}

void SeqSchedulerTester::FirstTickStandsForSeveral() {
    // SDD 3.4: the first handled tick that stands for several is recorded as tick 1, with its time moved back
    this->tickBatch(3);
    ASSERT_EQ(this->component.m_tick, 3U);
    ASSERT_EQ(this->component.m_baseCount, 1U);
    ASSERT_EQ(this->component.m_bases[0].firstTick, 1U);
    ASSERT_EQ(this->component.m_bases[0].firstTime.getSeconds(), static_cast<U32>(T0));
    this->cmdSetRow(0, Mode::START_RELATIVE, 4, "a.bin");
    this->cmdRowEnabled(0, true);
    this->tick();
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();  // tick 5: four periods after tick 1
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::PingOnFullQueue() {
    // A ping arriving on a full queue is dropped, not asserted on
    const U32 depth = static_cast<U32>(this->component.m_queue.getDepth());
    for (U32 i = 0; i < depth + 2; i++) {
        this->invoke_to_pingIn(0, i);
    }
    for (U32 i = 0; i < depth; i++) {
        this->component.doDispatch();
    }
    ASSERT_EQ(this->component.m_queue.getMessagesAvailable(), 0U);
    ASSERT_from_pingOut_SIZE(depth);
    ASSERT_from_pingOut(depth - 1, depth - 1);
}

void SeqSchedulerTester::SaveWriteFailureKeepsFile() {
    // REQ-SeqScheduler-033: a save that cannot write leaves the old file as it was
    if (::access("/dev/full", W_OK) != 0) {
        GTEST_SKIP() << "/dev/full, the source of the write failure, is not available on this platform";
    }
    this->startEmpty();
    this->absRow(0, 50, "a.bin");
    this->cmdSave();
    ASSERT_EVENTS_TableSaved_SIZE(1);
    U8 before[512];
    U8 after[512];
    const FwSizeType sizeBefore = this->readBytes(this->defaultFile(), before, sizeof(before));
    ASSERT_GT(sizeBefore, 0U);
    Fw::String tmp;
    tmp.format("%s.tmp", this->defaultFile());
    ASSERT_EQ(::symlink("/dev/full", tmp.toChar()), 0);  // every write to it fails
    this->absRow(1, 60, "b.bin");
    this->clearHistory();
    this->cmdSave();
    this->assertLastCmd(SeqScheduler::OPCODE_SAVE_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_TableSaveFailed_SIZE(1);
    ASSERT_EVENTS_TableSaveFailed(0, this->defaultFile(), SeqScheduler_SaveFailure::WRITE);
    const FwSizeType sizeAfter = this->readBytes(this->defaultFile(), after, sizeof(after));
    ASSERT_EQ(sizeAfter, sizeBefore);
    ASSERT_EQ(::memcmp(before, after, sizeBefore), 0);
    ASSERT_NE(::access(tmp.toChar(), F_OK), 0);  // the temporary file is not left behind
}

namespace {
//! Allocator double: counts the calls and can grant fewer bytes than asked for
class CountingAllocator final : public Fw::MemAllocator {
  public:
    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment) override {
        this->allocations++;
        if (this->grantShort) {
            size = size / 2;
        }
        return this->m_source.allocate(identifier, size, recoverable, alignment);
    }
    void deallocate(const FwEnumStoreType identifier, void* ptr) override {
        this->deallocations++;
        this->m_source.deallocate(identifier, ptr);
    }
    U32 allocations = 0;
    U32 deallocations = 0;
    bool grantShort = false;

  private:
    Fw::MallocAllocator m_source;
};
}  // namespace

void SeqSchedulerTester::ConfigureAllocation() {
    // REQ-SeqScheduler-004: one allocation, returned once by cleanup; a short allocation asserts
    this->component.cleanup();
    CountingAllocator allocator;
    this->component.configure(this->m_defaultFile, 8, TICK_USEC, allocator, 0);
    ASSERT_EQ(allocator.allocations, 1U);
    ASSERT_EQ(allocator.deallocations, 0U);
    this->component.cleanup();
    this->component.cleanup();
    ASSERT_EQ(allocator.allocations, 1U);
    ASSERT_EQ(allocator.deallocations, 1U);
    CountingAllocator shortAllocator;
    shortAllocator.grantShort = true;
    ASSERT_DEATH(this->component.configure(this->m_defaultFile, 8, TICK_USEC, shortAllocator, 0), "Assert");
    this->component.configure(this->m_defaultFile, this->m_rows, TICK_USEC, this->m_allocator, 0);
}

void SeqSchedulerTester::UnsolicitedDone() {
    // REQ-SeqScheduler-018
    this->startEmpty();
    this->absRow(0, 5, "a.bin");
    this->clearHistory();
    this->done();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_SeqDoneUnexpected_SIZE(1);
    ASSERT_EVENTS_SeqDoneUnexpected(0, Fw::CmdResponse::OK);
    ASSERT_EQ(this->state(0), State::PENDING);
    ASSERT_from_seqRunOut_SIZE(0);
}

void SeqSchedulerTester::CancelSameTick() {
    // REQ-SeqScheduler-060, 015, 017: with no sequence running the earlier row is skipped and no cancel is sent
    this->startEmpty();
    this->absRow(0, 2, "w.bin");
    this->absRow(1, 2, "c.bin", Policy::CANCEL_RUNNING);
    this->tick(2);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(1);
    ASSERT_EVENTS_RowSkippedByCancel(0, 0, 1);
    ASSERT_from_seqCancelOut_SIZE(0);
    this->assertRun(0, 1, "c.bin");
    ASSERT_from_seqRunOut_SIZE(1);
}

void SeqSchedulerTester::SecondCancelWaits() {
    // REQ-SeqScheduler-060, 015: a second cancel row skips the first and sends no cancel
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "c1.bin", Policy::CANCEL_RUNNING);
    this->absRow(2, 3, "c2.bin", Policy::CANCEL_RUNNING);
    this->tick(3);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(1);
    ASSERT_EVENTS_RowSkippedByCancel(0, 1, 2);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 2, "c2.bin");
    ASSERT_from_seqRunOut_SIZE(2);
}

void SeqSchedulerTester::CancelSkipsAhead() {
    // REQ-SeqScheduler-060, 015, 014: waiting and newly due rows ahead are skipped with WARNING_LO
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "wait.bin");
    this->cmdSetRow(2, Mode::ABSOLUTE, T0 + 3, "new.bin", Policy::WAIT_FOR_RUNNING, 100000);
    this->cmdRowEnabled(2, true);
    this->cmdSetRow(3, Mode::ABSOLUTE, T0 + 3, "c.bin", Policy::CANCEL_RUNNING, 500000);
    this->cmdRowEnabled(3, true);
    this->tick(4);
    ASSERT_EVENTS_RowSkippedByCancel_SIZE(2);
    ASSERT_EVENTS_RowSkippedByCancel(0, 1, 3);
    ASSERT_EVENTS_RowSkippedByCancel(1, 2, 3);
    ASSERT_from_seqCancelOut_SIZE(1);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 3, "c.bin");
}

void SeqSchedulerTester::NoCompletion() {
    // REQ-SeqScheduler-014, 030: with no completion report waiting rows stay waiting; a load frees no row
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 60, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "x.bin"}};
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->tick(50);
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_EQ(this->state(1), State::PENDING);
    this->writeFile(this->defaultFile(), rows, 1);
    this->cmdLoad();
    this->tick(11);
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_TLM_RowsWaiting(this->tlmHistory_RowsWaiting->size() - 1, 1);
    this->done();
    this->tick();
    this->assertRun(1, 0, "x.bin");
}

void SeqSchedulerTester::RunRefused() {
    // REQ-SeqScheduler-014, 020: a refused run is EXECUTED once its error report arrives, which frees later rows
    this->startEmpty();
    this->absRow(0, 1, "missing.bin");
    this->absRow(1, 1, "b.bin");
    this->tick();
    ASSERT_EQ(this->state(0), State::EXECUTING);
    this->done(Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->state(0), State::EXECUTED);
    ASSERT_EVENTS_SeqCompleted(0, 0, Fw::CmdResponse::EXECUTION_ERROR);
    this->tick();
    this->assertRun(1, 1, "b.bin");
}

void SeqSchedulerTester::ExecutingReplaced() {
    // REQ-SeqScheduler-020, 028, 030: a replaced EXECUTING row is not changed by the completion report
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->tick();
    ASSERT_EQ(this->state(0), State::EXECUTING);
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 500, "b.bin");
    ASSERT_EQ(this->state(0), State::PENDING);
    ASSERT_from_seqCancelOut_SIZE(0);
    this->done();
    ASSERT_EVENTS_SeqCompleted_SIZE(1);
    ASSERT_EQ(this->state(0), State::PENDING);
    ASSERT_TLM_LastExecutedRow_SIZE(0);
}

void SeqSchedulerTester::ToleranceFloor() {
    // REQ-SeqScheduler-044: a tolerance below the tick period acts as one tick period
    this->paramSet_LATE_TOLERANCE_SEC(0, Fw::ParamValid::VALID);
    this->paramSend_LATE_TOLERANCE_SEC(0, 0);
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0, "a.bin", Policy::WAIT_FOR_RUNNING, 100000);
    this->cmdRowEnabled(0, true);
    this->tick();  // 0.9 s past due: inside one tick period
    this->assertRun(0, 0, "a.bin");
    ASSERT_EVENTS_RowLate_SIZE(0);
}

void SeqSchedulerTester::DisabledRowSkipped() {
    // REQ-SeqScheduler-022, 056
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 1, "a.bin");
    this->clearHistory();
    this->tick();
    ASSERT_EVENTS_RowSkippedDisabled_SIZE(1);
    ASSERT_EVENTS_RowSkippedDisabled(0, 0);
    ASSERT_EQ(this->state(0), State::SKIPPED);
    ASSERT_from_seqRunOut_SIZE(0);
    this->cmdSave();
    this->cmdLoad();
    this->clearHistory();
    this->tick();
    ASSERT_EVENTS_RowSkippedDisabled_SIZE(0);
    ASSERT_EQ(this->state(0), State::SKIPPED);
}

void SeqSchedulerTester::ComponentDisabled() {
    // REQ-SeqScheduler-058
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "wait.bin");
    this->absRow(2, 4, "later.bin");
    this->absRow(3, 9, "after.bin");
    this->tick(2);  // row 0 runs, row 1 waits
    this->cmdEnabled(false);
    ASSERT_EVENTS_EnabledSet_SIZE(1);
    ASSERT_EVENTS_EnabledSet(0, false);
    Fw::Time timeDue;
    Fw::String name;
    ASSERT_FALSE(this->query(0, timeDue, name));
    this->tick(2);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_EQ(this->state(2), State::SKIPPED);
    ASSERT_EVENTS_RowSkippedDisabled_SIZE(2);
    ASSERT_from_seqCancelOut_SIZE(0);
    ASSERT_EQ(this->state(0), State::EXECUTING);
    this->done();
    this->tick();
    ASSERT_from_seqRunOut_SIZE(1);
    this->cmdEnabled(true);
    ASSERT_TRUE(this->query(0, timeDue, name));
    this->tick(4);
    this->assertRun(1, 3, "after.bin");
}

void SeqSchedulerTester::ReportActiveRow() {
    // REQ-SeqScheduler-057
    this->startEmpty();
    this->cmdReport();
    ASSERT_EVENTS_NoActiveRow_SIZE(1);
    this->absRow(0, 9, "late.bin");
    this->absRow(1, 5, "soon.bin");
    this->cmdReport();
    this->assertLastCmd(SeqScheduler::OPCODE_REPORT_ACTIVE_ROW, Fw::CmdResponse::OK);
    ASSERT_EVENTS_ActiveRow_SIZE(1);
    ASSERT_EVENTS_ActiveRow(0, 1, Fw::TimeValue(TimeBase::TB_WORKSTATION_TIME, 0, T0 + 5, 0), "soon.bin");
}

void SeqSchedulerTester::EnableByIndex() {
    // REQ-SeqScheduler-025, 029
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 50, "a.bin");
    this->cmdRowEnabled(0, true);
    ASSERT_EVENTS_RowEnabledSet_SIZE(1);
    ASSERT_EVENTS_RowEnabledSet(0, 0, true);
    ASSERT_TRUE(this->component.m_table[0].get_enabled());
    ASSERT_EQ(this->state(0), State::PENDING);
    this->cmdRowEnabled(0, false);
    ASSERT_FALSE(this->component.m_table[0].get_enabled());
    ASSERT_EQ(this->state(0), State::PENDING);
}

void SeqSchedulerTester::EnableByName() {
    // REQ-SeqScheduler-026, 029
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 50, "a.bin");
    this->cmdSetRow(1, Mode::ABSOLUTE, T0 + 50, "b.bin");
    this->cmdSetRow(2, Mode::ABSOLUTE, T0 + 50, "a.bin");
    this->cmdSeqEnabled("a.bin", true);
    ASSERT_EVENTS_SeqEnabledSet_SIZE(1);
    ASSERT_EVENTS_SeqEnabledSet(0, "a.bin", true, 2);
    ASSERT_TRUE(this->component.m_table[0].get_enabled());
    ASSERT_FALSE(this->component.m_table[1].get_enabled());
    ASSERT_TRUE(this->component.m_table[2].get_enabled());
    ASSERT_EQ(this->state(0), State::PENDING);
    this->cmdSeqEnabled("a.bin", false);
    ASSERT_FALSE(this->component.m_table[2].get_enabled());
}

void SeqSchedulerTester::SetRow() {
    // REQ-SeqScheduler-027, 028
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->tick();
    this->done();
    ASSERT_EQ(this->state(0), State::EXECUTED);
    this->clearHistory();
    this->cmdSetRow(0, Mode::START_RELATIVE, 77, "b.bin", Policy::SKIP_WHEN_RUNNING, 5);
    this->assertLastCmd(SeqScheduler::OPCODE_SET_ROW, Fw::CmdResponse::OK);
    ASSERT_EVENTS_RowSet_SIZE(1);
    ASSERT_EVENTS_RowSet(0, 0, Fw::TimeValue(TimeBase::TB_WORKSTATION_TIME, 0, 77, 5), Mode::START_RELATIVE, "b.bin",
                         Policy::SKIP_WHEN_RUNNING);
    const SeqScheduler_Row& row = this->component.m_table[0];
    ASSERT_EQ(row.get_state(), State::PENDING);
    ASSERT_EQ(row.get_mode(), Mode::START_RELATIVE);
    ASSERT_EQ(row.get_busyPolicy(), Policy::SKIP_WHEN_RUNNING);
    ASSERT_EQ(row.get_time().get_seconds(), 77U);
    ASSERT_EQ(row.get_time().get_useconds(), 5U);
    ASSERT_STREQ(row.get_fileName().toChar(), "b.bin");
    ASSERT_TRUE(row.get_enabled());
}

void SeqSchedulerTester::SetWaitingRow() {
    // REQ-SeqScheduler-028, 022: setting or disabling a waiting row removes it from the due rows
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->absRow(2, 2, "c.bin");
    this->tick(2);
    ASSERT_EQ(this->component.m_due.getSize(), 2U);
    this->cmdSetRow(1, Mode::ABSOLUTE, T0 + 500, "b.bin");
    ASSERT_EQ(this->component.m_due.getSize(), 1U);
    this->cmdRowEnabled(2, false);
    ASSERT_EQ(this->component.m_due.getSize(), 0U);
    this->tick();
    ASSERT_EQ(this->state(2), State::SKIPPED);
    ASSERT_EQ(this->state(1), State::PENDING);
    this->done();
    this->tick();
    ASSERT_from_seqRunOut_SIZE(1);
}

void SeqSchedulerTester::ArgumentErrors() {
    // REQ-SeqScheduler-034
    this->startEmpty();
    this->absRow(0, 50, "a.bin");
    this->clearHistory();
    this->cmdSetRow(this->m_rows, Mode::ABSOLUTE, T0, "x.bin");
    ASSERT_EVENTS_CmdRejected(0, SeqScheduler::OPCODE_SET_ROW, SeqScheduler_CmdRejection::ROW_INDEX);
    this->assertLastCmd(SeqScheduler::OPCODE_SET_ROW, Fw::CmdResponse::EXECUTION_ERROR);
    this->cmdSetRow(0, Mode::ABSOLUTE, T0, "x.bin", Policy::WAIT_FOR_RUNNING, 1000000);
    ASSERT_EVENTS_CmdRejected(1, SeqScheduler::OPCODE_SET_ROW, SeqScheduler_CmdRejection::TIME);
    this->cmdSetRow(0, Mode::ABSOLUTE, T0, "");
    ASSERT_EVENTS_CmdRejected(2, SeqScheduler::OPCODE_SET_ROW, SeqScheduler_CmdRejection::FILE_NAME);
    this->cmdRowEnabled(this->m_rows, true);
    ASSERT_EVENTS_CmdRejected(3, SeqScheduler::OPCODE_SET_ROW_ENABLED, SeqScheduler_CmdRejection::ROW_INDEX);
    this->cmdRowEnabled(1, true);
    ASSERT_EVENTS_CmdRejected(4, SeqScheduler::OPCODE_SET_ROW_ENABLED, SeqScheduler_CmdRejection::FILE_NAME);
    this->cmdSeqEnabled("", true);
    ASSERT_EVENTS_CmdRejected(5, SeqScheduler::OPCODE_SET_SEQ_ENABLED, SeqScheduler_CmdRejection::FILE_NAME);
    this->cmdSeqEnabled("none.bin", true);
    ASSERT_EVENTS_CmdRejected(6, SeqScheduler::OPCODE_SET_SEQ_ENABLED, SeqScheduler_CmdRejection::NO_MATCH);
    this->assertLastCmd(SeqScheduler::OPCODE_SET_SEQ_ENABLED, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_CmdRejected_SIZE(7);
    ASSERT_EVENTS_SIZE(7);
    // Table unchanged
    ASSERT_STREQ(this->component.m_table[0].get_fileName().toChar(), "a.bin");
    ASSERT_EQ(this->component.m_table[0].get_time().get_seconds(), T0 + 50);
    ASSERT_FALSE(this->component.m_table[1].get_enabled());
    // The limits themselves are accepted: the last row, 999999 microseconds, the longest file name a command
    // carries (FW_CMD_STRING_MAX_SIZE characters; a longer argument is cut to it by the framework)
    char longest[FW_CMD_STRING_MAX_SIZE + 1];
    (void)memset(longest, 'n', FW_CMD_STRING_MAX_SIZE);
    longest[FW_CMD_STRING_MAX_SIZE] = 0;
    this->cmdSetRow(this->m_rows - 1, Mode::ABSOLUTE, T0 + 500, longest, Policy::WAIT_FOR_RUNNING, 999999);
    this->assertLastCmd(SeqScheduler::OPCODE_SET_ROW, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CmdRejected_SIZE(7);
    ASSERT_EQ(this->component.m_table[this->m_rows - 1].get_time().get_useconds(), 999999U);
    ASSERT_EQ(this->component.m_table[this->m_rows - 1].get_fileName().length(),
              static_cast<FwSizeType>(FW_CMD_STRING_MAX_SIZE));
}

void SeqSchedulerTester::EditsDuringSequence() {
    // REQ-SeqScheduler-030
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 900, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "x.bin"}};
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->tick();
    this->cmdRowEnabled(0, false);
    this->cmdSeqEnabled("a.bin", true);
    this->cmdSetRow(1, Mode::ABSOLUTE, T0 + 900, "b.bin");
    this->writeFile(this->defaultFile(), rows, 1);
    this->cmdLoad();
    ASSERT_from_seqCancelOut_SIZE(0);
    ASSERT_EQ(this->component.m_seq.state, SeqScheduler::SeqState::RUNNING);
    this->done();
    ASSERT_EVENTS_SeqCompleted_SIZE(1);
    ASSERT_EQ(this->component.m_seq.state, SeqScheduler::SeqState::IDLE);
}

void SeqSchedulerTester::FailedLoadKeepsTable() {
    // REQ-SeqScheduler-003: a failed load command leaves rows, states, due rows and counts as they were
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->absRow(2, 4, "c.bin");
    this->tick(2);
    const U8 bad[] = {9, 0, 0, 0, 0, 0, 0};
    this->writeBytes(this->defaultFile(), bad, sizeof(bad));
    this->clearHistory();
    const SeqScheduler::LoadState loadState = this->component.m_loadState;
    this->cmdLoad();
    this->assertLastCmd(SeqScheduler::OPCODE_LOAD_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_TableLoadFailed_SIZE(1);
    ASSERT_EVENTS_TableLoadFailed(0, this->defaultFile(), SeqScheduler_LoadFailure::VERSION);
    ASSERT_EQ(this->component.m_loadState, loadState);  // SDD 5.3: unchanged by a failed load command
    ASSERT_EQ(this->state(0), State::EXECUTING);
    ASSERT_EQ(this->component.m_due.getSize(), 1U);
    ASSERT_STREQ(this->component.m_table[2].get_fileName().toChar(), "c.bin");
    this->done();
    this->tick();
    this->assertRun(0, 1, "b.bin");
}

void SeqSchedulerTester::SetRowAfterFailedLoad() {
    // REQ-SeqScheduler-003, 055: after a failed start-up load a row set by command is PENDING and runs
    const U8 bad[] = {9, 0, 0, 0, 0, 0, 0};
    this->writeBytes(this->defaultFile(), bad, sizeof(bad));
    this->tick();
    ASSERT_EVENTS_TableLoadFailed_SIZE(1);
    this->cmdSave();
    ASSERT_EVENTS_CmdRejected(0, SeqScheduler::OPCODE_SAVE_TABLE, SeqScheduler_CmdRejection::TABLE_EMPTY);
    this->absRow(0, 2, "a.bin");
    ASSERT_EQ(this->state(0), State::PENDING);
    this->cmdSave();
    ASSERT_EVENTS_TableSaved_SIZE(1);
    this->tick(2);
    this->assertRun(0, 0, "a.bin");
}

void SeqSchedulerTester::RowFields() {
    // REQ-SeqScheduler-002, 004: each field is read from the file and governs the row
    const FileRow rows[] = {
        {TimeBase::TB_SC_TIME, 123, 456, Mode::ABSOLUTE, Policy::CANCEL_RUNNING, 1, "one.bin"},
        {TimeBase::TB_NONE, 7, 0, Mode::START_RELATIVE, Policy::SKIP_WHEN_RUNNING, 0, "two.bin"},
        {TimeBase::TB_PROC_TIME, 9, 1, Mode::TIME_BASE_RELATIVE, Policy::WAIT_FOR_RUNNING, 1, "three.bin"}};
    this->writeFile(this->defaultFile(), rows, 3);
    this->cmdLoad();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_EVENTS_TableLoaded(0, this->defaultFile(), 3);
    for (FwSizeType i = 0; i < 3; i++) {
        const SeqScheduler_Row& row = this->component.m_table[i];
        ASSERT_EQ(row.get_time().get_timeBase(), rows[i].base);
        ASSERT_EQ(row.get_time().get_seconds(), rows[i].seconds);
        ASSERT_EQ(row.get_time().get_useconds(), rows[i].useconds);
        ASSERT_EQ(static_cast<U8>(row.get_mode()), rows[i].mode);
        ASSERT_EQ(static_cast<U8>(row.get_busyPolicy()), rows[i].policy);
        ASSERT_EQ(row.get_enabled(), rows[i].enabled == 1);
        ASSERT_STREQ(row.get_fileName().toChar(), rows[i].fileName);
        ASSERT_EQ(row.get_state(), State::PENDING);
    }
    // Rows beyond the file are empty
    ASSERT_EQ(this->component.m_table[3].get_fileName().length(), 0U);
}

void SeqSchedulerTester::Configure() {
    // REQ-SeqScheduler-004: the table has the configured row count; the memory is returned by cleanup
    ASSERT_EQ(this->component.m_rows, this->m_rows);
    ASSERT_EQ(this->component.m_due.getCapacity(), this->m_rows);
    ASSERT_NE(this->component.m_memory, nullptr);
    SeqSchedulerTester big(300);
    ASSERT_EQ(big.component.m_rows, 300U);
    big.startEmpty();
    big.absRow(299, 1, "last.bin");
    big.tick();
    big.assertRun(0, 299, "last.bin");
    big.component.cleanup();
    ASSERT_EQ(big.component.m_memory, nullptr);
    big.component.cleanup();  // a second cleanup does nothing
    // The limits of the row count: 1 and 65535 are taken
    SeqSchedulerTester one(1);
    ASSERT_EQ(one.component.m_rows, 1U);
    one.startEmpty();
    one.absRow(0, 1, "only.bin");
    one.tick();
    one.assertRun(0, 0, "only.bin");
    SeqSchedulerTester most(65535);
    ASSERT_EQ(most.component.m_rows, 65535U);
    ASSERT_EQ(most.component.m_due.getCapacity(), 65535U);
    // One past each limit, and a tick period of zero, assert
    big.m_defaultFile = "table.tbl";
    ASSERT_DEATH(big.component.configure(big.m_defaultFile, 0, TICK_USEC, big.m_allocator, 0), "Assert");
    ASSERT_DEATH(big.component.configure(big.m_defaultFile, 65536, TICK_USEC, big.m_allocator, 0), "Assert");
    ASSERT_DEATH(big.component.configure(big.m_defaultFile, 8, 0, big.m_allocator, 0), "Assert");
    const Fw::String noName("");
    ASSERT_DEATH(big.component.configure(noName, 8, TICK_USEC, big.m_allocator, 0), "Assert");
    // A second configure without cleanup asserts
    ASSERT_DEATH(this->component.configure(this->m_defaultFile, 8, TICK_USEC, this->m_allocator, 0), "Assert");
}

void SeqSchedulerTester::DefaultTableFile() {
    // REQ-SeqScheduler-001: the configured name is used when TABLE_FILE is not set or is empty
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 9, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"}};
    this->writeFile(this->defaultFile(), rows, 1);
    this->paramSet_TABLE_FILE(Fw::String(""), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->tick();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_EVENTS_TableLoaded(0, this->defaultFile(), 1);
    SeqSchedulerTester unset;
    unset.writeFile(unset.defaultFile(), rows, 1);
    unset.tick();
    ASSERT_EQ(unset.eventHistory_TableLoaded->size(), 1U);
    ASSERT_STREQ(unset.eventHistory_TableLoaded->at(0).fileName.toChar(), unset.defaultFile());
}

void SeqSchedulerTester::ShortTableFile() {
    // REQ-SeqScheduler-051, 054: fewer rows than the table; the rest are empty and never run
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 1, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"}};
    this->writeFile(this->defaultFile(), rows, 1);
    this->tick();
    ASSERT_EVENTS_TableLoaded(0, this->defaultFile(), 1);
    ASSERT_EVENTS_TableFileRowsDropped_SIZE(0);
    for (FwSizeType i = 1; i < this->m_rows; i++) {
        ASSERT_EQ(this->component.m_table[i].get_fileName().length(), 0U);
        ASSERT_FALSE(this->component.m_table[i].get_enabled());
    }
    this->tick(5);
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_EVENTS_RowSkippedDisabled_SIZE(0);
    ASSERT_TLM_RowsPending(this->tlmHistory_RowsPending->size() - 1, 0);
}

void SeqSchedulerTester::LongTableFile() {
    // REQ-SeqScheduler-052: more rows than the table loads the rows that fit, with WARNING_LO
    FileRow rows[10];
    for (U32 i = 0; i < 10; i++) {
        rows[i] = {
            TimeBase::TB_WORKSTATION_TIME, T0 + 100 + i, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"};
    }
    this->writeFile(this->defaultFile(), rows, 10);
    this->tick();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_EVENTS_TableLoaded(0, this->defaultFile(), this->m_rows);
    ASSERT_EVENTS_TableFileRowsDropped_SIZE(1);
    ASSERT_EVENTS_TableFileRowsDropped(0, this->defaultFile(), 10, this->m_rows);
    ASSERT_EQ(this->component.m_table[this->m_rows - 1].get_time().get_seconds(), T0 + 100 + 7);
}

void SeqSchedulerTester::LoadFailures() {
    // REQ-SeqScheduler-003: each load-failure reason warns once; at start-up the table is left empty
    static U8 bytes[4096];
    const FileRow good = {TimeBase::TB_WORKSTATION_TIME, T0 + 1, 0,      Mode::ABSOLUTE,
                          Policy::WAIT_FOR_RUNNING,      1,      "a.bin"};
    const FwSizeType rowAt = SeqScheduler::TABLE_FILE_HEADER_SIZE;
    struct Case {
        SeqScheduler_LoadFailure::T reason;
        I32 offset;    //!< Byte to change before the checksum is computed; -1 for none
        U8 value;      //!< Its new value
        I32 truncate;  //!< Bytes to remove from the end
        bool corrupt;  //!< Invert a row byte after the checksum
    };
    const Case cases[] = {
        {SeqScheduler_LoadFailure::SIZE, -1, 0, 1, false},
        {SeqScheduler_LoadFailure::SIZE, -1, 0, static_cast<I32>(SeqScheduler::TABLE_FILE_ROW_SIZE) + 3, false},
        {SeqScheduler_LoadFailure::VERSION, 0, 2, 0, false},
        {SeqScheduler_LoadFailure::CHECKSUM, -1, 0, 0, true},
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 11, 3, 0, false},    // mode
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 12, 3, 0, false},    // busy policy
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 13, 2, 0, false},    // enabled
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 7, 0x7F, 0, false},  // microseconds
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 1, 9, 0, false},     // time base
        {SeqScheduler_LoadFailure::ROW_CONTENT, static_cast<I32>(rowAt) + 14, 0, 0, false},    // enabled, empty name
    };
    for (FwSizeType c = 0; c < FW_NUM_ARRAY_ELEMENTS(cases); c++) {
        SeqSchedulerTester t;
        FwSizeType size = t.buildFile(bytes, sizeof(bytes), &good, 1);
        if (cases[c].offset >= 0) {
            bytes[cases[c].offset] = cases[c].value;
            Utils::HashBuffer digest;
            Utils::Hash::hash(bytes, size - HASH_DIGEST_LENGTH, digest);
            (void)memcpy(&bytes[size - HASH_DIGEST_LENGTH], digest.getBuffAddr(), HASH_DIGEST_LENGTH);
        }
        if (cases[c].corrupt) {
            bytes[rowAt + 5] = static_cast<U8>(~bytes[rowAt + 5]);
        }
        size -= static_cast<FwSizeType>(cases[c].truncate);
        t.writeBytes(t.defaultFile(), bytes, size);
        t.tick();
        ASSERT_EQ(t.eventHistory_TableLoadFailed->size(), 1U) << "case " << c;
        ASSERT_EQ(t.eventHistory_TableLoadFailed->at(0).reason, SeqScheduler_LoadFailure(cases[c].reason))
            << "case " << c;
        ASSERT_EQ(t.eventHistory_TableLoaded->size(), 0U);
        ASSERT_EQ(t.component.m_table[0].get_fileName().length(), 0U) << "case " << c;
        t.tick(3);
        ASSERT_EQ(t.eventHistory_TableLoadFailed->size(), 1U);
        ASSERT_EQ(t.fromPortHistory_seqRunOut->size(), 0U);
        ASSERT_EQ(t.tlmHistory_TableLoaded->at(t.tlmHistory_TableLoaded->size() - 1).arg, false);
    }
    // A name with no NUL in its field
    {
        SeqSchedulerTester t;
        const FwSizeType size = t.buildFile(bytes, sizeof(bytes), &good, 1);
        (void)memset(&bytes[rowAt + 14], 'x', FileNameStringSize + 1);
        Utils::HashBuffer digest;
        Utils::Hash::hash(bytes, size - HASH_DIGEST_LENGTH, digest);
        (void)memcpy(&bytes[size - HASH_DIGEST_LENGTH], digest.getBuffAddr(), HASH_DIGEST_LENGTH);
        t.writeBytes(t.defaultFile(), bytes, size);
        t.tick();
        ASSERT_EQ(t.eventHistory_TableLoadFailed->size(), 1U);
        ASSERT_EQ(t.eventHistory_TableLoadFailed->at(0).reason, SeqScheduler_LoadFailure::ROW_CONTENT);
    }
    // A file that cannot be opened: the name is a directory
    {
        SeqSchedulerTester t;
        t.paramSet_TABLE_FILE(Fw::String("."), Fw::ParamValid::VALID);
        t.paramSend_TABLE_FILE(0, 0);
        t.tick();
        ASSERT_EQ(t.eventHistory_TableLoadFailed->size(), 1U);
        ASSERT_EQ(t.eventHistory_TableFileMissing->size(), 0U);
    }
}

void SeqSchedulerTester::CommandedLoad() {
    // REQ-SeqScheduler-005: a load command replaces the table, discarding unsaved edits
    const FileRow rows[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 50, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 0, "file.bin"}};
    this->startEmpty();
    this->absRow(0, 60, "edit0.bin");
    this->absRow(3, 60, "edit3.bin");
    this->writeFile(this->defaultFile(), rows, 1);
    this->cmdLoad();
    this->assertLastCmd(SeqScheduler::OPCODE_LOAD_TABLE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_STREQ(this->component.m_table[0].get_fileName().toChar(), "file.bin");
    ASSERT_FALSE(this->component.m_table[0].get_enabled());
    ASSERT_EQ(this->component.m_table[3].get_fileName().length(), 0U);
}

void SeqSchedulerTester::LoadKeepsOrigins() {
    // REQ-SeqScheduler-008, 011: a load re-evaluates relative rows from the unchanged origin ticks
    const FileRow rows[] = {
        {TimeBase::TB_NONE, 6, 0, Mode::START_RELATIVE, Policy::WAIT_FOR_RUNNING, 1, "s.bin"},
        {TimeBase::TB_WORKSTATION_TIME, 8, 0, Mode::TIME_BASE_RELATIVE, Policy::WAIT_FOR_RUNNING, 1, "b.bin"}};
    this->startEmpty();
    this->tick(3);
    this->writeFile(this->defaultFile(), rows, 2);
    this->cmdLoad();
    this->tick(2);
    ASSERT_from_seqRunOut_SIZE(0);
    this->tick();  // tick 7 = tick 1 + 6
    this->assertRun(0, 0, "s.bin");
    this->done();
    this->tick(2);  // tick 9 = tick 1 + 8
    this->assertRun(1, 1, "b.bin");
}

void SeqSchedulerTester::SaveTable() {
    // REQ-SeqScheduler-031, 032: edits reach the file only on save; no row state; ends at the last used row
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->cmdSetRow(2, Mode::START_RELATIVE, 500, "c.bin", Policy::SKIP_WHEN_RUNNING, 7);
    ASSERT_FALSE(Os::FileSystem::exists(this->defaultFile()));
    this->tick();
    this->done();
    ASSERT_EQ(this->state(0), State::EXECUTED);
    this->cmdEnabled(false);
    this->cmdSave();
    this->assertLastCmd(SeqScheduler::OPCODE_SAVE_TABLE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TableSaved_SIZE(1);
    ASSERT_EVENTS_TableSaved(0, this->defaultFile());
    // The file equals the bytes this test builds from the same rows: layout, row count and checksum
    const FileRow expected[] = {
        {TimeBase::TB_WORKSTATION_TIME, T0 + 1, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 1, "a.bin"},
        {TimeBase::TB_NONE, 0, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 0, ""},
        {TimeBase::TB_WORKSTATION_TIME, 500, 7, Mode::START_RELATIVE, Policy::SKIP_WHEN_RUNNING, 0, "c.bin"}};
    static U8 want[4096];
    static U8 got[4096];
    const FwSizeType size = this->buildFile(want, sizeof(want), expected, 3);
    FwSizeType fileSize = 0;
    ASSERT_EQ(Os::FileSystem::getFileSize(this->defaultFile(), fileSize), Os::FileSystem::OP_OK);
    ASSERT_EQ(fileSize, size);
    Os::File file;
    ASSERT_EQ(file.open(this->defaultFile(), Os::File::OPEN_READ), Os::File::OP_OK);
    FwSizeType read = size;
    ASSERT_EQ(file.read(got, read), Os::File::OP_OK);
    file.close();
    ASSERT_EQ(memcmp(want, got, size), 0);
    // The component enable is not saved: a fresh instance starts enabled
    ASSERT_TRUE(SeqSchedulerTester().component.m_enabled);
}

void SeqSchedulerTester::SaveFailures() {
    // REQ-SeqScheduler-033: a save that cannot write warns and leaves the file as it was
    this->startEmpty();
    this->absRow(0, 50, "a.bin");
    this->paramSet_TABLE_FILE(Fw::String("no_such_dir_seqsched/t.tbl"), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->clearHistory();
    this->cmdSave();
    this->assertLastCmd(SeqScheduler::OPCODE_SAVE_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_TableSaveFailed_SIZE(1);
    ASSERT_EVENTS_TableSaveFailed(0, "no_such_dir_seqsched/t.tbl", SeqScheduler_SaveFailure::OPEN);
    ASSERT_EVENTS_TableSaved_SIZE(0);
    // Rename onto a directory fails
    (void)Os::FileSystem::createDirectory("seqsched_ut_dir");
    this->paramSet_TABLE_FILE(Fw::String("seqsched_ut_dir"), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->cmdSave();
    ASSERT_EVENTS_TableSaveFailed_SIZE(2);
    ASSERT_EVENTS_TableSaveFailed(1, "seqsched_ut_dir", SeqScheduler_SaveFailure::RENAME);
    this->removeFile("seqsched_ut_dir.tmp");
    (void)Os::FileSystem::removeDirectory("seqsched_ut_dir");
}

#define ASSERT_LAST_TLM(NAME, VALUE)                \
    ASSERT_GT(this->tlmHistory_##NAME->size(), 0U); \
    ASSERT_EQ(this->tlmHistory_##NAME->at(this->tlmHistory_##NAME->size() - 1).arg, VALUE)

void SeqSchedulerTester::TimeBaseRecordFull() {
    // REQ-SeqScheduler-009: a base beyond SeqSchedulerTimeBases warns and its rows stay PENDING
    this->startEmpty();  // records TB_WORKSTATION_TIME
    this->cmdSetRow(0, Mode::TIME_BASE_RELATIVE, 0, "a.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_DONT_CARE);
    this->cmdRowEnabled(0, true);
    const TimeBase::T fill[] = {TimeBase::TB_PROC_TIME, TimeBase::TB_SC_TIME, TimeBase::TB_NONE};
    ASSERT_EQ(FW_NUM_ARRAY_ELEMENTS(fill) + 1, static_cast<FwSizeType>(SeqSchedulerTimeBases));
    for (FwSizeType i = 0; i < FW_NUM_ARRAY_ELEMENTS(fill); i++) {
        this->setTime(this->m_seconds, 0, fill[i]);
        this->tick();
    }
    ASSERT_EVENTS_TimeBaseNotRecorded_SIZE(0);
    this->setTime(this->m_seconds, 0, TimeBase::TB_DONT_CARE);
    this->tick(3);
    ASSERT_EVENTS_TimeBaseNotRecorded_SIZE(1);  // once while the base stays
    ASSERT_EVENTS_TimeBaseNotRecorded(0, TimeBase::TB_DONT_CARE);
    this->setTime(this->m_seconds, 0, TimeBase::TB_SC_TIME);
    this->tick();
    this->setTime(this->m_seconds, 0, TimeBase::TB_DONT_CARE);
    this->tick();
    ASSERT_EVENTS_TimeBaseNotRecorded_SIZE(1);  // not again: it is still the last base that found the record full
    ASSERT_from_seqRunOut_SIZE(0);
    ASSERT_EQ(this->state(0), State::PENDING);
    Fw::Time timeDue;
    Fw::String name;
    ASSERT_FALSE(this->query(0, timeDue, name));
}

void SeqSchedulerTester::TimeBaseWarningThrottle() {
    // REQ-SeqScheduler-009: TimeBaseNotRecorded is throttled at 8 until a successful load clears the throttle
    this->startEmpty();  // records TB_WORKSTATION_TIME
    const TimeBase::T bases[] = {TimeBase::TB_PROC_TIME, TimeBase::TB_SC_TIME, TimeBase::TB_NONE};
    for (FwSizeType i = 0; i < FW_NUM_ARRAY_ELEMENTS(bases); i++) {
        this->setTime(this->m_seconds, 0, bases[i]);
        this->tick();
    }
    ASSERT_EQ(this->component.m_baseCount, static_cast<FwSizeType>(SeqSchedulerTimeBases));
    // White box: the default TimeBase enumeration leaves one base unrecorded, so three entries are overwritten
    for (FwSizeType i = 1; i < this->component.m_baseCount; i++) {
        this->component.m_bases[i].base = TimeBase::TB_WORKSTATION_TIME;
    }
    this->clearHistory();
    for (U32 i = 0; i < 12; i++) {
        this->setTime(this->m_seconds, 0, bases[i % FW_NUM_ARRAY_ELEMENTS(bases)]);
        this->tick();
    }
    ASSERT_EVENTS_TimeBaseNotRecorded_SIZE(8);
    const FileRow rows[] = {{TimeBase::TB_SC_TIME, 123, 0, Mode::ABSOLUTE, Policy::WAIT_FOR_RUNNING, 0, "one.bin"}};
    this->writeFile(this->defaultFile(), rows, 1);
    this->cmdLoad();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    for (U32 i = 0; i < 3; i++) {
        this->setTime(this->m_seconds, 0, bases[i % FW_NUM_ARRAY_ELEMENTS(bases)]);
        this->tick();
    }
    ASSERT_EVENTS_TimeBaseNotRecorded_SIZE(11);
}

void SeqSchedulerTester::WaitEndedRejudged() {
    // REQ-SeqScheduler-021, 013: a waiting row returned to PENDING by a load is judged again
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->absRow(2, 6, "c.bin");
    this->cmdSave();
    this->tick(6);
    ASSERT_EQ(this->component.m_due.getSize(), 2U);
    this->cmdLoad();
    ASSERT_EQ(this->component.m_due.getSize(), 0U);
    ASSERT_EQ(this->state(1), State::PENDING);
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->state(0), State::SKIPPED);
    ASSERT_EQ(this->state(1), State::SKIPPED);  // late: no longer waiting
    ASSERT_EQ(this->state(2), State::PENDING);  // within tolerance: waits again
    ASSERT_EQ(this->component.m_due.getSize(), 1U);
    ASSERT_from_seqRunOut_SIZE(0);
    this->done();
    this->tick();
    this->assertRun(0, 2, "c.bin");
}

void SeqSchedulerTester::LoadedRowStates() {
    // REQ-SeqScheduler-023, 056: loaded rows start PENDING; past rows are SKIPPED with no event
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 1, "b.bin", Policy::SKIP_WHEN_RUNNING);
    this->absRow(2, 100, "c.bin");
    this->tick();
    this->done();
    ASSERT_EQ(this->state(0), State::EXECUTED);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    this->cmdSave();
    this->tick(5);
    this->cmdLoad();
    for (FwSizeType i = 0; i < 3; i++) {
        ASSERT_EQ(this->state(i), State::PENDING);
    }
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->state(0), State::SKIPPED);
    ASSERT_EQ(this->state(1), State::SKIPPED);
    ASSERT_EQ(this->state(2), State::PENDING);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_from_seqRunOut_SIZE(0);
}

void SeqSchedulerTester::ReloadWithinTolerance() {
    // REQ-SeqScheduler-023, 021, 015: a load within the tolerance runs the row again
    this->startEmpty();
    this->absRow(0, 1, "a.bin", Policy::CANCEL_RUNNING);
    this->cmdSave();
    this->tick();
    this->assertRun(0, 0, "a.bin");
    this->cmdLoad();
    ASSERT_EQ(this->state(0), State::PENDING);
    this->tick();
    ASSERT_from_seqCancelOut_SIZE(1);  // it cancels its own first run
    ASSERT_EVENTS_SeqCancelled_SIZE(1);
    ASSERT_from_seqRunOut_SIZE(1);
    this->done();
    this->tick();
    ASSERT_from_seqRunOut_SIZE(2);
    this->assertRun(1, 0, "a.bin");
    // With no sequence running the row simply runs again
    SeqSchedulerTester t;
    t.startEmpty();
    t.absRow(0, 1, "a.bin");
    t.cmdSave();
    t.tick();
    t.done();
    t.cmdLoad();
    t.tick();
    ASSERT_EQ(t.fromPortHistory_seqRunOut->size(), 2U);
}

void SeqSchedulerTester::DumpTable() {
    // REQ-SeqScheduler-019, 035, 059: one record per row, in index order, with each row's state
    this->startEmpty();
    this->absRow(0, 1, "a.bin");
    this->absRow(1, 1, "b.bin", Policy::SKIP_WHEN_RUNNING);
    this->absRow(2, 2, "c.bin");
    this->absRow(3, 50, "d.bin", Policy::CANCEL_RUNNING);
    this->tick();
    this->done();
    this->tick();
    this->clearHistory();
    this->cmdDump();
    this->assertLastCmd(SeqScheduler::OPCODE_DUMP_TABLE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TableDumped_SIZE(1);
    ASSERT_EVENTS_TableDumpTruncated_SIZE(0);
    ASSERT_PRODUCT_SEND_SIZE(1);
    Fw::Buffer buffer = this->productSendHistory->at(0).buffer;
    Fw::DpContainer container(this->productSendHistory->at(0).id, buffer);
    ASSERT_EQ(container.deserializeHeader(), Fw::FW_SERIALIZE_OK);
    auto reader = buffer.getDeserializer();
    ASSERT_EQ(reader.deserializeSkip(Fw::DpContainer::DATA_OFFSET), Fw::FW_SERIALIZE_OK);
    const State::T states[] = {State::EXECUTED, State::SKIPPED, State::EXECUTING, State::PENDING};
    for (FwSizeType i = 0; i < this->m_rows; i++) {
        FwDpIdType id = 0;
        SeqScheduler_Row row;
        ASSERT_EQ(reader.deserializeTo(id), Fw::FW_SERIALIZE_OK);
        ASSERT_EQ(id, this->component.getIdBase() + SeqScheduler::RecordId::RowRecord);
        ASSERT_EQ(reader.deserializeTo(row), Fw::FW_SERIALIZE_OK);
        ASSERT_EQ(row, this->component.m_table[i]) << "row " << i;
        if (i < FW_NUM_ARRAY_ELEMENTS(states)) {
            ASSERT_EQ(row.get_state(), states[i]) << "row " << i;
        } else {
            ASSERT_EQ(row.get_fileName().length(), 0U);
        }
    }
    ASSERT_EQ(reader.getDeserializeSizeLeft(), HASH_DIGEST_LENGTH);  // only the data hash follows
}

void SeqSchedulerTester::DumpTruncated() {
    // REQ-SeqScheduler-059: a container that fills before the last row is sent with the rows that fit
    this->startEmpty();
    this->absRow(0, 50, "a.bin");
    this->m_dpLimit = Fw::DpContainer::MIN_PACKET_SIZE + 100;
    this->cmdDump();
    this->assertLastCmd(SeqScheduler::OPCODE_DUMP_TABLE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_TableDumped_SIZE(0);
    ASSERT_EVENTS_TableDumpTruncated_SIZE(1);
    ASSERT_PRODUCT_SEND_SIZE(1);
    const FwSizeType sent = this->eventHistory_TableDumpTruncated->at(0).rowsSent;
    ASSERT_GT(sent, 0U);
    ASSERT_LT(sent, this->m_rows);
    ASSERT_EQ(this->eventHistory_TableDumpTruncated->at(0).rows, this->m_rows);
    // A table larger than SeqSchedulerDpMaxBytes asks for no more than that
    SeqSchedulerTester big(300);
    big.startEmpty();
    const char longName[] = "0123456789012345678901234567890123456789012345678901234567890123456789.bin";
    for (FwSizeType i = 0; i < 300; i++) {
        big.cmdSetRow(i, Mode::ABSOLUTE, T0 + 500, longName);
        big.clearHistory();
    }
    big.cmdDump();
    ASSERT_EQ(big.m_dpAsked, static_cast<FwSizeType>(SeqSchedulerDpMaxBytes));
    ASSERT_EQ(big.eventHistory_TableDumpTruncated->size(), 1U);
    ASSERT_LT(big.eventHistory_TableDumpTruncated->at(0).rowsSent, 300U);
    ASSERT_EQ(big.productSendHistory->size(), 1U);
}

void SeqSchedulerTester::DumpNoBuffer() {
    // REQ-SeqScheduler-036: a refused product buffer fails the command with WARNING_HI
    this->startEmpty();
    this->m_dpGrant = false;
    this->cmdDump();
    this->assertLastCmd(SeqScheduler::OPCODE_DUMP_TABLE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_TableDumpFailed_SIZE(1);
    ASSERT_PRODUCT_SEND_SIZE(0);
}

void SeqSchedulerTester::QueryByPosition() {
    // REQ-SeqScheduler-037, 039: the i-th upcoming row, ordered by when it comes due, then index
    this->startEmpty();
    const U32 first = this->m_seconds;
    this->absRow(0, 9, "r0.bin");
    this->absRow(1, 5, "r1.bin");
    this->absRow(2, 5, "r2.bin");
    this->cmdSetRow(3, Mode::START_RELATIVE, 7, "r3.bin");
    this->cmdRowEnabled(3, true);
    const FwSizeType rows[] = {1, 2, 3, 0};
    const U32 due[] = {T0 + 5, T0 + 5, first + 7, T0 + 9};
    const char* const names[] = {"r1.bin", "r2.bin", "r3.bin", "r0.bin"};
    for (FwSizeType i = 0; i < 4; i++) {
        Fw::Time timeDue;
        Fw::String name;
        ASSERT_TRUE(this->query(i, timeDue, name)) << i;
        ASSERT_EQ(timeDue, Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0, due[i], 0)) << rows[i];
        ASSERT_STREQ(name.toChar(), names[i]);
    }
    Fw::Time timeDue;
    Fw::String name;
    ASSERT_FALSE(this->query(4, timeDue, name));
}

void SeqSchedulerTester::QueryRelative() {
    // REQ-SeqScheduler-046, 061: relative time due is fixed at the origin tick's system time
    this->startEmpty();
    const U32 first = this->m_seconds;
    this->cmdSetRow(0, Mode::START_RELATIVE, 100, "s.bin", Policy::WAIT_FOR_RUNNING, 5);
    this->cmdRowEnabled(0, true);
    this->cmdSetRow(1, Mode::TIME_BASE_RELATIVE, 200, "b.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_SC_TIME);
    this->cmdRowEnabled(1, true);
    this->tick(2);
    this->setTime(50, 0, TimeBase::TB_SC_TIME);
    this->tick();
    const U32 scFirst = this->m_seconds;
    for (U32 pass = 0; pass < 2; pass++) {
        Fw::Time timeDue;
        Fw::String name;
        ASSERT_TRUE(this->query(0, timeDue, name));
        ASSERT_EQ(timeDue, Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0, first + 100, 5));
        ASSERT_STREQ(name.toChar(), "s.bin");
        ASSERT_TRUE(this->query(1, timeDue, name));
        ASSERT_EQ(timeDue, Fw::Time(TimeBase::TB_SC_TIME, 0, scFirst + 200, 0));
        ASSERT_STREQ(name.toChar(), "b.bin");
        // Neither moves when system time or the time base changes
        this->setTime(this->m_seconds + 5000, 0, TimeBase::TB_WORKSTATION_TIME);
        this->tick();
    }
}

void SeqSchedulerTester::QueryNotUpcoming() {
    // REQ-SeqScheduler-039
    this->startEmpty();
    this->cmdSetRow(0, Mode::ABSOLUTE, T0 + 50, "disabled.bin");
    this->absRow(2, 1, "executed.bin");
    this->cmdSetRow(3, Mode::ABSOLUTE, T0 + 50, "otherbase.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_SC_TIME);
    this->cmdRowEnabled(3, true);
    this->cmdSetRow(4, Mode::TIME_BASE_RELATIVE, 5, "unknown.bin", Policy::WAIT_FOR_RUNNING, 0, TimeBase::TB_PROC_TIME);
    this->cmdRowEnabled(4, true);
    this->absRow(5, 1, "skipped.bin", Policy::SKIP_WHEN_RUNNING);
    this->tick();
    ASSERT_EQ(this->state(2), State::EXECUTING);
    ASSERT_EQ(this->state(5), State::SKIPPED);
    Fw::Time timeDue;
    Fw::String name;
    ASSERT_FALSE(this->query(0, timeDue, name));
    this->done();
    ASSERT_FALSE(this->query(0, timeDue, name));
    this->absRow(6, 60, "up.bin");
    ASSERT_TRUE(this->query(0, timeDue, name));
    ASSERT_STREQ(name.toChar(), "up.bin");
    ASSERT_FALSE(this->query(1, timeDue, name));
}

void SeqSchedulerTester::QueryInvalid() {
    // REQ-SeqScheduler-038, 039
    Fw::Time timeDue;
    Fw::String name;
    ASSERT_FALSE(this->query(0, timeDue, name));  // empty table, nothing loaded
    this->cmdSetRow(0, Mode::START_RELATIVE, 30, "s.bin");
    this->cmdRowEnabled(0, true);
    ASSERT_FALSE(this->query(0, timeDue, name));  // before the first tick
    this->tick();
    ASSERT_TRUE(this->query(0, timeDue, name));
    ASSERT_FALSE(this->query(1, timeDue, name));
    ASSERT_FALSE(this->query(this->m_rows, timeDue, name));
    ASSERT_FALSE(this->query(std::numeric_limits<FwSizeType>::max(), timeDue, name));
}

void SeqSchedulerTester::Telemetry() {
    // REQ-SeqScheduler-041
    this->tick();
    ASSERT_LAST_TLM(TableLoaded, true);
    ASSERT_LAST_TLM(Enabled, true);
    ASSERT_LAST_TLM(NextRow, this->m_rows);
    ASSERT_LAST_TLM(NextTimeDue, Fw::TimeValue(TimeBase::TB_NONE, 0, 0, 0));
    ASSERT_LAST_TLM(RunningRow, this->m_rows);
    ASSERT_LAST_TLM(RowsWaiting, 0U);
    ASSERT_LAST_TLM(RowsPending, 0U);
    ASSERT_LAST_TLM(RowsExecuted, 0U);
    ASSERT_LAST_TLM(RowsSkipped, 0U);
    ASSERT_TLM_LastExecutedRow_SIZE(0);
    this->absRow(0, 2, "a.bin");
    this->absRow(1, 2, "b.bin");
    this->absRow(2, 50, "c.bin");
    this->cmdSetRow(3, Mode::ABSOLUTE, T0 + 1, "d.bin");
    this->tick(2);
    ASSERT_LAST_TLM(RunningRow, 0U);
    ASSERT_LAST_TLM(RowsWaiting, 1U);
    ASSERT_LAST_TLM(RowsPending, 2U);
    ASSERT_LAST_TLM(RowsExecuted, 0U);
    ASSERT_LAST_TLM(RowsSkipped, 1U);
    this->done();
    this->tick();
    ASSERT_LAST_TLM(LastExecutedRow, 0U);
    ASSERT_LAST_TLM(RunningRow, 1U);
    ASSERT_LAST_TLM(RowsWaiting, 0U);
    ASSERT_LAST_TLM(RowsPending, 1U);
    ASSERT_LAST_TLM(RowsExecuted, 1U);
    ASSERT_LAST_TLM(NextRow, 2U);
    ASSERT_LAST_TLM(NextTimeDue, Fw::TimeValue(TimeBase::TB_WORKSTATION_TIME, 0, T0 + 50, 0));
    this->cmdEnabled(false);
    ASSERT_LAST_TLM(Enabled, false);
}

void SeqSchedulerTester::TwoInstances() {
    // REQ-SeqScheduler-042: two instances hold separate tables and run separate sequences
    SeqSchedulerTester other(3);
    this->startEmpty();
    other.startEmpty();
    ASSERT_STRNE(this->defaultFile(), other.defaultFile());
    this->absRow(0, 1, "mine.bin");
    other.absRow(0, 2, "theirs.bin");
    other.absRow(1, 1, "theirs1.bin", Policy::SKIP_WHEN_RUNNING);
    this->cmdSave();
    ASSERT_FALSE(Os::FileSystem::exists(other.defaultFile()));
    this->tick();
    other.tick();
    this->assertRun(0, 0, "mine.bin");
    other.assertRun(0, 1, "theirs1.bin");
    other.tick();  // their row 0 waits for their sequence, not for this one's
    ASSERT_EQ(other.fromPortHistory_seqRunOut->size(), 1U);
    this->done();
    other.tick();
    ASSERT_EQ(other.fromPortHistory_seqRunOut->size(), 1U);
    other.done();
    other.tick();
    other.assertRun(1, 0, "theirs.bin");
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_EQ(this->component.m_rows, this->m_rows);
    ASSERT_EQ(other.component.m_rows, 3U);
}

void SeqSchedulerTester::ParamReread() {
    // REQ-SeqScheduler-001, 005, 032: the file name is resolved at each load and save; the tolerance each tick
    const char alt[] = "seqsched_ut_alt.tbl";
    this->removeFile(alt);
    this->startEmpty();
    this->absRow(0, 500, "a.bin");
    this->paramSet_TABLE_FILE(Fw::String(alt), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->cmdSave();
    ASSERT_EVENTS_TableSaved_SIZE(1);
    ASSERT_EVENTS_TableSaved(0, alt);
    ASSERT_TRUE(Os::FileSystem::exists(alt));
    ASSERT_FALSE(Os::FileSystem::exists(this->defaultFile()));
    this->paramSet_TABLE_FILE(Fw::String(""), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->cmdLoad();
    ASSERT_EVENTS_TableLoadFailed_SIZE(1);  // the default file does not exist
    ASSERT_EVENTS_TableLoadFailed(0, this->defaultFile(), SeqScheduler_LoadFailure::OPEN);
    this->paramSet_TABLE_FILE(Fw::String(alt), Fw::ParamValid::VALID);
    this->paramSend_TABLE_FILE(0, 0);
    this->cmdLoad();
    ASSERT_EVENTS_TableLoaded_SIZE(1);
    ASSERT_EVENTS_TableLoaded(0, alt, 1);
    this->removeFile(alt);
    this->tick();  // the first tick after the load
    this->paramSet_LATE_TOLERANCE_SEC(10, Fw::ParamValid::VALID);
    this->paramSend_LATE_TOLERANCE_SEC(0, 0);
    this->cmdSetRow(1, Mode::ABSOLUTE, this->m_seconds - 5, "late1.bin");
    this->cmdRowEnabled(1, true);
    this->tick();
    this->assertRun(0, 1, "late1.bin");
    this->done();
    this->paramSet_LATE_TOLERANCE_SEC(2, Fw::ParamValid::VALID);
    this->paramSend_LATE_TOLERANCE_SEC(0, 0);
    this->cmdSetRow(2, Mode::ABSOLUTE, this->m_seconds - 5, "late2.bin");
    this->cmdRowEnabled(2, true);
    this->tick();
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_EVENTS_RowLate_SIZE(1);
    ASSERT_EVENTS_RowLate(0, 2);
}

void SeqSchedulerTester::RandomOperations() {
    // REQ-SeqScheduler-004, 013, 020, 029: random commands, ticks and completions against a reference model
    struct Model {
        bool used = false;
        bool enabled = false;
        U32 due = 0;
        char name[16] = "";
    };
    Model model[16];
    ASSERT_LE(this->m_rows, FW_NUM_ARRAY_ELEMENTS(model));
    STest::Random::seed();
    this->startEmpty();
    U32 runs = 0;
    for (U32 step = 0; step < 3000; step++) {
        const U32 op = STest::Pick::lowerUpper(0, 9);
        const FwSizeType row = STest::Pick::lowerUpper(0, static_cast<U32>(this->m_rows - 1));
        if (op <= 1) {
            Model& m = model[row];
            m.used = true;  // the enabled flag is unchanged by SET_ROW
            m.due = this->m_seconds + STest::Pick::lowerUpper(0, 6);
            (void)snprintf(m.name, sizeof(m.name), "s%u.bin", STest::Pick::lowerUpper(0, 3));
            const Policy::T policy = static_cast<Policy::T>(STest::Pick::lowerUpper(0, 2));
            this->cmdSetRow(row, Mode::ABSOLUTE, m.due, m.name, policy);
        } else if (op <= 3) {
            const bool enabled = STest::Pick::lowerUpper(0, 3) != 0;
            this->cmdRowEnabled(row, enabled);
            if (model[row].used) {
                model[row].enabled = enabled;
                this->assertLastCmd(SeqScheduler::OPCODE_SET_ROW_ENABLED, Fw::CmdResponse::OK);
            } else if (enabled) {
                this->assertLastCmd(SeqScheduler::OPCODE_SET_ROW_ENABLED, Fw::CmdResponse::EXECUTION_ERROR);
            }
        } else if (op == 4) {
            if (this->component.m_seq.state != SeqScheduler::SeqState::IDLE) {
                this->done();
            }
        } else {
            this->tick();
        }
        runs += static_cast<U32>(this->fromPortHistory_seqRunOut->size());
        // The table equals the model, and the row states are consistent
        FwSizeType executing = 0;
        FwSizeType due = 0;
        for (FwSizeType i = 0; i < this->m_rows; i++) {
            const SeqScheduler_Row& r = this->component.m_table[i];
            const Model& m = model[i];
            ASSERT_STREQ(r.get_fileName().toChar(), m.name) << "step " << step;
            ASSERT_EQ(r.get_enabled(), m.enabled) << "step " << step << " row " << i;
            if (!m.used) {
                ASSERT_EQ(r.get_state(), State::PENDING);
                ASSERT_FALSE(this->component.m_rowDue[i].due);
                continue;
            }
            ASSERT_EQ(r.get_time().get_seconds(), m.due);
            if (this->component.m_rowDue[i].due) {
                due++;
                ASSERT_EQ(r.get_state(), State::PENDING) << "step " << step << " row " << i;
                ASSERT_TRUE(m.enabled);
                ASSERT_LE(m.due, this->m_seconds);
            }
            switch (r.get_state()) {
                case State::EXECUTING:
                    executing++;
                    ASSERT_NE(this->component.m_seq.state, SeqScheduler::SeqState::IDLE);
                    ASSERT_EQ(this->component.m_seq.row, i);
                    ASSERT_LE(m.due, this->m_seconds);
                    break;
                case State::EXECUTED:
                case State::SKIPPED:
                    ASSERT_LE(m.due, this->m_seconds) << "step " << step << " row " << i;
                    break;
                default:
                    if ((op >= 5) && (!this->component.m_rowDue[i].due)) {
                        // After a tick a PENDING row that is not waiting has not come due
                        ASSERT_GT(m.due, this->m_seconds) << "step " << step << " row " << i;
                    }
                    break;
            }
        }
        ASSERT_LE(executing, 1U);
        ASSERT_EQ(this->component.m_due.getSize(), due);
        for (FwSizeType i = 0; i < this->fromPortHistory_seqRunOut->size(); i++) {
            ASSERT_EQ(this->component.m_seq.state, SeqScheduler::SeqState::RUNNING);
        }
        ASSERT_LE(this->fromPortHistory_seqRunOut->size(), 1U);  // at most one run per step
        this->clearHistory();
    }
    ASSERT_GT(runs, 20U);
}

// TESTS-END

}  // namespace Svc
