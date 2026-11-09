// ----------------------------------------------------------------------
// BufferManagerTestMain.cpp
// ----------------------------------------------------------------------

#include "BufferManagerTester.hpp"
#include "Fw/Test/UnitTest.hpp"
#include "STest/Pick/Pick.hpp"
#include "STest/Random/Random.hpp"
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"

using Svc::BufferManagerTester;
using Svc::BufferManagerTestState;

// ----------------------------------------------------------------------
// Directed tests
// ----------------------------------------------------------------------

TEST(Nominal, Setup) {
    BufferManagerTester tester;
    tester.setupBehavior();
}

TEST(Nominal, SingleBin) {
    BufferManagerTester tester;
    tester.singleBin();
}

TEST(Nominal, MultipleBins) {
    BufferManagerTester tester;
    tester.multipleBins();
}

TEST(Nominal, TrimmedReturn) {
    BufferManagerTester tester;
    tester.trimmedReturn();
}

TEST(Nominal, BufferSizeSelection) {
    BufferManagerTester tester;
    tester.bufferSizeSelection();
}

TEST(Nominal, EventThrottling) {
    BufferManagerTester tester;
    tester.eventThrottling();
}

TEST(Nominal, PayloadIntegrity) {
    BufferManagerTester tester;
    tester.payloadIntegrity();
}

TEST(Nominal, Lifecycle) {
    BufferManagerTester tester;
    tester.lifecycle();
}

TEST(OffNominal, SetupSizeOverflowAsserts) {
    BufferManagerTester tester;
    tester.setupSizeOverflowAsserts();
}

TEST(OffNominal, SetupBinOrderAsserts) {
    BufferManagerTester tester;
    tester.setupBinOrderAsserts();
}

TEST(OffNominal, SendInAsserts) {
    BufferManagerTester tester;
    tester.sendInAsserts();
}

TEST(OffNominal, UseBeforeSetupAsserts) {
    BufferManagerTester tester;
    tester.useBeforeSetupAsserts();
}

// ----------------------------------------------------------------------
// Rule-based tests
// ----------------------------------------------------------------------

namespace {
//! Steps per random scenario: enough for every bin of the small tables to be exhausted and refilled many times
const U32 RANDOM_STEPS = 10000;

//! Every rule, so the scenario tests share one declaration
struct Rules {
    BufferManagerTester::Lifecycle__Setup setup;
    BufferManagerTester::Lifecycle__Cleanup cleanup;
    BufferManagerTester::Lifecycle__CleanupHolding cleanupHolding;
    BufferManagerTester::Lifecycle__NotSetupAsserts notSetupAsserts;
    BufferManagerTester::Lifecycle__BinOrderAsserts binOrderAsserts;
    BufferManagerTester::Lifecycle__SizeOverflowAsserts sizeOverflowAsserts;
    BufferManagerTester::Lifecycle__EmptyBinsAsserts emptyBinsAsserts;
    BufferManagerTester::Lifecycle__SetupTwiceAsserts setupTwiceAsserts;
    BufferManagerTester::BufferGet__Ok get;
    BufferManagerTester::BufferGet__Between between;
    BufferManagerTester::BufferGet__Zero zero;
    BufferManagerTester::BufferGet__NoneAvailable none;
    BufferManagerTester::BufferReturn__Ok ret;
    BufferManagerTester::BufferReturn__Trimmed trimmed;
    BufferManagerTester::BufferReturn__Empty empty;
    BufferManagerTester::BufferReturn__Asserts returnAsserts;
    BufferManagerTester::Sched__Telemetry sched;
};

//! Apply every non-asserting rule at random against an already set up tester. The *Asserts rules fork a
//! process per ASSERT_DEATH and run only in the directed sequences below.
void runRandom(BufferManagerTester& tester, Rules& r) {
    STest::Rule<BufferManagerTester>* rules[] = {&r.setup, &r.cleanup, &r.cleanupHolding, &r.get,   &r.between, &r.zero,
                                                 &r.none,  &r.ret,     &r.trimmed,        &r.empty, &r.sched};
    STest::RandomScenario<BufferManagerTester> random("Random Rules", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<BufferManagerTester> bounded("Bounded Random Rules Scenario", random, RANDOM_STEPS);
    ASSERT_EQ(RANDOM_STEPS, bounded.run(tester));
}
}  // namespace

// Setup: allocator contract, telemetry before any request, every buffer obtainable, release on cleanup
TEST(Rules, Setup) {
    REQUIREMENT("FPRIME-BM-001");
    REQUIREMENT("FPRIME-BM-005");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    r.sched.apply(tester);
    while (tester.shadow.anyFree()) {
        r.get.apply(tester);
    }
    r.none.apply(tester);
    r.cleanup.apply(tester);
}

// Exhaust a single bin, check telemetry, return everything in random order, return an empty buffer
TEST(Rules, SingleBin) {
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-004");
    REQUIREMENT("FPRIME-BM-006");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesOneBin();
    r.sched.apply(tester);
    while (tester.shadow.anyFree()) {
        r.get.apply(tester);
    }
    r.none.apply(tester);
    r.sched.apply(tester);
    while (!tester.shadow.held.empty()) {
        r.ret.apply(tester);
    }
    r.empty.apply(tester);
    r.sched.apply(tester);
    tester.teardownRules();
}

// Exhaust three bins with random sizes, then drain them with full and trimmed returns
TEST(Rules, MultipleBins) {
    REQUIREMENT("FPRIME-BM-001");
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-007");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    while (tester.shadow.anyFree()) {
        r.get.apply(tester);
    }
    r.none.apply(tester);
    r.sched.apply(tester);
    while (!tester.shadow.held.empty()) {
        if (STest::Pick::lowerUpper(0, 1) == 0) {
            r.ret.apply(tester);
        } else {
            r.trimmed.apply(tester);
        }
    }
    r.empty.apply(tester);
    r.sched.apply(tester);
    tester.teardownRules();
}

