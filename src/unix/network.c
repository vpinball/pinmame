#include "xmame.h"

#ifdef XMAME_NET

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "driver.h"

#define PROTOCOL_DEBUG 2

#if PROTOCOL_DEBUG <= 1
#define NDEBUG
#endif
#include <assert.h>

#ifdef __GNUC__
#if PROTOCOL_DEBUG >= 3
#define _CHATTY_LOG(format, args...)       \
    fprintf(stderr_file,                   \
            "%s:%d: " format "\n",         \
             __FUNCTION__,                 \
             __LINE__ ,                    \
             ## args)
#else
#define _CHATTY_LOG(format, args...)       \
    fprintf(stderr_file, format "\n" , ## args)
#endif
#define _CHATTY_ERRNO_LOG(format, args...) \
    _CHATTY_LOG(format " (errno %d: %s)" , ## args, errno, strerror(errno))
#else
/* Fix this at some point */
#error "GCC required to compile NetMAME"
#endif

#define _PTR_SWAP(a, b) do { void *_scratch_ptr = a; a = b; b = _scratch_ptr; } while (0)
#define _MAX(a, b) (a >= b ? a : b)

enum net_role {
    NONE,
    MASTER,
    SLAVE
    /* SPECTATOR */ /* Future? */
};

static enum net_role _current_net_role = NONE;

#define DEFAULT_BIND_PORT "9000"

#define NET_MAX_PLAYERS 4

#define REGISTRATION_TIMEOUT (30 * 1000) /* ms */
#define REGISTRATION_RETRY_TIMEOUT (1 * 1000)
#define SYNC_TIMEOUT (15 * 1000)
#define SYNC_RETRY_TIMEOUT 50

#define MSG_HEADER_LEN 12
#define MAX_MSG_LEN 116 /* Includes header */
#define MAX_BLOCK_PART_SIZE (MAX_MSG_LEN - MSG_HEADER_LEN - sizeof(UINT32))

enum net_msg_type {
    JOIN = 1,
    REFUSE,
    START,
    BLOCK_INIT,
    BLOCK_PART,
    BLOCK_PROMPT,
    SET_SYNC_SKIP,
    ACK,
    INPUT_STATE,
    QUIT
};

enum net_block_type {
    NVRAM_BLOCK = 1,
    MACHINESTATE_BLOCK
};

/* Bitmap offsets for the master's START Message */
#define CONFIG_BITMAP_PARALLELSYNC 0
#define CONFIG_BITMAP_MACHINE_STATE_DEBUG 1

#define MSG_MAGIC "XNM"
#define MSG_MAGIC_LEN 3
#define PROTOCOL_VERSION "N0.6"
static char _truncated_build_version[10];

struct net_msg_info {
    enum net_msg_type msg_type;
    UINT32 counter;
    UINT32 sequence;
    char msg[MAX_MSG_LEN];
    char *data_start;
    int data_len;
};
static struct net_msg_info _inbound_scratch_msg_info;
static struct net_msg_info _outbound_scratch_msg_info;
/* The _previous/_current outbound message pointers are for the regular
   working of the protocol, after the handshaking is over.
   Used by both master and slaves.  In the case of slaves these are
   always INPUT_STATE message; in the master ACKables may be intermixed
   as well. */
static struct net_msg_info _saved_msg_info[_MAX(NET_MAX_PLAYERS - 1, 2)];
static struct net_msg_info *_previous_outbound_msg_info;
static struct net_msg_info *_current_outbound_msg_info;

static unsigned short _saved_input_state[MAX_INPUT_PORTS];
static unsigned short _inbound_input_state[MAX_INPUT_PORTS];
static unsigned short _outbound_input_state[MAX_INPUT_PORTS];
static unsigned short _new_input_state[MAX_INPUT_PORTS];


struct net_block_info {
    enum net_block_type type;
    unsigned size;
    char *buffer;
    char *bitmap;
    unsigned total_part_count;
    unsigned remaining_part_count;
};
static struct net_block_info _block_info;
#define MAX_REQUESTED_PARTS ((MAX_MSG_LEN - MSG_HEADER_LEN) / 4)

static char *_nvram_backup = NULL;
static unsigned _nvram_backup_len;

static int _input_remap = 0;
static int _parallel_sync = 0;
static int _machine_state_debug = 0;

#if PROTOCOL_DEBUG >= 1
unsigned _reminder_count = 0;
unsigned _inbound_duplicate_count = 0;
#endif

enum net_peer_state {
    INACTIVE = 1,
    UNSYNCED,
    SYNCED
};

enum net_protocol_state {
    REGISTRATION = 1,
    INPUT_EXCHANGE,
    ACKNOWLEDGEMENT,
    BLOCK_TRANSFER,
    QUITTING
};
static enum net_protocol_state _protocol_state;

struct net_peer_info {
    char name[MAX_MSG_LEN];
    int player_number;
    struct sockaddr_in addr;
    enum net_peer_state state;
    struct _net_send {
	enum net_msg_type msg_type;
	unsigned time_since;
    } current_send, previous_send;
    UINT32 last_outbound_sequence;
    UINT32 outbound_counter;
    UINT32 last_inbound_counter;
    struct net_msg_info early_inbound_msg_info;
};
static struct net_peer_info _peer_info[NET_MAX_PLAYERS];

static unsigned _original_player_count = 0, _current_player_count = 0;
static unsigned char _player_number;
static unsigned _unsynced_peer_count;
static int _non_state_msg_since_sync;

static int _bind_port;

/* For slave only */
static unsigned char _master_index;
#define _master_info (_peer_info[_master_index])

struct net_input_bit_id {
    unsigned port_index;
    unsigned short mask;
};
struct net_input_mapping {
    struct net_input_bit_id source;
    struct net_input_bit_id dest;
};
static struct net_input_mapping _input_map[MAX_INPUT_PORTS * sizeof(unsigned short)];
static unsigned _input_mapping_count;

/* FIXME:  Possible buffer overrun on read */
/* stderr_file is uninitialized at the time this is called */
static int
_parse_hostport(const char *hostport, struct sockaddr_in *addr)
{
    int host_ok = FALSE, port_ok = FALSE;
    struct hostent *hostent;
    char *host_str = strdup(hostport);
    char *port_str = strstr(host_str, ":");

    addr->sin_family = AF_INET;

    if (port_str != NULL) {
	*port_str = 0;
	port_str += 1;
	if (port_str[0] != 0) {
	    int scratch = atoi(port_str);
	    if (scratch < 1 || scratch > 65535) {
		fprintf(stderr,
			"Bad host:port \"%s\":"
			" port values below 1 or above 65535 are invalid\n",
			hostport);
	    } else {
		_master_info.addr.sin_port =
		    htons((unsigned short)atoi(port_str));
		port_ok = TRUE;
	    }
	}
    } else {
	_master_info.addr.sin_port =
	    htons((unsigned short)atoi(DEFAULT_BIND_PORT));
	port_ok = TRUE;
    }

    /* Resolve master's IP address */
    /* TODO:  Automatic handling of IPv6 address using hostent->h_addrtype ? */
    hostent = gethostbyname(host_str);
    if (hostent == NULL) {
	fprintf(stderr,
		"Can't resolve host \"%s\"\n",
		host_str);
    } else {
	memcpy(&(addr->sin_addr.s_addr),
	       hostent->h_addr,
	       hostent->h_length);
	host_ok = TRUE;
    }
    free(host_str);
    return (port_ok && host_ok);
}

static int
_parse_master_addr(struct rc_option *option, const char *arg, int priority)
{
    _current_net_role = SLAVE;
    _master_index = 0;
    _master_info.player_number = 1;
    sprintf(_master_info.name, "Master: %s", arg);
	
    return (_parse_hostport(arg, &(_master_info.addr))) ? 0 : -1;
}

#endif
struct rc_option network_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#ifdef XMAME_NET
   { "Network Related",       NULL,		rc_seperator,	 NULL,
     NULL,		      0,		0,		 NULL,
     NULL },
   { "master",		      NULL,		rc_int,		 &_original_player_count,
     NULL,		      2,		NET_MAX_PLAYERS, NULL,
     "Enable master mode. Set number of players" },
   { "slave",		      NULL,		rc_use_function, NULL,
     NULL,		      0,		0,		 _parse_master_addr,
     "Enable slave mode. Set master hostname[:port]" },
   { "bind",                  NULL,	        rc_int,	         &_bind_port,
     DEFAULT_BIND_PORT,       1,		65535,		 NULL,
     "Specify a UDP port on which to accept messages" },
   { "netmapkey",	      NULL,		rc_bool,	 &_input_remap,
     "1",		      0,		0,		 NULL,
     "Makes player 1 keys control whichever player number you're actually assigned by the master (for slave only)" },
   { "parallelsync",          NULL,		rc_bool,	 &_parallel_sync,
     "1",		      0,		0,		 NULL,
     "Perform network input sync in advance:  Causes ~ 16 ms input delay but more suitable for relatively slow machines/networks (for master only)" },
   { "statedebug",            NULL,             rc_bool,         &_machine_state_debug,
     "0",		      0,		0,		 NULL,
     "Check complete machine state against slaves at each frame -- extremely slow, for debugging (for master only)" },
#endif
   { NULL,		      NULL,		rc_end,		 NULL,
     NULL,		      0,		0,		 NULL,
     NULL }
};

#ifdef XMAME_NET

static int _socket_fd;

unsigned _frame_count = 0;
unsigned _current_sync_sequence = 0;
unsigned _min_state_sequence = 0, _prev_min_state_sequence = 0;

/* Variables used by the master to keep track of sync time and
   sync skipping -- see doc/network.txt */
#define SYNC_SKIP_TOLERANCE 3 /* ms */
#define SYNC_TIME_SMA_IGNORE_COUNT 5
#define SYNC_TIME_SMA_POINTS 60 /* SMA == simple moving average */
#define MAX_SYNC_SKIP 8
static unsigned _minimum_frame_for_valid_sma =
    SYNC_TIME_SMA_IGNORE_COUNT + SYNC_TIME_SMA_POINTS;
static unsigned _sync_skip = 0;
static unsigned _new_sync_skip = 0;
static struct timeval _sync_start_time;
static unsigned _sync_time[SYNC_TIME_SMA_POINTS];
static unsigned _sync_time_cursor = 0;
static double _sync_time_sma = 0;

static void
_net_shutdown(void);

static int
_tv_subtract(struct timeval *tv1, struct timeval *tv2)
{
    int result;
    result = (tv1->tv_sec - tv2->tv_sec) * 1000000;
    result += (tv1->tv_usec - tv2->tv_usec);
    return result;
}

#if PROTOCOL_DEBUG >= 1
struct timeval _start_time;

unsigned _min_sync_time = 0;
unsigned _max_sync_time = 0;
unsigned _total_sync_time = 0;
#endif

static int
_init_sockets(void)
{
    int result = FALSE;
    int scratch;

    struct sockaddr_in input_addr;
    
    if (_socket_fd = socket(AF_INET, SOCK_DGRAM, 0),
	_socket_fd < 0)
    {
	_CHATTY_ERRNO_LOG("Can't create socket");
    }
    else if (scratch = fcntl(_socket_fd, F_SETFL, O_NONBLOCK),
	     scratch == -1)
    {
	_CHATTY_ERRNO_LOG("Can't set socket non-blocking flag");
    } else {
	/* Assign domain and port number */
	/* TODO:  Make bind port configurable */
	input_addr.sin_family = AF_INET;
	input_addr.sin_addr.s_addr = INADDR_ANY;
	input_addr.sin_port = htons(_bind_port);

	/* bind socket */
	if (bind(_socket_fd,
		 (struct sockaddr *)&input_addr,
		 sizeof(input_addr)) == -1)
	{
	    _CHATTY_ERRNO_LOG("Can't bind socket");
	} else {
	    result = TRUE;
	}
    }
    return result;
}

static void
_prime_msg(struct net_msg_info *msg_info,
	   enum net_msg_type msg_type,
	   UINT32 sequence)
{
    memcpy(msg_info->msg, MSG_MAGIC, MSG_MAGIC_LEN);
    msg_info->msg_type = msg_type;
    msg_info->msg[3] = (unsigned char)msg_type;
    msg_info->sequence = sequence;
    (*(UINT32 *)(msg_info->msg + 8)) = ntohl(sequence);
    msg_info->data_start = msg_info->msg + MSG_HEADER_LEN;
    msg_info->data_len = 0;
}

static int
_write_to_msg(struct net_msg_info *msg_info,
	      const void *new_data,
	      unsigned new_data_len)
{
    int result = FALSE;
    if (msg_info->data_len + new_data_len + MSG_HEADER_LEN > MAX_MSG_LEN) {
	_CHATTY_LOG("Error: message overflow");
    } else {
	memcpy(msg_info->data_start + msg_info->data_len,
	       new_data,
	       new_data_len);
	msg_info->data_len += new_data_len;
	result = TRUE;
    }
    return result;
}

static void
_copy_msg(struct net_msg_info *src_msg,
	  struct net_msg_info *dest_msg)
{
    memcpy(dest_msg, src_msg, sizeof(struct net_msg_info));
    if (src_msg->data_start != NULL) {
	dest_msg->data_start =
	    dest_msg->msg + (src_msg->data_start - src_msg->msg);
    }
}

static void
_build_block_prompt_msg(struct net_msg_info *msg_info)
{
    unsigned bitmap_byte = 0, bitmap_bit = 0;
    UINT32 block_part = 0;
    unsigned total_parts_requested =
	(_block_info.remaining_part_count <= MAX_REQUESTED_PARTS) ?
	_block_info.remaining_part_count : MAX_REQUESTED_PARTS;
    unsigned parts_requested = 0;

    _prime_msg(msg_info, BLOCK_PROMPT, _current_sync_sequence);

    while (block_part < _block_info.total_part_count &&
	   parts_requested < total_parts_requested)
    {
	if ((_block_info.bitmap[bitmap_byte] & (1 << bitmap_bit)) == 0) {
	    UINT32 scratch = (UINT32)ntohl(block_part);
	    _write_to_msg(msg_info, &scratch, sizeof(UINT32));
	    parts_requested += 1;
	}
	block_part += 1;
	bitmap_bit += 1;
	if (bitmap_bit == 8) {
	    bitmap_bit = 0;
	    bitmap_byte += 1;
	}
    }
}

static int
_send_msg(struct net_msg_info *msg_info,
	  struct sockaddr_in *dest)
{
    int result = FALSE;
    if (sendto(_socket_fd,
	       msg_info->msg,
	       (msg_info->data_start - msg_info->msg) + msg_info->data_len,
	       0,
	       (struct sockaddr *)dest,
	       sizeof(*dest)) < 0)
    {
	_CHATTY_ERRNO_LOG("Can't send message");
    } else {
	result = TRUE;
    }
    return result;
}

static struct net_peer_info *
_peer_for_addr(struct sockaddr_in *addr)
{
    unsigned char i;
    struct net_peer_info *result = NULL;
    for (i = 0; i < _original_player_count - 1; i++) {
	if (memcmp(addr, &(_peer_info[i].addr), sizeof(*addr)) == 0) {
	    result = &(_peer_info[i]);
	    break;
	}
    }
    return result;
}

static struct net_peer_info *
_new_peer_with_addr(struct sockaddr_in *addr)
{
    unsigned char i = 0;
    struct net_peer_info *result = NULL;
    while (i < _original_player_count - 1 &&
	   _peer_info[i].state != INACTIVE)
    {
	i += 1;
    }
    if (i < _original_player_count - 1) {
	memcpy(&(_peer_info[i].addr),
	       addr,
	       sizeof(_peer_info[i].addr));
	result = &(_peer_info[i]);
    }
    return result;
}

static int
_send_msg_to_peer(struct net_msg_info *msg_info,
		  struct net_peer_info *dest)
{
    int result = FALSE;

    msg_info->counter = dest->outbound_counter;
    (*(UINT32 *)(msg_info->msg + 4)) = ntohl(dest->outbound_counter);

    if (_send_msg(msg_info, &(dest->addr))) {
	dest->outbound_counter += 1;
	if (msg_info->sequence == _current_sync_sequence) {
	    if (dest->last_outbound_sequence < msg_info->sequence) {
		dest->last_outbound_sequence = msg_info->sequence;
		dest->previous_send = dest->current_send;
	    }
	    dest->current_send.msg_type = msg_info->msg_type;
	    dest->current_send.time_since = 0;
	} else {
	    dest->previous_send.msg_type = msg_info->msg_type;
	    dest->previous_send.time_since = 0;
	}
	result = TRUE;
    }
    return result;
}

static int
_ok_to_resend(struct net_msg_info *msg_info,
	      struct net_peer_info *dest,
	      unsigned minimum_time_lapse)
{
    int result = FALSE;
    enum net_msg_type last_type;
    unsigned time_lapse;
    if (msg_info->sequence == _current_sync_sequence) {
	last_type = dest->current_send.msg_type;
	time_lapse = dest->current_send.time_since;
    } else {
	last_type = dest->previous_send.msg_type;
	time_lapse = dest->previous_send.time_since;
    }
    if (msg_info->msg_type != last_type || time_lapse >= minimum_time_lapse) {
	result = TRUE;
    }
    return result;
}

static unsigned
_block_part_size(unsigned block_part) {
    unsigned result;
    if (block_part + 1 == _block_info.total_part_count) {
	result = _block_info.size % MAX_BLOCK_PART_SIZE;
    } else {
	result = MAX_BLOCK_PART_SIZE;
    }
    return result;
}

static int
_send_block_part_to_peer(unsigned block_part,
			 struct net_peer_info *dest)
{
    unsigned part_size = _block_part_size(block_part);
    UINT32 scratch = (UINT32)ntohl(block_part);
    _prime_msg(&_outbound_scratch_msg_info,
	       BLOCK_PART,
	       _current_sync_sequence);
    _write_to_msg(&_outbound_scratch_msg_info, &scratch, sizeof(scratch));
    _write_to_msg(&_outbound_scratch_msg_info,
		  _block_info.buffer + block_part * MAX_BLOCK_PART_SIZE,
		  part_size);
#if PROTOCOL_DEBUG >= 3
    _CHATTY_LOG("Sending block part %d, seq %d to slave \"%s\"",
		block_part,
		_outbound_scratch_msg_info.sequence,
		dest->name);
#endif
    return _send_msg_to_peer(&_outbound_scratch_msg_info, dest);
}

static int
_rcv_msg(struct net_msg_info *msg_info, 
	 struct sockaddr_in *source_addr,
	 int timeout,
	 unsigned *time_used_p)
{
    int result = FALSE;
    int sel_result;
    unsigned time_used;
    unsigned socklen = 0;
    fd_set scratch_fds;
    struct timeval rcv_start_tv, rcv_end_tv, timeout_tv;
    struct timeval *timeout_tv_p = NULL;
    int msg_len;
    unsigned i;

    if (timeout != -1) {
	timeout_tv.tv_sec = timeout / 1000;
	timeout_tv.tv_usec = (timeout % 1000) * 1000;
	timeout_tv_p = &timeout_tv;
    }

    if (source_addr != NULL) {
	socklen = sizeof(*source_addr);
    }

    gettimeofday(&rcv_start_tv, NULL);
    do {
	FD_ZERO(&scratch_fds);
	FD_SET(_socket_fd, &scratch_fds);
	    
	sel_result = select(_socket_fd + 1,
			    &scratch_fds,
			    NULL,
			    NULL,
			    timeout_tv_p);
	if (sel_result < 0) {
	    _CHATTY_ERRNO_LOG("select() failure");
	} else if (sel_result == 0) {
	    timeout = 0;
	}
	else if (msg_len = recvfrom(_socket_fd,
				    msg_info->msg,
				    sizeof(msg_info->msg),
				    0,
				    (struct sockaddr *)source_addr,
				    &socklen),
		 msg_len < 0)
	{
	    _CHATTY_ERRNO_LOG("recvfrom() failure");
	}
	/* Packets without the magic header are discarded without error;
	   they are assumed to be unrelated traffic */
	else if (msg_len >= MSG_HEADER_LEN &&
		 memcmp(msg_info->msg, MSG_MAGIC, MSG_MAGIC_LEN) == 0)
	{
	    msg_info->msg_type = msg_info->msg[3];
	    msg_info->counter =
		(UINT32)ntohl(*((UINT32 *)(msg_info->msg + 4)));
	    msg_info->sequence =
		(UINT32)ntohl(*((UINT32 *)(msg_info->msg + 8)));
	    msg_info->data_start = msg_info->msg + MSG_HEADER_LEN;
	    msg_info->data_len = msg_len - MSG_HEADER_LEN;
	    result = TRUE;
	}
    } while (! result && timeout != 0);

    gettimeofday(&rcv_end_tv, NULL);
    time_used = _tv_subtract(&rcv_end_tv, &rcv_start_tv) / 1000;
    for (i = 0; i < _original_player_count - 1; i++) {
	if (_peer_info[i].state != INACTIVE) {
	    _peer_info[i].current_send.time_since += time_used;
	    _peer_info[i].previous_send.time_since += time_used;
	}
    }
    if (time_used_p != NULL) {
	*time_used_p = time_used;
    }

    return result;
}

/* Only slaves send reminders due to timeouts, never the master.
   This cuts down on unnecessary network traffic and simplifies
   the protocol as well.  The master can, however, send reminders
   if a slave gets ahead of it due to message lossage. */
static void
_send_reminder_msg(struct net_peer_info *dest,
		   unsigned retry_timeout)
{
    struct net_msg_info *reminder_msg = NULL;

    if (_current_net_role == SLAVE && dest == &(_master_info)) {
	switch (_protocol_state) {
	case REGISTRATION:
	case INPUT_EXCHANGE:
	    reminder_msg = _current_outbound_msg_info;
	    break;
	case ACKNOWLEDGEMENT:
	    _prime_msg(&_outbound_scratch_msg_info,
		       ACK,
		       _current_sync_sequence - 1);
	    reminder_msg = &_outbound_scratch_msg_info;
	    break;
	case BLOCK_TRANSFER:
	    _build_block_prompt_msg(&_outbound_scratch_msg_info);
	    reminder_msg = &_outbound_scratch_msg_info;
	default:
	    _CHATTY_LOG("Trying to send reminder message"
			" from invalid state %d",
			_protocol_state);
	    break;
	}
    } else {
	reminder_msg = _current_outbound_msg_info;
    }
    
    if (reminder_msg != NULL &&
	_ok_to_resend(reminder_msg, dest, retry_timeout))
    {
#if PROTOCOL_DEBUG >= 1
#if PROTOCOL_DEBUG >= 2
	_CHATTY_LOG("Sending reminder msg"
		    " type %d, sequence %d/counter %d to peer \"%s\"",
		    reminder_msg->msg_type,
		    reminder_msg->sequence,
		    dest->outbound_counter,
		    dest->name);
#endif
	_reminder_count += 1;
#endif
	_send_msg_to_peer(reminder_msg, dest);
    }
}

static struct net_peer_info *
_rcv_msg_from_peer(struct net_msg_info *msg,
		   unsigned timeout,
		   unsigned *time_used,
		   int allow_join)
{
    struct net_peer_info *source_peer = NULL;
    struct sockaddr_in source_addr;

    if (_rcv_msg(msg, &source_addr, timeout, time_used)) {
	source_peer = _peer_for_addr(&source_addr);
	if (source_peer == NULL) {
	    if (msg->msg_type == JOIN && allow_join) {
		source_peer = _new_peer_with_addr(&source_addr);
	    } else {
		_CHATTY_LOG("Error: Received message from unknown source");
	    }
	}
    }
    return source_peer;
}

static struct net_peer_info *
_pushy_rcv_msg_from_peer(struct net_msg_info *msg,
			 int retry_timeout,
			 int giveup_timeout,
			 unsigned *time_used,
			 int allow_join)
{
    struct net_peer_info *source_peer = NULL;
    
    *time_used = 0;
    do {
	unsigned rcv_timeout;
	unsigned try_time_used;
	unsigned char i;

	if (retry_timeout < giveup_timeout && giveup_timeout != -1) {
	    rcv_timeout = retry_timeout;
	} else {
	    rcv_timeout = giveup_timeout;   /* n.b. May be zero (no timeout) */
	}
	source_peer = _rcv_msg_from_peer(msg,
					 rcv_timeout,
					 &try_time_used,
					 allow_join);
	*time_used += try_time_used;
	if (giveup_timeout != -1) {
	    if (try_time_used < giveup_timeout) {
		giveup_timeout -= try_time_used;
	    } else {
		giveup_timeout = 0;
	    }
	}
	if (source_peer == NULL && giveup_timeout != 0) {
	    /* Resend messages to whichever peers are dawdling */
	    for (i = 0; i < _original_player_count - 1; i++) {
		if (_peer_info[i].state == UNSYNCED) {
		    _send_reminder_msg(&(_peer_info[i]),
				       retry_timeout);
		}
	    }
	}
    } while (source_peer == NULL && giveup_timeout != 0);

    return source_peer;
}

static void
_record_sync_start()
{
    gettimeofday(&_sync_start_time, NULL);
}

static void
_record_sync_end()
{
    struct timeval sync_end_time;
    unsigned sync_time;
    gettimeofday(&sync_end_time, NULL);
    sync_time = _tv_subtract(&sync_end_time, &_sync_start_time) / 1000;
    
    if (! _non_state_msg_since_sync) {
	int is_spike = FALSE;
	/* Smooth out the occasional unrepresentative super-long sync -- 
	   observed in testing, but I'm unsure as to the cause.  Other
	   processes suddenly hogging the CPU?  Dunno.  Anwyays, yay Stats
	   101, but I'm not going to bother calculating the bloody standard
	   deviation */
	if (_frame_count > SYNC_TIME_SMA_IGNORE_COUNT + SYNC_TIME_SMA_POINTS &&
	    sync_time > 4 * _sync_time_sma)
	{
	    is_spike = TRUE;
	}
	
	if (_current_net_role == MASTER &&
	    _frame_count > SYNC_TIME_SMA_IGNORE_COUNT &&
	    ! is_spike)
	{
	    _sync_time_sma -=
		(double)_sync_time[_sync_time_cursor] / (double)SYNC_TIME_SMA_POINTS;
	    _sync_time[_sync_time_cursor] = sync_time;
	    _sync_time_sma +=
		(double)_sync_time[_sync_time_cursor] / (double)SYNC_TIME_SMA_POINTS;
#if PROTOCOL_DEBUG >= 3
	    _CHATTY_LOG("Sync time: %d ms; _sync_time_sma now == %f ms",
			sync_time,
			_sync_time_sma);
#endif
	    _sync_time_cursor += 1;
	    if (_sync_time_cursor == SYNC_TIME_SMA_POINTS) {
		_sync_time_cursor = 0;
	    }
	}
    }
    _non_state_msg_since_sync = FALSE;

#if PROTOCOL_DEBUG >= 1
    if (_min_sync_time == 0 || (sync_time < _min_sync_time)) {
	_min_sync_time = sync_time;
    }
    if (_max_sync_time == 0 || (sync_time > _max_sync_time)) {
	_max_sync_time = sync_time;
    }
    _total_sync_time += sync_time;
#endif
}

static void
_handle_duplicate_msg(struct net_msg_info *duplicate_msg_info,
		      struct net_peer_info *source_peer,
		      struct net_msg_info *counterpart,
		      unsigned retry_timeout)
				      
{
#if PROTOCOL_DEBUG >= 1
#if PROTOCOL_DEBUG >= 2
    _CHATTY_LOG("Received duplicate message"
		" type %d sequence %d/counter %d from peer \"%s\"",
		duplicate_msg_info->msg_type,
		duplicate_msg_info->sequence,
		duplicate_msg_info->counter,
		source_peer->name);
#endif
    _inbound_duplicate_count += 1;
#endif
    /* At the time the peer sent this duplicate message, it had not yet
       received the requisite message from the local instance to sync.
       That can only be the current sync or the previous one, because
       a peer can only get 1 sync ahead of any other (see protocol
       documentation in doc/network-readme.txt).
       It may have received a copy of the requisite message between the
       duplicate message being sent and received, but there is no way
       to determine that, so re-send (but limit resend frequency to
       avoid flooding the network due to a sudden influx of duplicates) */
    if (duplicate_msg_info->counter > source_peer->last_inbound_counter &&
	_ok_to_resend(counterpart, source_peer, retry_timeout))
    {
#if PROTOCOL_DEBUG >= 2
	_CHATTY_LOG("... re-sending message type %d sequence %d",
		    counterpart->msg_type,
		    counterpart->sequence);
#endif
	_send_msg_to_peer(counterpart, source_peer);
    }
}

static void
_handle_early_msg(struct net_msg_info *early_msg_info,
		  struct net_peer_info *source_peer,
		  unsigned retry_timeout)
{
    /* A peer is already at the next frame and sending its
       state.  If that peer is synced for the current frame, the
       local instance is presumably stuck waiting for some other
       peer to sync.  If the early peer is not yet synced, the local
       instance missed its message for the current frame and it
       must be prompted to send it again.
       In both cases, copy the early message into a temporary space
       for later processing (unless this has already been done) */
    if (source_peer->state == UNSYNCED) {
	_send_reminder_msg(source_peer, retry_timeout);
    }
    if (early_msg_info->sequence > source_peer->early_inbound_msg_info.sequence)
    {
	_copy_msg(early_msg_info,
		  &(source_peer->early_inbound_msg_info));
    }
}

static void
_process_new_state_msg(struct net_msg_info *msg_to_process,
		       struct net_peer_info *source_peer,
		       unsigned short *input_port_deviations)
{
    unsigned i;
    for (i = 0; i < MAX_INPUT_PORTS; i++) {
	/* mask in the default deviations from each peer */
	input_port_deviations[i] |=
	    ntohs(((unsigned short *)msg_to_process->data_start)[i]);
    }
    assert(_protocol_state == INPUT_EXCHANGE);
    source_peer->state = SYNCED;
    _unsynced_peer_count -= 1;
    if (_unsynced_peer_count == 0 &&
	(_current_net_role == MASTER || PROTOCOL_DEBUG >= 1))
    {
	_record_sync_end();
    }
#if PROTOCOL_DEBUG >= 3
    _CHATTY_LOG("State of peer \"%s\" is now %d",
		source_peer->name,
		source_peer->state);
#endif
}

static int
_read_nvram(char **buffer, unsigned *len)
{
    int result = FALSE;
    void *file;

    file = mame_fopen(Machine->gamedrv->name,
		     NULL,
		     FILETYPE_NVRAM,
		     FALSE);
    if (file != NULL) {
	*len = mame_fsize(file);
	*buffer = (char *)malloc(*len);
	mame_fread(file, *buffer, *len);
	mame_fclose(file);
	result = TRUE;
    }
    return result;
}

static int
_write_nvram(char *buffer, unsigned len)
{
    int result = FALSE;
    void *file = mame_fopen(Machine->gamedrv->name,
			   NULL,
			   FILETYPE_NVRAM,
			   TRUE);
    if (file != NULL) {
	mame_fwrite(file, buffer, len);
	mame_fclose(file);
	result = TRUE;
    }
    return result;
}

static void
_finalize_nvram_transfer()
{
    /* This depends on osd_net_init() being invoked *before*
       the game driver's NVRAM handler */

    _read_nvram(&_nvram_backup, &_nvram_backup_len);
    if (! _write_nvram(_block_info.buffer, _block_info.size)) {
	if (_nvram_backup != NULL) {
	    free(_nvram_backup);
	    _nvram_backup = NULL;
	}
	_CHATTY_LOG("Unable to overwrite NVRAM with network master's copy;"
		    " this may lead to desync of the game");
    } else {
	_CHATTY_LOG("NVRAM transfer finalized");
    }
}

static void
_process_new_block_init_msg(struct net_msg_info *msg_to_process)
{
    _block_info.type = ntohl(*((UINT32 *)msg_to_process->data_start));
    _block_info.size = ntohl(*((UINT32 *)(msg_to_process->data_start + 4)));
    _block_info.buffer = malloc(_block_info.size);
    _block_info.total_part_count = _block_info.size / MAX_BLOCK_PART_SIZE;
    if (_block_info.size % MAX_BLOCK_PART_SIZE > 0) {
	_block_info.total_part_count += 1;
    }
    _block_info.remaining_part_count = _block_info.total_part_count;
    _block_info.bitmap = malloc((_block_info.total_part_count / 8) + 1);
    memset(_block_info.bitmap, 0, (_block_info.total_part_count / 8) + 1);
}

static void
_process_new_set_sync_skip_msg(struct net_msg_info *msg_to_process)
{
    _new_sync_skip = ntohl(*((UINT32 *)msg_to_process->data_start));
}

static void
_process_new_block_part_msg(struct net_msg_info *msg_to_process,
			    struct net_peer_info *source_peer)
{
    UINT32 block_part = ntohl(*(UINT32 *)msg_to_process->data_start);
    assert(source_peer == &(_master_info));

    assert(((_block_info.remaining_part_count == _block_info.total_part_count) && _protocol_state == ACKNOWLEDGEMENT) ||
	   ((_block_info.remaining_part_count < _block_info.total_part_count) && _protocol_state == BLOCK_TRANSFER));

    _protocol_state = BLOCK_TRANSFER;
    if (! (_block_info.bitmap[block_part / 8] & (1 << (block_part % 8)))) {
	memcpy(_block_info.buffer + MAX_BLOCK_PART_SIZE * block_part,
	       msg_to_process->data_start + 4,
	       msg_to_process->data_len - 4);
	_block_info.bitmap[block_part / 8] |= (1 << (block_part % 8));
	_block_info.remaining_part_count -= 1;
#if PROTOCOL_DEBUG >= 3
	_CHATTY_LOG("Got block part %d, seq %d; %d parts left",
		    block_part,
		    msg_to_process->sequence,
		    _block_info.remaining_part_count);
#endif
	if (_block_info.remaining_part_count == 0) {
	    switch (_block_info.type) {
	    case NVRAM_BLOCK:
		_finalize_nvram_transfer();
		break;
	    case MACHINESTATE_BLOCK:
		/* _finalize_state_transfer(); */
		break;
	    default:
		/* Sanity check? */
		break;
	    }
	    free(_block_info.buffer);
	    free(_block_info.bitmap);

	    _protocol_state = INPUT_EXCHANGE;
	    _prime_msg(&_outbound_scratch_msg_info,
		       ACK,
		       _current_sync_sequence);
	    _send_msg_to_peer(&_outbound_scratch_msg_info,
			      source_peer);
	    _current_sync_sequence += 1;
	}
    }
}

static void
_process_new_block_prompt_msg(struct net_msg_info *msg_to_process,
			      struct net_peer_info *source_peer)
{
    unsigned block_count = msg_to_process->data_len / sizeof(UINT32);
    unsigned i;
    for (i = 0; i < block_count; i++) {
	unsigned block_part = 
	    ntohl(((UINT32 *)msg_to_process->data_start)[i]);
	assert(block_part < _block_info.total_part_count);
	_send_block_part_to_peer(block_part, source_peer);
    }
}

static void
_build_start_msg_for_peer(struct net_msg_info *msg,
			  struct net_peer_info *peer,
			  unsigned sync_sequence)
{
    unsigned i;
    unsigned char scratch_msg_data[MAX_MSG_LEN];

    _prime_msg(msg, START, sync_sequence);
    scratch_msg_data[0] = peer->player_number;
    scratch_msg_data[1] =
	(_parallel_sync << CONFIG_BITMAP_PARALLELSYNC) |
	(_machine_state_debug << CONFIG_BITMAP_MACHINE_STATE_DEBUG);
    _write_to_msg(msg, scratch_msg_data, 2);
    for (i = 0; i < _current_player_count - 1; i++) {
	/* The accept message sent out to each slave includes the name and
	   IP address for every *other* slave */
	if (_peer_info[i].state != INACTIVE &&
	    _peer_info[i].player_number != peer->player_number)
	{
	    char *ip_string = inet_ntoa(_peer_info[i].addr.sin_addr);
	    char scratch[10];
	    sprintf(scratch, ":%d", _peer_info[i].addr.sin_port);
	    _write_to_msg(msg,
			  _peer_info[i].name,
			  strlen(_peer_info[i].name) + 1);
	    _write_to_msg(msg,
			  ip_string,
			  strlen(ip_string));
	    _write_to_msg(msg, scratch, strlen(scratch) + 1);
	}
    }
}
	    
static void
_inactivate_peer(struct net_peer_info *peer)
{
    if (peer->state == UNSYNCED) {
	_unsynced_peer_count -= 1;
    }
    peer->state = INACTIVE;
    _current_player_count -= 1;

    if (_current_player_count == 1) {
	_net_shutdown();
    }
    else if (peer == &(_master_info)) {
	/* Find a new master -- pick the one with the lowest player
	   number; since these have already been determined by the
	   original master's START message the remaining peers will
	   all pick the same one without needing to sync */
	unsigned i;
	unsigned lowest_player_number = _player_number;
	for (i = 0; i < _original_player_count; i++) {
	    if (_peer_info[i].state != INACTIVE &&
		 _peer_info[i].player_number < lowest_player_number)
	    {
		lowest_player_number = _peer_info[i].player_number;
		_master_index = i;
	    }
	}
	if (lowest_player_number == _player_number) {
	    _current_net_role = MASTER;
	}
    }
}

static void
_process_inbound_msg(struct net_msg_info *msg_to_process,
		     struct net_peer_info *source_peer,
		     unsigned short *input_port_deviations,
		     unsigned retry_timeout)
{

#if PROTOCOL_DEBUG >= 3
    _CHATTY_LOG("Processing message type %d, sequence %d/counter %d"
		" from peer \"%s\" (_current_sync_sequence == %d, "
		"_min_state_sequence == %d)",
		msg_to_process->msg_type,
		msg_to_process->sequence,
		msg_to_process->counter,
		source_peer->name,
		_current_sync_sequence,
		_min_state_sequence);
#endif

    if (msg_to_process->msg_type == QUIT &&
	msg_to_process->sequence <= _current_sync_sequence)
	/* This is the only acceptable case from an inactive peer */
    {
	_prime_msg(&_outbound_scratch_msg_info,
		   QUIT,
		   msg_to_process->sequence);
	if (source_peer->state != INACTIVE) {
	    _CHATTY_LOG("Peer \"%s\" quit", source_peer->name);
	    if (_protocol_state != QUITTING) {
		_send_msg_to_peer(&_outbound_scratch_msg_info,
				  source_peer);
	    }
	    _inactivate_peer(source_peer);
	} else {
	    _handle_duplicate_msg(msg_to_process,
				  source_peer,
				  &_outbound_scratch_msg_info,
				  retry_timeout);
	}
    }
    else if (source_peer->state == INACTIVE) {
	/* Shouldn't really be receiving this message, but it
	   could just be out of order, from a peer that quit earlier.
	   Log an error anyways?  Maybe depending on PROTOCOL_DEBUG? */
	/* _CHATTY_LOG("Warning: Received message (type %d)"
		" from inactive peer \"%s\"",
		msg_to_process->msg_type,
		_peer_info[peer_index].name); */
    }
    else if (msg_to_process->sequence > _current_sync_sequence) {
	/* Receiving an early ACK is nonsensical.  Sanity check? */
	_handle_early_msg(msg_to_process, source_peer, retry_timeout);
    }
    else if (msg_to_process->msg_type == INPUT_STATE) {
	if (msg_to_process->sequence >= _prev_min_state_sequence &&
	    msg_to_process->sequence < _min_state_sequence)
	{
	    assert(_previous_outbound_msg_info->sequence < _current_sync_sequence);
	    _handle_duplicate_msg(msg_to_process,
				  source_peer,
				  _previous_outbound_msg_info,
				  retry_timeout);
	}
	else if (msg_to_process->sequence >= _min_state_sequence) {
	    if (_current_net_role == MASTER &&
		_protocol_state == ACKNOWLEDGEMENT)
	    {
		/* Received slave's input state before its ACK, which
		   is to be expected */
		_handle_early_msg(msg_to_process, source_peer, retry_timeout);
	    }
	    else if (_protocol_state != QUITTING) { 
		/* Already checked for early messages,
		   so state is for current frame */
		if (source_peer->state == SYNCED) {
		    _handle_duplicate_msg(msg_to_process,
					  source_peer,
					  _current_outbound_msg_info,
					  retry_timeout);
		} else { 
		    if (_current_net_role == SLAVE) {
			_protocol_state = INPUT_EXCHANGE;
		    }
		    _process_new_state_msg(msg_to_process,
					   source_peer,
					   input_port_deviations);
		}
	    }
	} /* else message is old enough to ignore */
    }
    else if (msg_to_process->msg_type == BLOCK_PART) {
	assert(_current_net_role == SLAVE && source_peer == &(_master_info));
	/* _BLOCK messages are never repeated without a request, so
	   anything sequenced before _current_sync_sequence must be
	   out of order */
	if (msg_to_process->sequence == _current_sync_sequence &&
	    _protocol_state != QUITTING)
	{
	    _process_new_block_part_msg(msg_to_process, source_peer);
	    _non_state_msg_since_sync = TRUE;
	}
    }
    else if (msg_to_process->msg_type == START ||
	     msg_to_process->msg_type == BLOCK_INIT ||
	     msg_to_process->msg_type == SET_SYNC_SKIP)
    {
	/* All ACKables handled here */
	assert(_current_net_role == SLAVE && source_peer == &(_master_info));
	if (msg_to_process->sequence == _current_sync_sequence - 1) {
	    _prime_msg(&_outbound_scratch_msg_info,
		       ACK,
		       msg_to_process->sequence);
	    _handle_duplicate_msg(msg_to_process,
				  source_peer,
				  &_outbound_scratch_msg_info,
				  retry_timeout);
	}
	else if (msg_to_process->sequence == _current_sync_sequence &&
		 _protocol_state != QUITTING)
	{
	    /* All ACKables handled here */
	    /* START processing performed outside _process_inbound_msg(),
	       only BLOCK_INIT needs to be handled here.  If it's START
	       it must be a duplicate */
	    assert(msg_to_process->msg_type != START);
	    /* No need to check if this has already been received for the
	       current sync sequence as receiving an ACKable immediately
	       sends the slave to the next sync sequence */
	    if (msg_to_process->msg_type == BLOCK_INIT) {
		_process_new_block_init_msg(msg_to_process);
		_protocol_state = ACKNOWLEDGEMENT;
	    }
	    else if (msg_to_process->msg_type == SET_SYNC_SKIP) {
		_process_new_set_sync_skip_msg(msg_to_process);
		_protocol_state = INPUT_EXCHANGE;
	    }
	    _prime_msg(&_outbound_scratch_msg_info,
		       ACK,
		       _current_sync_sequence);
	    _send_msg_to_peer(&_outbound_scratch_msg_info, source_peer);
	    
	    _non_state_msg_since_sync = TRUE;
	    _current_sync_sequence += 1;

	} /* else must be even older than _current_sync_sequence - 1
	     and therefore out of order */
    }
    else if (msg_to_process->msg_type == ACK) {
	assert(_current_net_role == MASTER);
	if (msg_to_process->sequence == _current_sync_sequence - 1) {
	    if (_protocol_state == REGISTRATION) {
		_build_start_msg_for_peer(&_outbound_scratch_msg_info,
					  source_peer,
					  _current_sync_sequence - 1);
		_handle_duplicate_msg(msg_to_process,
				      source_peer,
				      &_outbound_scratch_msg_info,
				      retry_timeout);
	    } else {
		_handle_duplicate_msg(msg_to_process,
				      source_peer,
				      _previous_outbound_msg_info,
				      retry_timeout);
	    }
	}
	else if (msg_to_process->sequence == _current_sync_sequence) {
	    /* Can't be QUITTING since an ACK implies that an ACKable
	       was sent out for this sequence */
	    if (source_peer->state == UNSYNCED) {
		source_peer->state = SYNCED;
		_unsynced_peer_count -= 1;
		if (_unsynced_peer_count == 0 &&
		    (_current_net_role == MASTER || PROTOCOL_DEBUG >= 1))
		{
		    _record_sync_end();
		}
	    }
	    /* Ignore duplicate ACKs -- the master has nothing of
	       interest to send the slave until remaining slaves
	       sync up and the next input state goes out */
	    _non_state_msg_since_sync = TRUE;
	} /* else must be even older than _current_sync_sequence - 1
	     and therefore out of order */
    }
    else if (msg_to_process->msg_type == BLOCK_PROMPT) {
	/* Can't be QUITTING since a BLOCK_PROMPT implies that a block
	   transfer was initiated during this frame */
	assert(_current_net_role == MASTER);
	if (msg_to_process->counter > source_peer->last_inbound_counter) {
	    _process_new_block_prompt_msg(msg_to_process,
					  source_peer);
	    _non_state_msg_since_sync = TRUE;
	}
    } else {
	_CHATTY_LOG("Bogus message type %d, sequence %d, counter %d"
		    " from peer \"%s\"",
		    msg_to_process->msg_type,
		    msg_to_process->sequence,
		    msg_to_process->counter,
		    source_peer->name);
    }
    if (msg_to_process->counter > source_peer->last_inbound_counter) {
	source_peer->last_inbound_counter = msg_to_process->counter;
    }
}

static void
_inbound_presync(unsigned short input_port_deviations[MAX_INPUT_PORTS])
{
    struct net_peer_info *source_peer;
    while (source_peer = _rcv_msg_from_peer(&_inbound_scratch_msg_info,
					    0,
					    NULL,
					    FALSE),
	   source_peer != NULL)
    {
	_process_inbound_msg(&_inbound_scratch_msg_info,
			     source_peer,
			     input_port_deviations,
			     SYNC_RETRY_TIMEOUT);
    }
}

/* This function listens to the network and waits for LOCAL_STATE and ACK
   messages.  It exits when all of the peers have satisfactorily reported,
   or a timeout elapses.
*/
static void
_inbound_sync(unsigned short input_port_deviations[MAX_INPUT_PORTS])
{
    unsigned i;
    /* This function has the following amount of time to complete.  Any
       peers that haven't synced up when the time is up are
       dropped permanently. */
    unsigned time_before_giveup = SYNC_TIMEOUT;

    struct net_peer_info *source_peer;

    /* 1. Process early messages and count peers without early messages */
    for (i = 0; i < _original_player_count - 1; i++) {
	if (_peer_info[i].state == UNSYNCED) {
	    struct net_msg_info *early_msg =
		&(_peer_info[i].early_inbound_msg_info);
	    if ((_protocol_state == INPUT_EXCHANGE &&
		 early_msg->sequence >= _min_state_sequence &&
		 early_msg->sequence <= _current_sync_sequence) ||
		(early_msg->sequence == _current_sync_sequence))
	    {
		_process_inbound_msg(&(_peer_info[i].early_inbound_msg_info),
				     &(_peer_info[i]),
				     input_port_deviations,
				     SYNC_RETRY_TIMEOUT);
	    }
	}
    }

    /* 2. Process new messages from network until all peers sync */
    while (_unsynced_peer_count > 0 && time_before_giveup > 0) {
	unsigned time_used;
	if (_current_net_role == SLAVE) {
	    source_peer = _pushy_rcv_msg_from_peer(&_inbound_scratch_msg_info,
						   SYNC_RETRY_TIMEOUT,
						   time_before_giveup,
						   &time_used,
						   FALSE);
	} else {
	    /* Master never sends reminders due to timeouts */
	    source_peer = _rcv_msg_from_peer(&_inbound_scratch_msg_info,
					     time_before_giveup,
					     &time_used,
					     FALSE);
	}
	if (time_used >= time_before_giveup) {
	    time_before_giveup = 0;
	} else {
	    time_before_giveup -= time_used;
	}
	if (source_peer != NULL) {
	    /* Successfully received message before timeout; process it */
	    _process_inbound_msg(&_inbound_scratch_msg_info,
				 source_peer,
				 input_port_deviations,
				 SYNC_RETRY_TIMEOUT);
	}
    }

    /* 3. At this point all peers have reported their states,
       or have timed out */
    /* TODO:  Dropping peers is susceptible to desync -- fix (if possible?) */
    i = 0;
    while (_unsynced_peer_count > 0 && i < _original_player_count - 1) {
	if (_peer_info[i].state == UNSYNCED) {
	    _CHATTY_LOG("No messages from peer \"%s\" in too long;"
			" disconnecting it",
			_peer_info[i].name);
	    _inactivate_peer(&(_peer_info[i]));
	    _unsynced_peer_count -= 1;
	}
	i += 1;
    }

    _current_sync_sequence += 1;
    if (_protocol_state == INPUT_EXCHANGE) {
	_prev_min_state_sequence = _min_state_sequence;
	_min_state_sequence = _current_sync_sequence;
    }

#if PROTOCOL_DEBUG >= 3
    _CHATTY_LOG("_current_sync_sequence incremented to %d",
		_current_sync_sequence);
#endif
}

static int
_register_to_master(void)
{
    int result = FALSE;

    char scratch[MAX_MSG_LEN];
    struct net_peer_info *peer;
    unsigned peer_index, peer_player_number;
    unsigned msg_data_left;
    char *msg_read_pointer;

    _CHATTY_LOG("Slave Mode; Registering to \"%s\"", _master_info.name);
        
    _previous_outbound_msg_info = &(_saved_msg_info[0]);
    _current_outbound_msg_info = &(_saved_msg_info[1]);

    gethostname(scratch, MAX_MSG_LEN);
	
    _prime_msg(_current_outbound_msg_info, JOIN, 0);
    _write_to_msg(_current_outbound_msg_info, scratch, strlen(scratch));
    sprintf(scratch, "/%d", getpid());
    _write_to_msg(_current_outbound_msg_info,
		  scratch,
		  strlen(scratch) + 1);
    _write_to_msg(_current_outbound_msg_info,
		  _truncated_build_version,
		  strlen(_truncated_build_version) + 1);
    _write_to_msg(_current_outbound_msg_info,
		  PROTOCOL_VERSION,
		  strlen(PROTOCOL_VERSION) + 1);
    _write_to_msg(_current_outbound_msg_info,
		  Machine->gamedrv->name,
		  strlen(Machine->gamedrv->name) + 1);
    /* First attempt to send a message to the address -- possible
       problems include bad address and/or port */
    if (! _send_msg_to_peer(_current_outbound_msg_info, &(_master_info))) {
	_CHATTY_LOG("Unable to send JOIN message to master");
    } else {
	/* Until START message receipt, local instance and master
	   are the only two peers involved */
	unsigned time_used, time_before_giveup = SYNC_TIMEOUT;
	_protocol_state = REGISTRATION;
	_master_info.state = UNSYNCED;
	_original_player_count = 2;
	while (peer = _pushy_rcv_msg_from_peer(&_inbound_scratch_msg_info,
					       REGISTRATION_RETRY_TIMEOUT,
					       time_before_giveup,
					       &time_used,
					       FALSE),
	       peer != NULL &&
	       _inbound_scratch_msg_info.msg_type != REFUSE &&
	       _inbound_scratch_msg_info.msg_type != START)
	{
	    /* NVRAM override happens here */
	    _process_inbound_msg(&_inbound_scratch_msg_info,
				 &(_master_info),
				 NULL,
				 REGISTRATION_RETRY_TIMEOUT);
	    /* HACK:  Override protocol state set by _process_inbound_msg() */
	    if (_protocol_state == INPUT_EXCHANGE) {
		_protocol_state = ACKNOWLEDGEMENT;
	    }
	}
	    
	if (peer == NULL) {
	    _CHATTY_LOG("Registration to \"%s\" timed out", _master_info.name);
	}
	else if (_inbound_scratch_msg_info.msg_type == REFUSE) {
	    _CHATTY_LOG("\"%s\" refused registration", _master_info.name);
	    if (_inbound_scratch_msg_info.data_len > 0) {
		_CHATTY_LOG("Reason given:  %.50s",
			    _inbound_scratch_msg_info.data_start);
	    } else {
		_CHATTY_LOG("No reason given");
	    }
	} else /* _inbound_scratch_msg_info.msg_type == START */ {
	    /* Process START message */
	    _player_number = _inbound_scratch_msg_info.data_start[0];
	    _parallel_sync =
		(_inbound_scratch_msg_info.data_start[1] & (1 << CONFIG_BITMAP_PARALLELSYNC));
	    _machine_state_debug = 
		(_inbound_scratch_msg_info.data_start[1] & (1 << CONFIG_BITMAP_MACHINE_STATE_DEBUG));

	    _current_player_count = 2;
	    peer_index = 0;
	    peer_player_number = 1;
	    /* 1 byte for _player_number + 1 byte for config bitmap == 2 */
	    msg_read_pointer = _inbound_scratch_msg_info.data_start + 2;
	    msg_data_left = _inbound_scratch_msg_info.data_len - 2;
	    while (msg_data_left > 0) {
		peer_index += 1;
		peer_player_number += 1;

		if (peer_player_number == _player_number) {
		    peer_player_number += 1;
		}

		strncpy(_peer_info[peer_index].name,
			msg_read_pointer,
			msg_data_left);
		msg_data_left -= (strlen(msg_read_pointer) + 1);
		msg_read_pointer += (strlen(msg_read_pointer) + 1);
		
		if (msg_data_left == 0) {
		    _CHATTY_LOG("Master returned malformed accept message:"
				" no IP address for player %d",
				peer_player_number);
		}
		else if (! _parse_hostport(msg_read_pointer,
					   &(_peer_info[peer_index].addr)))
		{
		    _CHATTY_LOG("Master returned malformed accept message:"
				" bogus peer host:port \"%s\" for player %d",
				msg_read_pointer,
				peer_player_number);
		    break;
		} else {
		    msg_data_left -= (strlen(msg_read_pointer) + 1);
		    msg_read_pointer += (strlen(msg_read_pointer) + 1);
		    
		    _peer_info[peer_index].player_number = peer_player_number;
		    _peer_info[peer_index].state = UNSYNCED;
		    _peer_info[peer_index].early_inbound_msg_info.sequence = 0;
		    _peer_info[peer_index].outbound_counter = 0;
		    _peer_info[peer_index].last_inbound_counter = 0;
		}
		_current_player_count += 1;
	    }
	    if (msg_data_left == 0) {
		_original_player_count = _current_player_count;

		_prime_msg(&_outbound_scratch_msg_info,
			   ACK,
			   _current_sync_sequence);
		_send_msg_to_peer(&_outbound_scratch_msg_info,
				  &(_master_info));
		_protocol_state = ACKNOWLEDGEMENT;
		_current_sync_sequence += 1;
		
		_CHATTY_LOG("START message processed;"
			    " registration as player %d confirmed",
			    (unsigned)_player_number);
		_CHATTY_LOG("Protocol config:"
			    " parallelsync %s, machine state debug %s",
			    (_parallel_sync ? "enabled" : "disabled"),
			    (_machine_state_debug ? "enabled" : "disabled"));
		result = TRUE;
	    }
	}
    }
    return result;
}


static int
_approve_join_msg(struct net_msg_info *msg_info,
		  struct net_peer_info *source_peer)
{
    int result = FALSE;
    const char * comparison_string[4];
    const char * comparison_string_name[] = { "build version",
					      "protocol version",
					      "game name" };
    unsigned i = 0;

    unsigned msg_data_left = msg_info->data_len;
    char *msg_read_pointer = msg_info->data_start;
    char scratch_msg_data[MAX_MSG_LEN];
	    
    /* Compiler pacification */
    comparison_string[0] = _truncated_build_version;
    comparison_string[1] = PROTOCOL_VERSION;
    comparison_string[2] = Machine->gamedrv->name;
    comparison_string[3] = NULL;

    strncpy(source_peer->name, msg_read_pointer, msg_data_left);
    msg_data_left -= (strlen(source_peer->name) + 1);
    msg_read_pointer += (strlen(source_peer->name) + 1);

    while (comparison_string[i] != NULL) {
	if (strncmp(msg_read_pointer,
		    comparison_string[i],
		    msg_data_left) != 0)
	{
	    sprintf(scratch_msg_data,
		    "Wrong %s; need %s not %.10s",
		    comparison_string_name[i],
		    comparison_string[i],
		    msg_read_pointer);

	    _prime_msg(&_outbound_scratch_msg_info, REFUSE, 0);
	    _write_to_msg(&_outbound_scratch_msg_info,
			  scratch_msg_data,
			  strlen(scratch_msg_data));
	    _send_msg(&_outbound_scratch_msg_info, &(source_peer->addr));
	    break;
	}
	msg_data_left -= (strlen(comparison_string[i]) + 1);
	msg_read_pointer += strlen(comparison_string[i]) + 1;
	i += 1;
    }
    if (comparison_string[i] == NULL) {
	result = TRUE;
    }
    return result;
}

static int
_await_slave_registrations(void)
{
    unsigned char slave_index;
    unsigned i;
    unsigned expected_ack_count;

    _CHATTY_LOG("Master Mode: Waiting for %d more player%s.",
		_original_player_count - 1,
		(_original_player_count - 1 == 1) ? "" : "s");

    _current_player_count = 1; /* Count only master to being with */

    for (slave_index = 0;
	 slave_index < _original_player_count - 1;
	 slave_index++)
    {
	memset(&(_peer_info[slave_index].addr.sin_addr),
	       0,
	       sizeof(&(_peer_info[slave_index].addr.sin_addr)));
	_peer_info[slave_index].state = INACTIVE;
    }
    if (_read_nvram(&_block_info.buffer, &_block_info.size)) {
	UINT32 scratch;

	_block_info.type = NVRAM_BLOCK;
	_block_info.total_part_count = _block_info.size / MAX_BLOCK_PART_SIZE;
	if (_block_info.size % MAX_BLOCK_PART_SIZE > 0) {
	    _block_info.total_part_count += 1;
	}

	_current_outbound_msg_info = &(_saved_msg_info[1]);
	_prime_msg(_current_outbound_msg_info, BLOCK_INIT, 0);
	scratch = htonl(NVRAM_BLOCK);
	_write_to_msg(_current_outbound_msg_info,
		      &scratch,
		      sizeof(scratch));
	scratch = htonl(_block_info.size);
	_write_to_msg(_current_outbound_msg_info,
		      &scratch,
		      sizeof(scratch));
    } else {
	_block_info.buffer = NULL;
    }

    _protocol_state = ACKNOWLEDGEMENT;
    expected_ack_count = 0;
    while (expected_ack_count > 0 ||
	   _current_player_count < _original_player_count)
    {
	struct net_peer_info *source_peer;
	source_peer = _rcv_msg_from_peer(&_inbound_scratch_msg_info,
					 -1,
					 NULL,
					 TRUE);

	if (_inbound_scratch_msg_info.msg_type != JOIN &&
	    source_peer->state != INACTIVE)
	{
	    /* BLOCK_INIT for the NVRAM transfer happens here */
	    _process_inbound_msg(&_inbound_scratch_msg_info,
				 source_peer,
				 NULL,
				 REGISTRATION_RETRY_TIMEOUT);
	    if (source_peer->state == SYNCED) {
		expected_ack_count -= 1;
	    }
	}
	else if (_inbound_scratch_msg_info.msg_type == JOIN) {
	    /* New or re-registration, possibly unknown peer. 
	       When a known slave re-registers (possibly due to packet loss)
	       the master must double-check its handshake parameters
	       (MAME/net version, game name) because it's possible they've
	       changed -- this happens if it's a new process on the same
	       host, which the master has no way of detecting */

	    if (_approve_join_msg(&_inbound_scratch_msg_info, source_peer)) {
		source_peer->outbound_counter = 0;
		if (source_peer->state == INACTIVE) {
		    /* New peer */
		    _current_player_count += 1;
		    source_peer->state = UNSYNCED;
		    if (_block_info.buffer != NULL) {
			_send_msg_to_peer(_current_outbound_msg_info,
					  source_peer);
			expected_ack_count += 1;
		    }
		} else {
		    if (_block_info.buffer != NULL) {
			_handle_duplicate_msg(&_inbound_scratch_msg_info,
					      source_peer,
					      _current_outbound_msg_info,
					      REGISTRATION_RETRY_TIMEOUT);
		    }
		}
		source_peer->last_inbound_counter =
		    _inbound_scratch_msg_info.counter;
	    } else {
		if (source_peer->state != INACTIVE) {
		    /* Existing peer re-registration failed */
		    if (_block_info.buffer != NULL &&
			source_peer->state == UNSYNCED)
		    {
			expected_ack_count -= 1;
		    }
		    source_peer->state = INACTIVE;
		    _current_player_count -= 1;
		}
	    }
	} else /* Neither JOIN nor already active peer */ {
	    _CHATTY_LOG("Error: Unexpected message type (%d)"
			" from unknown peer while awaiting JOIN requests",
			_inbound_scratch_msg_info.msg_type);
	}
    }

    /* At this point the master has accepted enough registrations to
       start the game, so it sends out the NVRAM block if necessary */
    if (_block_info.buffer != NULL) {
	_current_sync_sequence += 1;
	_protocol_state = BLOCK_TRANSFER;
	for (slave_index = 0;
	     slave_index < _current_player_count - 1;
	     slave_index++)
	{
	    for (i = 0; i < _block_info.total_part_count; i++) {
		_send_block_part_to_peer(i, &(_peer_info[slave_index]));
	    }
	    _peer_info[slave_index].state = UNSYNCED;
	}
	_unsynced_peer_count = _current_player_count - 1;
	_inbound_sync(NULL);
    }

    /* Finally, the START messages */
    _protocol_state = REGISTRATION;
    for (slave_index = 0;
	 slave_index < _current_player_count - 1;
	 slave_index++)
    {
	_peer_info[slave_index].player_number = slave_index + 2;
	_build_start_msg_for_peer(&_outbound_scratch_msg_info,
				  &(_peer_info[slave_index]),
				  _current_sync_sequence);

	if (! _send_msg_to_peer(&_outbound_scratch_msg_info,
				&(_peer_info[slave_index])))
	{
	    /* At this point it's too late to back out and start accepting
	       registrations again, so we'll have to forget about this
	       one.  The other slaves don't know and will have to time
	       out */
	    _CHATTY_ERRNO_LOG("Error: socket error sending START message"
			      " to slave \"%s\" (player %d)",
			      _peer_info[slave_index].name,
			      _peer_info[slave_index].player_number);
	} else {
	    _CHATTY_LOG("\"%s\" registration accepted as player %d.",
			_peer_info[slave_index].name,
			_peer_info[slave_index].player_number);
	    _peer_info[slave_index].state = UNSYNCED;
	    _peer_info[slave_index].early_inbound_msg_info.sequence = 0;
	}
    }

    /* Await ACKs for START messages */
    _unsynced_peer_count = _current_player_count - 1;
    _inbound_sync(NULL);

    _previous_outbound_msg_info = &(_saved_msg_info[0]);
    _current_outbound_msg_info = &(_saved_msg_info[1]);

    return TRUE;
}

static int
_map_input_port(unsigned unmapped_bit_index,
		struct net_input_bit_id *mapped_bit_id)
{
    struct input_map_reference_t {
	UINT32 playermask;
	UINT32 start;
	UINT32 coin;
    } input_map_reference[] = { { IPF_PLAYER2, IPT_START2, IPT_COIN2 },
				{ IPF_PLAYER3, IPT_START3, IPT_COIN3 },
				{ IPF_PLAYER4, IPT_START4, IPT_COIN4 } };

    unsigned candidate_port_index, candidate_bit_index;
    UINT32 unmapped_type = Machine->gamedrv->input_ports[unmapped_bit_index].type;
    UINT32 target_type = IPT_UNKNOWN;
    int result = FALSE;

    if (Machine->gamedrv->input_ports[0].type != IPT_PORT) {
	_CHATTY_LOG("Unable to build key map:  "
		    "Port definitions do not begin with IPT_PORT");        
    }
    else if (_player_number > 1) {      /* Map player n input bit to player 1,
					   all others are no-op */
	if ((unmapped_type & IPF_PLAYERMASK) ==
	    input_map_reference[_player_number - 2].playermask)
	{
	    target_type = ((unmapped_type & ~IPF_PLAYERMASK) | IPF_PLAYER1);
	}
	else if (unmapped_type == input_map_reference[_player_number - 2].start)
	{
	    target_type = IPT_START1;
	}
	else if (unmapped_type == input_map_reference[_player_number - 2].coin)
	{
	    target_type = IPT_COIN1;
	}

	if (target_type != IPT_UNKNOWN) {
	    UINT32 candidate_type;
	    candidate_bit_index = 1;
	    candidate_port_index = 0;
	    while (candidate_port_index < MAX_INPUT_PORTS &&
		   (candidate_type = Machine->gamedrv->input_ports[candidate_bit_index].type,
		    candidate_type != IPT_END) &&
		   ! result)
	    {
		if (candidate_type == IPT_PORT) {
		    candidate_port_index += 1;
		}
		else if (candidate_type == target_type) {
		    mapped_bit_id->port_index = candidate_port_index;
		    mapped_bit_id->mask = Machine->gamedrv->input_ports[candidate_bit_index].mask;
		    result = TRUE;
		}
		candidate_bit_index += 1;
	    }
	}
    }
    return result;
}

static int
_build_net_keymap(void)
{
    int result = FALSE;

    _input_mapping_count = 0;

    if (Machine->gamedrv->input_ports[0].type != IPT_PORT) {
	_CHATTY_LOG("Unable to build key map:  "
		    "Port definitions do not begin with IPT_PORT");        
    } else {
	UINT32 current_type;
	unsigned unmapped_bit_index = 1, unmapped_port_index = 0;

	while (unmapped_port_index < MAX_INPUT_PORTS &&
	       (current_type = Machine->gamedrv->input_ports[unmapped_bit_index].type,
		current_type != IPT_END))
	{
	    unsigned previous_mapping_index;
	    int duplicate_found = 0;
	    if (current_type == IPT_PORT) {
		unmapped_port_index += 1;
	    }
	    /* Check for duplicates (eg, same input port for player 1 button 1
	       and player 1 start in Gauntlet) */
	    previous_mapping_index = 0;
	    while (previous_mapping_index < _input_mapping_count &&
		   ! duplicate_found)
	    {
		unsigned previously_mapped_port_index = 
		    _input_map[previous_mapping_index].source.port_index;
		unsigned short previously_mapped_mask =
		    _input_map[previous_mapping_index].source.mask;
		if (unmapped_port_index == previously_mapped_port_index &&
		    Machine->gamedrv->input_ports[unmapped_bit_index].mask == previously_mapped_mask)
		{
		    duplicate_found = 1;
		}
		previous_mapping_index += 1;
	    }
	    if (! duplicate_found &&
		_map_input_port(unmapped_bit_index, &(_input_map[_input_mapping_count].dest)))
	    {
		_input_map[_input_mapping_count].source.port_index = unmapped_port_index;
		_input_map[_input_mapping_count].source.mask =
		    Machine->gamedrv->input_ports[unmapped_bit_index].mask;
		_input_mapping_count += 1;
	    }
	    unmapped_bit_index += 1;
	}
	result = TRUE;
    }
    return result;
}

static void
_remap_input_state(unsigned short input_state[MAX_INPUT_PORTS])
{
    unsigned mapping_index;
    for (mapping_index = 0;
	 mapping_index < _input_mapping_count;
	 mapping_index++)
    {
	unsigned short source_mask = _input_map[mapping_index].source.mask;
	unsigned short source_bit =
	    input_state[_input_map[mapping_index].source.port_index] & source_mask;
	unsigned short dest_mask = _input_map[mapping_index].dest.mask;
	unsigned short dest_bit = 
	    input_state[_input_map[mapping_index].dest.port_index] & dest_mask;

	if (source_bit == 0) {
	    input_state[_input_map[mapping_index].dest.port_index] &= ~dest_mask;
	} else {
	    input_state[_input_map[mapping_index].dest.port_index] |= dest_mask;
	}
	if (dest_bit == 0) {
	    input_state[_input_map[mapping_index].source.port_index] &= ~source_mask;
	} else {
	    input_state[_input_map[mapping_index].source.port_index] |= source_mask;
	}
    }
}

#if PROTOCOL_DEBUG >= 3
static void
_log_port_array(unsigned short *port_values, unsigned port_count)
{
    unsigned i;
    for (i = 0; i < port_count; i++) {
	fprintf(stderr_file,
		"%x%s",
		port_values[i],
		(i < port_count - 1) ? " " : "\n");
    }
}
#endif

static void
_build_input_state_msg(struct net_msg_info *msg)
{
    unsigned i;
    
#if PROTOCOL_DEBUG >= 3
    fprintf(stderr_file,
	    "Frame %d outbound/saved deviations: ",
	    _frame_count);
    _log_port_array(_outbound_input_state, MAX_INPUT_PORTS);
#endif

    _prime_msg(msg, INPUT_STATE, _current_sync_sequence);
    for (i = 0; i < MAX_INPUT_PORTS; i++) {
	unsigned short scratch = htons(_outbound_input_state[i]);
	_write_to_msg(msg, &scratch, sizeof(scratch));
    }
}

static void
_build_set_sync_skip_msg(struct net_msg_info *msg)
{
    UINT32 scratch = (UINT32)htonl(_new_sync_skip);
    _prime_msg(msg, SET_SYNC_SKIP, _current_sync_sequence);
    _write_to_msg(msg, &scratch, sizeof(scratch));
}

static void
_outbound_sync(struct net_msg_info *msg)
{
    unsigned peer_index;

#if PROTOCOL_DEBUG >= 3
    _CHATTY_LOG("Sending message type %d sequence %d to peers",
		msg->msg_type,
		msg->sequence);
#endif
    if (_current_net_role == MASTER || PROTOCOL_DEBUG >= 1) {
	_record_sync_start();
    }

    for (peer_index = 0;
	 peer_index < _original_player_count - 1;
	 peer_index++)
    {
	if (_peer_info[peer_index].state != INACTIVE) {
	    _send_msg_to_peer(_current_outbound_msg_info,
			      &(_peer_info[peer_index]));
	    _peer_info[peer_index].state = UNSYNCED;
#if PROTOCOL_DEBUG >= 3
	    _CHATTY_LOG("State of peer \"%s\" is now %d",
			_peer_info[peer_index].name,
			_peer_info[peer_index].state);
#endif
	}
    }
    _unsynced_peer_count = _current_player_count - 1;

    /* Now determine the new protocol state based on the outbound message
       type (n.b. BLOCK_TRANSFER not handled here) */
    if (msg->msg_type == INPUT_STATE) {
	_protocol_state = INPUT_EXCHANGE;
    } else {
	_protocol_state = ACKNOWLEDGEMENT;
    }
}

static void
_accumulate_new_input(unsigned short input_port_values[MAX_INPUT_PORTS],
		      unsigned short input_port_defaults[MAX_INPUT_PORTS])
{
    unsigned i;
    /* The idea here is to accumulate deviations from the
       *outbound* state (not the default state), in order
       to try to minimize "lost" keystrokes */
    for (i = 0; i < MAX_INPUT_PORTS; i++) {
	_new_input_state[i] |= 
	    (_outbound_input_state[i] ^ (input_port_values[i] ^ input_port_defaults[i]));
    }
    
}

static unsigned
_sync_skip_adjustment()
{
    unsigned adjusted_skip = _sync_skip;

    if (_frame_count >= _minimum_frame_for_valid_sma) {
	float ms_per_frame =
	    (float)1000 / (float)Machine->drv->frames_per_second;
	float lower_boundary =
	    (float)_sync_skip * ms_per_frame - (float)SYNC_SKIP_TOLERANCE;
	float upper_boundary =
	    (float)(_sync_skip + 1) * ms_per_frame + (float)SYNC_SKIP_TOLERANCE;
	if (_sync_skip > 0 && _sync_time_sma < lower_boundary) {
	    adjusted_skip -= 1;
#if PROTOCOL_DEBUG >= 2
	    _CHATTY_LOG("Sync time SMA is now %.2f (< %.2f)",
			_sync_time_sma,
			lower_boundary);
#endif
	}
	else if (_sync_skip < MAX_SYNC_SKIP && _sync_time_sma > upper_boundary)
	{
	    adjusted_skip += 1;
#if PROTOCOL_DEBUG >= 2
	    _CHATTY_LOG("Sync time SMA is now %.2f (> %.2f)",
			_sync_time_sma,
			upper_boundary);
#endif
	}
    }
    return adjusted_skip;
}

void
osd_net_sync(unsigned short input_port_values[MAX_INPUT_PORTS],
	     unsigned short input_port_defaults[MAX_INPUT_PORTS])
{
    if (_current_net_role != NONE) {
	unsigned i;
	int skip_this_frame = (_frame_count % (_sync_skip + 1) != 0);

	if (_input_remap && _player_number != 1) {
	    _remap_input_state(input_port_values);
	    _remap_input_state(input_port_defaults);
	}

	_accumulate_new_input(input_port_values, input_port_defaults);

	if (skip_this_frame) {

	    if (_unsynced_peer_count > 0) {
		_inbound_presync(_inbound_input_state);
	    }

	} else {

	    for (i = 0; i < MAX_INPUT_PORTS; i++) {
		_saved_input_state[i] =
		    _outbound_input_state[i] | _inbound_input_state[i];
		_outbound_input_state[i] ^= _new_input_state[i];
	    }
	    memset(_new_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(input_port_values[0]));
	    memset(_inbound_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(input_port_values[0]));

	    /* Step 1:  If parallel sync is enabled, _inbound_sync() first */
	    if (_parallel_sync && _frame_count > 0) {
		_inbound_sync(_saved_input_state);
	    }

	    /* Step 2: Sync skip adjustment, if necessary */
	    if (_new_sync_skip != _sync_skip) {
		_CHATTY_LOG("Now setting skip to %d (_frame_count == %d)",
			    _new_sync_skip,
			    _frame_count);
		_sync_skip = _new_sync_skip;
		_minimum_frame_for_valid_sma =
		    _frame_count + SYNC_TIME_SMA_POINTS;
	    }
	    else if (_current_net_role == MASTER &&
		     (_new_sync_skip = _sync_skip_adjustment(),
		      _new_sync_skip != _sync_skip))
	    {
		_PTR_SWAP(_current_outbound_msg_info,
			  _previous_outbound_msg_info);
		_build_set_sync_skip_msg(_current_outbound_msg_info);
		_outbound_sync(_current_outbound_msg_info);
		_inbound_sync(_inbound_input_state);
	    }

	    /* Step 3: _outbound_sync() */
	    _PTR_SWAP(_current_outbound_msg_info, _previous_outbound_msg_info);
	    _build_input_state_msg(_current_outbound_msg_info);
	    _outbound_sync(_current_outbound_msg_info);
	    /* Convert deviations from the saved input state into
	       deviations from the default state */
	    
	    /* Step 4: If parallel sync is disabled, _inbound_sync() last */
	    if (! _parallel_sync) {
		_inbound_sync(_saved_input_state);
	    }
	}

	/* Change array from default deviations back into actual values */
	for (i = 0; i < MAX_INPUT_PORTS; i++) {
	    input_port_values[i] =
		_saved_input_state[i] ^ input_port_defaults[i];
	}

#if PROTOCOL_DEBUG >= 3
	fprintf(stderr_file,
		"Frame %d final values: ",
		_frame_count);
	_log_port_array(input_port_values, MAX_INPUT_PORTS);
#endif
	_frame_count += 1;
    } /* else this shouldn't even be called */
}
    
/*
 * Initialise network
 * - the master opens a socket and waits for slaves
 * - the slaves register to the master
 */
int
osd_net_init(void)
{
    int result = OSD_NOT_OK;

    if (_original_player_count > 0) {
	if (_current_net_role == SLAVE) {
	    _CHATTY_LOG("Can't be both Slave and Master");
	} else {
	    _current_net_role = MASTER;
	}
    }
	
    if (_current_net_role == NONE) {
	result = OSD_OK;
    } else {
	/* BRITTLE. The truncated build version is used in
	   the handshake to match MAME versions -- is it worth it? */
	char *build_version_end = strchr(build_version, ' ');
	if (build_version_end == NULL) {
	    strcpy(_truncated_build_version, build_version);
	} else {
	    strncpy(_truncated_build_version,
		    build_version,
		    (build_version_end - build_version));
	}

	if (_init_sockets()) {
	    if (_current_net_role == MASTER) {
		if (_await_slave_registrations()) {
		    unsigned i;
		    result = OSD_OK;
		    for (i = 0; i < SYNC_TIME_SMA_POINTS; i++) {
			_sync_time[i] = 0;
		    }
		}
	    } else {
		if (_register_to_master()) {
		    result = OSD_OK;
		    if (_input_remap)
			if (! _build_net_keymap()) {
			    _input_remap = 0;
		    }
		}
	    }
	    memset(_saved_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(_saved_input_state[0]));
	    memset(_inbound_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(_inbound_input_state[0]));
	    memset(_outbound_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(_outbound_input_state[0]));
	    memset(_new_input_state,
		   0,
		   MAX_INPUT_PORTS * sizeof(_new_input_state[0]));

#if PROTOCOL_DEBUG >= 1
	    gettimeofday(&_start_time, NULL);
#endif
	}
    }
    return result;
}

int
osd_net_active(void)
{
    return (_current_net_role != NONE);
}

static void
_bid_farewell(void)
{
    unsigned peer_index = 0;

    _protocol_state = QUITTING;
    for (peer_index = 0;
	 peer_index < _original_player_count - 1;
	 peer_index++)
    {
	if (_peer_info[peer_index].state != INACTIVE) {
	    _prime_msg(&_outbound_scratch_msg_info,
		       QUIT,
		       _current_sync_sequence);
	    _send_msg_to_peer(&_outbound_scratch_msg_info, 
			      &(_peer_info[peer_index]));
	    _peer_info[peer_index].state = UNSYNCED;
	}
    }
    _inbound_sync(NULL);
}

#define MILLISECONDS_PART(ms) (ms % 1000)
#define SECONDS_PART(ms) ((ms / 1000) - (ms / (60 * 1000) * 60))
#define MINUTES_PART(ms) (ms / (60 * 1000))
/*
 * Close all opened sockets
 */
static void
_net_shutdown(void)
{
#if PROTOCOL_DEBUG >= 1
    struct timeval end_time;
    unsigned total_time;
#endif
    
    close(_socket_fd);

#if PROTOCOL_DEBUG >= 1
    gettimeofday(&end_time, NULL);

    total_time = _tv_subtract(&end_time, &_start_time) / 1000;
    _CHATTY_LOG("Protocol stats:");
    _CHATTY_LOG("Current sync sequence:  %d", _current_sync_sequence);
    _CHATTY_LOG("Duplicate inbound messages:  %d, reminder messages sent: %d",
		_inbound_duplicate_count,
		_reminder_count);
    _CHATTY_LOG("Total sync time:  %02dm%02d.%ds (%02dm%02d.%ds real time elapsed)",
		MINUTES_PART(_total_sync_time),
		SECONDS_PART(_total_sync_time),
		MILLISECONDS_PART(_total_sync_time),
		MINUTES_PART(total_time),
		SECONDS_PART(total_time),
		MILLISECONDS_PART(total_time));
    _CHATTY_LOG("Average sync time:  %d ms",
		_total_sync_time / _current_sync_sequence);
    _CHATTY_LOG("Min sync time:  %d ms", _min_sync_time);
    _CHATTY_LOG("Max sync time:  %d ms", _max_sync_time);
#endif
    _current_net_role = NONE;
}

void
osd_net_close(void)
{
#if PROTOCOL_DEBUG >= 2
    _CHATTY_LOG("Bye");
#endif    
    if (_current_net_role != NONE) {
	_bid_farewell(); /* _net_shutdown() will also get called
			    through _inbound_sync() */
    }
    if (_nvram_backup != NULL) {
	_write_nvram(_nvram_backup, _nvram_backup_len);
    }
}

#endif /* XMAME_NET */
