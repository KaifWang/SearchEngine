//
// server_class.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Put the server info inside a class
//
#include <pthread.h>

#include "../configs/crawler_cfg.hpp"
#include "../network_utility.hpp"
#include "../thread_signal.hpp"
#include "../thread_queue.hpp" // mimic frontier
#include "../raii_fh.hpp"

#define NUM_CONCURRENT_K 5

class Server;

/*
struct ThreadParamFh
   {
   Server* Instance;
   int Fh;
   const char* Host;
   const char* Port;

   explicit ThreadParamFh ( Server* instance )
      : Instance ( instance ), Fh ( -1 ), Host ( nullptr ), Port ( nullptr ) {}
   explicit ThreadParamFh ( Server* instance, int fh )
      : Instance ( instance ), Fh ( fh ), Host ( nullptr ), Port ( nullptr ) {}
   explicit ThreadParamFh ( Server* instance, const char* host, const char* port )
      : Instance ( instance ), Fh ( -1 ), Host ( host ), Port ( port ) {}
   explicit ThreadParamFh ( Server* instance, int fh, const char* host, const char* port )
      : Instance ( instance ), Fh ( fh ), Host ( host ), Port ( port ) {}
   };
*/

struct ThreadParamFh
   {
   Server* Instance;
   int Fh;

   explicit ThreadParamFh ( Server* instance, int fh )
      : Instance ( instance ), Fh ( fh ) {}
   };

struct ThreadParamUrl
   {
   Server* Instance;
   string Url;

   explicit ThreadParamUrl ( Server* instance, const string& url )
      : Instance ( instance ), Url ( url ) {}
   };

struct ThreadParamServer
   {
   Server* Instance;

   explicit ThreadParamServer ( Server* instance )
      : Instance ( instance ) {}
   };

/*
struct ThreadParamConnSend
   {
   const char* Host;
   const char* Port;
   const char* Msg;

   explicit ThreadParamConnSend ( const char* host, const char* port, const char* msg )
      : Host ( host ), Port ( port ), Msg ( msg ) {}
   };

void* tcp_connect_send_thread ( void* arg )
   {
   ThreadParamConnSend* param = ( ThreadParamConnSend* ) arg;
   string msg_copy ( param->Msg );
   string host_copy ( param->Host );
   string serv_copy ( param->Port );

   printf ( "# (tcp_connect_send_thread) send %s to host %s port %s\n", msg_copy.c_str(), host_copy.c_str(),
         serv_copy.c_str() );

   if ( tcp_connect_send ( host_copy.c_str(), serv_copy.c_str(), msg_copy.c_str() ) < 0 )
      {
      printf ( "E (tcp_connect_send_thread) unable to send msg to %s %s\n", host_copy.c_str(), serv_copy.c_str() );
      return ( void* ) -1;
      }

   printf ( "@ (tcp_connect_send_thread) send msg to %s %s success\n", host_copy.c_str(), serv_copy.c_str() );
   }
*/

