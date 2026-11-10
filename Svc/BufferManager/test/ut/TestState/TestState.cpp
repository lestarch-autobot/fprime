// ======================================================================
// \title  TestState.cpp
// \brief  Shadow helper implementations for BufferManager test state
// ======================================================================
#include "Svc/BufferManager/test/ut/TestState/TestState.hpp"
#include <cstddef>
#include "Fw/Types/Assert.hpp"
#include "STest/Pick/Pick.hpp"

namespace Svc {

namespace {
const U32 CONTEXT_MGR_SHIFT = 16;
const U32 CONTEXT_ID_MASK = 0xFFFF;
}  // namespace

U32 BufferManagerTestState::contextOf(U16 mgrId, U32 bufferId) {
    return (static_cast<U32>(mgrId) << CONTEXT_MGR_SHIFT) | bufferId;
}

U32 BufferManagerTestState::managerIdOf(const Fw::Buffer& buffer) {
    return buffer.getContext() >> CONTEXT_MGR_SHIFT;
}

U32 BufferManagerTestState::bufferIdOf(const Fw::Buffer& buffer) {
    return buffer.getContext() & CONTEXT_ID_MASK;
}

bool BufferManagerTestState::isPowerOfTwo(FwSizeType value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

bool BufferManagerTestState::isAligned(const Fw::Buffer& buffer, FwSizeType alignment) {
    FW_ASSERT(isPowerOfTwo(alignment), static_cast<FwAssertArgType>(alignment));
    return (reinterpret_cast<PlatformPointerCastType>(buffer.getData()) % alignment) == 0;
}

// ----------------------------------------------------------------------
// Updates mirroring component actions
// ----------------------------------------------------------------------

void BufferManagerTestState::stageBins(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType poolAlignment) {
    this->table = bins;
    this->alignment = poolAlignment;
}

void BufferManagerTestState::configure() {
    this->isSetup = true;
    this->everSetup = true;
    this->numBins = 0;
    this->totalBuffs = 0;
    this->held.clear();
    this->currBuffs = 0;
    this->highWater = 0;
    for (U16 bin = 0; bin < BUFFERMGR_MAX_NUM_BINS; bin++) {
        this->binSize[bin] = 0;
        this->binFree[bin] = 0;
    }
    for (U16 bin = 0; bin < BUFFERMGR_MAX_NUM_BINS; bin++) {
        if (this->table.bins[bin].numBuffers != 0) {
            this->binSize[this->numBins] = this->table.bins[bin].bufferSize;
            this->binFree[this->numBins] = this->table.bins[bin].numBuffers;
            this->totalBuffs += this->table.bins[bin].numBuffers;
            this->numBins++;
        }
    }
}

void BufferManagerTestState::recordCleanup() {
    this->held.clear();
    this->isSetup = false;
}

void BufferManagerTestState::recordGet(const Fw::Buffer& buffer, U16 bin, FwSizeType requested, U8 pattern) {
    FW_ASSERT(bin < this->numBins);
    FW_ASSERT(this->binFree[bin] != 0);
    this->binFree[bin]--;
    this->held.push_back(HeldBuffer{buffer, bin, requested, pattern});
    this->currBuffs++;
    if (this->currBuffs > this->highWater) {
        this->highWater = this->currBuffs;
    }
}

void BufferManagerTestState::recordNoBuffer() {
    this->noBuffs++;
    if (this->noBuffsEvents < EVENT_THROTTLE) {
        this->noBuffsEvents++;
    }
}

void BufferManagerTestState::recordZeroRequest() {
    this->noBuffs++;
    if (this->zeroEvents < EVENT_THROTTLE) {
        this->zeroEvents++;
    }
}

void BufferManagerTestState::recordReturn(FwSizeType index) {
    FW_ASSERT(index < this->held.size());
    this->binFree[this->held[index].bin]++;
    this->held.erase(this->held.begin() + static_cast<std::ptrdiff_t>(index));
    this->currBuffs--;
}

void BufferManagerTestState::recordEmptyReturn() {
    this->emptyBuffs++;
    if (this->emptyBuffsEvents < EVENT_THROTTLE) {
        this->emptyBuffsEvents++;
    }
}

void BufferManagerTestState::recordTelemetry() {
    this->tlmSent = true;
    this->lastTotalBuffs = this->totalBuffs;
    this->lastCurrBuffs = this->currBuffs;
    this->lastHighWater = this->highWater;
    this->lastNoBuffs = this->noBuffs;
    this->lastEmptyBuffs = this->emptyBuffs;
}

// ----------------------------------------------------------------------
// Queries used by rule preconditions and actions
// ----------------------------------------------------------------------

bool BufferManagerTestState::anyFree() const {
    return this->largestFreeSize() != 0;
}

FwSizeType BufferManagerTestState::largestFreeSize() const {
    FwSizeType largest = 0;
    for (U16 bin = 0; bin < this->numBins; bin++) {
        if (this->binFree[bin] != 0 && this->binSize[bin] > largest) {
            largest = this->binSize[bin];
        }
    }
    return largest;
}

FwSizeType BufferManagerTestState::largestBinSize() const {
    FwSizeType largest = 0;
    for (U16 bin = 0; bin < this->numBins; bin++) {
        if (this->binSize[bin] > largest) {
            largest = this->binSize[bin];
        }
    }
    return largest;
}

bool BufferManagerTestState::expectedBin(FwSizeType size, U16& bin) const {
    for (U16 candidate = 0; candidate < this->numBins; candidate++) {
        if (this->binFree[candidate] != 0 && this->binSize[candidate] >= size) {
            bin = candidate;
            return true;
        }
    }
    return false;
}

bool BufferManagerTestState::hasGapBelow(U16 bin) const {
    FW_ASSERT(bin > 0 && bin < this->numBins, bin);
    return this->binFree[bin] != 0 && this->binSize[bin] > this->binSize[bin - 1] + 1;
}

bool BufferManagerTestState::hasBetweenBinSize() const {
    for (U16 bin = 1; bin < this->numBins; bin++) {
        if (this->hasGapBelow(bin)) {
            return true;
        }
    }
    return false;
}

FwSizeType BufferManagerTestState::pickBetweenBinSize() const {
    FW_ASSERT(this->hasBetweenBinSize());
    // Pick one of the qualifying bins, then a size in the gap below it
    U16 candidates[BUFFERMGR_MAX_NUM_BINS] = {};
    U16 count = 0;
    for (U16 bin = 1; bin < this->numBins; bin++) {
        if (this->hasGapBelow(bin)) {
            candidates[count] = bin;
            count++;
        }
    }
    const U16 bin = candidates[STest::Pick::lowerUpper(0, count - 1)];
    return static_cast<FwSizeType>(STest::Pick::lowerUpper(static_cast<U32>(this->binSize[bin - 1]) + 1,
                                                           static_cast<U32>(this->binSize[bin]) - 1));
}

FwSizeType BufferManagerTestState::pickServableSize() const {
    const FwSizeType largest = this->largestFreeSize();
    FW_ASSERT(largest != 0);
    // Favor the bin boundaries, which are where selection decisions change
    if (STest::Pick::lowerUpper(0, 1) == 0) {
        U16 freeBins = 0;
        for (U16 bin = 0; bin < this->numBins; bin++) {
            if (this->binFree[bin] != 0) {
                freeBins++;
            }
        }
        // At least one bin is free (largest != 0), so the pick always lands on a free bin below
        U16 pick = static_cast<U16>(STest::Pick::lowerUpper(0, freeBins - 1));
        for (U16 bin = 0; bin < this->numBins; bin++) {
            if (this->binFree[bin] != 0) {
                if (pick == 0) {
                    return this->binSize[bin];
                }
                pick--;
            }
        }
        FW_ASSERT(0);  // unreachable: pick < freeBins
    }
    return static_cast<FwSizeType>(STest::Pick::lowerUpper(1, static_cast<U32>(largest)));
}

FwSizeType BufferManagerTestState::pickUnservableSize() const {
    // Anything above the largest free buffer is unservable; range up to two past the largest bin so the
    // over-every-bin case is reached even when every bin has a free buffer
    const FwSizeType largest = this->largestFreeSize();
    return static_cast<FwSizeType>(
        STest::Pick::lowerUpper(static_cast<U32>(largest) + 1, static_cast<U32>(this->largestBinSize()) + 2));
}

FwSizeType BufferManagerTestState::pickHeldIndex() const {
    FW_ASSERT(!this->held.empty());
    return static_cast<FwSizeType>(STest::Pick::lowerUpper(0, static_cast<U32>(this->held.size()) - 1));
}

bool BufferManagerTestState::isHeldId(U32 id) const {
    for (const HeldBuffer& entry : this->held) {
        if (bufferIdOf(entry.buffer) == id) {
            return true;
        }
    }
    return false;
}

bool BufferManagerTestState::overlapsHeld(const Fw::Buffer& buffer) const {
    const U8* const start = buffer.getData();
    const U8* const end = start + buffer.getCapacity();
    for (const HeldBuffer& entry : this->held) {
        const U8* const heldStart = entry.buffer.getData();
        const U8* const heldEnd = heldStart + entry.buffer.getCapacity();
        if (start < heldEnd && heldStart < end) {
            return true;
        }
    }
    return false;
}

}  // namespace Svc
