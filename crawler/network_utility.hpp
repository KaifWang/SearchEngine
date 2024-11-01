//
// network_utility.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Some basic network utility functions
//

/*
 * NOTE: CONTACT XIAO SONG IF YOU NEED TO CHANGE ANY PART OF THIS FILE.
 * CRAWLER HAVE A HEAVY DEPENDENCY ON THIS FILE
 */

#pragma once

#include <cassert> // assert
#include <iostream>
#include <pthread.h> // concurrent for lock, etc
#include <openssl/ssl.h> // ssl connection
#include <openssl/err.h> // ssl error
#include <sys/types.h>   // socket connction
#include <sys/socket.h>  // socket connection
#include <netdb.h>       // dns resolve
#include <unistd.h>      // open, close socket
#include <fcntl.h>
#include <cxxabi.h>  // abi::__forced_unwind& 

#include "configs/crawler_cfg.hpp" // configuration
#include "url_parser.hpp"          // url parser
#include "raii_fh.hpp"         // socket fh raii
#include "raii_ssl.hpp"       // ssl raii
#include "raii_thread_mtx.hpp" // mtx raii
#include "url_fixer.hpp"

// Lock for getaddrinfo
// Note:
//    On FreeBDS, MacOS, getaddinfo is not thread save
pthread_mutex_t* get_tcp_gloabl_mtx()
   {
   static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; /* ONLY RUN ONCE */
   return &mtx;
   }

pthread_mutex_t* get_ssl_gloabl_mtx()
   {
   static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; /* ONLY RUN ONCE */
   return &mtx;
   }

// Make TCP Socket Connection & Return Valid Socket File Handler
// Return:
//    -1 : making tcp connection failed (incure any error during connection)
//    sockFd (positive number) : make tcp connection success
// Caller:
//    caller should in charge of close the socket file handler
// Reference: lib/tcp_connect.c
// Note:
//    1. socket connection have timeout of 10 second, if no connection is made, consider this invalid socket
//    2. this function hold get_tcp_gloabl_mtx lock through its lifetime
int tcp_connect ( const char* host, const char* serv )
   {
   // Doc:
   // getaddrinfo: https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html
   // get_strerror https://man7.org/linux/man-pages/man3/gai_strerror.3p.html#:~:text=The%20gai_strerror%20%28%29%20function%20shall%20return%20a%20text,getnameinfo%20%28%29%20functions%20listed%20in%20the%20%3Cnetdb.h%3E%20header.
   // get_strerror is NOT thread safe

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (tcp_connect) try to build tcp connect on host `%s` port `%s`\n", pthread_self(), host, serv );
   #endif

   /* (1) DNS resolve host */
   int sockFd, n;
   struct addrinfo hints, *res, *ressave;
   bzero ( &hints, sizeof ( struct addrinfo ) );
   hints.ai_family = AF_UNSPEC;     // IPv4 & IPv6
   hints.ai_socktype = SOCK_STREAM; // Socket Stream for TCP
   hints.ai_flags = AI_CANONNAME;   // canonical name is the first return
   hints.ai_protocol = IPPROTO_TCP; // TCP

      {
      MtxRAII mtxRaii ( get_tcp_gloabl_mtx() );
      if ( ( n = getaddrinfo ( host, serv, &hints, &res ) ) != 0 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (tcp_connect) get host %s serv %s addrinfo failed\n", pthread_self(), host, serv );
         printf ( "E [t %ld] (tcp_connect) ERROR %s\n", pthread_self(), gai_strerror ( n ) );
         fflush ( stdout );
         #endif
         return -1;
         }
      }

   ressave = res;

   /* (2) Create Socket */
   do
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "@ [t %ld] (tcp_connect) try sock i\n", pthread_self() );
      #endif

      sockFd = socket ( res->ai_family, res->ai_socktype, res->ai_protocol );
      if ( sockFd < 0 )
         continue; /* ignore this one */

      if ( connect ( sockFd, res->ai_addr, res->ai_addrlen ) == 0 )
         break; /* success */

      close ( sockFd );
      }
   while ( ( res = res->ai_next ) != nullptr );

   if ( res == nullptr ) /* errno set from final connect() */
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_connect) get socket of host `%s` port `%s` failed\n", pthread_self(), host, serv );
      fflush ( stdout );
      #endif

      return -1;
      }

   // Set Socket send, recev,
   struct timeval timeout
      {
      3, 0
      };
   if ( setsockopt ( sockFd, SOL_SOCKET, SO_RCVTIMEO, ( const char* ) &timeout, sizeof ( timeout ) ) < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_connect) unable to set socket timeout\n", pthread_self() );
      fflush ( stdout );
      #endif

      #ifdef __APPLE__
         return sockFd;
      #else
         return -1;
      #endif
      }

   freeaddrinfo ( ressave );

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "* [t %ld] (tcp_connect) TCP CONNECT SUCCESS on host `%s` port `%s` on socket %d\n", pthread_self(), host, serv, sockFd );
   #endif

   return sockFd;
   }

