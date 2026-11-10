// ----------------------------------------------------------------------
// TestMain.cpp
// ----------------------------------------------------------------------

#include <cstring>
#include "BufferManagerTester.hpp"
#include "STest/Random/Random.hpp"
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"

using Svc::BufferManagerTester;

// ----------------------------------------------------------------------
// Directed tests
// ----------------------------------------------------------------------

TEST(Nominal, Setup) {
    BufferManagerTester tester;
    tester.setupBehavior();
}

TEST(Nominal, OneSize) {
    BufferManagerTester tester;
    tester.oneBufferSize();
}

TEST(Nominal, MultSize) {
    BufferManagerTester tester;
    tester.multBuffSize();
}

TEST(Nominal, BufferSizeTrimmed) {
    BufferManagerTester tester;
    tester.bufferSizeTrimmed();
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

//! Apply the port and lifecycle rules at random for numSteps steps against an already set up tester
void runRandom(BufferManagerTester& tester, Rules& r, const U32 numSteps) {
    STest::Rule<BufferManagerTester>* rules[] = {&r.setup, &r.cleanup, &r.cleanupHolding, &r.get,   &r.between, &r.zero,
                                                 &r.none,  &r.ret,     &r.trimmed,        &r.empty, &r.sched};
    STest::RandomScenario<BufferManagerTester> random("Random Rules", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<BufferManagerTester> bounded("Bounded Random Rules Scenario", random, numSteps);
    const U32 ran = bounded.run(tester);
    printf("Ran %u steps.\n", ran);
    ASSERT_EQ(numSteps, ran);
}
}  // namespace

// Setup: allocator contract, telemetry before any request, every buffer obtainable, release on cleanup
TEST(Rules, Setup) {
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
TEST(Rules, OneSize) {
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesOneBin();
    r.sched.apply(tester);
    while (tester.shadow.anyFree()) {
        r.get.apply(tester);
    }
    r.none.apply(tester);
    r.sched.apply(tester);
    while (!tester.shadow.shadow_held.empty()) {
        r.ret.apply(tester);
    }
    r.empty.apply(tester);
    r.sched.apply(tester);
    tester.teardownRules();
}

// Exhaust three bins with random sizes, then drain them with full and trimmed returns
TEST(Rules, MultSize) {
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    while (tester.shadow.anyFree()) {
        r.get.apply(tester);
    }
    r.none.apply(tester);
    r.sched.apply(tester);
    while (!tester.shadow.shadow_held.empty()) {
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
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesOneBuffer();
    r.get.apply(tester);
    for (U32 i = 0; i < Svc::BufferManagerTestState::EVENT_THROTTLE + 2; i++) {
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

// setup() rejects misordered bins and wrapping sizes, then accepts equal sizes and gaps
TEST(Rules, SetupAsserts) {
    BufferManagerTester tester;
    Rules r;
    r.binOrderAsserts.apply(tester);
    r.sizeOverflowAsserts.apply(tester);
    r.emptyBinsAsserts.apply(tester);
    Svc::BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = Svc::BufferManagerTester::BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = Svc::BufferManagerTester::BIN0_BUFFER_SIZE;
    bins.bins[1].numBuffers = 1;
    bins.bins[3].bufferSize = Svc::BufferManagerTester::BIN1_BUFFER_SIZE;
    bins.bins[3].numBuffers = 1;
    tester.setupRules(bins);
    r.get.apply(tester);
    tester.teardownRules();
}

// bufferSendIn rejects a corrupt or foreign buffer and a double return
TEST(Rules, SendInAsserts) {
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    r.get.apply(tester);
    r.returnAsserts.apply(tester);
    tester.teardownRules();
}

// Randomized test against the three-bin configuration
TEST(Rules, Random) {
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesDefault();
    runRandom(tester, r, 10000);
    tester.teardownRules();
}

// Randomized test against a random bin configuration
TEST(Rules, RandomBins) {
    BufferManagerTester tester;
    Rules r;
    tester.setupRulesRandom();
    runRandom(tester, r, 10000);
    tester.teardownRules();
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
