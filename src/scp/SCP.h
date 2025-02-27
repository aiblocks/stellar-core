#pragma once

// Copyright 2020 AiBlocks Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>

#include "crypto/SecretKey.h"
#include "lib/json/json-forwards.h"
#include "scp/SCPDriver.h"

namespace aiblocks
{
class Node;
class Slot;
class LocalNode;
typedef std::shared_ptr<SCPQuorumSet> SCPQuorumSetPtr;

class SCP
{
    SCPDriver& mDriver;

  public:
    SCP(SCPDriver& driver, NodeID const& nodeID, bool isValidator,
        SCPQuorumSet const& qSetLocal);

    SCPDriver&
    getDriver()
    {
        return mDriver;
    }
    SCPDriver const&
    getDriver() const
    {
        return mDriver;
    }

    enum EnvelopeState
    {
        INVALID, // the envelope is considered invalid
        VALID    // the envelope is valid
    };

    // this is the main entry point of the SCP library
    // it processes the envelope, updates the internal state and
    // invokes the appropriate methods
    EnvelopeState receiveEnvelope(SCPEnvelopeWrapperPtr envelope);

    // Submit a value to consider for slotIndex
    // previousValue is the value from slotIndex-1
    bool nominate(uint64 slotIndex, ValueWrapperPtr value,
                  Value const& previousValue);

    // stops nomination for a slot
    void stopNomination(uint64 slotIndex);

    // Local QuorumSet interface (can be dynamically updated)
    void updateLocalQuorumSet(SCPQuorumSet const& qSet);
    SCPQuorumSet const& getLocalQuorumSet();

    // Local nodeID getter
    NodeID const& getLocalNodeID();

    // returns the local node descriptor
    std::shared_ptr<LocalNode> getLocalNode();

    Json::Value getJsonInfo(size_t limit, bool fullKeys = false);

    // summary: only return object counts
    // index = 0 for returning information for all slots
    Json::Value getJsonQuorumInfo(NodeID const& id, bool summary,
                                  bool fullKeys = false, uint64 index = 0);

    // Purges all data relative to all the slots whose slotIndex is smaller
    // than the specified `maxSlotIndex`.
    void purgeSlots(uint64 maxSlotIndex);

    // Returns whether the local node is a validator.
    bool isValidator();

    // returns the validation state of the given slot
    bool isSlotFullyValidated(uint64 slotIndex);

    // Helpers for monitoring and reporting the internal memory-usage of the SCP
    // protocol to system metric reporters.
    size_t getKnownSlotsCount() const;
    size_t getCumulativeStatemtCount() const;

    // returns the latest messages sent for the given slot
    std::vector<SCPEnvelope> getLatestMessagesSend(uint64 slotIndex);

    // forces the state to match the one in the envelope
    // this is used when rebuilding the state after a crash for example
    void setStateFromEnvelope(uint64 slotIndex, SCPEnvelopeWrapperPtr e);

    // check if we are holding some slots
    bool empty() const;

    // invokes f for all latest messages
    // if forceSelf, return messages for self even if not fully validated
    // f returns false to stop processing, true otherwise
    void processCurrentState(uint64 slotIndex,
                             std::function<bool(SCPEnvelope const&)> const& f,
                             bool forceSelf);

    // iterates through slots, starting from ledgerSeq
    void processSlotsAscendingFrom(uint64 startIndex,
                                   std::function<bool(uint64)> const& f);

    // returns the latest message from a node
    // or nullptr if not found
    SCPEnvelope const* getLatestMessage(NodeID const& id);

    // returns messages that contributed to externalizing the slot
    // (or empty if the slot didn't externalize)
    std::vector<SCPEnvelope> getExternalizingState(uint64 slotIndex);

    // ** helper methods to stringify ballot for logging
    std::string getValueString(Value const& v) const;
    std::string ballotToStr(SCPBallot const& ballot) const;
    std::string ballotToStr(std::unique_ptr<SCPBallot> const& ballot) const;
    std::string envToStr(SCPEnvelope const& envelope,
                         bool fullKeys = false) const;
    std::string envToStr(SCPStatement const& st, bool fullKeys = false) const;

  protected:
    std::shared_ptr<LocalNode> mLocalNode;
    std::map<uint64, std::shared_ptr<Slot>> mKnownSlots;

    // Slot getter
    std::shared_ptr<Slot> getSlot(uint64 slotIndex, bool create);

    friend class TestSCP;
};
}
