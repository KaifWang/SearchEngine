// Helper class to create sockets for communication between index servers
// and front end servers

// Created by Kai Wang (kaifan@umich.edu)

#include <arpa/inet.h>		// htons(), ntohs()
#include <netdb.h>		// gethostbyname(), struct hostent
#include <netinet/in.h>		// struct sockaddr_in
#include <stdio.h>		// perror(), fprintf()
#include <string.h>		// memcpy()
#include <sys/socket.h>		// getsockname()
#include <unistd.h>		// stderr
#include <iostream>
#include <vector>

#define COMMUNICATION_PORT 6000
using namespace std;

int getPortNum( int sockfd ) 
	{
	struct sockaddr_in addr;
	socklen_t length = sizeof( addr );
	if ( getsockname( sockfd, ( sockaddr * ) &addr, &length ) == -1 ) 
		{
		perror( "ERROR getting port of socket" );
		return -1;
		}
	// Use ntohs to convert from network byte order to host byte order.
	return ntohs( addr.sin_port );
	}

int setupServerSocekt( int port )
	{
    int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfd == -1 ) 
		{
		perror( "ERROR opening stream socket" );
		return -1;
		}

	// (2) Set the "reuse port" socket option
	int yesval = 1;
	if ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, 
			&yesval, sizeof( yesval ) ) == -1 ) 
		{
		perror( "ERROR setting socket options" );
		return -1;
		}

	// (3) Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;
    addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( port );

	// (3b) Bind to the port.
	if ( ::bind( sockfd, ( sockaddr * ) &addr, sizeof( addr ) ) == -1 ) 
		{
		perror( "ERROR binding stream socket" );
		return -1;
		}

	// (3c) Detect which port was chosen.
	port = getPortNum( sockfd );
    cout << "\n@@@ port " << port << endl;
	printf( "Server listening on port %d...\n", port );

	// (4) Begin listening for incoming connections.
	listen( sockfd, 30 );
	return sockfd;
	}

int sendMessage( const char* hostname, int portNum, string message )
	{
    int sockfd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( sockfd == -1 ) 
		{
		perror( "ERROR opening stream socket" );
		return -1;
		}
    struct sockaddr_in serv_addr;
    struct hostent* server;
    server = gethostbyname( hostname );
    if ( server == NULL )
		{
        perror( "ERROR opening server" );
        return -1;
    	}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( ( u_short ) portNum );
    memcpy( &serv_addr.sin_addr, server->h_addr, ( size_t ) server->h_length );
    if( connect( sockfd,( struct sockaddr * )&serv_addr, 
			sizeof( serv_addr ) ) < 0 )
    	{
        perror( "ERROR fail to connect" );
        return -1;
    	}

    if ( send( sockfd, message.c_str( ), message.length( ) + 1, 0 ) == -1 ) 
		{
        perror( "fail to send msg to server\n" );
        return -1;
    	}
    close( sockfd );
	cout << "Message sent to " << hostname << endl;
    return sockfd;
	}