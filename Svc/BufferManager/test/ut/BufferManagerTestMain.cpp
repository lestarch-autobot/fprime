// ----------------------------------------------------------------------
// TestMain.cpp
// ----------------------------------------------------------------------

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

// White-box test of the memory layout built by setup(); expected to change if the layout does
TEST(Implementation, ContiguousLayout) {
    BufferManagerTester tester;
    tester.contiguousLayout();
}

// ----------------------------------------------------------------------
// Rule-based tests
// ----------------------------------------------------------------------

// Exhaust a single bin, check telemetry, return everything in random order, return an empty buffer
TEST(Rules, OneSize) {
    BufferManagerTester tester;
    BufferManagerTester::BufferGet__Ok get;
    BufferManagerTester::BufferGet__NoneAvailable none;
    BufferManagerTester::BufferReturn__Ok ret;
    BufferManagerTester::BufferReturn__Empty empty;
    BufferManagerTester::Sched__Telemetry sched;

    tester.setupRulesOneBin();
    sched.apply(tester);
    while (tester.shadow.anyFree()) {
        get.apply(tester);
    }
    none.apply(tester);
    sched.apply(tester);
    while (!tester.shadow.shadow_held.empty()) {
        ret.apply(tester);
    }
    empty.apply(tester);
    sched.apply(tester);
    tester.teardownRules();
}

// Exhaust three bins with random sizes, then drain them with full and trimmed returns
TEST(Rules, MultSize) {
    BufferManagerTester tester;
    BufferManagerTester::BufferGet__Ok get;
    BufferManagerTester::BufferGet__NoneAvailable none;
    BufferManagerTester::BufferReturn__Ok ret;
    BufferManagerTester::BufferReturn__Trimmed trimmed;
    BufferManagerTester::BufferReturn__Empty empty;
    BufferManagerTester::Sched__Telemetry sched;

    tester.setupRulesDefault();
    while (tester.shadow.anyFree()) {
        get.apply(tester);
    }
    none.apply(tester);
    sched.apply(tester);
    while (!tester.shadow.shadow_held.empty()) {
        if (STest::Pick::lowerUpper(0, 1) == 0) {
            ret.apply(tester);
        } else {
            trimmed.apply(tester);
        }
    }
    empty.apply(tester);
    sched.apply(tester);
    tester.teardownRules();
}

// Randomized test: apply rules in a random sequence against the three-bin configuration
TEST(Rules, Random) {
    const U32 numRulesToApply = 10000;
    BufferManagerTester tester;
    BufferManagerTester::BufferGet__Ok get;
    BufferManagerTester::BufferGet__NoneAvailable none;
    BufferManagerTester::BufferReturn__Ok ret;
    BufferManagerTester::BufferReturn__Trimmed trimmed;
    BufferManagerTester::BufferReturn__Empty empty;
    BufferManagerTester::Sched__Telemetry sched;
    STest::Rule<BufferManagerTester>* rules[] = {&get, &none, &ret, &trimmed, &empty, &sched};

    tester.setupRulesDefault();
    STest::RandomScenario<BufferManagerTester> random("Random Rules", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<BufferManagerTester> bounded("Bounded Random Rules Scenario", random, numRulesToApply);
    const U32 numSteps = bounded.run(tester);
    printf("Ran %u steps.\n", numSteps);
    ASSERT_EQ(numRulesToApply, numSteps);
    tester.teardownRules();
}

// Randomized test against a random bin configuration
TEST(Rules, RandomBins) {
    const U32 numRulesToApply = 10000;
    BufferManagerTester tester;
    BufferManagerTester::BufferGet__Ok get;
    BufferManagerTester::BufferGet__NoneAvailable none;
    BufferManagerTester::BufferReturn__Ok ret;
    BufferManagerTester::BufferReturn__Trimmed trimmed;
    BufferManagerTester::BufferReturn__Empty empty;
    BufferManagerTester::Sched__Telemetry sched;
    STest::Rule<BufferManagerTester>* rules[] = {&get, &none, &ret, &trimmed, &empty, &sched};

    tester.setupRulesRandom();
    STest::RandomScenario<BufferManagerTester> random("Random Rules", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<BufferManagerTester> bounded("Bounded Random Rules Scenario", random, numRulesToApply);
    const U32 numSteps = bounded.run(tester);
    printf("Ran %u steps.\n", numSteps);
    ASSERT_EQ(numRulesToApply, numSteps);
    tester.teardownRules();
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
