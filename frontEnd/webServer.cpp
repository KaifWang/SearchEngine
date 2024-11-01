// Linux tiny HTTP server.
// Nicole Hamilton  nham@umich.edu

// This variation of LinuxTinyServer supports a simple plugin interface
// to allow "magic paths" to be intercepted.  (But the autograder will
// not test this feature.)

// Linux tiny HTTP server.
// Zhicheng Huang skyhuang@umich.edu
// Leo Yan leoyan@umich.edu
// Nicole Hamilton  nham@umich.edu
// Kai Wang kaifan@umich.edu
// Patrick Su patsu@umich.edu

// This variation of LinuxTinyServer supports a simple plugin interface
// to allow "magic paths" to be intercepted.  (But the autograder will
// not test this feature.)

// Usage:  LinuxTinyServer port rootdirectory

// Compile with g++ -pthread LinuxTinyServer.cpp -o LinuxTinyServer
// To run under WSL (Windows Subsystem for Linux), may have to elevate
// with sudo if the bind fails.

// LinuxTinyServer does not look for default index.htm or similar
// files.  If it receives a GET request on a directory, it will refuse
// it, returning an HTTP 403 error, access denied.

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <cassert>
#include <math.h>
#include <signal.h>
#include "communicationServer.h"
#include "../ranker/ranker.h"

// MAX concurrent connection = 100, 
extern vector<vector<DocumentScore> > resultList;
extern vector<size_t> numResponseList;
extern pthread_mutex_t resultListMutex;
extern pthread_mutex_t seqNumMutex;
extern vector<pthread_cond_t> readerCvs;
//#include <openssl/ssl.h> // ssl connection
using namespace std;

 // The constructor for any plugin should set Plugin = this so that
 // LinuxTinyServer knows it exists and can call it.


// helper function for filteredWord
static bool symbolCheck( char c )
   {
   if ( c == '!' )
      return true;
   if ( c == '@' )
      return true;
   if ( c == '#' )
      return true;
   if ( c == '$' )
      return true;
   if ( c == '%' )
      return true;
   if ( c == '^' )
      return true;
   if ( c == '*' )
      return true;

   // In case we need some of them

   if ( c == '{' )
      return true;
   if ( c == '}' )
      return true;
   if ( c == '[' )
      return true;
   if ( c == ']' )
      return true;
   if ( c == '<' )
      return true;
   if ( c == '>' )
      return true;
   
   if ( c == ':' )
      return true;
   if ( c == ';' )
      return true;
   if ( c == '?' )
      return true;
   if ( c == ',' )
      return true;
   if ( c == '.' )
      return true;

   // change if unary operator is implemented in the parser
   if ( c == '-' )
      return true;
   if ( c == '+' )
      return true;
   return false;
   }


// helper function for queryParser constructor
// Filter out the invalid chars for parser
static char *filteredWord( char *input )
   {
   char* output = new char[ strlen( input ) + 1];
   char* result = output;
   char* curr = input;
   char* end = input + strlen( input );

   while ( curr != end )
      {
      // To lower or otherwise copy
      if (  *curr >= 'A' &&  *curr <= 'Z' )
         *output = 'a' - 'A' +  *curr;
      else
         *output = *curr;

      // Special Case 1: And
      // one space before, one space after, otherwise replace with whitespace
		// quick&& fox will be quick   fox
		// quick &fox will be quick  fox
		// quick && fox will be quick && fox
      if ( *curr == '&' )
         {
         // && Case
         if ( ( curr + 2 ) &&  *( curr + 1 ) == '&' )
            {
            // check whether to "destroy" i.e. set whitespace chars
            if (  *( curr + 2 ) != ' ' ||  *( curr - 1 ) != ' ' )
               {
               *output = ' ';
               *( output + 1 ) = ' ';
               }
            else
               *( output + 1 ) = *( curr + 1 );
            ++output;
            ++curr;
            }
         // & Case
         // check whether to "destroy" i.e. set whitespace chars
         else if ( ( curr + 1 ) &&  *( curr + 1 ) != ' ' ||  *( curr - 1 ) != ' ' )
            *output = ' ';
         }

      // Special Case 2: Or
		// one space before, one space after, otherwise replace with whitespace
		// quick|| fox will be quick   fox
		// quick |fox will be quick  fox
		// quick || fox will be quick | fox
      if ( *curr == '|' )
         {
         // || Case
         if ( ( curr + 2 ) && *( curr + 1 ) == '|' )
            {
            // check whether to "destroy" i.e. set whitespace chars
            if ( *( curr + 2 ) != ' ' || *( curr - 1 ) != ' ' )
               {
               *output = ' ';
               *( output + 1 ) = ' ';
               }
            else
               *( output + 1 ) = *( curr + 1 );
            ++output;
            ++curr;
            }
         // | Case
         // check whether to "destroy" i.e. set whitespace chars
         else if ( ( curr + 1 ) &&  *( curr + 1 ) != ' ' ||  *( curr - 1 ) != ' ' )
            *output = ' ';
         }


      // Special Case 3: ( )
      // "Sky(" <-- invalid " (Sky" <-- valid
      // ")Sky" <-- invalid  "Sky) " <-- valid
      // Note: Be careful with boundary
      // "(hello world)" is valid even though there are no space before (
      // and no space after )
      if ( curr != input &&  *curr == '(' )
         {
         if ( *( curr - 1 ) != ' ' )
            *output = ' ';
         }
      
      if ( curr + 1 != end &&  *curr == ')' )
         {
         if ( ( curr + 1 ) &&  *( curr + 1 ) != ' ' )
            *output = ' ';
         }

		// Do nothing for \"

      // Only include special symbols that parser can recognize
		// #quick $fox% will be treated as quick fox
      if ( symbolCheck( *curr ) )
         *output = ' ';

      ++curr;
      ++output;
      }
	result[strlen(input)] = '\0';
   return result;
   }