class Server
   {
   public:
      ThreadSignal threadSignal;
      ThreadWaitQueue<string> threadQueue;

      pthread_t listen_url_thread;
      pthread_t listen_signal_thread;

      /* Thread for listen end signal */
   public:
      void* listenSignalThread ( void* )
         {
         //ThreadParamFh* param = ( ThreadParamFh* ) arg;
         //const char* host = "localhost";
         //const char* port = UMSE_CRAWLER_LISTEN_SIGNAL_PORT;
         printf ( "# (listenSignalThread) start on host %s port %s\n", "localhost", UMSE_CRAWLER_LISTEN_SIGNAL_PORT );

         socklen_t addrlen, len;
         struct sockaddr*	cliaddr;

         /* (1) create listen socket */
         int listenFd = tcp_listen ( "localhost", UMSE_CRAWLER_LISTEN_SIGNAL_PORT, &addrlen );
         FhRAII listenFdRAII ( listenFd );
         if ( listenFd < 0 )
            {
            printf ( "E (listenSignalThread) error when createing listen thread\n" );
            return ( void* ) -1;
            }

         /* (2) listen for close signal */
         printf ( "@ (listenSignalThread) wait for end signal\n" );

         cliaddr = ( struct sockaddr* ) malloc ( addrlen );

         while ( threadSignal.is_alive() )
            {
            // Get a new connection
            len = addrlen;
            int connecFh = accept ( listenFd, cliaddr, &len );
            FhRAII connecFhRAII ( connecFh );
            if ( connecFh < 0 )
               {
               printf ( "E (listenSignalThread) accept failed\n" );
               return ( void* ) -1;
               }

            printf ( "@ (listenSignalThread) receive message from from %s\n", cliaddr->sa_data );

            string content;
            if ( tcp_read ( connecFh, content ) < 0 )
               {
               printf ( "E (listenSignalThread) read connect socket %d failed\n", connecFh );
               return ( void* ) -1;
               }

            if ( content == UMSE_CRAWLER_END_SIGNAL )
               {
               printf ( "@ (listenSignalThread) receive end message, try to end whole process\n" );
               threadSignal.set_terminate();
               threadQueue.set_terminate();
               break;
               }
            else
               printf ( "@ (listenSignalThread) receive message %s but not end message\n", content.c_str() );
            }

         /*
         if ( close ( listenFd ) == -1 )
            {
            printf ( "E (listenSignalThread) close socket %d failed\n", listenFd );
            return ( void* ) -1;
            }
         */

         printf ( "* (listenSignalThread) thread end\n" );
         return ( void* ) 1;
         }

      static void* listenSignalThreadHelper ( void* arg )
         {
         ThreadParamServer* param = ( ThreadParamServer* ) arg;
         printf ( "# (listenSignalThreadHelper) start on host %s port %s\n", "localhost", UMSE_CRAWLER_LISTEN_SIGNAL_PORT );
         return param->Instance->listenSignalThread ( nullptr );
         }

      /* Thread for listen new url */
   public:
      void* listenMsgThreadHandler ( void* arg )
         {
         //ThreadParamFh* param = ( ThreadParamFh* ) arg;
         int fh = ( intptr_t ) arg;
         FhRAII fhRAII ( fh );
         printf ( "# (listenMsgThreadHandler) start to handle socket %d\n", fh );

         string content;
         if ( tcp_read ( fh, content ) < 0 )
            {
            printf ( "E (listenMsgThreadHandler) read from socket %d failed\n", fh );
            return ( void* ) -1;
            }

         printf ( "@ (listenMsgThreadHandler) read %s from socket %d\n", content.c_str(), fh );

         // A back up way of sending end signal
         if ( content == UMSE_CRAWLER_END_SIGNAL )
            {
            printf ( "* (listenMsgThreadHandler) receive end signal %s from socket %d\n", content.c_str(), fh );
            threadSignal.set_terminate();
            threadQueue.set_terminate();
            return ( void* ) 1;
            }

         threadQueue.push ( content );

         printf ( "* (listenMsgThreadHandler) finish to handle socket %d, current queue size is %zu\n", fh, threadQueue.size() );
         return ( void* ) 1;
         }

      static void* listenMsgThreadHandlerHelper ( void* arg )
         {
         ThreadParamFh* param = ( ThreadParamFh* ) arg;
         printf ( "# (listenMsgThreadHandlerHelper) start \n" );
         return param->Instance->listenMsgThreadHandler ( ( void* ) param->Fh );
         }

      void* listenMsgThreadMain ( void* )
         {
         //ThreadParamFh* param = ( ThreadParamFh* ) arg;
         //const char* host = "localhost";
         //const char* port = UMSE_CRAWLER_LISTEN_MSG_PORT;
         //printf ( "# (listenMsgThreadMain) start on host %s port %s\n", param->Host, param->Port );
         printf ( "# (listenMsgThreadMain) start on host %s port %s\n", "localhost", UMSE_CRAWLER_LISTEN_MSG_PORT );

         socklen_t addrlen, len;
         sockaddr* cliaddr;
         vector<pthread_t> threads_container;

         /* (1) create listen socket */
         int listenFd = tcp_listen ( "localhost", UMSE_CRAWLER_LISTEN_MSG_PORT, &addrlen );
         FhRAII listenFdRAII ( listenFd );
         if ( listenFd < 0 )
            {
            printf ( "E (listenMsgThreadMain) error when createing listen thread\n" );
            return ( void* ) -1;
            }

         /* (2) listen for new message */
         cliaddr = ( struct sockaddr* ) malloc ( addrlen );

         while ( threadSignal.is_alive() )
            {
            // Get a new connection
            len = addrlen;
            int connecFh = accept ( listenFd, cliaddr, &len );
            FhRAII connecFhRAII ( connecFh );
            if ( connecFh < 0 )
               {
               printf ( "E (listenMsgThreadMain) accept failed\n" );
               return ( void* ) -1;
               }

            printf ( "\n\n@ (listenMsgThreadMain) receive new connection, make thread to handle\n" );

            // Create listen socket
            pthread_t new_thread;
            ThreadParamFh param_listen_url_handler ( this, connecFh );
            if ( pthread_create ( &new_thread, nullptr, &listenMsgThreadHandlerHelper, ( void* ) &param_listen_url_handler ) != 0 )
               {
               printf ( "E (listenMsgThreadMain) create new thread failed\n" );
               return ( void* ) -1;
               }
            threads_container.push_back ( new_thread );

            // Check if previous thread have ended, if ended, remove them from vector
            // This help avoid saving too much things on the vector<pthread_t> container
            for ( auto iter = threads_container.begin(); iter != threads_container.end(); /* NOTHING */ )
               {
               if ( pthread_kill ( *iter, 0 ) == ESRCH )
                  {
                  printf ( "@ (listenMsgThreadMain) thread %d have end, remove it from container\n", *iter );
                  iter = threads_container.erase ( iter );
                  }
               else
                  ++iter;
               }
            }

         printf ( "@ (listenMsgThreadMain) receive end message, wait for all child to finish\n" );
         for ( unsigned int i = 0; i < threads_container.size(); ++i )
            pthread_join ( threads_container[i], nullptr );

         printf ( "* (listenMsgThreadMain) finish\n" );
         return ( void* ) 1;
         }

      static void* listenMsgThreadMainHelper ( void* arg )
         {
         ThreadParamServer* param = ( ThreadParamServer* ) arg;
         printf ( "# (listenMsgThreadMainHelper) start on host %s port %s\n", "localhost", UMSE_CRAWLER_LISTEN_MSG_PORT );
         return param->Instance->listenMsgThreadMain ( nullptr );
         }

      /* Threads for handling processes */
   public:
      void* processThread ( void* arg )
         {
         ThreadParamUrl* param = ( ThreadParamUrl* ) arg;
         printf ( "# (processThread) start to handle %s\n", param->Url.c_str() );

         // Download

         // Parse

         // Add to frontier

         // Send new found url
         if ( tcp_connect_send ( "localhost", UMSE_CRAWLER_DEBUG_PORT, param->Url.c_str() ) < 0 )
            {
            printf ( "E (processThread) unable to send msg to %s %s\n", "localhost", UMSE_CRAWLER_DEBUG_PORT );
            return ( void* ) -1;
            }

         printf ( "* (processThread) finish handle %s\n", param->Url.c_str() );

         return ( void* ) 1;
         }

      static void* processThreadHelper ( void* arg )
         {
         ThreadParamUrl* param = ( ThreadParamUrl* ) arg;
         printf ( "# (processThreadHelper) start\n" );
         return param->Instance->processThread ( arg );
         }

      void run()
         {
         printf ( "# (run) start\n" );
         ThreadParamServer param_listen_url ( this );
         ThreadParamServer param_listen_signal ( this );

         pthread_create ( &listen_url_thread, nullptr, listenMsgThreadMainHelper, ( void* ) &param_listen_url );
         pthread_create ( &listen_signal_thread, nullptr, listenSignalThreadHelper, ( void* ) &param_listen_signal );

         vector<string> topk;

         printf ( "@ (run) begin start k thread\n" );
         while ( threadSignal.is_alive() )
            {
            printf ( "@ (run) get top k i\n" );
            threadQueue.get_top_k ( topk, NUM_CONCURRENT_K );

            if ( ! threadSignal.is_alive() )
               break;

            printf ( "@ (run) create k thread i\n" );
            vector<pthread_t> threads_container ( NUM_CONCURRENT_K );
            for ( unsigned int i = 0; i < NUM_CONCURRENT_K; ++i )
               {
               ThreadParamUrl param_process ( this, topk[i] );
               pthread_create ( &threads_container[i], nullptr, processThreadHelper, ( void* ) &param_process );
               }

            printf ( "@ (run) join k thread i\n" );
            for ( unsigned int i = 0; i < NUM_CONCURRENT_K; ++i )
               pthread_join ( threads_container[i], nullptr );

            printf ( "@ (run) finish k\n" );
            }

         printf ( "@ (run) finish all process thread, wait for listen url & signal thread\n" );

         pthread_join ( listen_url_thread, nullptr );
         pthread_join ( listen_signal_thread, nullptr );

         printf ( "* (run) finish\n" );
         }
   };

int main ( int argc, const char** argv )
   {
   Server server;
   server.run();
   printf ( "* (main) main function end\n" );
   }