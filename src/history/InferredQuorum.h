#pragma once

// Copyright 2020 AiBlocks Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/SecretKey.h"
#include "herder/QuorumTracker.h"
#include "main/Config.h"
#include "overlay/AiBlocksXDR.h"
#include "util/HashOfHash.h"
#include <string>
#include <unordered_map>

namespace aiblocks
{

struct InferredQuorum
{
    InferredQuorum();
    InferredQuorum(QuorumTracker::QuorumMap const& qmap);
    std::unordered_map<Hash, SCPQuorumSet> mQsets;
    std::unordered_map<PublicKey, std::vector<Hash>> mQsetHashes;
    std::unordered_map<PublicKey, size_t> mPubKeys;
    void noteSCPHistory(SCPHistoryEntry const& hist);
    void noteQset(SCPQuorumSet const& qset);
    void noteQsetHash(PublicKey const& pk, Hash const& hash);
    void notePubKey(PublicKey const& pk);
    std::string toString(Config const& cfg, bool fullKeys) const;
    void writeQuorumGraph(Config const& cfg, std::ostream& out) const;
    QuorumTracker::QuorumMap getQuorumMap() const;
};
}