struct TalkData
   {
   int* clientSocket;
   string rootDir;
   };
// Root directory for the website, taken from argv[ 2 ].
// (Yes, a global variable since it never changes.)
char *RootDirectory;

//  Multipurpose Internet Mail Extensions (MIME) types
struct MimetypeMap
   {
   const char *Extension, *Mimetype;
   };

const MimetypeMap MimeTable[ ] =
   {
   // List of some of the most common MIME types in sorted order.
   // https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
   ".3g2",     "video/3gpp2",
   ".3gp",     "video/3gpp",
   ".7z",      "application/x-7z-compressed",
   ".aac",     "audio/aac",
   ".abw",     "application/x-abiword",
   ".arc",     "application/octet-stream",
   ".avi",     "video/x-msvideo",
   ".azw",     "application/vnd.amazon.ebook",
   ".bin",     "application/octet-stream",
   ".bz",      "application/x-bzip",
   ".bz2",     "application/x-bzip2",
   ".csh",     "application/x-csh",
   ".css",     "text/css",
   ".csv",     "text/csv",
   ".doc",     "application/msword",
   ".docx",    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
   ".eot",     "application/vnd.ms-fontobject",
   ".epub",    "application/epub+zip",
   ".gif",     "image/gif",
   ".htm",     "text/html",
   ".html",    "text/html",
   ".ico",     "image/x-icon",
   ".ics",     "text/calendar",
   ".jar",     "application/java-archive",
   ".jpeg",    "image/jpeg",
   ".jpg",     "image/jpeg",
   ".js",      "application/javascript",
   ".json",    "application/json",
   ".mid",     "audio/midi",
   ".midi",    "audio/midi",
   ".mpeg",    "video/mpeg",
   ".mpkg",    "application/vnd.apple.installer+xml",
   ".odp",     "application/vnd.oasis.opendocument.presentation",
   ".ods",     "application/vnd.oasis.opendocument.spreadsheet",
   ".odt",     "application/vnd.oasis.opendocument.text",
   ".oga",     "audio/ogg",
   ".ogv",     "video/ogg",
   ".ogx",     "application/ogg",
   ".otf",     "font/otf",
   ".pdf",     "application/pdf",
   ".png",     "image/png",
   ".ppt",     "application/vnd.ms-powerpoint",
   ".pptx",    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
   ".rar",     "application/x-rar-compressed",
   ".rtf",     "application/rtf",
   ".sh",      "application/x-sh",
   ".svg",     "image/svg+xml",
   ".swf",     "application/x-shockwave-flash",
   ".tar",     "application/x-tar",
   ".tif",     "image/tiff",
   ".tiff",    "image/tiff",
   ".ts",      "application/typescript",
   ".ttf",     "font/ttf",
   ".vsd",     "application/vnd.visio",
   ".wav",     "audio/x-wav",
   ".weba",    "audio/webm",
   ".webm",    "video/webm",
   ".webp",    "image/webp",
   ".woff",    "font/woff",
   ".woff2",   "font/woff2",
   ".xhtml",   "application/xhtml+xml",
   ".xls",     "application/vnd.ms-excel",
   ".xlsx",    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
   ".xml",     "application/xml",
   ".xul",     "application/vnd.mozilla.xul+xml",
   ".zip",     "application/zip"
   };


