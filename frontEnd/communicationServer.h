// The Front End Communication Server that talks to index servers
// Recieves and parsed search result message from index Servers

// Created by Kai Wang (kaifan@umich.edu)

#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>

#include "../ranker/ranker.h"
#include "../configs/config.h"
#include "../utility/socket_helper.h"

// Helper for parsing messages
// return the next token of the stream, seperated by DOCUMENT_MESSAGE_DELIMITER

// MAX concurrent connection = 100, 
vector< vector< DocumentScore > > resultList( MAX_NUM_CONCURRENT_CONNECTION, vector< DocumentScore >( NUM_TOP_DOCUMENTS_RETURNED ) );
vector< bool > availableSeqNum( MAX_NUM_CONCURRENT_CONNECTION, true );
vector< size_t > numResponseList ( MAX_NUM_CONCURRENT_CONNECTION, 0 );
pthread_mutex_t resultListMutex = PTHREAD_MUTEX_INITIALIZER;
vector < pthread_cond_t > readerCvs( MAX_NUM_CONCURRENT_CONNECTION );

static char* getNextToken( const char* &itr )
   {
   const char* token_start = itr;
   while( *itr && *itr != DOCUMENT_MESSAGE_DELIMITER )
      itr ++;
   char* token = new char[ itr - token_start + 1 ];
   memset( token, 0, itr - token_start + 1 );
   strncpy( token, token_start, itr - token_start );
   itr ++;
   return token;
   } 

// Define message format to send top Documents back to the front end
// Vector<DocumentScore>
// seqNum@numDoc@url@title@abstract@score@url@tiltl@abstract@score ...
static vector< DocumentScore > decodeMsg( string msg, size_t &seqNum ) 
   {
   const char* itr = msg.c_str( );
   // Get sequence number
   seqNum = atoi( getNextToken( itr ) );
   //Read num of documents
   size_t numDoc = atoi( getNextToken( itr ) );
   vector< DocumentScore > docScores( numDoc );
   for( size_t i = 0 ; i < numDoc; i ++ )
      {
      char* title = getNextToken( itr );
      char* abstract = getNextToken( itr );
      char* url = getNextToken( itr );
      char* scoreStream = getNextToken(itr);
      size_t score = atoi( scoreStream );
      DocumentScore docScore{ string( title ), string( abstract ), 
            string( url ), score };
      docScores[ i ] = docScore;
      delete title;
      delete abstract;
      delete url;
      delete scoreStream;
      } 
   return docScores;
   }

static void* handleMessage( void* arg )
   {
   int connectionfd = *( ( int* ) arg );
   // (1) Receive message from client.
   string msg;
   char buffer[ 1 ]; //retrieve 1 byte at a time
   int bytes_recvd = 0;
   // Call recv() enough times to consume all the data the client sends.
   do 
      {
         // Receive as many additional bytes as we can in one call to recv()
         bytes_recvd = recv( connectionfd, buffer, 1, 0 );
         if( bytes_recvd < 0 ){ break; }
         msg += string( buffer, bytes_recvd );
      } while ( buffer[ 0 ] != '\0' );  // recv() returns 0 when client closes
   cout << "Message received from Index" << endl;
   close( connectionfd );
   size_t seqNum = 0;
   vector< DocumentScore > allDocsScore = decodeMsg( msg, seqNum ); 
   cout << "Top url: " << allDocsScore[ 0 ].URL << 
         " has score " << allDocsScore[ 0 ].score << endl;
   pthread_mutex_lock( &resultListMutex );
   for( size_t i = 0; i < allDocsScore.size( ); i ++ )
      InsertionSort( allDocsScore[ i ], resultList[ seqNum ] );
   numResponseList[ seqNum ] ++ ;
   cout << "response current size: " << numResponseList[ seqNum ]  << endl;
   if ( numResponseList[ seqNum ] >= NUM_INDEX_MACHINE )
      pthread_cond_broadcast( &readerCvs[ seqNum ] );
   pthread_mutex_unlock( &resultListMutex );
   delete arg;
   return NULL;
   }

static void* startCommunicationServer( void * )
   {
   int sockfd = setupServerSocekt( COMMUNICATION_PORT );
   // (5) Serve incoming connections one by one forever.
   while( true ) 
      {
      int* connectionfd = ( int* ) malloc( sizeof( int ) );
      *connectionfd = accept( sockfd, 0, 0 );
      if ( *connectionfd == -1 ) 
         {
         cout << "Error accepting connection" << endl;
         continue;
         }
      pthread_t child;
      pthread_create( &child, NULL, handleMessage, ( void * )connectionfd );
      pthread_detach( child );
      }
   }

static void DistributeMessage( string msg, size_t seqNum )
   {
   ifstream machineIPs( INDEX_IP_FILE );
   string serverName;
   // Constrcut sent query with sequence number;
   char seqBuf[ 10 ];
   memset(seqBuf, 0, sizeof( seqBuf ) );
   sprintf( seqBuf, "%lu", seqNum );
   string seqString( seqBuf );
   string sendMsg = seqString + "@" + msg;
   while( machineIPs >> serverName )
      sendMessage( serverName.c_str( ), COMMUNICATION_PORT, sendMsg );
   }


