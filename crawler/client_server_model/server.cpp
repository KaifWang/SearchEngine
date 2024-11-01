//
// server.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#include <string>
#include <vector> // use vector to keep track of all the child thread
#include <pthread.h>
#include <unistd.h>
#include <signal.h> // pthread_kill
#include "../network_utility.hpp"
#include "../thread_signal.hpp"

using std::string;

struct ThreadParam
   {
   const char* host;
   const char* port;
   ThreadSignal* singal;

   ThreadParam ( const char* host_, const char* port_, ThreadSignal* singal_ )
      : port ( port_ ), host ( host_ ), singal ( singal_ ) {}
   };

// Handler thread
void* run_server_handler ( void* arg )
   {
   int connecFh = ( intptr_t ) arg;
   printf ( "# (run_server_handler) start to read data from socket %d\n", connecFh );

   string content;
   if ( tcp_read ( connecFh, content ) < 0 )
      {
      printf ( "E (run_server_handler) read from socket %d failed\n", connecFh );
      exit ( 1 );
      }
   printf ( "@ (run_server_handler) read from socket %d success, data is %s\n", connecFh, content.c_str() );
   printf ( "* (run_server_handler) end on handle socket %d\n", connecFh );
   return ( void* ) 1;
   }

// Main listen & schedual thread
void* run_server_main ( void* arg )
   {
   // Doc
   // accept: https://www.man7.org/linux/man-pages/man2/accept.2.html
   // pthread_kill https://man7.org/linux/man-pages/man3/pthread_kill.3.html

   ThreadParam* param = ( ThreadParam* ) arg;

   printf ( "# (run_server_main) start on host %s port %s\n", param->host, param->port );

   socklen_t addrlen, len;
   struct sockaddr*	cliaddr;
   int listenFd, connecFh;
   vector<pthread_t> threads_container;

   listenFd = tcp_listen ( param->host, param->port, &addrlen );
   if ( listenFd < 0 )
      {
      printf ( "E (run_server_main) error when createing listen thread\n" );
      exit ( 1 );
      }

   cliaddr = ( struct sockaddr* ) malloc ( addrlen );

   while ( param->singal->is_alive() )
      {
      // Get a new connection
      len = addrlen;
      if ( ( connecFh = accept ( listenFd, cliaddr, &len ) ) < 0 )
         {
         printf ( "E (run_server_main)accept failed\n" );
         exit ( 1 );
         }

      printf ( "\n@ (run_server_main) receive new connection, make thread to handle\n" );

      // Create listen socket
      pthread_t new_thread;
      if ( pthread_create ( &new_thread, nullptr, &run_server_handler, ( void* ) connecFh ) != 0 )
         {
         printf ( "E (run_server_main) create new thread failed\n" );
         exit ( 1 );
         }
      threads_container.push_back ( new_thread );

      // Check if previous thread have ended, if ended, remove them from vector
      // This help avoid saving too much things on the vector<pthread_t> container
      for ( auto iter = threads_container.begin(); iter != threads_container.end(); /* NOTHING */ )
         {
         if ( pthread_kill ( *iter, 0 ) == ESRCH )
            {
            printf ( "@ (run_server_main) thread %ld have end, remove it from container\n", *iter );
            iter = threads_container.erase ( iter );
            }
         else
            ++iter;
         }
      }

   printf ( "@ (run_server_main) receive end message, wait for all child to finish\n" );
   for ( unsigned int i = 0; i < threads_container.size(); ++i )
      pthread_join ( threads_container[i], nullptr );

   printf ( "* (run_server_main) finish\n" );
   return ( void* ) 1;
   }


// Listen for singal
void* signal_handler ( void* arg )
   {
   ThreadParam* param = ( ThreadParam* ) arg;

   printf ( "# (signal_handler) start on host %s port %s\n", param->host, param->port );

   socklen_t addrlen, len;
   struct sockaddr*	cliaddr;
   int listenFd, connecFh;

   /* (1) create listen socket */
   listenFd = tcp_listen ( param->host, param->port, &addrlen );
   if ( listenFd < 0 )
      {
      printf ( "E (signal_handler) error when createing listen thread\n" );
      exit ( 1 );
      }

   /* (2) listen for close signal */
   printf ( "@ (signal_handler) wait for end signal\n" );

   cliaddr = ( struct sockaddr* ) malloc ( addrlen );

   while ( true )
      {
      // Get a new connection
      len = addrlen;
      if ( ( connecFh = accept ( listenFd, cliaddr, &len ) ) < 0 )
         {
         printf ( "E (signal_handler) accept failed\n" );
         exit ( 1 );
         }

      printf ( "@ (signal_handler) receive message from from %s\n", cliaddr->sa_data );

      string content;
      if ( tcp_read ( connecFh, content ) < 0 )
         {
         printf ( "E (signal_handler) read connect socket %d failed\n", connecFh );
         exit ( 1 );
         }

      if ( content == UMSE_CRAWLER_END_SIGNAL )
         {
         printf ( "@ (signal_handler) receive end message, try to end whole process\n" );
         param->singal->set_terminate();
         break;
         }
      else
         printf ( "@ (signal_handler) receive message %s but not end message\n", content.c_str() );
      }

   printf ( "* (signal_handler) thread end\n" );
   return ( void* ) 1;
   }


int main ( int argc, const char** argv )
   {
   if ( argc != 4 )
      {
      printf ( "Useage : ./server host port_url port_signal\n" );
      exit ( 1 );
      }

   // Shared ThreadSignal
   ThreadSignal ThreadSignal;

   ThreadParam param_listenurl ( argv[1], argv[2], &ThreadSignal );
   ThreadParam param_listensignal ( argv[1], argv[3], &ThreadSignal );

   pthread_t tmain;
   pthread_t tsingal;

   pthread_create ( &tmain, nullptr, &run_server_main, ( void* ) &param_listenurl );
   pthread_create ( &tsingal, nullptr, &signal_handler, ( void* ) &param_listensignal );

   pthread_join ( tsingal, nullptr );
   pthread_join ( tmain, nullptr );

   printf ( "* main() function finish\n" );
   }