// Connect to host port and send message
// Return:
//    -1 : if anypart goes wrong
//    1 : if success
// Note:
//    1. This function hold get_tcp_gloabl_mtx lock half wau
//
int tcp_connect_send ( const char* host, const char* serv, const char* msg )
   {
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (tcp_connect_send) start on msg %s to host `%s` port `%s`\n", pthread_self(), msg, host, serv );
   fflush ( stdout );
   #endif

   int sockFd = tcp_connect ( host, serv );
   FhRAII FhRAII ( sockFd ); // RAII for socket file hand
   if ( sockFd < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_connect_send) socket connect to %s %s failed\n", pthread_self(), host, serv );
      fflush ( stdout );
      #endif
      return -1;
      }

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (tcp_connect_send) try send request\n", pthread_self() );
   fflush ( stdout );
   #endif

   int len = send ( sockFd, msg, strlen ( msg ), 0 );

   if ( len < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_connect_send) unable to send msg to %s %s\n", pthread_self(), host, serv );
      fflush ( stdout );
      #endif

      return -1;
      }

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "* [t %ld] (tcp_connect_send) send finish, helper function finish\n", pthread_self() );
   fflush ( stdout );
   #endif

   return 1;
   }

// Make TCP Socket Listen & Return Valid Socket File Handler
// Return:
//    -1 : if makeing tcp connection failed (incure any error during connection)
//    sockFd (positive number): make tcp connection success
// Caller:
//    caller should in charge of close the socket file handler
// Note:
//    1. host need to be a valid hostname on current machine (e.g. https://localhost)
//    2. socket connection have timeout of 10 second, if no connection is made, consider this invalid socket
//    3. this function hold  get_tcp_gloabl_mtx lock through its lifetime
// Reference: lib/tcp_listen.c
int tcp_listen ( const char* host, const char* serv, socklen_t* addrlen = nullptr, int backlog = 100 )
   {
   // Doc:
   // getaddrinfo: https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html
   // listen : https://man7.org/linux/man-pages/man2/listen.2.html
   // getsockopt: https://man7.org/linux/man-pages/man2/getsockopt.2.html
   // bind: https://man7.org/linux/man-pages/man2/bind.2.html

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (tcp_listen) TCP try to create listen on %s:%s\n",  pthread_self(), host, serv );
   #endif

   /* (1) DNS resolve host */
   int listenFd, n;
   struct addrinfo hints, *res, *ressave;

   bzero ( &hints, sizeof ( struct addrinfo ) );
   hints.ai_family = AF_UNSPEC;     // IPv4 & IPv6
   hints.ai_socktype = SOCK_STREAM; // Socket Stream for TCP
   hints.ai_flags = AI_PASSIVE;     // Passive for binding later

      {
      MtxRAII mtxRaii ( get_tcp_gloabl_mtx() );
      if ( ( n = getaddrinfo ( host, serv, &hints, &res ) ) != 0 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (tcp_listen) get host %s serv %s addrinfo failed\n", pthread_self(), host, serv );
         printf ( "E [t %ld] (tcp_listen) ERROR %s\n", pthread_self(), gai_strerror ( n ) );
         fflush ( stdout );
         #endif
         return -1;
         }
      }

   ressave = res;

   do
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "@ [t %ld] (tcp_listen) try sock i\n",  pthread_self() );
      #endif

      /* (2) Create Socket */
      listenFd = socket ( res->ai_family, res->ai_socktype, res->ai_protocol );
      if ( listenFd < 0 )
         continue; /* error, try next one */

      /* (3) Set "reuse port" socket option*/
      // TODO NOT SURE ABOUT THIS
      const int on = 1;
      if ( setsockopt ( listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof ( on ) ) < 0 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (tcp_listen) error setting socket option for %s:%s\n",  pthread_self(), host, serv );
         fflush ( stdout );
         #endif

         continue; // try next one
         }

      /* (4) Bind to port */
      // Note: we do not use the make_server_sockaddr function in EECS482 example
      //    because we hide the DNS resolve process using getaddrinfo & support IPv4&6 at the same time
      if ( bind ( listenFd, res->ai_addr, res->ai_addrlen ) == 0 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "@ [t %ld] (tcp_listen) bind socket %d success\n",  pthread_self(), listenFd );
         #endif

         break; /* success */
         }

      if ( close ( listenFd ) == -1 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (tcp_listen) close socket failed\n",  pthread_self() );
         fflush ( stdout );
         #endif

         continue; // try next one
         }
      }
   while ( ( res = res->ai_next ) != NULL );

   if ( res == nullptr ) /* errno set from final connect() */
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_listen) get socket of %s:%s failed\n",  pthread_self(), host, serv );
      fflush ( stdout );
      #endif
      return -1;
      }

   freeaddrinfo ( ressave );

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (tcp_listen) try listen socket %d\n",  pthread_self(), listenFd );
   #endif

   /* (5) Begin listening for incoming connection */
   if ( listen ( listenFd, backlog ) < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (tcp_listen) listen on %s:%s failed\n",  pthread_self(), host, serv );
      fflush ( stdout );
      #endif
      return -1;
      }

   /* (6) Save address len info for inter communication */
   if ( addrlen != nullptr )
      *addrlen = res->ai_addrlen;

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "* [t %ld] (tcp_listen) finish\n",  pthread_self() );
   fflush ( stdout );
   #endif
   return listenFd;
   }