// Bin selection: between-bin sizes, over-max with everything free, size 0, spill-over and reuse
TEST(Rules, BufferSizeSelection) {
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-009");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    r.between.apply(tester);
    r.ret.apply(tester);
    r.none.apply(tester);
    while (tester.shadow.anyFree()) {
        if (r.between.precondition(tester) && STest::Pick::lowerUpper(0, 1) == 0) {
            r.between.apply(tester);
        } else {
            r.get.apply(tester);
        }
    }
    r.ret.apply(tester);
    r.get.apply(tester);
    r.zero.apply(tester);
    r.sched.apply(tester);
    tester.teardownRules();
}

// Events throttle while the counters keep counting
TEST(Rules, EventThrottling) {
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-004");
    REQUIREMENT("FPRIME-BM-009");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesOneBuffer();
    r.get.apply(tester);
    for (U32 i = 0; i < BufferManagerTestState::EVENT_THROTTLE + 2; i++) {
        r.none.apply(tester);
        r.empty.apply(tester);
        r.zero.apply(tester);
    }
    r.sched.apply(tester);
    tester.teardownRules();
}

// Use before setup, single-buffer pool, setup over a live pool, idempotent cleanup, use after
// cleanup, then setup/cleanup cycles (one with a buffer still out) whose telemetry restarts per pool
TEST(Rules, Lifecycle) {
    REQUIREMENT("FPRIME-BM-011");
    REQUIREMENT("FPRIME-BM-012");
    BufferManagerTester tester;
    Rules r;
    r.notSetupAsserts.apply(tester);
    tester.setupRulesOneBuffer();
    r.get.apply(tester);
    r.none.apply(tester);
    r.setupTwiceAsserts.apply(tester);
    r.cleanup.apply(tester);
    r.notSetupAsserts.apply(tester);
    r.setup.apply(tester);
    r.get.apply(tester);
    r.sched.apply(tester);
    r.cleanupHolding.apply(tester);
    tester.setupRulesDefault();  // a different bin table: TotalBuffs changes
    r.sched.apply(tester);
    r.zero.apply(tester);
    r.sched.apply(tester);
    r.cleanup.apply(tester);
}

// setup() rejects misordered bins, wrapping sizes and empty configurations, then accepts equal sizes and gaps
TEST(Rules, SetupAsserts) {
    REQUIREMENT("FPRIME-BM-008");
    REQUIREMENT("FPRIME-BM-010");
    BufferManagerTester tester;
    Rules r;
    r.binOrderAsserts.apply(tester);
    r.sizeOverflowAsserts.apply(tester);
    r.emptyBinsAsserts.apply(tester);
    tester.setupRulesEqualAndGapped();
    r.get.apply(tester);
    tester.teardownRules();
}

// bufferSendIn rejects a corrupt or foreign buffer and a double return
TEST(Rules, SendInAsserts) {
    COMMENT("bufferSendIn asserts on a buffer that is not one it handed out (SDD 3.6.2)");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    r.get.apply(tester);
    r.returnAsserts.apply(tester);
    tester.teardownRules();
}

// Randomized test against the three-bin configuration
TEST(Rules, Random) {
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-006");
    REQUIREMENT("FPRIME-BM-007");
    REQUIREMENT("FPRIME-BM-012");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    runRandom(tester, r);
    tester.teardownRules();
}

// Randomized test against a random bin configuration
TEST(Rules, RandomBins) {
    REQUIREMENT("FPRIME-BM-001");
    REQUIREMENT("FPRIME-BM-002");
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesRandom();
    runRandom(tester, r);
    tester.teardownRules();
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
