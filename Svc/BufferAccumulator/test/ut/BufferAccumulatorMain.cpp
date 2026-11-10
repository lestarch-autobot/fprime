// ======================================================================
// \title  Main.cpp
// \author bocchino, mereweth
// \brief  Test drain mode
//
// \copyright
// Copyright (c) 2017 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "Accumulate.hpp"
#include "BufferAccumulatorTester.hpp"
#include "Drain.hpp"
#include "DrainLimit.hpp"
#include "Errors.hpp"
#include "Health.hpp"

// ----------------------------------------------------------------------
// Test Errors
// ----------------------------------------------------------------------

TEST(TestErrors, QueueFull) {
    Svc::Errors::BufferAccumulatorTester tester;
    tester.QueueFull();
}

TEST(TestErrors, PartialDrain) {
    Svc::Errors::BufferAccumulatorTester tester;
    tester.PartialDrain();
}

// ----------------------------------------------------------------------
// Test Accumulate
// ----------------------------------------------------------------------

TEST(TestAccumulate, OK) {
    Svc::Accumulate::BufferAccumulatorTester tester;
    tester.OK();
}

// ----------------------------------------------------------------------
// Test Drain
// ----------------------------------------------------------------------

TEST(TestDrain, OK) {
    Svc::Drain::BufferAccumulatorTester tester;
    tester.OK();
}

TEST(TestDrain, InitialMode) {
    Svc::Drain::BufferAccumulatorTester tester(false);
    tester.InitialMode();
}

TEST(TestPartialDrain, OK) {
    Svc::Drain::BufferAccumulatorTester tester;
    tester.PartialDrainOK();
}

TEST(TestPartialDrain, NoBlock) {
    Svc::Drain::BufferAccumulatorTester tester;
    tester.PartialDrainNoBlock();
}

// ----------------------------------------------------------------------
// Test DrainLimit
// ----------------------------------------------------------------------

TEST(TestDrainLimit, DefaultLimit) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.DefaultLimit();
}

TEST(TestDrainLimit, InitialLimit) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.InitialLimit();
}

TEST(TestDrainLimit, InitialLimitInvalid) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.InitialLimitInvalid();
}

TEST(TestDrainLimit, SetLimitOK) {
    Svc::DrainLimit::BufferAccumulatorTester tester;
    tester.SetLimitOK();
}

TEST(TestDrainLimit, SetLimitInvalid) {
    Svc::DrainLimit::BufferAccumulatorTester tester;
    tester.SetLimitInvalid();
}

TEST(TestDrainLimit, WindowedDrain) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.WindowedDrain();
}

TEST(TestDrainLimit, WindowedPartialDrain) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.WindowedPartialDrain();
}

TEST(TestDrainLimit, SetLimitDuringPartialDrain) {
    Svc::DrainLimit::BufferAccumulatorTester tester;
    tester.SetLimitDuringPartialDrain();
}

TEST(TestDrainLimit, LowerLimitBelowOutstanding) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.LowerLimitBelowOutstanding();
}

TEST(TestDrainLimit, UnexpectedReturn) {
    Svc::DrainLimit::BufferAccumulatorTester tester;
    tester.UnexpectedReturn();
}

TEST(TestDrainLimit, AccumulateMidFlight) {
    Svc::DrainLimit::BufferAccumulatorTester tester(false);
    tester.AccumulateMidFlight();
}

TEST(TestDrainLimit, StallAtLimit) {
    Svc::DrainLimit::BufferAccumulatorTester tester;
    tester.StallAtLimit();
}

// ----------------------------------------------------------------------
// Test Health
// ----------------------------------------------------------------------

TEST(TestHealth, Ping) {
    Svc::Health::BufferAccumulatorTester tester;
    tester.Ping();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