const char *Mimetype( const string filename )
   {
   // TODO: if a matching a extentsion is found return the corresponding
   // MIME type.
      const char * filenameC = filename.c_str( );
      char *extension = new char [100];
      char * temp = extension;
      const char * last = &filenameC[ strlen( filenameC ) - 1 ];
      const char * cur = last;
      while ( *cur != '.' )
         cur--;
      while ( cur <= last )
         *temp++ = *cur++;
      *temp = '\0';
      // caller handle delete
      char * MimeType = new char[ 100 ];
      for ( auto& Mime: MimeTable )
         {
         if ( !strcmp( Mime.Extension, extension ) )
            strcpy( MimeType, Mime.Mimetype );
         }
      if ( strlen( MimeType ) )
         return MimeType;
   delete[ ] extension;
      
   // Anything not matched is an "octet-stream", treated
   // as an unknown binary, which can be downloaded.
   return "application/octet-stream";
   }


int HexLiteralCharacter( char c )
   {
   // If c contains the Ascii code for a hex character, return the
   // binary value; otherwise, -1.

   int i;

   if ( '0' <= c && c <= '9' )
      i = c - '0';
   else
      if ( 'a' <= c && c <= 'f' )
         i = c - 'a' + 10;
      else
         if ( 'A' <= c && c <= 'F' )
            i = c - 'A' + 10;
         else
            i = -1;

   return i;
   }


string UnencodeUrlEncoding( string &path )
   {
   // Unencode any %xx encodings of characters that can't be
   // passed in a URL.

   // (Unencoding can only shorten a string or leave it unchanged.
   // It never gets longer.)
   const char *start = path.c_str( ), *from = start;
   string result;
   char c, d;
   while ( ( c = *from++ ) != 0 )
      if ( c == '%' )
         {
         c = *from;
         if ( c )
            {
            d = *++from;
            if ( d )
               {
               int i, j;
               i = HexLiteralCharacter( c );
               j = HexLiteralCharacter( d );
               if ( i >= 0 && j >= 0 )
                  {
                  from++;
                  result += ( char )( i << 4 | j );
                  }
               else
                  {
                  // If the two characters following the %
                  // aren't both hex digits, treat as
                  // literal text.

                  result += '%';
                  from--;
                  }
               }
            }
         }
      else
         result += c;

   return result;
   }


bool SafePath( const char *path )
   {
   // must start with slash
   if ( *path != '/' )
      return false;
   // The path must start with a /.
   const char *c = path;
   int depth = 0;
   while ( *c )
      {
      if ( *c == '/' )
         depth++;
      else if ( *c == '.' && *( c - 1 ) == '.' )
         depth -= 2;
      if ( depth < 0 )
         return false;
      c++;
      }
   return true;
   }

// return is input is all whitespace or empty
bool isEmpty( string input )
   {
   const char *cur = input.c_str( );
   while ( *cur++ )
      {
      if ( *cur != ' ' )
         return false;
      }
   return true;
   }


off_t FileSize( int f )
   {
   // Return -1 for directories.
   struct stat fileInfo;
   fstat( f, &fileInfo );
   if ( ( fileInfo.st_mode & S_IFMT ) == S_IFDIR )
      return -1;
   return fileInfo.st_size;
   }


void AccessDenied( int talkSocket )
   {
   const char accessDenied[ ] = "HTTP/1.1 403 Access Denied\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";

   cout << accessDenied;
   send( talkSocket, accessDenied, sizeof( accessDenied ) - 1, MSG_NOSIGNAL );
   }

   
void FileNotFound( int talkSocket )
   {
   const char fileNotFound[ ] = "HTTP/1.1 404 Not Found\r\n"
         "Content-Length: 0\r\n"
         "Connection: close\r\n\r\n";
   cout << fileNotFound;
   send( talkSocket, fileNotFound, sizeof( fileNotFound ) - 1, MSG_NOSIGNAL );
   }

   // return if the path matches a searching query
   bool IsSearching( string path )
   {
      return *( path.c_str( ) + 1 ) == '?';
   }

