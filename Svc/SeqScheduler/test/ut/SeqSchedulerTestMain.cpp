// ======================================================================
// \title  SeqSchedulerTestMain.cpp
// \brief  cpp file for SeqScheduler component test main function
// ======================================================================

#include "STest/Random/Random.hpp"
#include "SeqSchedulerTester.hpp"

TEST(SeqScheduler, StartupLoad) {
    Svc::SeqSchedulerTester tester;
    tester.StartupLoad();
}

TEST(SeqScheduler, MissingTableFile) {
    Svc::SeqSchedulerTester tester;
    tester.MissingTableFile();
}

TEST(SeqScheduler, AbsoluteDue) {
    Svc::SeqSchedulerTester tester;
    tester.AbsoluteDue();
}

TEST(SeqScheduler, AbsoluteOtherBase) {
    Svc::SeqSchedulerTester tester;
    tester.AbsoluteOtherBase();
}

TEST(SeqScheduler, CancelBetweenDue) {
    Svc::SeqSchedulerTester tester;
    tester.CancelBetweenDue();
}

TEST(SeqScheduler, CancelLastOfSeveralDue) {
    Svc::SeqSchedulerTester tester;
    tester.CancelLastOfSeveralDue();
}

TEST(SeqScheduler, TwoCancelsDue) {
    Svc::SeqSchedulerTester tester;
    tester.TwoCancelsDue();
}

TEST(SeqScheduler, Ping) {
    Svc::SeqSchedulerTester tester;
    tester.Ping();
}

TEST(SeqScheduler, StartRelativeDue) {
    Svc::SeqSchedulerTester tester;
    tester.StartRelativeDue();
}

TEST(SeqScheduler, TimeBaseRelativeDue) {
    Svc::SeqSchedulerTester tester;
    tester.TimeBaseRelativeDue();
}

TEST(SeqScheduler, TickRounding) {
    Svc::SeqSchedulerTester tester;
    tester.TickRounding();
}

TEST(SeqScheduler, TicksWhileBusy) {
    Svc::SeqSchedulerTester tester;
    tester.TicksWhileBusy();
}

TEST(SeqScheduler, LoadBeforeFirstTick) {
    Svc::SeqSchedulerTester tester;
    tester.LoadBeforeFirstTick();
}

TEST(SeqScheduler, WaitForRunning) {
    Svc::SeqSchedulerTester tester;
    tester.WaitForRunning();
}

TEST(SeqScheduler, CancelRunning) {
    Svc::SeqSchedulerTester tester;
    tester.CancelRunning();
}

TEST(SeqScheduler, SkipWhenRunning) {
    Svc::SeqSchedulerTester tester;
    tester.SkipWhenRunning();
}

TEST(SeqScheduler, SameTickOrder) {
    Svc::SeqSchedulerTester tester;
    tester.SameTickOrder();
}

TEST(SeqScheduler, DueOrderEarlyTicks) {
    Svc::SeqSchedulerTester tester;
    tester.DueOrderEarlyTicks();
}

TEST(SeqScheduler, WaitingOrder) {
    Svc::SeqSchedulerTester tester;
    tester.WaitingOrder();
}

TEST(SeqScheduler, WaitingNotLate) {
    Svc::SeqSchedulerTester tester;
    tester.WaitingNotLate();
}

TEST(SeqScheduler, LateRow) {
    Svc::SeqSchedulerTester tester;
    tester.LateRow();
}

TEST(SeqScheduler, DisableThenEnableSkipsWaiting) {
    Svc::SeqSchedulerTester tester;
    tester.DisableThenEnableSkipsWaiting();
}

TEST(SeqScheduler, UnsolicitedDone) {
    Svc::SeqSchedulerTester tester;
    tester.UnsolicitedDone();
}

TEST(SeqScheduler, CancelSameTick) {
    Svc::SeqSchedulerTester tester;
    tester.CancelSameTick();
}

TEST(SeqScheduler, SecondCancelWaits) {
    Svc::SeqSchedulerTester tester;
    tester.SecondCancelWaits();
}

TEST(SeqScheduler, CancelSkipsAhead) {
    Svc::SeqSchedulerTester tester;
    tester.CancelSkipsAhead();
}

TEST(SeqScheduler, NoCompletion) {
    Svc::SeqSchedulerTester tester;
    tester.NoCompletion();
}

TEST(SeqScheduler, RunRefused) {
    Svc::SeqSchedulerTester tester;
    tester.RunRefused();
}

TEST(SeqScheduler, ExecutingReplaced) {
    Svc::SeqSchedulerTester tester;
    tester.ExecutingReplaced();
}

TEST(SeqScheduler, ToleranceFloor) {
    Svc::SeqSchedulerTester tester;
    tester.ToleranceFloor();
}

TEST(SeqScheduler, DisabledRowSkipped) {
    Svc::SeqSchedulerTester tester;
    tester.DisabledRowSkipped();
}