// Currently no retry
int accept_retry ( int fd, struct sockaddr* sa, socklen_t* salenptr )
   {
   return accept ( fd, sa, salenptr ) ;
   }

// Make SSL_Library Initialization & return global ctx
// The second time this function is call, `init` and `ctx` DO NOT get new value
// Note:
//    under current design, ctx may incure memory leak when the process end
//    one can manyally close the ctx connection with process end.
SSL_CTX* get_gloabl_ssl_ctx()
   {
   static int init = SSL_library_init();               /* ONLY RUN ONCE */
   static SSL_CTX* ctx = SSL_CTX_new ( SSLv23_method() ); /* ONLY RUN ONCE */
   return ctx;
   }

// SSLv23_client_method
// SSLv23_method

// Make SSL connection on top of Socket Connection
// Return:
//      nullptr : if connection failed
//      ptr to ssl: if success
// Caller:
//    caller should in charge of close the ssl handler
// Note:
//    1. ssl connection have timeout of 10 second, if no connection is made, consider this invalid ssl connection
//    2. this function hold get_tcp_gloabl_mtx lock through its lifetime
SSL* ssl_connect ( int sockFd, SSL_CTX* ctx )
   {
   // Doc:
   // SSL_set_fd https://www.openssl.org/docs/manmaster/man3/SSL_set_fd.html
   // SSL_connect https://www.openssl.org/docs/man1.0.2/man3/SSL_connect.html
   // Most of the socket operation is not thread safe

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (ssl_connect) try to build ssl connection on tcp socket %d\n", pthread_self(), sockFd );
   #endif

   if ( ctx == nullptr )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (ssl_connect) ctx is nullptr, no connection can be made\n", pthread_self() );
      #endif
      return nullptr;
      }

   //fcntl(sockFd, F_SETFL, O_NONBLOCK);
   // Make new ssl conneciton
   SSL* ssl = nullptr;
      {
      MtxRAII mtxRaii ( get_ssl_gloabl_mtx() );
      ssl = SSL_new ( ctx );
      if ( ssl == nullptr )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (ssl_connect) SSL_new failed\n", pthread_self() );
         fflush ( stdout );
         #endif
         return nullptr;
         }
      }

   // Bind ssl with socket fh
   if ( SSL_set_fd ( ssl, sockFd ) != 1 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (ssl_connect) unable to set ssl file handler\n", pthread_self() );
      fflush ( stdout );
      #endif
      return nullptr;
      }

   // Make new ssl connect
   ERR_clear_error();
   int tmp = SSL_connect ( ssl );
   if ( tmp != 1 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (ssl_connect) unable to connect to ssl file handler (safe to ignore)\n", pthread_self() );
      char buf[250];
      printf ( "E [t %ld] (ssl_connect) SSL Error `%s` (safe to ignore)\n", pthread_self(), ERR_error_string ( SSL_get_error ( ssl, tmp ), buf ) );
      fflush ( stdout );
      #endif
      return nullptr;
      }

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "* [t %ld] (ssl_connect) SSL CONNECT SUCCESS %s on socket %d\n", pthread_self(), SSL_get_cipher ( ssl ), sockFd );
   #endif

   return ssl;
   }

