/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    Network Portions Copyright (C) 1998 Peter Eberhardy and Michael Adcock
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  net32.c

 ***************************************************************************/
#ifdef MAME_NET

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <winbase.h>
#include <assert.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include "MAME32.h"
#include "M32Util.h"
#include "net32.h"
#include "network.h"
#include "status.h"
#include "driver.h"
#include "mame.h"

// #define NET_DEBUG
#ifdef NET_DEBUG
#define dprintf(x)  printf x;
#else
#define dprintf(x)
#endif

/***************************************************************************
    Macro defines
***************************************************************************/

/* Macro to crack chat messages */
#define MAKEDWORD(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | (d))

/* Force Play */
#define CHAT_PLAY MAKEDWORD('P','L','A','Y')
/* Game Select */
#define CHAT_GAME MAKEDWORD('G','A','M','E')
/* Join Chat */
#define CHAT_JOIN MAKEDWORD('J','O','I','N')
/* Chat text */
#define CHAT_TEXT MAKEDWORD('C','H','A','T')
/* Chat exit */
#define CHAT_EXIT MAKEDWORD('E','X','I','T')

#define MAX_CHAT_BUFFERS 5

/***************************************************************************
    function prototypes
***************************************************************************/

static int Network_TCPIP_init(void);
static int Network_TCPIP_game_init(void);
static int Network_TCPIP_send(int player, unsigned char buf[], int *size);
static int Network_TCPIP_recv(int player, unsigned char buf[], int *size);
static int Network_TCPIP_sync(void);
static int Network_TCPIP_input_sync(void);
static int Network_TCPIP_game_exit(void);
static int Network_TCPIP_exit(void);
static int Network_TCPIP_add_player(void);
static int Network_TCPIP_remove_player(int player);
static int Network_TCPIP_chat_send(unsigned char buf[], int *size);
static int Network_TCPIP_chat_recv(int player, unsigned char buf[], int *size);

