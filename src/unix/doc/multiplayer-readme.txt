	Multiplayer Network XMame N0.5
	(Rewrite by Steve Freeland, caucasatron@yahoo.ca)
	------------------------------
	mame version 0.60.1

Usage (for an n-player game):
-----
Start a master:
    xmame.<display method> -master <n> <other options> <game name>
Start n - 1 slaves:
    xmame.<display method> -slave <master hostname> <other options> <game name>
Currently there can only be one slave per machine, although a slave
can share a machine with the master.
Optional, uh, options:
-netmapkey:  When the slave is invoked with this switch, it will remap
control so that regardless of whatever player number the slave is
assigned, the controls used are those configured for player 1
(which you're likely to have configured to something convenient).
-parallelsync:  This option (given to the master) improves overall
performance at the price of responsiveness:  Your input will be
processed with a delay of roughly 16 milliseconds.  If your reflexes
are fast enough to make that a noticeable problem, may I suggest a
career in professional table tennis?

Message format
--------------
Each UDP packet has a 4-byte header followed by a body.
The header has a 3-byte magic string "XNM" followed by a 1-byte message type.
The contents of the message body depends on the message type, and
there may in fact not be a body.

Message types:  See enum msg_type_t in src/unix/network.c in case
-------------   this doc is out of date
JOIN:  This is the first message the slaves send to the master for
handshaking.  The body contains a string identifying the slave
(<hostname>/<pid>), the slave's MAME build version and net protocol
version, and the name of the game being played.  If any of the latter
3 do not match the master's information, the master will refuse the
join.  The slave id string is currently only used for logging.

ACCEPT:  This is the master's reply to the slave if the protocol
versions and game name match.  The body contains is the player number
assigned to the slave (usually 2 to 4; the master is always player 1),
the id strings and IP addresses for all the other slaves, and any
extra information required about the protocol (currently: whether
parallelsync is enabled)

REFUSE:  This is the master's reply to the slave if the protocol
versions and/or game name do not match.  There is no body.

LOCAL_STATE:  This message is sent out each frame (when osd_net_sync() is
invoked) by each peer to all the other peers.  It contains a
sequence number (see Protocol below) and a byte order-normalized copy
of the slave's current input_port_values[] array.  Each peer uses
incoming messages of this type to compute the input global state.

QUIT:  Sent when a client or the master exits and allows the remaining
participants to handle the exit gracefully without having wait for a timeout.

ACKs and ACKable messages:  Certain message types are designated
"ACKable":  these are intermixed with the regular LOCAL_STATE messages
which make up the bulk of the traffic.  An ACKable is sent out by a
peer at the beginning of a frame *instead* of a LOCAL_STATE message.
The peer then waits for the recipient to send an ACK (acknowledgement)
message in response, at which point the LOCAL_STATE can be sent out.
Currently the following message types are "ACKable":  ACCEPT and QUIT.

Protocol
--------
Each frame, upon invocation of osd_net_sync(), each peer sends its
local input state to each of the other peers, then waits for the
incoming messages containing the local input state for each of the other
peers to arrive.  When all have been received the global input state
is computed, osd_net_sync() exits and emulation proceeds exactly as if
the input had been received through a local input device rather than
the network.
Since the protocol uses UDP messages rather than TCP (in order to
minimize the impact of latency) there is no guarantee that a message
sent will arrive at its destination, or how long it will take even if
it does arrive.  Therefore a peer may need to re-send a local input
state if conditions arise which indicate the previous message did not
reach its destination in time.

                               +--------+
			       | SYNCED |
			       +--------+
                                    |
== Frame n begins ==================+======================================
                                    |
                 +------------------+-----------------+
  Send ACKable n |		                      | Send state n
                 v                                    v
     +----------------------+              +----------------------+
     | ack_expected   = yes | rcv ACK n    | ack_expected   = no  |
     | state_expected = yes |------------->| state_expected = yes |
     +----------------------+ send STATE n +----------------------+
          |  ^   |   ^  |                      |  ^   |   ^  |
      rcv |  |   |   |  | rcv STATE        rcv |  |   |   |  | rcv STATE
  ACKable |  |   |   |  | n - 1        ACKable |  |   |   |  | n - 1
        n |  |   |   |  |                    n |  |   |   |  |
          v  |   |   |  v                      v  |   |   |  v 
 send ACK |  |   |   |  | send STATE  send ACK |  |   |   |  | send STATE
        n |  |   |   |  | n - 1              n |  |   |   |  | n - 1
          +--+   |   +--+                      +--+   |   +--+
                 |                                    |
     rcv STATE n |                                    |
                 v                                    |
     +----------------------+                         |
     | ack_expected   = yes |                         | rcv
     | state_expected = no  |                         | STATE
     +----------------------+           +--+          | n
                 |                      |  |          |
             rcv |         send STATE n |  |          |
             ACK |                      |  |          |
               n |                      ^  |          |
                 v          rcv STATE n |  |          |
            send |                      |  v          |
           STATE |       +---------------------+      |
               n |       | ack_expected = no   |      |
		 +------>| state_expected = no |<-----+
                         +---------------------+
                                    |
                                    | Other peers sync
                                    |
== Frame n ends ====================+======================================
                                    |                  
                                    v
                               +--------+              
	                       | SYNCED |              
			       +--------+

    "Parallel sync" mode works by using the input state from the frame
before the current one.  The local input state collected by the MAME
core is backed up temporarily at the beginning of frame n and
the previously stored state (from frame n - 1) is retrieved and mixed
with the peers' input states for frame n - 1.  That's where the slight
loss of responsiveness comes from.  The advantage of this is that
because the remote input states used for a given frame were send out
on the network during *previous* frame, they have had a full frame
(16 ms for most games) to cross the network.  In non-parallel sync
mode, the time available for network latency is 16 ms minus however
much time it takes to emulate the game for that frame, which on slower
machines could leave almost nothing.  The tradeoff should be
advantageous on all but very fast machines (relative to the game
being emulated) on very fast networks.

TODO
----
. Adjust for high latency -- coarser sync frequency?	
. Handle factors which may cause a variance in the initial state in the
  different emulated machines -- NVRAM and high scores.  Anything else?
. Sync analog controls
. Handle UI-originated events, eg pause, reset
. More flexible network port config?
. Talk to the core team about integrating the protocol-level stuff
  into the platform-independant network.c

This version of netmame is based on Eric Totel's old version (also
specific to xmame), which is no longer maintained, and most especially
on a lot of network code from the core which also seems to have been
abandoned, and for which I can't seem to find the appropriate
attribution.