// Create GET Message
// Return:
//    nullptr: if failed (there might be memory allocation fail)
//    char* : to write lines
// Caller:
//    caller should in charge of delete the resouce allocated
#define GET_CMD "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36(xiaosx@umich.edu)\r\nAccept: %s \r\nAccept-Encoding: identity\r\n\r\n"
void create_get_msg ( const char* path, const char* host, const char* filetype, string& get_msg_str )
   {
   // Doc:
   // snprintf https://www.cplusplus.com/reference/cstdio/snprintf/

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (create_get_msg) create get msg for host `%s` path `%s`\n", pthread_self(), host, ( path == nullptr ? "" : path ) );
   #endif

   // If invalid hostname, no get message is made
   if ( host == nullptr )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (create_get_msg) hostname is nullptr, no get msg is made\n", pthread_self() );
      fflush ( stdout );
      #endif

      get_msg_str.clear();
      return;
      }

   try
      {
      unsigned int msg_size = strlen ( GET_CMD ) + 1 + ( path == nullptr ? 0 : strlen ( path ) ) + strlen ( host ) + ( filetype == nullptr ? 3 : strlen ( filetype ) );
      char* get_msg = new char[msg_size];
      snprintf ( get_msg, msg_size, GET_CMD, ( path == nullptr ? "" : path ), host, ( filetype == nullptr ? "*/*" : filetype ) );
      get_msg_str = string ( get_msg );
      delete[] get_msg;
      }
   #ifndef __APPLE__
   catch ( abi::__forced_unwind& )
      {
      printf ( "E [t %ld]  (create_get_msg) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!LISTEN SIGNAL THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
      fflush ( stdout );
      throw;
      }
   #endif
   catch ( ... )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (create_get_msg) CREATE GET MSG FAILED\n", pthread_self() );
      fflush ( stdout );
      #endif
      }

   //#ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   //printf ( "* [t %ld] (create_get_msg) get msg is \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(), get_msg_str.c_str() );
   //#endif

   }


// Read From Socket
// This function is used to receive end signal & receive new urls
// Return:
//    1 : if read success
//    -1 : if read failed
// Note:
//    this function will close the socket file handler when read finish
// TODO : currently not support return -1 on read fail
int tcp_read ( int sockFd, string& content, unsigned int buffer_size = 4096 )
   {
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (tcp_read) start listen on socket %d\n", pthread_self(), sockFd );
   #endif

   FhRAII sockRaii ( sockFd );
   content.clear();

   char buffer[buffer_size];
   memset ( buffer, 0, sizeof ( buffer ) );
   int bytes = 0;
   while ( ( bytes = recv ( sockFd, buffer, buffer_size, 0 ) ) > 0 )
      {
      // Below logic should never be triggered. Just keep it here for safety
      if ( bytes < 0 )
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (tcp_read) read from socket %d failed\n", pthread_self(), sockFd );
         fflush ( stdout );
         #endif
         break;
         }

      content.append ( buffer, bytes );
      }

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "* [t %ld] (tcp_read) read `%s` from [socket %d] finish\n", pthread_self(), content.c_str(), sockFd );
   fflush ( stdout );
   #endif

   return 1;
   }