static void DecodeChatMessage(HWND hWnd, BOOL is_client, char *buf, int size);
static void SendPlayerList(int player);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDNetwork  Network = 
{
    { Network_TCPIP_init },            /* init          */
    { Network_TCPIP_game_init },       /* game_game     */
    { Network_TCPIP_send },            /* send          */
    { Network_TCPIP_recv },            /* recv          */
    { Network_TCPIP_sync },            /* sync          */
    { Network_TCPIP_input_sync },      /* input_sync    */
    { Network_TCPIP_game_exit },       /* game_exit     */
    { Network_TCPIP_exit },            /* exit          */
    { Network_TCPIP_add_player },      /* add_player    */
    { Network_TCPIP_remove_player },   /* remove_player */
    { Network_TCPIP_chat_send },       /* chat_send     */
    { Network_TCPIP_chat_recv },       /* chat_recv     */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tNetwork_private
{
    WSADATA wsadata;                    /* Winsock data     */
    SOCKET  sServer;                    /* server socket    */
    SOCKET  sConnecting;                /* listening socket */
    SOCKET  sClient[NET_MAX_PLAYERS];   /* client sockets   */
    char *  playerName[NET_MAX_PLAYERS];/* client names     */
    BOOL    bClient;                    /* TRUE if client   */
    BOOL    bNetGameOnly;               /* TRUE if net was started for game only */
    BOOL    bInitialized;               /* Net initialized  */
    int     iOldNumClients;             /* wanted number of clients     */
    int     iNumClients;                /* number of connectec clients  */
    fd_set  ssetReadable;               /* used by select   */
    fd_set  ssetWriteable;              /* used by select   */
    struct  timeval  tvWait;            /* used by select   */
    HWND    hWndChat;                   /* chat window handle   */
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tNetwork_private This;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/***************************************************************************
    Internal functions  
 ***************************************************************************/
static int Network_TCPIP_init( void )
{
    /* figure out our options and some general network setup */
    net_options         *options = GetNetOptions();
    struct sockaddr_in  sin;
    struct protoent     *ppe;
    struct hostent      *phe;
    int                 type;
    char                *protocol;
    SOCKET              s;
    int                 i;

    This.bNetGameOnly = FALSE;
    This.bInitialized = FALSE;
    This.iOldNumClients = 0;
    This.iNumClients = 0;

    for (i = 0; i < NET_MAX_PLAYERS; i++)
        This.playerName[i] = NULL;

    protocol = options->net_transport;

    /* request 2.0 winsock but accept any */
    /* we don't really care and stick to berkely functions */
    if ( WSAStartup( MAKEWORD( 2, 0 ), &(This.wsadata) ) )
    {
        ErrorMsg( "Unable to start winsock support" );
        return NET_ERROR;
    }

    /* figure out who we are */
    switch (options->net_tcpip_mode)
    {
    case NET_ID_PLAYER_CLIENT:  /* acting as client */
        This.bClient = TRUE;
        break;
    case NET_ID_PLAYER_SERVER:  /* acting as server */
        This.bClient = FALSE;
        break;
    default:                    /* somehow we have an invalid mode */
        return NET_ERROR;
    }

    /* common initialization */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family   = AF_INET;

    if ((sin.sin_port = htons((unsigned short)options->net_tcpip_port)) == 0)
        return NET_ERROR;
    
    /* Lookup protocol */
    if ((ppe = getprotobyname(protocol)) == 0)
    {
        ErrorMsg("can't get \"%s\" protocol entry\n", protocol);
        return NET_ERROR;
    }

    /* determine which type of socket to create */
    if (strcmp(protocol,"udp") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    /* now open our socket */
    if ((s = socket(PF_INET, type, ppe->p_proto)) == INVALID_SOCKET)
    {
        ErrorMsg("Request to create socket failed");
        return NET_ERROR;
    }

    if ( This.bClient ) /* client connect */
    {
        char *hostname = options->net_tcpip_server;

        /* Lookup hostname */
        if ((phe = gethostbyname(hostname)) != 0)
        {
            memcpy( (char *)&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if (( sin.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE )
        {
            ErrorMsg("can't get \"%s\" host entry\n", hostname);
            return NET_ERROR;
        }

        /* connect it to the target host */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) != 0)
        {
            ErrorMsg("Can't connect to %s:%d\n", hostname, options->net_tcpip_port);
            return NET_ERROR;
        }
        This.sServer = s;

    }   /* end client */
    else            /* server listen */
    {
        This.iOldNumClients = options->net_num_clients;

        sin.sin_addr.s_addr = htonl( INADDR_ANY );

        /* Bnd the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR)
        {
            ErrorMsg("Cant' bind to %d port", options->net_tcpip_port);
            return NET_ERROR;
        }
        
        /* listen if not using "udp" */
        if (type == SOCK_STREAM && listen(s, SOMAXCONN) == SOCKET_ERROR)
        {
            ErrorMsg("Can't listen on %d port\n", options->net_tcpip_port);
            return NET_ERROR;
        }

        This.sConnecting = s;

    }   /* end server */

    This.bInitialized = TRUE;

    return This.bClient;
}

static int Network_TCPIP_game_init(void)
{
    dprintf(("Entering: Network_TCPIP_game_init() - bInitialize = %d\n",
        This.bInitialized))

    /* start the net if needed */
    if (This.bInitialized == FALSE)
    {
        if (net_init() != 0)
            return NET_ERROR;
        else
            This.bNetGameOnly = TRUE;
    }

    if (This.bClient) /* We are connected */
    {
        dprintf(("Client: Network_TCPIP_game_init() - SUCCESS\n"))
        return 0;
    }

    /* wait for any remaining clients to connect */
	while(This.iNumClients < This.iOldNumClients)
	{
		do {
			FD_ZERO( &(This.ssetReadable) );
			FD_SET( This.sConnecting, &(This.ssetReadable) );
			This.tvWait.tv_sec = 0; 
			This.tvWait.tv_usec = 50000;
		} while ( select( 0, &(This.ssetReadable), NULL, NULL, &(This.tvWait) ) == 0 );

		This.iNumClients = net_add_player();
	}

    dprintf(("Server: Network_TCPIP_game_init() - SUCCESS\n"))
    return 0;
}

static int Network_TCPIP_send(int player, unsigned char buf[], int *size)
{
    SOCKET s;

    /* pick the socket */
    s = ( This.bClient ) ? This.sServer : This.sClient[player];

    //TODO: figure out how to timeout here
    *size = send( s, buf, *size, 0 );

    return 0;
}

static int Network_TCPIP_recv(int player, unsigned char buf[], int *size)
{
    SOCKET s;

    /* pick the socket */
    s = ( This.bClient ) ? This.sServer : This.sClient[player];

#if 0   /* this might be used if joining games becomes possible */

    if( This.bClient )
        s = This.sServer;
    else /* server */
    {
        s = This.sClient[player];
        /* poll to see if theres a new client */
        FD_ZERO( &(This.ssetReadable) );
        FD_SET( This.sConnecting, &(This.ssetReadable) );
        This.tvWait.tv_sec = 0; 
        This.tvWait.tv_usec = 0;
        select( 0, &(This.ssetReadable), NULL, NULL, &(This.tvWait) );

        /* if we're the server check for new clients at this time */
        if ( FD_ISSET( This.sConnecting, &(This.ssetReadable) ) )
        {
            printf("\tnew client is available!\n");
            //TODO: add a new client at this point
            //net_add_client();
        }
    }
#endif
    //TODO: should probably be able to timeout here
    *size = recv( s, buf, *size, 0 );
    dprintf(("Network_TCPIP_recv() %d bytes\n", *size))
    return 0;
}

static int Network_TCPIP_sync(void)
{
    return 0;
}

static int Network_TCPIP_input_sync(void)
{
    return 0;
}

static int Network_TCPIP_game_exit(void)
{
    /* if the game wasn't started by osd_game_init().
     * we should allow quitting the game but not closing
     * the connection.  It should be marked as not playing
     * and notify the server.
     */
    if (This.bNetGameOnly == FALSE)
        return 0;

    return net_exit( NET_QUIT_QUIT );
}

static int Network_TCPIP_exit(void)
{
    /* Now we do the osdepended exit part... */
    int fail = 0; /* so we're optimistic */

    if (This.bInitialized == FALSE)
    {
        return 0;
    }

    /* clean up open sockets */
    if ( This.bClient )
    {
        if ( closesocket( This.sServer ) )
        {
            ErrorMsg( "Problem closing communication socket" );
            fail = 1;
        }
    }
    else /* server */
    {
        int i;

        for ( i = 0; i < This.iNumClients; i++ )
        {
            /* deallocate player names */
            if (This.playerName[i])
            {
                free(This.playerName[i]);
                This.playerName[i] = NULL;
            }
            if( closesocket( This.sClient[i] ) )
            {
                ErrorMsg( "Problem closing comm socket with player %d", i );
                fail = 1;
            }
        }
    }

    /* only server needs to close connecting socket at this point */
    if ( !This.bClient && closesocket( This.sConnecting ) )
    {
        ErrorMsg( "Problem closing connecting socket\n" );
        fail = 1;
    }

    /* windows wants us to unload winsock support at this point */
    if ( WSACleanup() )
    {
        ErrorMsg( "Unable to release winsock support" );
        fail = 1;
    }

    This.bInitialized = FALSE;
    This.bNetGameOnly = FALSE;

    return fail;
}

static int Network_TCPIP_add_player(void)
{
    struct sockaddr_in saClient;
    int cbSASize;
    int iNewClient = This.iNumClients;
    cbSASize = sizeof( struct sockaddr_in );

    /* accept connection from client and switch to comm socket */
    This.sClient[iNewClient] = accept( This.sConnecting, (struct sockaddr*)&saClient, &cbSASize );
    if ( This.sClient[iNewClient] == INVALID_SOCKET )
    {
        ErrorMsg( "Accepting client picked up a bad socket" );
        return NET_ERROR;
    }

    return iNewClient+1; /* will increment number of clients */
}

static int Network_TCPIP_remove_player(int player)
{
    if (!This.bClient)
    {
        /* deallocate player name */
        if (This.playerName[player])
        {
            free(This.playerName[player]);
            This.playerName[player] = NULL;
        }
    }
    return --This.iNumClients;
}

static int Network_TCPIP_chat_send(unsigned char buf[], int *size)
{
    /* If we're the server, send this text to all hosts, if client
     * send this only to the server.
     */
    if (This.bClient)
        net_chat_send(buf, size);
    else
    {
        int i;

        for (i = 0; i < This.iNumClients; i++)
        {
            net_send( i, NET_CHAT_CHAT, buf, *size );
        }
        /* server needs to send itself a message too */
        DecodeChatMessage(This.hWndChat, 0, buf, *size);
    }

    return 0; 
}

static int Network_TCPIP_chat_recv(int player, unsigned char buf[], int *size)
{
    //char temp[300];

    /* if we are the server, send this to all hosts */
    /* then post a message to ourselves */
    net_chat_recv(player, buf, size);

    /* If we are the server, echo this chat to everyone */
    if (!This.bClient)
        Network_TCPIP_chat_send(buf, size);
    else
        /* Client needs to post itself a message */
        DecodeChatMessage(This.hWndChat, player, buf, *size);

    return 0;
}

static void DecodeChatMessage(HWND hWnd, int player, char *buf, int size)
{
    static char msg[MAX_CHAT_BUFFERS][NET_MAX_DATA];
    static int  cur_msg = 0;
    DWORD       dwType = MAKEDWORD(buf[0],buf[1],buf[2],buf[3]);
    char *      pMsg;
    
    if (size < 5 && dwType != CHAT_PLAY)
        return;

    cur_msg = (cur_msg + 1) % MAX_CHAT_BUFFERS;
    pMsg = msg[cur_msg];
    memset(pMsg,'\0',NET_MAX_DATA);

    if (size > 4)
        strcpy(pMsg,&buf[4]);

    switch (dwType)
    {
    case CHAT_JOIN: /* New player joined chat */
        if (!This.bClient)
        {
            SendPlayerList(player);
            This.playerName[player] = strdup(pMsg);
        }
        PostMessage(hWnd, WM_WINSOCK_CHAT_JOIN, 0, (LPARAM)pMsg);
        return;
    case CHAT_TEXT: /* Normal chat text */
        PostMessage(hWnd, WM_WINSOCK_CHAT_TEXT, 0, (LPARAM)pMsg);
        return;
    case CHAT_PLAY: /* Server is starting game */
        Network_TCPIP_end_chat();
        PostMessage(hWnd, WM_WINSOCK_START_GAME, 0, 0);
        break;
    case CHAT_GAME: /* Server changed game selection */
        PostMessage(hWnd, WM_WINSOCK_SELECT_GAME, 0, (LPARAM)pMsg);
        break;
    case CHAT_EXIT:  /* Player quit chat */
        PostMessage(hWnd, WM_WINSOCK_CHAT_EXIT, 0, (LPARAM)pMsg);
        break;
    }
}

/* Send list of players to the client */
static void SendPlayerList(int player)
{
    /* If we're the server, send the list of players to
     * the player specified by 'player'
     */
    if (! This.bClient)
    {
        /* We rotate the buffers because the send
         * is non-blocking and may be buffer up
         */
        net_options *netOpts = GetNetOptions();
        char        buf[NET_MAX_PLAYERS][NET_MAX_PACKET];
        int         bufno = 0;
        int         i;

        for (i = 0; i < This.iNumClients; i++)
        {
            bufno = (bufno + 1) % NET_MAX_PLAYERS;
            /* Send the connected player list as JOIN messages */
            if (This.playerName[i])
            {
                sprintf(buf[i], "JOIN%s", This.playerName[i]);
                net_send( player, NET_CHAT_CHAT, buf[i], strlen(buf[i]));
                Sleep(200); /* If we don't delay the packets get munged */
            }
        }
        bufno = (bufno + 1) % NET_MAX_PLAYERS;
        
        /* Tell 'em who we are too */
        sprintf(buf[bufno], "JOIN%s", netOpts->net_player);
        net_send( player, NET_CHAT_CHAT, buf[bufno], strlen(buf[bufno]));
        Sleep(200); /* If we don't delay the packets get munged */

        bufno = (bufno + 1) % NET_MAX_PLAYERS;

        /* Now send them the currently selected game name */
        sprintf(buf[bufno], "GAME%s", GetDefaultGame());
        net_send( player, NET_CHAT_CHAT, buf[bufno], strlen(buf[bufno]));
    }
}

/***************************************************************************
    external functions
***************************************************************************/

/* Enable windows socket messages */
void Network_TCPIP_poll_sockets(HWND hWnd, BOOL server)
{
    SOCKET s;

    This.hWndChat = hWnd;
    
    /* do WSAAsyncSelect() to get message from sockets */
    s = (server) ? This.sConnecting : This.sServer;
    
    WSAAsyncSelect(s, hWnd, WM_WINSOCK_SELECT, FD_READ | FD_ACCEPT | FD_CLOSE);
}

/* Turn off non-blocking mode for all sockets */
void Network_TCPIP_end_chat(void)
{
    unsigned long param = 0;

    dprintf(("Network_TCPIP_end_chat() called\n"))

    if (This.bClient)
    {
        WSAAsyncSelect(This.sServer, This.hWndChat, 0, 0);
        ioctlsocket (This.sServer, FIONBIO, &param);
    }
    else
    {
        int i;

        WSAAsyncSelect(This.sConnecting, This.hWndChat, 0, 0);

        for (i = 0; i < This.iNumClients; i++)
        {
            WSAAsyncSelect(This.sClient[i], This.hWndChat, 0, 0);
            ioctlsocket (This.sClient[i], FIONBIO, &param);
        }
        This.hWndChat = 0;
    }
    dprintf(("Network_TCPIP_end_chat() return\n"))
}

void OnWinsockSelect(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    static unsigned char buf[NET_MAX_PACKET];
    static int      size;
	int             nRc;
	int             error = WSAGETSELECTERROR(lParam);
	unsigned long   data = 0;
    int             numPlayers;
    int             i;

    if (error == 0)
    {
        switch(WSAGETSELECTEVENT(lParam))
        {
        case FD_ACCEPT:
            /* Add a new player */
            Sleep(100); /* With out this it hangs??? */
            if ((numPlayers = net_add_player()) != -1)
                This.iNumClients = numPlayers;
            break;
            
        case FD_READ:
            /* check to make sure we *really* have data waiting,
             * because the message-peek in the search for the
             * right socket could cause an extra FD_READ
             */

            nRc = ioctlsocket(wParam, FIONREAD, (u_long FAR *) &data);
            
            if(nRc == SOCKET_ERROR || data <= 0)
                return;

            if ( MAME32App.m_pNetwork != NULL )
            {
                size = NET_MAX_PACKET;

                memset(buf,'\0',NET_MAX_PACKET);

                if (This.bClient)
                {
                    MAME32App.m_pNetwork->chat_recv(0, buf,&size);
                }
                else
                {
                    for (i = 0; i < This.iNumClients; i++)
                    {
                        if (This.sClient[i] == (SOCKET)wParam)
                        {
                            MAME32App.m_pNetwork->chat_recv(i, buf, &size);
                            break;
                        }
                    }
                }
            }
            break;
            
        case FD_CLOSE:
            /* the other side closed the connection.  close the socket,
             * to clean up internal resources.
             */
            // MessageBox(0,"Remote closed socket","OnWinsockSelect", IDOK);
            for (i = 0; i < This.iNumClients; i++)
            {
                if (This.sClient[i] == (SOCKET)wParam)
                {
                    net_remove_player( i );
                    break;
                }
            }
            break;
        }
    }
	return;
}



#endif /* MAME_NET */
