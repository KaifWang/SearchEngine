// The Index Servers that recieves query messages from front end and 
// sends results message back to the front end

// Created by Kai Wang (kaifan@umich.edu)


#include <pthread.h>
#include <cstdlib>
#include <chrono>
#include <signal.h>
#include "../utility/socket_helper.h"
#include "../queryParser/queryParser.h"
#include "../configs/config.h"

using namespace std::chrono;

// Define message format to send top Documents back to the front end
// Vector<DocumentScore>
// seqNum@numDoc@url@title@abstract@score@url@tiltl@abstract@score ...
static string encodeMsg( vector< DocumentScore > &docScores, size_t seqNum )
   {
   // Encode seq number
   char seqBuf[ 10 ];
   memset( seqBuf, 0, sizeof( seqBuf ) );
   sprintf( seqBuf, "%lu", seqNum );
   string msg( seqBuf );

   // Encode numDocs
   char numDocBuf[ 10 ];
   memset( numDocBuf, 0, sizeof( numDocBuf ) );
   sprintf( numDocBuf, "%lu", docScores.size( ) );
   msg += DOCUMENT_MESSAGE_DELIMITER;
   msg += string( numDocBuf );

   for( size_t i = 0 ; i < docScores.size() ; i++ )
      {
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[ i ].title;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[ i ].abstract;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[ i ].URL;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      char scoreBuf[ 10 ];
      memset( scoreBuf, 0, sizeof( scoreBuf ) );
      sprintf( scoreBuf, "%lu", docScores[ i ].score );
      msg += string( scoreBuf );
      }
   return msg;
   }

// Search through index directory and combine results from all index chunks
// For each index chunk
// Pass it to query parser to transfrom into an ISR structure
// Then create a Ranker object that search through the index and rank the docs
// Combine results from all index chunks at the end by insertion sort
static vector< DocumentScore > retrieveResult( string query )
   {
   char queryWords[ MAX_QUERY_SIZE ];
   memset( queryWords, 0, sizeof( queryWords ) );
   strncpy( queryWords, query.c_str( ), MAX_QUERY_SIZE - 1 );
   vector< vector< DocumentScore > > allDocsScore;
   #ifdef DEBUG
   vector<vector<DebugScore> > allInfo;
   #endif

   string dirName = FINAL_INDEX;
   DIR* handle = opendir( dirName.c_str( ) );
   if ( handle )
      {
      struct dirent * entry;
      while ( ( entry = readdir( handle ) ) )
         {
         struct stat statbuf;
         string fileName = dirName + '/' + entry->d_name;
         if ( stat( fileName.c_str( ), &statbuf ) )
            cerr << dirName << " failed, errono = " << errno << endl;
         if( IndexFile ( fileName.c_str( ) ) )
            {
            HashFile indexChunk( fileName.c_str( ) );
            const HashBlob *hashblob = indexChunk.Blob( );
            QueryParser parser( queryWords, &indexChunk );
            vector< string > flatternWords = parser.GetFlattenedVector( );
            ISROr* parsedISRQuery = parser.getParsedISRQuery( );
			   Ranker ranker (parsedISRQuery, flatternWords, &indexChunk );
            ranker.RankDocuments( );
            allDocsScore.push_back( ranker.getTopDocuments( ) );
            #ifdef DEBUG
            allInfo.push_back(ranker.getDebugInfo());
            #endif
            }
         }
      }
   vector< DocumentScore > combinedTopDocs( NUM_TOP_DOCUMENTS_RETURNED );
   #ifdef DEBUG
   vector< DebugScore > combinedInfo( 3 );
   #endif
   for( size_t i = 0; i < allDocsScore.size( ); i ++ )
      {
      for( size_t j = 0; j < allDocsScore[ i ].size( ) ; j++ )
         {
         InsertionSort( allDocsScore[ i ][ j ], combinedTopDocs );
         #ifdef DEBUG
         InsertionSort( allInfo[ i ][ j ], combinedInfo );
         #endif
         }
      }
   #ifdef DEBUG
   for( size_t i = 0; i < combinedInfo.size( ); i++ )
      combinedInfo[ i ].print( );
   #endif
   return combinedTopDocs;
   }


static void* handleQueryMessage( void* arg )
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
   cout << "Msg received: " << msg << endl;
   close( connectionfd );
   if( msg == "" || msg.empty( ) )
      {
      cout << "ERROR: Receive messge " << msg << " with length " 
            << msg.length( ) << endl;
      return NULL;
      }
   // Retrieve seqNum and query
   size_t seqNum = 0;
   string query = "";
   for( size_t i = 0; i < msg.length( ) ; i++ )
      {
      if(msg[ i ] == '@')
         {
         seqNum = atoi( string( msg, 0, i ).c_str( ) );
         query = string( msg, i + 1, string::npos );
         break;
         }
      }
   if( query == "" || query.empty( ) )
      {
      cout << "ERROR: Receive messge " << msg << " with length " 
            << msg.length( ) << endl;
      return NULL;
      }
   cout << seqNum << " : " << query << endl;
   time_point< std::chrono::steady_clock > start, end;
   start = steady_clock::now( );
   vector< DocumentScore > result = retrieveResult( query );
   end = steady_clock::now( );
   duration< double > elapsed_seconds = end - start;
   cout << "Search index spends " << 
      elapsed_seconds.count( ) << " seconds" << endl;
   //Do no send the result back to front end if timeout
   if( elapsed_seconds.count( ) > SEARCH_INDEX_TIMEOUT - 3 ) 
   { 
   cout << "Time out: search takes more than " << 
         SEARCH_INDEX_TIMEOUT - 3 << "s" << endl;
   return NULL; 
   }
   string sendMsg = encodeMsg( result, seqNum );
   if( sendMessage( FRONT_END_IP, 6000, sendMsg ) == -1 )
      cout << "ERROR: Fail to send message back to front end." << endl;
   return NULL;
   }

static int startServer( int port ) 
   {
   int sockfd = setupServerSocekt( port );
   // (5) Serve incoming connections one by one forever.
   while ( true ) 
      {
      int* connectionfd = ( int* ) malloc( sizeof( int ) );
      *connectionfd = accept( sockfd, 0, 0 );
      if ( *connectionfd == -1 ) 
         {
         cout << "Error accepting connection" << endl;
         continue;
         }
      cout << "accepted" << endl;
      pthread_t child;
      //Should start a new thread here
      pthread_create( &child, NULL, handleQueryMessage, ( void * )connectionfd );
      pthread_detach( child );
      //handle_msg(connectionfd);
      }
   }

   int main( int argc, char* argv[ ] )
   {
   signal( SIGPIPE, SIG_IGN );
   startServer( COMMUNICATION_PORT );
   return 0;
   }