TEST(SeqScheduler, ComponentDisabled) {
    Svc::SeqSchedulerTester tester;
    tester.ComponentDisabled();
}

TEST(SeqScheduler, ReportActiveRow) {
    Svc::SeqSchedulerTester tester;
    tester.ReportActiveRow();
}

TEST(SeqScheduler, EnableByIndex) {
    Svc::SeqSchedulerTester tester;
    tester.EnableByIndex();
}

TEST(SeqScheduler, EnableByName) {
    Svc::SeqSchedulerTester tester;
    tester.EnableByName();
}

TEST(SeqScheduler, SetRow) {
    Svc::SeqSchedulerTester tester;
    tester.SetRow();
}

TEST(SeqScheduler, SetWaitingRow) {
    Svc::SeqSchedulerTester tester;
    tester.SetWaitingRow();
}

TEST(SeqScheduler, ArgumentErrors) {
    Svc::SeqSchedulerTester tester;
    tester.ArgumentErrors();
}

TEST(SeqScheduler, EditsDuringSequence) {
    Svc::SeqSchedulerTester tester;
    tester.EditsDuringSequence();
}

TEST(SeqScheduler, FailedLoadKeepsTable) {
    Svc::SeqSchedulerTester tester;
    tester.FailedLoadKeepsTable();
}

TEST(SeqScheduler, SetRowAfterFailedLoad) {
    Svc::SeqSchedulerTester tester;
    tester.SetRowAfterFailedLoad();
}

TEST(SeqScheduler, RowFields) {
    Svc::SeqSchedulerTester tester;
    tester.RowFields();
}

TEST(SeqScheduler, Configure) {
    Svc::SeqSchedulerTester tester;
    tester.Configure();
}

TEST(SeqScheduler, DefaultTableFile) {
    Svc::SeqSchedulerTester tester;
    tester.DefaultTableFile();
}

TEST(SeqScheduler, ShortTableFile) {
    Svc::SeqSchedulerTester tester;
    tester.ShortTableFile();
}

TEST(SeqScheduler, LongTableFile) {
    Svc::SeqSchedulerTester tester;
    tester.LongTableFile();
}

TEST(SeqScheduler, LoadFailures) {
    Svc::SeqSchedulerTester tester;
    tester.LoadFailures();
}

TEST(SeqScheduler, CommandedLoad) {
    Svc::SeqSchedulerTester tester;
    tester.CommandedLoad();
}

TEST(SeqScheduler, LoadKeepsOrigins) {
    Svc::SeqSchedulerTester tester;
    tester.LoadKeepsOrigins();
}

TEST(SeqScheduler, SaveTable) {
    Svc::SeqSchedulerTester tester;
    tester.SaveTable();
}

TEST(SeqScheduler, SaveFailures) {
    Svc::SeqSchedulerTester tester;
    tester.SaveFailures();
}

TEST(SeqScheduler, TimeBaseRecordFull) {
    Svc::SeqSchedulerTester tester;
    tester.TimeBaseRecordFull();
}

TEST(SeqScheduler, WaitEndedRejudged) {
    Svc::SeqSchedulerTester tester;
    tester.WaitEndedRejudged();
}

TEST(SeqScheduler, LoadedRowStates) {
    Svc::SeqSchedulerTester tester;
    tester.LoadedRowStates();
}

TEST(SeqScheduler, ReloadWithinTolerance) {
    Svc::SeqSchedulerTester tester;
    tester.ReloadWithinTolerance();
}

TEST(SeqScheduler, DumpTable) {
    Svc::SeqSchedulerTester tester;
    tester.DumpTable();
}

TEST(SeqScheduler, DumpTruncated) {
    Svc::SeqSchedulerTester tester;
    tester.DumpTruncated();
}

TEST(SeqScheduler, DumpNoBuffer) {
    Svc::SeqSchedulerTester tester;
    tester.DumpNoBuffer();
}

TEST(SeqScheduler, QueryByPosition) {
    Svc::SeqSchedulerTester tester;
    tester.QueryByPosition();
}

TEST(SeqScheduler, QueryRelative) {
    Svc::SeqSchedulerTester tester;
    tester.QueryRelative();
}

TEST(SeqScheduler, QueryNotUpcoming) {
    Svc::SeqSchedulerTester tester;
    tester.QueryNotUpcoming();
}

TEST(SeqScheduler, QueryInvalid) {
    Svc::SeqSchedulerTester tester;
    tester.QueryInvalid();
}

TEST(SeqScheduler, Telemetry) {
    Svc::SeqSchedulerTester tester;
    tester.Telemetry();
}

TEST(SeqScheduler, TwoInstances) {
    Svc::SeqSchedulerTester tester;
    tester.TwoInstances();
}

TEST(SeqScheduler, ParamReread) {
    Svc::SeqSchedulerTester tester;
    tester.ParamReread();
}

TEST(SeqScheduler, RandomOperations) {
    Svc::SeqSchedulerTester tester;
    tester.RandomOperations();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
