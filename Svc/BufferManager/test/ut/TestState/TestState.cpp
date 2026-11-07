// ======================================================================
// \title  TestState.cpp
// \brief  Shadow helper implementations for BufferManager test state
// ======================================================================
#include "Svc/BufferManager/test/ut/TestState/TestState.hpp"
#include <cstring>
#include "Fw/Types/Assert.hpp"
#include "STest/Pick/Pick.hpp"

namespace Svc {

BufferManagerTestState::BufferManagerTestState()
    : shadow_isSetup(false),
      shadow_everSetup(false),
      shadow_numBins(0),
      shadow_totalBuffs(0),
      shadow_currBuffs(0),
      shadow_highWater(0),
      shadow_noBuffs(0),
      shadow_emptyBuffs(0),
      shadow_noBuffsEvents(0),
      shadow_zeroEvents(0),
      shadow_emptyBuffsEvents(0),
      shadow_tlmSent(false),
      shadow_lastTotalBuffs(0),
      shadow_lastCurrBuffs(0),
      shadow_lastHighWater(0),
      shadow_lastNoBuffs(0),
      shadow_lastEmptyBuffs(0) {
    (void)memset(&this->shadow_bins, 0, sizeof(this->shadow_bins));
    (void)memset(this->shadow_binSize, 0, sizeof(this->shadow_binSize));
    (void)memset(this->shadow_binFree, 0, sizeof(this->shadow_binFree));
}

void BufferManagerTestState::configure(const BufferManagerComponentImpl::BufferBins& bins) {
    // Everything that outlives the pool in the component survives a re-setup here too: the lifetime
    // counters, the event throttles (owned by the autocoded base) and the last telemetry written
    const BufferManagerTestState previous = *this;
    *this = BufferManagerTestState();
    this->shadow_noBuffs = previous.shadow_noBuffs;
    this->shadow_emptyBuffs = previous.shadow_emptyBuffs;
    this->shadow_noBuffsEvents = previous.shadow_noBuffsEvents;
    this->shadow_emptyBuffsEvents = previous.shadow_emptyBuffsEvents;
    this->shadow_zeroEvents = previous.shadow_zeroEvents;
    this->shadow_tlmSent = previous.shadow_tlmSent;
    this->shadow_lastTotalBuffs = previous.shadow_lastTotalBuffs;
    this->shadow_lastCurrBuffs = previous.shadow_lastCurrBuffs;
    this->shadow_lastHighWater = previous.shadow_lastHighWater;
    this->shadow_lastNoBuffs = previous.shadow_lastNoBuffs;
    this->shadow_lastEmptyBuffs = previous.shadow_lastEmptyBuffs;
    this->shadow_bins = bins;
    this->shadow_isSetup = true;
    this->shadow_everSetup = true;
    for (U16 bin = 0; bin < BUFFERMGR_MAX_NUM_BINS; bin++) {
        if (bins.bins[bin].numBuffers != 0) {
            this->shadow_binSize[this->shadow_numBins] = bins.bins[bin].bufferSize;
            this->shadow_binFree[this->shadow_numBins] = bins.bins[bin].numBuffers;
            this->shadow_totalBuffs += bins.bins[bin].numBuffers;
            this->shadow_numBins++;
        }
    }
}

FwSizeType BufferManagerTestState::totalPayloadBytes() const {
    FwSizeType total = 0;
    for (U16 bin = 0; bin < BUFFERMGR_MAX_NUM_BINS; bin++) {
        total += this->shadow_bins.bins[bin].bufferSize * this->shadow_bins.bins[bin].numBuffers;
    }
    return total;
}

FwSizeType BufferManagerTestState::pickBetweenBinSize() const {
    FwSizeType candidates[BUFFERMGR_MAX_NUM_BINS] = {};
    U16 count = 0;
    for (U16 bin = 1; bin < this->shadow_numBins; bin++) {
        if (this->shadow_binFree[bin] != 0 && this->shadow_binSize[bin] > this->shadow_binSize[bin - 1] + 1) {
            candidates[count] = static_cast<FwSizeType>(STest::Pick::lowerUpper(
                static_cast<U32>(this->shadow_binSize[bin - 1]) + 1, static_cast<U32>(this->shadow_binSize[bin]) - 1));
            count++;
        }
    }
    return (count == 0) ? 0 : candidates[STest::Pick::lowerUpper(0, count - 1)];
}

bool BufferManagerTestState::isHeldId(U32 id) const {
    for (const HeldBuffer& held : this->shadow_held) {
        if ((held.buffer.getContext() & 0xFFFF) == id) {
            return true;
        }
    }
    return false;
}

bool BufferManagerTestState::anyFree() const {
    return this->largestFreeSize() != 0;
}

FwSizeType BufferManagerTestState::largestFreeSize() const {
    FwSizeType largest = 0;
    for (U16 bin = 0; bin < this->shadow_numBins; bin++) {
        if (this->shadow_binFree[bin] != 0 && this->shadow_binSize[bin] > largest) {
            largest = this->shadow_binSize[bin];
        }
    }
    return largest;
}

FwSizeType BufferManagerTestState::largestBinSize() const {
    FwSizeType largest = 0;
    for (U16 bin = 0; bin < this->shadow_numBins; bin++) {
        if (this->shadow_binSize[bin] > largest) {
            largest = this->shadow_binSize[bin];
        }
    }
    return largest;
}

bool BufferManagerTestState::expectedBin(FwSizeType size, U16& bin) const {
    for (U16 candidate = 0; candidate < this->shadow_numBins; candidate++) {
        if (this->shadow_binFree[candidate] != 0 && this->shadow_binSize[candidate] >= size) {
            bin = candidate;
            return true;
        }
    }
    return false;
}

FwSizeType BufferManagerTestState::pickServableSize() const {
    const FwSizeType largest = this->largestFreeSize();
    FW_ASSERT(largest != 0);
    // Favor the bin boundaries, which are where selection decisions change
    if (STest::Pick::lowerUpper(0, 1) == 0) {
        U16 freeBins = 0;
        for (U16 bin = 0; bin < this->shadow_numBins; bin++) {
            if (this->shadow_binFree[bin] != 0) {
                freeBins++;
            }
        }
        // At least one bin is free (largest != 0), so the pick always lands on a free bin below
        U16 pick = static_cast<U16>(STest::Pick::lowerUpper(0, freeBins - 1));
        for (U16 bin = 0; bin < this->shadow_numBins; bin++) {
            if (this->shadow_binFree[bin] != 0) {
                if (pick == 0) {
                    return this->shadow_binSize[bin];
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
    FW_ASSERT(!this->shadow_held.empty());
    return static_cast<FwSizeType>(STest::Pick::lowerUpper(0, static_cast<U32>(this->shadow_held.size()) - 1));
}

bool BufferManagerTestState::overlapsHeld(const Fw::Buffer& buffer) const {
    const U8* const start = buffer.getData();
    const U8* const end = start + buffer.getCapacity();
    for (const HeldBuffer& held : this->shadow_held) {
        const U8* const heldStart = held.buffer.getData();
        const U8* const heldEnd = heldStart + held.buffer.getCapacity();
        if (start < heldEnd && heldStart < end) {
            return true;
        }
    }
    return false;
}

void BufferManagerTestState::recordGet(const Fw::Buffer& buffer, U16 bin, FwSizeType requested, U8 pattern) {
    FW_ASSERT(bin < this->shadow_numBins);
    FW_ASSERT(this->shadow_binFree[bin] != 0);
    this->shadow_binFree[bin]--;
    this->shadow_held.push_back(HeldBuffer{buffer, bin, requested, pattern});
    this->shadow_currBuffs++;
    if (this->shadow_currBuffs > this->shadow_highWater) {
        this->shadow_highWater = this->shadow_currBuffs;
    }
}

void BufferManagerTestState::recordZeroRequest() {
    this->shadow_noBuffs++;
    if (this->shadow_zeroEvents < EVENT_THROTTLE) {
        this->shadow_zeroEvents++;
    }
}

void BufferManagerTestState::dropHeld() {
    this->shadow_held.clear();
    this->shadow_currBuffs = 0;
}

void BufferManagerTestState::recordNoBuffer() {
    this->shadow_noBuffs++;
    if (this->shadow_noBuffsEvents < EVENT_THROTTLE) {
        this->shadow_noBuffsEvents++;
    }
}

void BufferManagerTestState::recordReturn(FwSizeType index) {
    FW_ASSERT(index < this->shadow_held.size());
    this->shadow_binFree[this->shadow_held[index].bin]++;
    this->shadow_held.erase(this->shadow_held.begin() + static_cast<std::ptrdiff_t>(index));
    this->shadow_currBuffs--;
}

void BufferManagerTestState::recordEmptyReturn() {
    this->shadow_emptyBuffs++;
    if (this->shadow_emptyBuffsEvents < EVENT_THROTTLE) {
        this->shadow_emptyBuffsEvents++;
    }
}

}  // namespace Svc