// get page number when displaying result
int getPage( string path )
   {
   const char * numStart = ( path.c_str( ) + 2 );
   const char * numEnd = numStart;
   while ( *numEnd != '?' )
      numEnd++;
   int numLength = numEnd - numStart;
   string pageString = path.substr( 2, numLength );
   return stoi( pageString );
   }

// return the using input for a query
string SearchingInput ( string path )
   {
   const char * begin = path.c_str( ) + 2;
   const char * cur = begin;
   while ( *cur != '?' )
      cur++;
   return string( path.c_str( ) +  ( cur - path.c_str( ) + 1 ) );
   }

int QueryTooLong( string input, int clientSocket, string longQuery )
   {
   longQuery.append(" <div id = \"too-many\">" );
   longQuery.append( "<span>For the efficiency of the search, please limit your query length to " );
   longQuery.append( to_string( MAX_QUERY_SIZE ) + " characters long</span><br><br>" );

   longQuery.append( "<div class=\"result\">" );
   longQuery.append( "<h2><a href=\"" );
   longQuery.append( "https://lsa.umich.edu/sweetland/undergraduates/writing-support.html" );
   longQuery.append( "\">" );
   longQuery.append( "If it is hard to condense your query, here is a good place to get help" );
   longQuery.append( "</a></h2>" );
   longQuery.append( "<h3>" );
   longQuery.append( "https://lsa.umich.edu/sweetland/undergraduates/writing-support.html" );
   longQuery.append( "</h3>" );
   longQuery.append( "<p>" );
   longQuery.append( "Sweetland Center for Writing" );
   longQuery.append( "..." );
   longQuery.append( "</p>" );
   longQuery.append( "</div></div>" );

   size_t htmlSize = longQuery.length( ) + 1;
   char header[ 200 ];
   const char * type = "text/html";
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )htmlSize, type );

   // send the content of buffer to the socketFD
   int sendHeader = send( clientSocket, header, strlen(header), MSG_NOSIGNAL);
   int sendContent = send( clientSocket, longQuery.c_str( ), htmlSize, MSG_NOSIGNAL );
   if ( sendContent < 0 )
      {
      cerr << "Error sending header\n";
      return -1;
      }
   else
      {
      cout << "Successfully send SEARCH RESULTS to client\n";
      return 0;
      }
   }

int TooMany( string input, int clientSocket, int numResults, string TooManyContent )
   {
   TooManyContent.append( "<div id = \"too-many\">"
                           "<span>To improve your experience, we recorded the top " + to_string( numResults ) + " results and "
                           "omitted the less relevent ones</span><br>"
                           "<span>For your search - </span>"
                           "<span class = \"too-many-input\">" + input + "</span>"
                           "<p>Please head back to the previous pages or start a new search</p>"
                           "</div>" );
   
   size_t htmlSize = TooManyContent.length( ) + 1;
   char header[ 200 ];
   const char * type = "text/html";
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )htmlSize, type );

   // send the content of buffer to the socketFD
   int sendHeader = send( clientSocket, header, strlen(header), MSG_NOSIGNAL );
   int sendContent = send( clientSocket, TooManyContent.c_str( ), htmlSize, MSG_NOSIGNAL );
   if ( sendContent < 0 )
      {
      cerr << "Error sending header\n";
      return -1;
      }
   else
      {
      cout << "Successfully send SEARCH RESULTS to client\n";
      return 0;
      }
   }

int WhiteSpace( int clientSocket, string WhiteSpaceContent )
   {
   WhiteSpaceContent.append("<div id = \"too-many\">"
                           "<span>Your search contains all whitespaces<br>"
                           "<span><b>Please enter a meaningful search query</b></span>");
   WhiteSpaceContent.append( "</div>" );
   
   size_t htmlSize = WhiteSpaceContent.length( ) + 1;
   char header[ 200 ];
   const char * type = "text/html";
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )htmlSize, type );

   // send the content of buffer to the socketFD
   int sendHeader = send( clientSocket, header, strlen(header), MSG_NOSIGNAL );
   int sendContent = send( clientSocket, WhiteSpaceContent.c_str( ), htmlSize, MSG_NOSIGNAL );
   if ( sendContent < 0 )
      {
      cerr << "Error sending header\n";
      return -1;
      }
   else
      {
      cout << "Successfully send SEARCH RESULTS to client\n";
      return 0;
      }
   }

