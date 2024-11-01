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

struct Server
   {
   CrawlerServer* Instance;

   explicit Server ( CrawlerServer* instance )
      : Instance ( instance ) {}
   };


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
         Server* param = ( Server* ) arg;
         printf ( "# (listenSignalThreadHelper) start on host %s port %s\n", "localhost", UMSE_CRAWLER_LISTEN_SIGNAL_PORT );
         return param->Instance->listenSignalThread ( nullptr );
         }

      /* Thread for listen new url */
   public:
      void* listenMsgThreadHandler ( void* arg )
         {
         //ThreadParamFh* param = ( ThreadParamFh* ) arg;
         int fh = ( intptr_t ) arg;
         //FhRAII fhRAII ( fh );
         printf ( "# (listenMsgThreadHandler) [t %ld] [socket %d] start to handle socket \n", pthread_self(), fh );

         string content;
         if ( tcp_read ( fh, content ) < 0 )
            {
            printf ( "E (listenMsgThreadHandler) [t %ld] [socket %d] read from socket failed\n", pthread_self(), fh );
            return ( void* ) -1;
            }

         printf ( "@ (listenMsgThreadHandler) [t %ld] [socket %d] read from socket msg `%s`\n", pthread_self(), fh, content.c_str() );

         // A back up way of sending end signal
         if ( content == UMSE_CRAWLER_END_SIGNAL )
            {
            printf ( "* (listenMsgThreadHandler) [t %ld] [socket %d] receive end signal\n", pthread_self(), fh );
            threadSignal.set_terminate();
            threadQueue.set_terminate();
            return ( void* ) 1;
            }

         threadQueue.push ( content );

         /*
         if ( close ( fh ) == -1 )
            {
            #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
            printf ( "E (listenMsgThreadHandler) [t %ld] [socket %d] UNABLE TO CLOSE SOCKET\n", pthread_self(), fh );
            #endif
            }
         */

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

         int listenFd, connecFh;
         socklen_t addrlen, len;
         sockaddr* cliaddr;
         vector<pthread_t> threadsContainer;
         vector<ThreadParamFh> paramContainer;

         /* (1) create listen socket */
         listenFd = tcp_listen ( "localhost", UMSE_CRAWLER_LISTEN_MSG_PORT, &addrlen );
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
            try
               {
               connecFh = accept_retry ( listenFd, cliaddr, &len );
               }
            catch ( ... )
               {
               printf ( "E (CrawlerServer::listenMsgThreadMain) accept throw error\n" );
               continue;
               }

            if ( connecFh < 0 )
               {
               printf ( "E (listenMsgThreadMain) accept failed\n" );
               continue;
               }

            printf ( "\n@ (listenMsgThreadMain) receive new connection on [%d], make thread to handle\n", connecFh );

            // Create listen socket
            pthread_t thread_t;
            paramContainer.emplace_back ( this, connecFh );
            threadsContainer.emplace_back ( thread_t );

            #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
            printf ( "@ (listenMsgThreadMain) let thread %d handle socket %d\n", threadsContainer.at ( threadsContainer.size() - 1 ), paramContainer.at ( paramContainer.size() - 1 ).Fh );
            #endif

            pthread_create ( &threadsContainer.at ( threadsContainer.size() - 1 ), nullptr, &listenMsgThreadHandlerHelper, ( void* ) &paramContainer.at ( paramContainer.size() - 1 ) );

            // Check if previous thread have ended, if ended, remove them from vector
            // This help avoid saving too much things on the vector<pthread_t> container
            for ( auto iter = threadsContainer.begin(); iter != threadsContainer.end(); /* NOTHING */ )
               {
               if ( pthread_kill ( *iter, 0 ) == ESRCH )
                  {
                  #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
                  printf ( "@ (CrawlerServer::listenMsgThreadMain) thread %d have end, remove it from container\n", *iter );
                  #endif

                  paramContainer.erase ( paramContainer.begin() + ( iter - threadsContainer.begin() ) );
                  iter = threadsContainer.erase ( iter );
                  }
               else
                  ++iter;
               }
            }

         printf ( "@ (listenMsgThreadMain) receive end message, wait for all child to finish\n" );
         for ( unsigned int i = 0; i < threadsContainer.size(); ++i )
            pthread_join ( threadsContainer[i], nullptr );

         printf ( "* (listenMsgThreadMain) finish\n" );
         return ( void* ) 1;
         }

      static void* listenMsgThreadMainHelper ( void* arg )
         {
         Server* param = ( Server* ) arg;
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
         Server param_listen_url ( this );
         //Server param_listen_signal ( this );

         pthread_create ( &listen_url_thread, nullptr, listenMsgThreadMainHelper, ( void* ) &param_listen_url );
         //pthread_create ( &listen_signal_thread, nullptr, listenSignalThreadHelper, ( void* ) &param_listen_signal );

         /*
          vector<string> topk;

          printf ( "@ (run) begin start k thread\n" );
          while ( threadSignal.is_alive() )
             {
             printf ( "@ (run) get top k i\n" );
             threadQueue.get_top_k ( topk, NUM_CONCURRENT_K );

             if ( ! threadSignal.is_alive() )
                break;

             printf ( "@ (run) create k thread i\n" );
             vector<pthread_t> threadsContainer ( NUM_CONCURRENT_K );
             for ( unsigned int i = 0; i < NUM_CONCURRENT_K; ++i )
                {
                ThreadParamUrl param_process ( this, topk[i] );
                pthread_create ( &threadsContainer[i], nullptr, processThreadHelper, ( void* ) &param_process );
                }

             printf ( "@ (run) join k thread i\n" );
             for ( unsigned int i = 0; i < NUM_CONCURRENT_K; ++i )
                pthread_join ( threadsContainer[i], nullptr );

             printf ( "@ (run) finish k\n" );
             }

          printf ( "@ (run) finish all process thread, wait for listen url & signal thread\n" );
         */

         pthread_join ( listen_url_thread, nullptr );
         //pthread_join ( listen_signal_thread, nullptr );

         printf ( "* (run) finish\n" );
         }
   };

int main ( int argc, const char** argv )
   {
   Server server;
   server.run();
   printf ( "* (main) main function end\n" );
   }