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
    : shadow_numBins(0),
      shadow_totalBuffs(0),
      shadow_currBuffs(0),
      shadow_highWater(0),
      shadow_noBuffs(0),
      shadow_emptyBuffs(0),
      shadow_noBuffsEvents(0),
      shadow_emptyBuffsEvents(0),
      shadow_tlmSent(false),
      shadow_lastTotalBuffs(0),
      shadow_lastCurrBuffs(0),
      shadow_lastHighWater(0),
      shadow_lastNoBuffs(0),
      shadow_lastEmptyBuffs(0) {
    (void)memset(this->shadow_binSize, 0, sizeof(this->shadow_binSize));
    (void)memset(this->shadow_binFree, 0, sizeof(this->shadow_binFree));
}

void BufferManagerTestState::configure(const BufferManagerComponentImpl::BufferBins& bins) {
    *this = BufferManagerTestState();
    for (U16 bin = 0; bin < BUFFERMGR_MAX_NUM_BINS; bin++) {
        if (bins.bins[bin].numBuffers != 0) {
            this->shadow_binSize[this->shadow_numBins] = bins.bins[bin].bufferSize;
            this->shadow_binFree[this->shadow_numBins] = bins.bins[bin].numBuffers;
            this->shadow_totalBuffs += bins.bins[bin].numBuffers;
            this->shadow_numBins++;
        }
    }
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
            freeBins = static_cast<U16>(freeBins + ((this->shadow_binFree[bin] != 0) ? 1 : 0));
        }
        U16 pick = static_cast<U16>(STest::Pick::lowerUpper(0, freeBins - 1));
        for (U16 bin = 0; bin < this->shadow_numBins; bin++) {
            if (this->shadow_binFree[bin] != 0) {
                if (pick == 0) {
                    return this->shadow_binSize[bin];
                }
                pick--;
            }
        }
        FW_ASSERT(0);
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

void BufferManagerTestState::recordGet(const Fw::Buffer& buffer, U16 bin, FwSizeType requested) {
    FW_ASSERT(bin < this->shadow_numBins);
    FW_ASSERT(this->shadow_binFree[bin] != 0);
    this->shadow_binFree[bin]--;
    this->shadow_held.push_back(HeldBuffer{buffer, bin, requested});
    this->shadow_currBuffs++;
    if (this->shadow_currBuffs > this->shadow_highWater) {
        this->shadow_highWater = this->shadow_currBuffs;
    }
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