int NoResult( string input, int clientSocket, string noResultContent )
   {
   noResultContent.append( "<div id = \"noresult\">"
                     "<span>Your search - </span>"
                     "<span class = \"noresult-input\">" + input + "</span>"
                     "<span>- does not match any documents.</span>"
                     "<p>Suggestions:</p>"
                     "<li>Make sure all words are spelled correctly.</li>"
                     "<li>Try different keywords.</li>"
                     "<li>Try more general keywords.</li>"
                     "</div>" );
   
   size_t htmlSize = noResultContent.length( ) + 1;
   char header[ 200 ];
   const char * type = "text/html";
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )htmlSize, type );

   // send the content of buffer to the socketFD
   int sendHeader = send( clientSocket, header, strlen(header), MSG_NOSIGNAL );
   int sendContent = send( clientSocket, noResultContent.c_str( ), htmlSize, MSG_NOSIGNAL );
   if ( sendContent < 0 )
      {
      cerr << "Error sending header\n";
      return -1;
      }
   else
      {
      cout << "Successfully send SEARCH RESULTS to client\n";
      return 0;
      }
   }

// return the number of valid results in a result vector (that is not null)
int numValidResults( vector<DocumentScore> results)
   {
   int counter = 0;
   for ( auto dd: results )
      {
      if ( dd.title.length( ) != 0 && dd.abstract.length( ) != 0 )
         counter++;
      }
   return counter;
   }