// Download Webpage
// Return:
//    1 : download webpage success
//    -1 :
//    0: url have redirection, need to handle redirection
// Param:
//    url : url to download
//    content : where content is write to
//    remove_header : either remove the header of the response
//    buffer_size : buffer size to read socket
// NOTE: we're only downloading https webpage and thus ssl connection is always required
#define DOWNLOAD_WEBPAGE_SUCCESS 1
#define DOWNLOAD_WEBPAGE_URL_REDIRECT 0
#define DOWNLOAD_WEBPAGE_FAIL -1
int download_webpage_ssl ( const char* url, string& content, bool remove_header = true, unsigned int buffer_size = 4096 )
   {
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (download_webpage_ssl) try to download %s\n", pthread_self(), url );
   #endif

   content.clear();

   /* (1) Parse the URL */
   ParsedUrl parsedUrl ( url );

   // only handle http page
   if ( !parsedUrl.isHttps )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) url %s not HTTPS, not handle it\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      return DOWNLOAD_WEBPAGE_FAIL;
      }

   /* (2) Make socket connection */
   int sockFd = tcp_connect ( parsedUrl.Host, "443" );
   FhRAII sockRaii ( sockFd );
   if ( sockFd < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) socket connection failed %s\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      return DOWNLOAD_WEBPAGE_FAIL;
      }

   /* (3) Make ssl conneciton */
   SSL* ssl = ssl_connect ( sockFd, get_gloabl_ssl_ctx() );
   SSLRAII sslRaii ( ssl ); // RAII is able to handle nullptr
   if ( ssl == nullptr )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) unable to create ssl conneciton for url %s\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      return DOWNLOAD_WEBPAGE_FAIL;
      }

   /* (4) create get message */
   string getMsgStr = "";
   create_get_msg ( parsedUrl.Path, parsedUrl.Host, "text/html,text/plain,application/xhtml+xml,application/xml", getMsgStr );
   if ( getMsgStr.empty() )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) create get message failed\n", pthread_self() );
      fflush ( stdout );
      #endif
      return DOWNLOAD_WEBPAGE_FAIL;
      }

   /* (5) send get message */
   ERR_clear_error();
   int len =  SSL_write ( ssl, getMsgStr.c_str(), getMsgStr.size() );
   if ( len < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) unable to send get message through ssl for url %s\n", pthread_self(), url );
      char buf[250];
      printf ( "E [t %ld] (download_webpage_ssl) SSL Error %s\n", pthread_self(), ERR_error_string ( SSL_get_error ( ssl, len ), buf ) );
      fflush ( stdout );
      #endif
      return DOWNLOAD_WEBPAGE_FAIL;
      }

   /* (6) read from socket return data */
   char buffer[buffer_size];
   int bytes = 0;

   const char* endofHeader = "\r\n\r\n";
   bool readHeader = remove_header;

   string httpStatus = "";
   bool readStatus = remove_header; // if not remove header, not handling redirection

   ERR_clear_error();

   while ( ( bytes = SSL_read ( ssl, buffer, buffer_size ) ) > 0 )
      {
      //#ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      //printf ( "\n@ [t %ld] (download_webpage_ssl) read buffer: \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(), string(buffer, bytes).c_str() );
      //#endif

      /* (5.1) Read HTTP Status & Redirection */
      if ( readStatus )
         {
         char* statusBegin = strstr ( buffer, " " );
         if ( statusBegin == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (download_webpage_ssl) url %s extract status begin fail\n", pthread_self(), url );
            fflush ( stdout );
            #endif
            return DOWNLOAD_WEBPAGE_FAIL;
            }

         char* statusEnd = strstr ( ++statusBegin, " " );
         if ( statusEnd == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (download_webpage_ssl) url %s extract status end fail\n", pthread_self(), url );
            fflush ( stdout );
            #endif
            return DOWNLOAD_WEBPAGE_FAIL;
            }

         // https://www.cplusplus.com/reference/string/string/string/
         // string constructor 5
         httpStatus = string ( statusBegin, statusEnd - statusBegin );
         readStatus = false;

         // Redirection
         if ( httpStatus == "301" || httpStatus == "302" || httpStatus == "303" || httpStatus == "307" )
            return DOWNLOAD_WEBPAGE_URL_REDIRECT;

         else if ( httpStatus == "400" || httpStatus == "403" || httpStatus == "999" )
            {
            // 999 由于配置SSL证书
            // 400 403 由于反爬虫机制
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (download_webpage_ssl) url %s RETURN STATUS %s, failed to download, but SAFE TO IGNORE\n", pthread_self(), url, httpStatus.c_str() );
            fflush ( stdout );
            #endif
            return DOWNLOAD_WEBPAGE_FAIL;
            }

         else if ( httpStatus != "200" )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (download_webpage_ssl) url %s have INVALID RETURN STATUS %s, failed to download\n", pthread_self(), url, httpStatus.c_str() );
            fflush ( stdout );
            #endif
            return DOWNLOAD_WEBPAGE_FAIL;
            }

         } // end if read status

      /* (5.3) Read Header */
      if ( readHeader )
         {
         // Should finish read status before read header
         if ( readStatus )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (download_webpage) read header before read status, error\n", pthread_self() );
            fflush ( stdout );
            #endif

            return DOWNLOAD_WEBPAGE_FAIL;
            }
         else
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "@ [t %ld] (download_webpage) finish read status `%s`\n", pthread_self(), httpStatus.c_str() );
            fflush ( stdout );
            #endif
            }
         const char* p;
         const char* nextmatch = endofHeader;

         for ( p = buffer; p < buffer + bytes; p++ )
            {
            if ( *p == *nextmatch )
               {
               // Advance to the next char to match.
               // If at the end, stop skipping and
               // write out the rest of the buffer.
               p++;
               nextmatch++;
               while ( *nextmatch && *p == *nextmatch )
                  {
                  p++;
                  nextmatch++;
                  }

               // find end of header
               if ( *nextmatch == '\0' )
                  {
                  readHeader = false;

                  // https://www.cplusplus.com/reference/string/string/append/
                  // string append 4

                  #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
                  printf ( "@ [t %ld] (download_webpage) finish read header\n", pthread_self() );
                  fflush ( stdout );
                  #endif

                  content.append ( p, bytes - ( p - buffer ) );

                  // for safety, clear out all memory
                  //memset ( buffer, 0, buffer_size );

                  break;
                  }
               // not find end of header
               else
                  nextmatch = endofHeader;
               }
            else
               // start over if not a match. (use next recv to find header end)
               nextmatch = endofHeader;
            } // end of for loop
         } // end of if readHeader

      // read main http content
      else
         content.append ( buffer, bytes );

      } // end of while loop to read socket

   /* (7) return success handle code */
   if ( httpStatus == "200" )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "* [t %ld] (download_webpage_ssl) download %s finish\n", pthread_self(), url );
      fflush ( stdout );
      #endif

      return DOWNLOAD_WEBPAGE_SUCCESS;
      }
   else
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (download_webpage_ssl) download %s have wrong status `%s` when return\n", pthread_self(), url, httpStatus.c_str() );
      char buf[250];
      printf ( "E [t %ld] (download_webpage_ssl) SSL Error `%s`, `%d`\n", pthread_self(), ERR_error_string ( SSL_get_error ( ssl, bytes ), buf ), SSL_get_error ( ssl, bytes ) );
      fflush ( stdout );
      #endif

      return DOWNLOAD_WEBPAGE_FAIL;
      }

   printf ( "E [t %ld] SYS LEVEL ERROR HAPPEN, PROCESS WOULD END\n ", pthread_self() );
   fflush ( stdout );
   return DOWNLOAD_WEBPAGE_FAIL;
   }

