#pragma once

// Copyright 2020 AiBlocks Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "overlay/Peer.h"
#include "overlay/AiBlocksXDR.h"

/**
 * OverlayManager maintains a virtual broadcast network, consisting of a set of
 * remote TCP peers (TCPPeer), a mechanism for flooding messages to all peers
 * (FloodGate), and a mechanism for sending and receiving anycast request/reply
 * pairs (ItemFetcher).
 *
 * Overlay network messages are defined as the XDR structure type
 * `AiBlocksMessage`, in the file src/xdr/AiBlocks-overlay.x
 *
 * They are minimally framed using the Record Marking (RM) standard of RFC5531
 * (https://tools.ietf.org/html/rfc5531#page-16) and the RM-framed messages are
 * transmitted over TCP/IP sockets, between peers.
 *
 * The `AiBlocksMessage` union contains 3 logically distinct kinds of message:
 *
 *  - Messages directed to or from a specific peer, with or without a response:
 *    HELLO, GET_PEERS, PEERS, DONT_HAVE, ERROR_MSG
 *
 *  - One-way broadcast messages informing other peers of an event:
 *    TRANSACTION and SCP_MESSAGE
 *
 *  - Two-way anycast messages requesting a value (by hash) or providing it:
 *    GET_TX_SET, TX_SET, GET_SCP_QUORUMSET, SCP_QUORUMSET, GET_SCP_STATE
 *
 * Anycasts are initiated and serviced two instances of ItemFetcher
 * (mTxSetFetcher and mQuorumSetFetcher). Anycast messages are sent to
 * directly-connected peers, in sequence until satisfied. They are not
 * flooded between peers.
 *
 * Broadcasts are initiated by the Herder and sent to both the Herder _and_ the
 * local FloodGate, for propagation to other peers.
 *
 * The OverlayManager tracks its known peers in the Database and shares peer
 * records with other peers when asked.
 */

namespace aiblocks
{

class LoadManager;
class PeerAuth;
class PeerBareAddress;
class PeerManager;
class SurveyManager;

class OverlayManager
{
  public:
    static std::unique_ptr<OverlayManager> create(Application& app);

    // Drop all PeerRecords from the Database
    static void dropAll(Database& db);

    // Flush all FloodGate and ItemFetcher state for ledgers older than
    // `ledger`.
    // This is called by LedgerManager when a ledger closes.
    virtual void ledgerClosed(uint32_t lastClosedledgerSeq) = 0;

    // Send a given message to all peers, via the FloodGate. This is called by
    // Herder.
    virtual void broadcastMessage(AiBlocksMessage const& msg,
                                  bool force = false) = 0;

    // Make a note in the FloodGate that a given peer has provided us with a
    // given broadcast message, so that it is inhibited from being resent to
    // that peer. This does _not_ cause the message to be broadcast anew; to do
    // that, call broadcastMessage, above.
    // Returns true if this is a new message
    // fills msgID with msg's hash
    virtual bool recvFloodedMsgID(AiBlocksMessage const& msg, Peer::pointer peer,
                                  Hash& msgID) = 0;

    bool
    recvFloodedMsg(AiBlocksMessage const& msg, Peer::pointer peer)
    {
        Hash msgID;
        return recvFloodedMsgID(msg, peer, msgID);
    }

    // removes msgID from the floodgate's internal state
    // as it's not tracked anymore, calling "broadcast" with a (now forgotten)
    // message with the ID msgID will cause it to be broadcast to all peers
    virtual void forgetFloodedMsg(Hash const& msgID) = 0;

    // Return a list of random peers from the set of authenticated peers.
    virtual std::vector<Peer::pointer> getRandomAuthenticatedPeers() = 0;

    // Return a list of random peers from the set of inbound authenticated
    // peers.
    virtual std::vector<Peer::pointer> getRandomInboundAuthenticatedPeers() = 0;

    // Return a list of random peers from the set of outbound authenticated
    // peers.
    virtual std::vector<Peer::pointer>
    getRandomOutboundAuthenticatedPeers() = 0;

    // Return an already-connected peer at the given address; returns a
    // `nullptr`-valued pointer if no such connected peer exists.
    virtual Peer::pointer getConnectedPeer(PeerBareAddress const& address) = 0;

    // Add new pending inbound connection.
    virtual void addInboundConnection(Peer::pointer peer) = 0;

    // Add new pending outbound connection. Return true if connection was added.
    virtual bool addOutboundConnection(Peer::pointer peer) = 0;

    // Remove peer from the in-memory set of connected peers. Can only be
    // called on peers in Peer::CLOSING state.
    virtual void removePeer(Peer* peer) = 0;

    // Try to move peer from pending to authenticated list. If there is no room
    // for provided peer, it is checked if it is a "preferred" peer (as
    // specified in the config file's PREFERRED_PEERS/PREFERRED_PEER_KEYS
    // setting) - if so, one random non-preferred peer is removed.
    //
    // If moving peer to authenticated list succeeded, true is returned.
    virtual bool acceptAuthenticatedPeer(Peer::pointer peer) = 0;

    virtual bool isPreferred(Peer* peer) const = 0;

    // Return the current in-memory set of inbound pending peers.
    virtual std::vector<Peer::pointer> const&
    getInboundPendingPeers() const = 0;

    // Return the current in-memory set of outbound pending peers.
    virtual std::vector<Peer::pointer> const&
    getOutboundPendingPeers() const = 0;

    // Return the current in-memory set of pending peers.
    virtual std::vector<Peer::pointer> getPendingPeers() const = 0;

    // Return number of pending peers
    virtual int getPendingPeersCount() const = 0;

    // Return the current in-memory set of inbound authenticated peers.
    virtual std::map<NodeID, Peer::pointer> const&
    getInboundAuthenticatedPeers() const = 0;

    // Return the current in-memory set of outbound authenticated peers.
    virtual std::map<NodeID, Peer::pointer> const&
    getOutboundAuthenticatedPeers() const = 0;

    // Return the current in-memory set of authenticated peers.
    virtual std::map<NodeID, Peer::pointer> getAuthenticatedPeers() const = 0;

    // Return number of authenticated peers
    virtual int getAuthenticatedPeersCount() const = 0;

    // Attempt to connect to a peer identified by peer address.
    virtual void connectTo(PeerBareAddress const& address) = 0;

    // returns the list of peers that sent us the item with hash `h`
    virtual std::set<Peer::pointer> getPeersKnows(Hash const& h) = 0;

    // Return the persistent overlay metrics structure.
    virtual OverlayMetrics& getOverlayMetrics() = 0;

    // Return the persistent p2p authentication-key cache.
    virtual PeerAuth& getPeerAuth() = 0;

    // Return the persistent peer-load-accounting cache.
    virtual LoadManager& getLoadManager() = 0;

    // Return the persistent peer manager
    virtual PeerManager& getPeerManager() = 0;

    virtual SurveyManager& getSurveyManager() = 0;

    // start up all background tasks for overlay
    virtual void start() = 0;
    // drops all connections
    virtual void shutdown() = 0;

    virtual bool isShuttingDown() const = 0;

    virtual void recordMessageMetric(AiBlocksMessage const& aiblocksMsg,
                                     Peer::pointer peer) = 0;

    virtual void updateFloodRecord(AiBlocksMessage const& oldMsg,
                                   AiBlocksMessage const& newMsg) = 0;

    virtual ~OverlayManager()
    {
    }
};
}