// handles search
int HandleSearch ( string input, int clientSocket, int numPage )
   {
   string htmlContent =
               "<!DOCTYPE html>"
                "  <html>"

                "  <head>"
                "      <meta charset=\"UTF-8\">"
                "      <meta name=\"viewpoint\" content=\"width=device-width, initial-scale=1.0\">"
                "      <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">"
                "      <link rel=\"stylesheet\" href=\"css/style.css\">"
                "      <title>Search Us</title>"
                "  </head>"
                "  <body>";
   htmlContent.append( "<form id=\"search\" action=\"/?1?\" method=\"get\" target=\"\""
            "onsubmit=\"location.href = this.action + this.searchinput.value; return false;\">"
            "<h2><a href=\"/index.html\">Search Us</a></h2>"
            "<div id=\"header\" type=\"text\">"
            "<input id=\"searchinput\" type=\"text\" name = \"searchContent\" value=\"" );
   //htmlContent.append( input );
   string inputTmp = input;
   size_t index = 0;
   while ( true ) {
      /* Locate the substring to replace. */
      index = inputTmp.find( "\"", index );
      if (index == std::string::npos) break;
      /* Make the replacement. */
      inputTmp.replace( index, 1, "&quot;" );
      /* Advance index forward */
      index += 6;
   }
   htmlContent.append( inputTmp );
   htmlContent.append( "\" required><button type=\"submit\" id=\"search-button\">Search</button>" );
   htmlContent.append( "</form></div>" );

   // if input evaluates to empty
   if ( isEmpty( input ) )
      return WhiteSpace( clientSocket, htmlContent );

   string longQuery;
   // notify user when their query is too long
   if ( input.length( ) + 1 >= MAX_QUERY_SIZE )
      return QueryTooLong( input, clientSocket, htmlContent );

   // clean up input
   string inputCopy = input;
   char *inputClean = filteredWord( const_cast<char*>( inputCopy.c_str( ) ) );
   string inputCleanString( inputClean );
   delete inputClean;
   // Wait for at most 50 seconds for index servers to respond
   pthread_mutex_lock( &resultListMutex );

   size_t seqNum = 0;
   // Find lowest seq number availble
   for( size_t i = 0; i < availableSeqNum.size( ) ; i ++ )
   {
      if( availableSeqNum[ i ] )
      {
         seqNum = i;
         availableSeqNum[ i ] = false;
         break;
      }
   }

   //send queries to all index servers;
   DistributeMessage( inputCleanString, seqNum );
   cout << "Sucessfuly distribute message." << endl;
   struct timespec timeToWait;
   clock_gettime( CLOCK_REALTIME, &timeToWait );
   timeToWait.tv_sec += SEARCH_INDEX_TIMEOUT;
   while( numResponseList[seqNum] < NUM_INDEX_MACHINE )
      {
      int rt = pthread_cond_timedwait( &readerCvs[ seqNum ], &resultListMutex, &timeToWait );
      if( rt == ETIMEDOUT )
         {
         cout << "Timeout on wait" << endl;
         break;
         }
      }
   cout << "results received from " << numResponseList[ seqNum ] << " index" << endl;
   vector<DocumentScore> rawResults = resultList[ seqNum ];

   // Reset all variables after this request is finished
   availableSeqNum[ seqNum ] = true;
   numResponseList[ seqNum ] = 0;
   resultList[seqNum] = vector<DocumentScore>( NUM_TOP_DOCUMENTS_RETURNED );
   pthread_mutex_unlock( &resultListMutex );

   int numResults = numValidResults( rawResults );
   // when no result
   if ( numResults == 0 )
      return NoResult( input, clientSocket, htmlContent );
   // if numPage exceeds the number of results we have
   if ( numPage > ceil( ( double )numResults / RESULTS_PER_PAGE ) )
      return TooMany( input, clientSocket, numResults, htmlContent );
   // show results
   htmlContent.append( "<div id=\"search-result\">" );
   int resultStart = ( numPage - 1 ) * RESULTS_PER_PAGE;
   int resultEnd = ( numResults < resultStart + RESULTS_PER_PAGE ) ? numResults : resultStart + RESULTS_PER_PAGE;
   for ( int i = resultStart; i < resultEnd; ++i )
      {
      DocumentScore r = rawResults[ i ];
      htmlContent.append( "<div class=\"result\">" );
      htmlContent.append( "<h2><a href=\"" );
      htmlContent.append( r.URL );
      htmlContent.append( "\">" );
      htmlContent.append( r.title );
      htmlContent.append( "</a></h2>" );
      htmlContent.append( "<h3>" );
      htmlContent.append( r.URL );
      htmlContent.append( "</h3>" );
      htmlContent.append( "<p>" );
      htmlContent.append( r.abstract );
      htmlContent.append( "..." );
      htmlContent.append( "</p>" );
      htmlContent.append( "</div>" );
      }
   htmlContent.append( "</div>");

   // notify user we omit less relevant results at the last valid page
   if ( numPage == ceil ( ( double )numResults / RESULTS_PER_PAGE ) )
   {
      htmlContent.append( "<div id = \"too-many\">"
                           "<span>To improve your experience, we recorded the top " + to_string( numResults ) + " results and "
                           "omitted the less relevent ones</span><br>"
                           "<span>For your search - </span>"
                           "<span class = \"too-many-input\">" + input + "</span>"
                           "<p>Please head back to the previous pages or start a new search</p>"
                           "</div>" );
   }

   // page
   string htmlEnd = "<div class=\"page\"><ul>";
   if ( numPage != 1 )
      {
      htmlEnd.append( "<li class=\"previous\">");
      htmlEnd.append( "<a href=\"/?" + to_string( numPage - 1 ) + "?" + input + "\">" );
      htmlEnd.append( "Previous</a></li>");
      }
   for ( int i = 1; i <= ceil( ( double )numResults / RESULTS_PER_PAGE ); ++i )
      {
      htmlEnd.append( "<li><a href=\"/?" + to_string( i ) + "?" + input + "\">");
      if ( numPage == i )
         htmlEnd.append( "<b>" );
      htmlEnd.append( to_string( i ) );
      if ( numPage == i )
         htmlEnd.append( "</b>" );
      htmlEnd.append( "</a></li>" );
      }
   if ( numPage != ceil ( ( double )numResults / RESULTS_PER_PAGE ) )
      {
      htmlEnd.append( "<li class=\"next\">");
      htmlEnd.append( "<a href=\"/?" + to_string( numPage + 1 ) + "?" + input + "\">" );
      htmlEnd.append( "Next</a></li>");
      }
   htmlEnd.append( "</ul></div>" );
   htmlContent.append( htmlEnd );
   htmlContent.append( "</body></html>" );
   
   // send back to user
   size_t htmlSize = htmlContent.length( ) + 1;
   char header[ 200 ];
   memset(header, 0, sizeof( header ) );
   const char * type = "text/html";
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )htmlSize, type );
   // send the content of buffer to the socketFD
   int sendHeader = send( clientSocket, header, strlen(header), MSG_NOSIGNAL );
   int sendContent = send( clientSocket, htmlContent.c_str( ), htmlSize, MSG_NOSIGNAL );
   if ( sendHeader < 0 )
      cerr << "Error sending header\n";
   if ( sendContent < 0 )
      cerr << "Error sending fileContent\n";
   else
      cout << "Successfully send SEARCH RESULTS to client\n";
   return 0;
   }