// Handle Redirection URL
// Param:
//    url : url that have been previously check need to be redirected
//    max_redirect : max number of redirection
// Return:
//    empty string : if handle redirect failed
//    string of new url : if handle redirect success
// NOTE: we're only downloading https webpage and thus ssl connection is always required

// Return
//    -1 if failed
//    1 if success find final url
//    0 if redirection continue
#define REDIRECT_ONCE_FAIL -1
#define REDIRECT_ONCE_SUCCESS 1
#define REDIRECT_ONCE_CONTINUE 0
int handle_redirect_ssl_once ( const char* initial_url, string& redirected_url, unsigned int buffer_size = 12288 )
   {
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (handle_redirect_ssl_once) try to find redirect of %s\n", pthread_self(), initial_url );
   #endif

   /* (1) Parse the URL */
   redirected_url.clear();
   string url_str;
   const char* url = nullptr;
   url = initial_url;

   ParsedUrl parsedUrl ( url );

   // only handle http page
   if ( !parsedUrl.isHttps )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (handle_redirect_ssl_once) (EXTREMEELY WRONG) url %s not HTTPS, not handle it\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      exit(-1);
      }

   /* (2) Make socket connection */
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (handle_redirect_ssl_once) start to make tcp connection\n", pthread_self() );
   #endif

   int sockFd = tcp_connect ( parsedUrl.Host, "443" );
   FhRAII sockRaii ( sockFd );
   if ( sockFd < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (handle_redirect_ssl_once) socket connection failed %s\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      return -1;
      }

   /* (3) Make ssl conneciton */
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (handle_redirect_ssl_once) start to make ssl connection\n", pthread_self() );
   #endif

   SSL* ssl = ssl_connect ( sockFd, get_gloabl_ssl_ctx() );
   SSLRAII sslRaii ( ssl );
   if ( ssl == nullptr )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (handle_redirect_ssl_once) unable to create ssl conneciton for url %s\n", pthread_self(), url );
      fflush ( stdout );
      #endif
      return -1;
      }

   /* (4) create get message */
   string getMsgStr;
   create_get_msg ( parsedUrl.Path, parsedUrl.Host, "text/html,application/xhtml+xml,application/xml", getMsgStr );
   if ( getMsgStr.empty() )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (handle_redirect_ssl_once) create get message failed\n", pthread_self() );
      fflush ( stdout );
      #endif
      return -1;
      }

   /* (5) send get message */
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (handle_redirect_ssl_once) start to send get msg\n", pthread_self() );
   #endif

   ERR_clear_error();
   int len =  SSL_write ( ssl, getMsgStr.c_str(), getMsgStr.size() );
   if ( len < 0 )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "E [t %ld] (handle_redirect_ssl_once) unable to send get message through ssl for url %s\n", pthread_self(), url );
      char buf[250];
      printf ( "E [t %ld] (handle_redirect_ssl_once) SSL Error %s\n", pthread_self(), ERR_error_string ( SSL_get_error ( ssl, len ), buf ) );
      fflush ( stdout );
      #endif
      return -1;
      }

   /* (6) read from socket return data */
   char buffer[buffer_size];
   int bytes = 0;

   string httpStatus;
   bool readStatus = true;

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "@ [t %ld] (handle_redirect_ssl_once) start to read return msg\n", pthread_self() );
   #endif

   ERR_clear_error();
   while ( ( bytes = SSL_read ( ssl, buffer, buffer_size ) ) > 0 )
      {
      //printf ( "\n@ [t %ld] (download_webpage) read buffer: \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(), string(buffer, bytes).c_str() );

      if ( readStatus )
         {
         /* (5.1) Read HTTP Status & Redirection */
         char* statusBegin = strstr ( buffer, " " );
         if ( statusBegin == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (handle_redirect_ssl_once) url %s extract status begin fail\n", pthread_self(), url );
            fflush ( stdout );
            #endif
            return -1;
            }

         char* statusEnd = strstr ( ++statusBegin, " " );
         if ( statusEnd == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (handle_redirect_ssl_once) url %s extract status end fail\n", pthread_self(), url );
            fflush ( stdout );
            #endif
            return -1;
            }

         httpStatus = string ( statusBegin, statusEnd - statusBegin );
         readStatus = false;
         }

      if ( httpStatus == "301" || httpStatus == "302" || httpStatus == "303" || httpStatus == "307" )
         {
         char* redirectUrlBegin1 = strstr ( buffer, "Location: " );
         char* redirectUrlBegin2 = strstr ( buffer, "location: " );
         char* redirectUrlBegin = redirectUrlBegin1 == nullptr ? redirectUrlBegin2 : redirectUrlBegin1;
         if ( redirectUrlBegin == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "@ [t %ld] (handle_redirect_ssl_once) no new url in this buffer, read next buffer\n", pthread_self() );
            fflush ( stdout );
            #endif
            continue;
            }

         redirectUrlBegin += 10;
         char* redirectUrlEnd = strstr ( redirectUrlBegin, "\r\n" );
         if ( redirectUrlEnd == nullptr )
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "E [t %ld] (handle_redirect_ssl_once) url %s extract redirect end fail\n", pthread_self(), url );
            fflush ( stdout );
            #endif
            return -1;
            }

         redirected_url = string ( redirectUrlBegin, redirectUrlEnd - redirectUrlBegin );
         fix_url_to_correct_format(redirected_url, parsedUrl, url);
         
         if ( strstr ( redirected_url.c_str(), "http://" ) != nullptr )
            {
            return -1;
            }

         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "@ [t %ld] (handle_redirect_ssl_once) `%s` -> `%s`\n", pthread_self(), url, redirected_url.c_str() );
         #endif

            return 0;
            }

      else if ( httpStatus == "200" )
         {
         redirected_url = url;
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "* [t %ld] (handle_redirect_ssl_once) redirect end, url %s is final url\n", pthread_self(), redirected_url.c_str() );
         fflush ( stdout );
         #endif

         return 1;
         }

      else
         {
         #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
         printf ( "E [t %ld] (download_webpage) url %s have invalid return status %s, failed to download\n", pthread_self(), url, httpStatus.c_str() );
         fflush ( stdout );
         #endif
         return -1;
         }
      } // end of while loop

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "E [t %ld] (download_webpage) no redirected url find\n", pthread_self() );
   fflush ( stdout );
   #endif

   return -1;
   }

string handle_redirect_ssl ( const char* url, unsigned int maxRedirect = 5, unsigned int buffer_size = 12288 )
   {
   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "# [t %ld] (handle_redirect) try to find redirect of %s\n", pthread_self(), url );
   #endif

   string oldUrl ( url );
   string newUrl;
   unsigned int redirectCount = 0;

   while ( redirectCount < maxRedirect )
      {
      #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
      printf ( "@ [t %ld] (handle_redirect) iteration %d\n", pthread_self(), redirectCount );
      #endif

      switch ( handle_redirect_ssl_once ( oldUrl.c_str(), newUrl, buffer_size ) )
         {
         case REDIRECT_ONCE_CONTINUE:
            redirectCount++;
            oldUrl = newUrl;
            newUrl.clear();

            break;
         case REDIRECT_ONCE_SUCCESS:
            {
            #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
            printf ( "* [t %ld] (handle_redirect) SUCCESS `%s` -> `%s`\n", pthread_self(), url, oldUrl.c_str() );
            fflush ( stdout );
            #endif

            return newUrl;
            }
         case REDIRECT_ONCE_FAIL:
            return string();
         }
      }

   #ifdef _UMSE_CRAWLER_NETWORK_UTILITY_LOG
   printf ( "E [t %ld] (handle_redirect) redirect too many time, failed\n", pthread_self() );
   fflush ( stdout );
   #endif

   return string();
   }