// Look for a GET message, then reply with the
// requested file.
void *Talk( void *talkData )
   {
   // Cast from void * to int * to recover the talk socket id
   // then delete the copy passed on the heap.
   TalkData* td = (TalkData*) talkData;
   int* socketFD = td->clientSocket;
   string rootDirectory = td->rootDir;
   cout << "Talk starts with " << *socketFD << endl;
   // Read the request from the socket and parse it to extract
   // the action and the path, unencoding any %xx encodings.
   char buffer[ 10240 ];
   int bytes;
   bytes = recv( *socketFD, ( void* )buffer, sizeof( buffer ), 0 );
   if ( bytes < 0 )
      cerr << "Error recv message from client\n";
   // extract action and path
   char *left = buffer;
   char *right = left;
   char temp[200];
   char * cur = temp;
   while ( *right != ' ' )
      *cur++ = *right++;
   *cur = '\0';
   string action( temp );
   
   while ( *right == ' ' )
      right++;
   cur = temp;
   while ( *right != ' ' )
      *cur++ = *right++;
   *cur = '\0';
   string path( temp );
      
   // check if path is search
   if ( IsSearching( path ) )
      {
      string userInput = SearchingInput ( path );
      userInput = UnencodeUrlEncoding( userInput );
      HandleSearch( userInput, *socketFD, getPage( path ) );
      close( *socketFD );
      delete socketFD;
      delete td;
      return nullptr;
      }
      
   // unencoding path
   path = UnencodeUrlEncoding( path );
   string fullPath = rootDirectory + path;
   path = UnencodeUrlEncoding( fullPath );
   cout << "Actual path = " << fullPath << endl;

   // If it isn't intercepted, action must be "GET" and
   // the path must be safe.
    if ( action != "GET" || !SafePath( path.c_str( ) ) )
      {
      AccessDenied( *socketFD );
      close( *socketFD );
      delete socketFD;
      delete td;
      return nullptr;
      }
   
   // If the path refers to a directory, access denied.
   struct stat statbuf;
   if ( stat( fullPath.c_str( ), &statbuf ) )
      cerr << "stat of " << fullPath << " failed, errno = " << errno << endl;
   if ( ( statbuf.st_mode & S_IFDIR ) == S_IFDIR )
      {
      AccessDenied( *socketFD );
      close( *socketFD );
      delete socketFD;
      delete td;
      return nullptr;
      }
   
   
   // Open the file
   // If the path refers to a file, write it to the socket.
   int fileID = open( fullPath.c_str( ), O_RDONLY );
   if ( fileID < 0 )
      cerr << "open file failed with errno " << errno << endl;
   char * fileContent = new char[ 2048000 ];
   // map its contents into memory
   auto fileSize = FileSize( fileID );
   char* map = ( char* )mmap( NULL, fileSize, PROT_READ, MAP_SHARED, fileID, 0 );

   if ( map == MAP_FAILED || fileID < 0 )
      {
      FileNotFound( *socketFD );
      close( *socketFD );
      delete socketFD;
      delete td;
      return nullptr;
      }

   char header[ 200 ];
   memset( header, 0, 200 );
   const char * type = Mimetype( fullPath );
   sprintf( header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: closes\r\nContent-Type: %s\r\n\r\n", ( int )fileSize, type );
   // delete type as caller
   if ( strcmp( type, "application/octet-stream" ) != 0 )
      delete [ ] type;
   close( fileID );
   // send the content of buffer to the socketFD
   int sendHeader = send( *socketFD, header, strlen(header), 0 );
   int sendContent = send( *socketFD, map, fileSize, 0 );
   if ( sendHeader < 0 )
      cerr << "Error sending header\n";
   if ( sendContent < 0 )
      cerr << "Error sending fileContent\n";
   cout << "Successfully send to client\n";
   munmap( map, fileSize );
   // Close the socket and return nullptr and delete talk socket as child thread
   delete[ ] fileContent;
   close( *socketFD );
   delete socketFD;
   delete td;
   return nullptr;
   }



int main( int argc, char **argv )
   {
   signal(SIGPIPE, SIG_IGN);
   if ( argc != 3 )
      {
      cerr << "Usage:  " << argv[ 0 ] << " port rootdirectory" << endl;
      return 1;
      }

   int port = atoi( argv[ 1 ] );
   RootDirectory = argv[ 2 ];

   // Initalize all readerCVs
   for( size_t i = 0; i < readerCvs.size( ) ; i++ )
      pthread_cond_init( &readerCvs[ i ], NULL );
   // Start the communication server to receive data from index server
   pthread_t communicationServerThread;
   pthread_create( &communicationServerThread, nullptr, startCommunicationServer, nullptr );
   pthread_detach( communicationServerThread );
   
   // Discard any trailing slash.  (Any path specified in
   // an HTTP header will have to start with /.)
   char *r = RootDirectory;
   if ( *r )
      {
      do
         r++;
      while ( *r );
      r--;
      if ( *r == '/' )
         *r = 0;
      }
   // We'll use two sockets, one for listening for new
   // connection requests, the other for talking to each
   // new client.

   int listenSocket, talkSocket;

   // Create socket address structures to go with each
   // socke
   struct sockaddr_in listenAddress,  talkAddress;
   socklen_t talkAddressLength = sizeof( talkAddress );
   memset( &listenAddress, 0, sizeof( listenAddress ) );
   memset( &talkAddress, 0, sizeof( talkAddress ) );
   
   // Fill in details of where we'll listen.
   
   // We'll use the standard internet family of protocols.
   listenAddress.sin_family = AF_INET;

   // htons( ) transforms the port number from host (our)
   // byte-ordering into network byte-ordering (which could
   // be different).
   listenAddress.sin_port = htons( port );
   

   // INADDR_ANY means we'll accept connections to any IP
   // assigned to this machine.
   listenAddress.sin_addr.s_addr = htonl( INADDR_ANY );

   // Create the listenSocket, specifying that we'll r/w
   // it as a stream of bytes using TCP/IP.
   listenSocket = socket( AF_INET, SOCK_STREAM, 0 );

   // Bind the listen socket to the IP address and protocol
   // where we'd like to listen for connections.
   if( ( ::bind( listenSocket, ( struct sockaddr* ) &listenAddress, sizeof( struct sockaddr_in ) ) ) < 0 )
      {
      cerr << "error binding" << endl;
      cerr << errno << endl;
      }

   // Begin listening for clients to connect to us.
   listen(listenSocket, SOMAXCONN);
   cout << "Listening on " << port << endl;
   // The second argument to listen( ) specifies the maximum
   // number of connection requests that can be allowed to
   // stack up waiting for us to accept them before Linux
   // starts refusing or ignoring new ones.
   //
   // SOMAXCONN is a system-configured default maximum socket
   // queue length.  (Under WSL Ubuntu, it's defined as 128
   // in /usr/include/x86_64-linux-gnu/bits/socket.h.)

   // Accept each new connection and create a thread to talk with
   // the client over the new talk socket that's created by Linux
   // when we accept the connection.
   while ( ( talkSocket = accept( listenSocket, ( struct sockaddr* ) &talkAddress, &talkAddressLength)) && talkSocket != -1)
      {
      // Create and detach a child thread to talk to the
      // client using pthread_create and pthread_detach.
      pthread_t child;
      // When creating a child thread, you get to pass a void *,
      // usually used as a pointer to an object with whatever
      // information the child needs.
      TalkData* td = new TalkData;
      td->clientSocket = new int (talkSocket);
      td->rootDir = RootDirectory;
      pthread_create( &child, nullptr, Talk, td );
      pthread_detach( child );
      // The talk socket is passed on the heap rather than with a
      // pointer to the local variable because we're going to quickly
      // overwrite that local variable with the next accept( ).  Since
      // this is multithreaded, we can't predict whether the child will
      // run before we do that.  The child will be responsible for
      // freeing the resource.  We do not wait for the child thread
      // to complete.
      //
      // (A simpler alternative in this particular case would be to
      // caste the int talksocket to a void *, knowing that a void *
      // must be at least as large as the int.  But that would not
      // demonstrate what to do in the general case.)
      }
   close( listenSocket );
   }
