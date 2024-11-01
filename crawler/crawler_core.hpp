//
// crawler_core.h
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
// Wenxuan Zhao zwenxuan@umich.edu
//
// CrawlerServer OOP
//
#pragma once

#include <cstdio>    // printf
#include <pthread.h> // concurrent
#include <unistd.h>  // sleep
#include <exception> // exception
#include <cxxabi.h>  // abi::__forced_unwind& 
#include <signal.h>  // pthread_kill

#include "configs/crawler_cfg.hpp" // configuration
#include "crawler_frontier.hpp" // composition
#include "crawler_filter.hpp" // composition

#include "url_parser.hpp"      // parse url
#include "network_utility.hpp" // network related helper funcitons
#include "html_parser.hpp"     // html parser
#include "thread_signal.hpp"   // signal for dead lock
#include "result_handler.hpp"  // handle parsed result
#include "raii_fh.hpp"         // socket raii
#include "valid_url.hpp"       // validate urls
#include "hash_helper.hpp"     // hash hostname to machine id
#include "url_fixer.hpp"       // fix url
#include "raii_thread_mtx.hpp" // mtx raii


#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

class CrawlerServer;

/* Struct for thread parameter */
struct TP
   {
   CrawlerServer* Instance;
   explicit TP ( CrawlerServer* instance ) : Instance ( instance ) {}
   };

struct TPIntPthread
   {
   CrawlerServer* Instance;
   int Data;
   pthread_t ThreadId;
   int UniqueId;
   static int count;
   explicit TPIntPthread ( CrawlerServer* instance, int data ) : Instance ( instance ), Data ( data )
      {
      UniqueId = ++count;
      }
   };

int TPIntPthread::count = 0;

bool operator< ( const TPIntPthread& lhs, const TPIntPthread& rhs )
   {
   return lhs.UniqueId < rhs.UniqueId;
   }

struct ThreadInfo
   {
   pthread_t Thread_t;
   CrawlerServer* Instance;
   int Data;
   bool Finish;
   explicit ThreadInfo ( CrawlerServer* instance, int data ) : Instance ( instance ), Data ( data ), Finish ( false ) {}
   };

class SiganlRAII
   {
      pthread_mutex_t* Mtx;
      bool* Finish;
   public:
      SiganlRAII ( pthread_mutex_t* mtx, bool* finish ) : Finish ( finish ), Mtx ( mtx ) {}
      ~SiganlRAII()
         {
         MtxRAII raii ( Mtx );
         *Finish = true;
         }
   };

struct TPIntVector
   {
   CrawlerServer* Instance;
   int Data;
   vector<ThreadInfo>* ThreadsContainer;
   explicit TPIntVector ( CrawlerServer* instance, int data, vector<ThreadInfo>* threadContainer ) : Instance ( instance ), Data ( data ), ThreadsContainer ( threadContainer ) {}
   };

/* Crawler Server */
class CrawlerServer
   {
      /* Composition Class */
   private:
      Frontier frontier;
      Filter filter;
      ResultHandler resultHandler;

      /* Output Folder */
   private:
      string parsedOutputDir;
      string htmlOutputDir;

      /* Signal for end*/
   private:
      ThreadSignal threadSignal;

      /* Parallel download, parse, handle parse result */
   private:
      const size_t numConcurrentK;

      vector<string> topKUrls;
      vector<ThreadInfo> processThreadsContainer;
      pthread_mutex_t processEndMtx = PTHREAD_MUTEX_INITIALIZER;

      /* Parallel send url */
   private:
      const size_t numMachines; // total number of machine
      size_t currMachineId;
      int refreshMachineHostnameCounter; // counter that periodically refresh macnhien hostname
      string currMachineHostname; // current machine hostname
      string machineHostnamesFilename; // filename that store all the machine hostname
      vector<string> machineHostnames; // container that store all the machine hostname

      vector<string> machineNewUrls;
      pthread_mutex_t machineNewUrlsMtx = PTHREAD_MUTEX_INITIALIZER;
      vector<ThreadInfo> sendUrlThreadsContainer;

      /* Check alive support */
   private:
      string heartBeatFilename;

   private:
      void makeHeartBeat()
         {
         try
            {
            int file_d = open( heartBeatFilename.c_str(), O_RDWR, 0);
            if ( file_d == -1 )
               {
               printf ( "E [t %ld] (CrawlerServer::makeHeartBeat) failed to write heart beat message to %s\n", pthread_self(), heartBeatFilename.c_str() );
               return;
               }
            lseek(file_d, 0, SEEK_CUR);
            string line = "hb\n";
            write(file_d, line.c_str(), strlen(line.c_str()) - 1);
            close(file_d);
            }
         catch ( ... )
            {
            printf ( "E [t %ld] (CrawlerServer::makeHeartBeat) throw error\n", pthread_self() );
            return;
            }
         }

   public:
      explicit CrawlerServer ( size_t machineId ) : numMachines ( UMSE_CRAWLER_NUM_MACHINE ), numConcurrentK ( UMSE_CRAWLER_NUM_CONCURRENT )
         {
         printf ( "# [t %ld] (CrawlerServer::CrawlerServer) start init\n", pthread_self() );
         machineHostnamesFilename = string ( UMSE_CRAWLER_HOSTNAMES_FILENAME );
         currMachineId = machineId;
         refreshMachineHostnameCounter = 0;
         if ( loadMachinesHostnames() < 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::CrawlerServer) load machine hostname failed, unable to start crawler\n", pthread_self() );
            fflush ( stdout );
            exit ( -1 );
            }
         currMachineHostname = machineHostnames.at ( currMachineId );
         printf ( "@ [t %ld] machineHostnames.size `%zu`\n", pthread_self(), machineHostnames.size() );
         machineNewUrls.resize ( numMachines, string() );
         printf ( "@ [t %ld] machineNewUrls.size `%zu`\n", pthread_self(), machineNewUrls.size() );
         printf ( "@ [t %ld] currMachineId `%zu`\n", pthread_self(), currMachineId );
         printf ( "@ [t %ld] num concurrent `%zu`\n", pthread_self(), numConcurrentK );
         topKUrls.reserve ( numConcurrentK );
         processThreadsContainer.reserve ( numConcurrentK );
         sendUrlThreadsContainer.reserve ( numMachines );
         machineNewUrls.reserve ( numMachines );
         machineHostnames.reserve ( numMachines );
         parsedOutputDir = string ( UMSE_CRAWLER_PARSED_OUTPUT_FOLDER );
         htmlOutputDir = string ( UMSE_CRAWLER_HTMLDOWNLOAD_OUTPUT_FOLDER );
         heartBeatFilename = string ( UMSE_CRAWLER_CHECK_ALIVE_CHECKPOINT );
         }

      ~CrawlerServer() = default;

      /* Helper function that load machine hostnames */
   private:
      // Load hostname configuration file
      // Return:
      //    -1 if load filie succes
      //     1 if failed
      int loadMachinesHostnames()
         {
         int file_d = open( machineHostnamesFilename.c_str(), O_RDWR, 0 );
         if ( file_d == -1 )
            {
            printf ( "E [t %ld] (CrawlerServer::loadMachinesHostnames) unable to open hostname file\n", pthread_self() );
            return -1;
            }

         machineHostnames.clear();
         machineHostnames.reserve ( numMachines );

         string line;
         char buf;
         while ( read(file_d, buf, 1) > 0 )
            {
            if ( buf == '\n' )
               machineHostnames.push_back ( line );
            else 
               line += buf;
            }
            machineHostnames.push_back ( line );

         if ( machineHostnames.size() != numMachines )
            {
            printf ( "E [t %ld] (CrawlerServer::loadMachinesHostnames) hostname file have %lu machine, but %zu machine in total\n", pthread_self(), machineHostnames.size(), numMachines );
            return -1;
            }

         machineHostnames.push_back ( "localhost" );
         for ( size_t i = 0; i < machineHostnames.size(); ++i )
            printf ( "@ [t %ld] (CrawlerServer::loadMachinesHostnames) machine %zu hostname %s\n",  pthread_self(), i, machineHostnames.at ( i ).c_str() );

         return 1;
         }

      /* Thread for listen new url */
   public:
      void* listenMsgThreadHandler ( void* arg )
         {
         int fh = ( intptr_t ) arg;
         printf ( "# [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] start to handle socket \n", pthread_self(), fh );

         string content;
         if ( tcp_read ( fh, content ) < 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] EXTREMELY DANGEROUS, INDICATING INTERMACHINE COMM FAIL read from socket failed\n", pthread_self(), fh );
            fflush ( stdout );
            return ( void* ) -1;
            }

         printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] read from socket msg `%s`\n", pthread_self(), fh, content.c_str() );
         //fflush ( stdout );

         //if ( content == UMSE_CRAWLER_END_SIGNAL )
         //   {
         //   printf ( "* [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] receive end signal\n", pthread_self(), fh );
         //   fflush ( stdout );
         //   threadSignal.set_terminate();
         //   frontier.set_terminate();
         //   return ( void* ) 1;
         //   }

         if ( content.size() == 0 || content.size() == 1 || strstr ( content.c_str(), "https" ) == nullptr )
            {
            printf ( "E [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] receive content `%s` invalid\n", pthread_self(), fh, content.c_str() );
            fflush ( stdout );
            return ( void* ) 1;
            }

         // Parse the incoming urls
         // Two input format
         vector<string> newUrls;
         //std::istringstream iss ( content );
         int index = 0;
         string url;
         content += " ";
         while ( index < content.length() )
            {
            if ( content[index] == " " )
               {
               if ( url.size() == 0 || url.size() == 1 )
                  break;

               if ( filter.canAddToFrontier ( url ) )
                  {
                  printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] receive url i `%s` CAN ADD to frontier\n", pthread_self(), fh, url.c_str() );
                  newUrls.push_back ( url );
                  }
               else
                  printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] receive url i `%s` CAN NOT ADD to frontier\n", pthread_self(), fh, url.c_str() );

               url = "";
               }
            else
               {
                url += content[index];
               }
            index++;
            }

         printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] before add url, FRONTIER SIZE `%zu`, add `%ld` urls to frontier\n", pthread_self(), fh, frontier.size(), newUrls.size() );
         fflush ( stdout );

         if ( newUrls.size() )
            frontier.AddUrl ( newUrls );

         printf ( "* [t %ld] (CrawlerServer::listenMsgThreadHandler) [socket %d] end\n\n", pthread_self(), fh );
         fflush ( stdout );
         return ( void* ) 1;
         }

      static void* listenMsgThreadHandlerHelper ( void* arg )
         {
         TPIntPthread* param = ( TPIntPthread* ) arg;
         int fh = param->Data;
         printf ( "# [t %ld] (CrawlerServer::listenMsgThreadHandlerHelper) start on thread `%ld` socket `%d`\n", pthread_self(), param->ThreadId, param->Data );
         return param->Instance->listenMsgThreadHandler ( ( void* ) fh );
         }

      void* listenMsgThreadMain ( void* )
         {
         printf ( "# [t %ld] (CrawlerServer::listenMsgThreadMain) start on host %s port %s\n", pthread_self(), currMachineHostname.c_str(), UMSE_CRAWLER_LISTEN_MSG_PORT );
         int connecFh, listenFd;
         socklen_t addrlen, len;
         sockaddr* cliaddr;

         set<TPIntPthread> listenMsgThreadParamContainer;

         // (1) create listen socket
         listenFd = tcp_listen ( currMachineHostname.c_str(), UMSE_CRAWLER_LISTEN_MSG_PORT, &addrlen );
         FhRAII listenFdRAII ( listenFd );
         printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) create listen socket %d success\n", pthread_self(), listenFd );
         if ( listenFd < 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::listenMsgThreadMain) error when createing listen thread\n", pthread_self() );
            fflush ( stdout );
            exit ( -1 );
            }

         // (2) listen for new message
         printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) START LISTEN MSG ON SOCKET [%d]\n", pthread_self(), listenFd );
         cliaddr = ( struct sockaddr* ) malloc ( addrlen );

         int count = 0;

         while ( threadSignal.is_alive() )
            {
            printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) wait for msg\n", pthread_self() );
            //fflush ( stdout );

            len = addrlen;
            try
               {
               connecFh = accept_retry ( listenFd, cliaddr, &len );
               if ( connecFh < 0 )
                  {
                  printf ( "E [t %ld] (CrawlerServer::listenMsgThreadMain) accept failed\n", pthread_self() );
                  fflush ( stdout );
                  continue;
                  }
               printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) new message come\n", pthread_self() );
               }
            catch ( ... )
               {
               printf ( "E [t %ld] (CrawlerServer::listenMsgThreadMain) accept throw error\n", pthread_self() );
               fflush ( stdout );
               continue;
               }

            printf ( "\n@ [t %ld] (CrawlerServer::listenMsgThreadMain) receive new connection on [socket %d], curr have `%ld` listen thread alive, make thread to handle, current FRONTIER SIZE %ld\n", pthread_self(), connecFh, listenMsgThreadParamContainer.size(), frontier.size()  );
            //fflush ( stdout );

            // (2.2) clean up old threads
            count++;
            if ( count % 500 == 0 )
               {
               printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) start to clean up threads\n", pthread_self() );
               count = 0;
               for ( auto iter = listenMsgThreadParamContainer.begin(); iter != listenMsgThreadParamContainer.end(); /* NOTHING */ )
                  {
                  fflush ( stdout );
                  if ( pthread_kill ( iter->ThreadId, 0 ) == ESRCH )
                     {
                     printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) thread have end, remove it from container\n", pthread_self() );
                     fflush ( stdout );
                     iter = listenMsgThreadParamContainer.erase ( iter );
                     }
                  else
                     ++iter;
                  }
               }

            // (2.1) listen for new message
            printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) let thread handle socket `%d`\n", pthread_self(), connecFh );
            auto thread_param_iter = listenMsgThreadParamContainer.insert ( TPIntPthread ( this, connecFh ) );
            if ( thread_param_iter.second )
               {
               int code = pthread_create ( ( pthread_t* ) ( & ( thread_param_iter.first->ThreadId ) ), nullptr, &listenMsgThreadHandlerHelper, ( void* ) & ( *thread_param_iter.first ) ); // cast away const
               if ( code != 0 )
                  {
                  printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) pthread_create monitor thread listenMsgThreadHandlerHelper failed\n", pthread_self() );
                  //fflush ( stdout );
                  handle_error_en ( code, "pthread_create" );
                  }
               }
            else
               {
               printf ( "E [t %ld] (CrawlerServer::listenMsgThreadMain) (EXTREMELY WRONG) param or thread exist before, no thread is create to handle msg\n", pthread_self() );
               fflush ( stdout );
               exit ( -1 );
               }
            #ifdef __APPLE__
            pthread_yield_np();
            #else
            pthread_yield();
            #endif

            } // end of while loop for listen signal

         printf ( "@ [t %ld] (CrawlerServer::listenMsgThreadMain) receive end message, wait for all child to finish\n", pthread_self() );
         //fflush ( stdout );

         for ( auto iter = listenMsgThreadParamContainer.begin(); iter != listenMsgThreadParamContainer.end(); ++iter )
            pthread_join ( iter->ThreadId, nullptr );

         printf ( "* [t %ld] (CrawlerServer::listenMsgThreadMain) finish\n", pthread_self() );
         fflush ( stdout );
         return ( void* ) 1;
         }

      static void* listenMsgThreadMainHelper ( void* arg )
         {
         TP* param = ( TP* ) arg;
         return param->Instance->listenMsgThreadMain ( nullptr );
         }

      /* Thread for listen end signal */
   public:
      void* listenSignalThread ( void* )
         {
         printf ( "# [t %ld] (CrawlerServer::listenSignalThread) start on host %s port %s\n", pthread_self(), currMachineHostname.c_str(), UMSE_CRAWLER_LISTEN_SIGNAL_PORT );
         int listenFd, connecFh;
         socklen_t addrlen, len;
         struct sockaddr*	cliaddr;

         /* (1) create listen socket */
         listenFd = tcp_listen ( currMachineHostname.c_str(), UMSE_CRAWLER_LISTEN_SIGNAL_PORT, &addrlen );
         FhRAII listenFdRAII ( listenFd );
         printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) create listen socket %d success\n", pthread_self(), listenFd );
         if ( listenFd < 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::listenSignalThread) error when createing listen thread\n", pthread_self() );
            fflush ( stdout );
            exit ( -1 );
            }

         /* (2) listen for close signal */
         printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) START LISTEN END SIGNAL ON [%d]\n", pthread_self(), listenFd );
         cliaddr = ( struct sockaddr* ) malloc ( addrlen );
         while ( threadSignal.is_alive() )
            {
            printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) wait for msg\n", pthread_self() );
            // Get a new connection
            len = addrlen;
            try
               {
               connecFh = accept ( listenFd, cliaddr, &len );

               if ( connecFh < 0 )
                  {
                  printf ( "E [t %ld] (CrawlerServer::listenSignalThread) accept failed\n", pthread_self() );
                  fflush ( stdout );
                  continue;
                  }
               }
            catch ( ... )
               {
               printf ( "E [t %ld] (CrawlerServer::listenSignalThread) accept throw error\n", pthread_self() );
               fflush ( stdout );
               continue;
               }

            printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) receive message from socket [%d]\n", pthread_self(), listenFd );

            string content;
            if ( tcp_read ( connecFh, content ) < 0 )
               {
               printf ( "E [t %ld] (CrawlerServer::listenSignalThread) READ CONNECTED SOCKET %d FAIL, CONTINUE\n", pthread_self(), connecFh );
               fflush ( stdout );
               continue;
               }

            if ( content == UMSE_CRAWLER_END_SIGNAL )
               {
               printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) receive end message, try to end whole process\n", pthread_self() );
               fflush ( stdout );
               threadSignal.set_terminate();
               frontier.set_terminate();
               break;
               }
            else
               printf ( "@ [t %ld] (CrawlerServer::listenSignalThread) receive message %s but not end message\n", pthread_self(), content.c_str() );
            } // end of while loop

         printf ( "* [t %ld] (CrawlerServer::listenSignalThread) thread end\n", pthread_self() );
         fflush ( stdout );
         return ( void* ) 1;
         }

      static void* listenSignalThreadHelper ( void* arg )
         {
         TP* param = ( TP* ) arg;
         return param->Instance->listenSignalThread ( nullptr );
         }

      /* Thread for send new found url to corr machine */
   public:
      void* sendUrlThread ( void* arg )
         {
         int target_machine_id = ( intptr_t ) arg;
         SiganlRAII signalRaii ( &processEndMtx, &sendUrlThreadsContainer.at ( target_machine_id ).Finish );

         if ( machineNewUrls.at ( target_machine_id ).size() > 30000 )
            {
            printf ( "@ [t %ld] (CrawlerServer::sendUrlThread) try to send machine id `%d` hostname `%s` msg `%s`\n", pthread_self(), target_machine_id,  machineHostnames.at ( target_machine_id ).c_str(),  machineNewUrls.at ( target_machine_id ).c_str() );
            if ( tcp_connect_send ( machineHostnames.at ( target_machine_id ).c_str(), UMSE_CRAWLER_LISTEN_MSG_PORT, machineNewUrls.at ( target_machine_id ).c_str() ) < 0 )
               {
               printf ( "E [t %ld] (CrawlerServer::sendUrlThread) SEND URL TO MACHINE FAILED MID `%d` on host `%s``\n", pthread_self(), target_machine_id, machineHostnames.at ( target_machine_id ).c_str() );
               printf ( "# [t %ld] (CrawlerServer::sendUrlThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FAILED TO MAKE INTER-MACHINE COMMUNICATION!!!!!!!!!\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self() );
               fflush ( stdout );
               }
            else
               {
               printf ( "* [t %ld] (CrawlerServer::sendUrlThread) SEND URL TO MACHINE SUCCESS MID `%d` on host `%s` urls `%s`\n", pthread_self(), target_machine_id, machineHostnames.at ( target_machine_id ).c_str(), machineNewUrls.at ( target_machine_id ).c_str() );
               fflush ( stdout );
               machineNewUrls.at ( target_machine_id ) = "";
               }
            }

         return ( void* ) 1;
         }

      static void* sendUrlThreadHelper ( void* arg )
         {
         ThreadInfo* param = ( ThreadInfo* ) arg;
         int target_machine_id = param->Data;
         return param->Instance->sendUrlThread ( ( void* ) target_machine_id );
         }

      /* add new found url to container & wait for it to be send later */
   public:
      // Add url to machineNewUrls in order to send all of its at the same time
      // This function would only called by process thread, so it's ok for this function and its component not have a catch throw logic
      void addNewFoundUrl ( const string& url )
         {
         MtxRAII raii ( &machineNewUrlsMtx );

         ParsedUrl parsedUrl ( url );
         size_t target_machine_id = hostname_to_machine ( url.c_str(), numMachines );

         // If the url is to local machine, then send to frontier directly
         if ( target_machine_id == currMachineId && filter.canAddToFrontier ( url ) )
            {
            printf ( "* [t %ld] (CrawlerServer::addNewFoundUrl) url `%s` add to local machine `%zu` frontier\n", pthread_self(), url.c_str(), currMachineId );
            frontier.AddUrl ( url );
            return;
            }

         machineNewUrls.at ( target_machine_id ) += url;
         machineNewUrls.at ( target_machine_id ) += " ";
         }

      /* Main download, parse, handle thread */
   public:
      /* Core Functionality */
      // Doc
      // Killing a thread that contain the lock https://stackoverflow.com/questions/14268080/cancelling-a-thread-that-has-a-mutex-locked-does-not-unlock-the-mutex
      void* processThread ( void* arg )
         {
         int id = ( intptr_t ) arg;
         SiganlRAII signalRaii ( &processEndMtx, &processThreadsContainer.at ( id ).Finish );

         string url ( topKUrls.at ( id ) );
         printf ( "# [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!START!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );

         // (1) download html page
         string content;
         try
            {
            printf ( "# [t %ld] (CrawlerServer::processThread) [%s] TRY DOWNLOAD\n", pthread_self(), url.c_str()  );
            switch ( download_webpage_ssl ( url.c_str(), content ) )
               {
               case DOWNLOAD_WEBPAGE_SUCCESS:
                  break;

               case DOWNLOAD_WEBPAGE_FAIL:
                  printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FAILED 1 download failed!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
                  fflush ( stdout );
                  return ( void* ) -1;

               case DOWNLOAD_WEBPAGE_URL_REDIRECT:
                  printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] redirection happen, try to redirection\n", pthread_self(), url.c_str() );
                  //fflush ( stdout );

                  string redirectedUrl = handle_redirect_ssl ( url.c_str() );

                  printf ( "@ [t %ld] (CrawlerServer::processThread) call helper function finish\n", pthread_self() );
                  //fflush ( stdout );

                  if ( !redirectedUrl.empty() && redirectedUrl.size() < 200 && strstr ( redirectedUrl.c_str(), "https://" ) != nullptr )
                     {
                     printf ( "@ [t %ld] (CrawlerServer::processThread) `%s` redirect to `%s`\n", pthread_self(), url.c_str(), redirectedUrl.c_str() );
                     //fflush ( stdout );

                     addNewFoundUrl ( redirectedUrl );

                     printf ( "* [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!REDIRECT!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
                     return ( void* ) -1;
                     }
                  else
                     {
                     printf ( "E [t %ld] (CrawlerServer::processThread) handle redirection on `%s` failed, may due to too large redirected url, not https, empty redirect\n", pthread_self(), url.c_str() );
                     fflush ( stdout );

                     printf ( "* [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!REDIRECT FAILED!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
                     return ( void* ) -1;
                     }
               }
            printf ( "# [t %ld] (CrawlerServer::processThread) [%s] DOWNLOAD FINISH\n", pthread_self(), url.c_str()  );
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "# [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FAILED 3 download throw error!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
            fflush ( stdout );
            return ( void* ) -1;
            }

         // (2) parse html page
         // use try catch mechnisim to avoid parser failed. member variable will be destroyed after catch exit
         HtmlParser htmlParser ( content );
         try
            {
            printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] TRY PARSE\n", pthread_self(), url.c_str()  );
            htmlParser.parse();
            printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] PARSE FINISH\n", pthread_self(), url.c_str()  );
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "# [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FAILED 5 parse throw error!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
            fflush ( stdout );
            return ( void* ) -1;
            }

         // (3) save parsed result
         try
            {
            printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] TRY SAVE PARSED RESULT\n", pthread_self(), url.c_str() );
            if ( resultHandler.saveParseResultToFile ( htmlParser, url.c_str(), parsedOutputDir ) < 0 )
               {
               printf ( "E [t %ld] (CrawlerServer::processThread) [%s] error when try save parsed result \n", pthread_self(), url.c_str() );
               fflush ( stdout );
               }
            printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] SAVE PARSED RESULT FINISH\n", pthread_self(), url.c_str() );
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FAILED 6 save parsed result throw error!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
            fflush ( stdout );
            return ( void* ) -1;
            }

         // (4) Send new found url to given machine
         printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] TRY HANDLING PARSED URL\n", pthread_self(), url.c_str() );

         // (4.1) ignore the page if too much url found
         if ( htmlParser.links.size() > 1000 )
            {
            printf ( "* [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!IGNORE too much link!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str() );
            return ( void* ) -1;
            }

         // (4.2) correct the html, save it in container, let the run() to send all urls at once
         string baseUrl = ( htmlParser.base != "" ? htmlParser.base : url );
         ParsedUrl parsedUrl ( baseUrl.c_str( ) );
         for ( auto iter = htmlParser.links.begin(); iter != htmlParser.links.end(); ++iter )
            {
            printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] before fix url`%s`\n", pthread_self(), url.c_str(), iter->URL.c_str() );
            fix_url_to_correct_format ( iter->URL, parsedUrl, baseUrl );

            if ( valid_url ( iter->URL ) )
               {
               printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] anchor url `%s` VALID\n", pthread_self(), url.c_str(), iter->URL.c_str() );
               try
                  {
                  addNewFoundUrl ( iter->URL );
                  }
               #ifndef __APPLE__
               catch ( abi::__forced_unwind& )
                  {
                  printf ( "E [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
                  fflush ( stdout );
                  throw;
                  }
               #endif
               catch ( ... )
                  {
                  printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] add new found url `%s` throw exception \n", pthread_self(), url.c_str(), iter->URL.c_str() );
                  return ( void* ) -1;
                  }
               }
            else
               printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] anchor url `%s` INVALID\n", pthread_self(), url.c_str(), iter->URL.c_str() );
            }
         printf ( "@ [t %ld] (CrawlerServer::processThread) [%s] HANDLING PARSED URL FINISH\n", pthread_self(), url.c_str() );
         
         // VERY VERY IMPORTANT! DO NOT DELET BELOW LINE
         sleep ( 0.001 );

         printf ( "* [t %ld] (CrawlerServer::processThread) [%s] FINISH HANDLING URL\n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FINISH!!!!!!!!!\n`%s`\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), url.c_str(), url.c_str() );
         fflush ( stdout );
         return ( void* ) 1;
         }

      static void* processThreadHelper ( void* arg )
         {
         ThreadInfo* param = ( ThreadInfo* ) arg;
         int id = param->Data;
         return param->Instance->processThread ( ( void* ) id  );
         }

      /* Kill Thread */
   public:
      void* monitorThread ( void* arg )
         {
         TPIntVector* param = ( TPIntVector* ) arg;
         int sleep_time = param->Data;
         vector<ThreadInfo>* threads_container_ptr = param->ThreadsContainer;

         printf ( "# [t %ld] (monitorThread::monitorThread) start, sleep for `%d` min\n", pthread_self(), sleep_time );
         fflush ( stdout );

         sleep ( 60 * sleep_time );

         printf ( "@ [t %ld] (monitorThread::monitorThread) off sleep, kill all child\n", pthread_self() );
         fflush ( stdout );

         for ( size_t i = 0; i < threads_container_ptr->size(); ++i )
            {
            MtxRAII raii ( &processEndMtx );
            if ( !threads_container_ptr->at ( i ).Finish )
               {
               printf ( "@ [t %ld] thread `%zu` `%ld` not yet finish, call cancel\n", pthread_self(), i, threads_container_ptr->at ( i ).Thread_t );
               fflush ( stdout );
               pthread_cancel ( threads_container_ptr->at ( i ).Thread_t );
               }
            else
               {
               printf ( "@ [t %ld] thread %zu already finish, no nothing\n", pthread_self(), i );
               fflush ( stdout );
               }
            }

         sleep ( 60 * sleep_time ); // to ensure monitor thread is killed.

         printf ( "* [t %ld] (monitorThread::monitorThread) reach end\n", pthread_self() );
         return ( void* ) 1;
         }

      static void* monitorThreadHelper ( void* arg )
         {
         TPIntVector* param = ( TPIntVector* ) arg;
         return param->Instance->monitorThread ( arg );
         }

      /* Driver Thread */
   public:
      // Main scheduler
      // Useage:
      //      CrawlerServer cwl;
      //      cwl.run(config_filename)
      void run()
         {
         printf ( "\n\n# [t %ld] (CrawlerServer::run) @@@@@@@CRAWLER START@@@@@@@\n", pthread_self() );
         int code = 0;

         pthread_t listenMsgThreadMainHelper_t;
         TP listenMsgThreadMainHelper_param ( this );
         code = pthread_create ( &listenMsgThreadMainHelper_t, nullptr, listenMsgThreadMainHelper, ( void* ) &listenMsgThreadMainHelper_param );
         if ( code != 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::run) pthread_create listenMsgThreadMainHelper failed\n", pthread_self() );
            fflush ( stdout );
            handle_error_en ( code, "pthread_create" );
            }

         pthread_t listenSignalThreadHelper_t;
         TP listenSignalThreadHelper_param ( this );
         code = pthread_create ( &listenSignalThreadHelper_t, nullptr, listenSignalThreadHelper, ( void* ) &listenSignalThreadHelper_param );
         if ( code != 0 )
            {
            printf ( "E [t %ld] (CrawlerServer::run) pthread_create listenSignalThreadHelper failed\n", pthread_self() );
            fflush ( stdout );
            handle_error_en ( code, "pthread_create" );
            }

         sleep ( 5 ); // sleep for 10 second to make sure all required socket are set correctly

         while ( threadSignal.is_alive() )
            {
            makeHeartBeat();

            // 1. get topk urls to process
            printf ( "@ [t %ld] (CrawlerServer::run) START TO get top k `%zu` on frontier with size `%zu`\n", pthread_self(), numConcurrentK, frontier.size() );
            fflush ( stdout );

            topKUrls.clear();
            frontier.getTopKUrls ( topKUrls );
            if ( topKUrls.empty() )
               break; // For fault tolerence, topk urls should never be empty

            makeHeartBeat();

            // Note: this function should be called after frontier.getTopKUrls
            if ( ! threadSignal.is_alive() )
               break;

            // 2. create k thread to download, parse, handle result
            printf ( "@ [t %ld] (CrawlerServer::run) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!CREATE K (%zu) THREADS!!!!!!!!!\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), numConcurrentK );
            fflush ( stdout );

            processThreadsContainer.clear();
            for ( size_t i = 0; i < numConcurrentK; ++i )
               {
               processThreadsContainer.push_back ( ThreadInfo( this, i )  );

               printf ( "@ [t %ld] (CrawlerServer::run) HANDLE URL i `%zu` on `%s`\n", pthread_self(), i, topKUrls.at ( i ).c_str() );
               code = pthread_create ( &processThreadsContainer.at ( i ).Thread_t, nullptr, processThreadHelper, ( void* ) &processThreadsContainer.at ( i ) );
               if ( code != 0 )
                  {
                  printf ( "E [t %ld] (CrawlerServer::run) (process) pthread_create thread `%ld` failed\n", pthread_self(), i );
                  fflush ( stdout );
                  handle_error_en ( code, "pthread_create" );
                  }
               #ifdef __APPLE__
               pthread_yield_np();
               #else
               pthread_yield();
               #endif
               }

            // 3. create monitor thread that would kill all k threads
            printf ( "@ [t %ld] (CrawlerServer::run) START TO create monitor thread for process url\n", pthread_self() );
            fflush ( stdout );

            pthread_t monitorThreadHelper_t1;
            TPIntVector monitorThreadHelper_param1 ( this, 2, &processThreadsContainer );
            code = pthread_create ( &monitorThreadHelper_t1, nullptr, monitorThreadHelper, ( void* ) &monitorThreadHelper_param1 );
            if ( code != 0 )
               {
               printf ( "E [t %ld] (CrawlerServer::run) pthread_create monitor thread failed\n", pthread_self() );
               fflush ( stdout );
               handle_error_en ( code, "pthread_create" );
               }

            // 4. wait k thread to finihs by join
            printf ( "@ [t %ld] (CrawlerServer::run) START TO wait join k threads i\n", pthread_self() );
            fflush ( stdout );

            for ( size_t i = 0; i < numConcurrentK; ++i )
               {
               printf ( "@ [t %ld] (CrawlerServer::run) (processing) try to join thread `%zu` for url `%s`\n", pthread_self(), i, topKUrls.at ( i ).c_str() );
               fflush ( stdout );

               pthread_join ( processThreadsContainer.at ( i ).Thread_t, nullptr );

               printf ( "@ [t %ld] (CrawlerServer::run) (processing) finish join thread for url `%s`\n", pthread_self(), topKUrls.at ( i ).c_str() );
               fflush ( stdout );
               }

            // 5. end the monitor threads
            printf ( "@ [t %ld] (CrawlerServer::run) START TO kill monitor threads for process data\n", pthread_self() );
            fflush ( stdout );

            pthread_cancel ( monitorThreadHelper_t1 );

            // 6. Send new found urls to given machine
            printf ( "@ [t %ld] (CrawlerServer::run) START TO send urls to given machine\n", pthread_self() );
            fflush ( stdout );

            if ( machineNewUrls.size() != numMachines )
               {
               printf ( "E [t %ld] (CrawlerServer::run) machineNewUrls have wrong size `%zu`\n", pthread_self(), machineNewUrls.size() );
               fflush ( stdout );
               exit ( -1 );
               }

            makeHeartBeat();

            sendUrlThreadsContainer.clear();
            for ( size_t i = 0; i < numMachines; ++i )
               {
               sendUrlThreadsContainer.push_back ( ThreadInfo ( this, i ) );
               if ( i != currMachineId )
                  {
                  printf ( "@ [t %ld] (CrawlerServer::run) SEND URL to machine `%ld`\n", pthread_self(), i );
                  fflush ( 0 );
                  code = pthread_create ( &sendUrlThreadsContainer.at ( i ).Thread_t, nullptr, sendUrlThreadHelper, ( void* ) &sendUrlThreadsContainer.at ( i ) );
                  if ( code != 0 )
                     {
                     printf ( "E [t %ld] (CrawlerServer::run) (send url) pthread_create thread `%ld` failed\n", pthread_self(), i );
                     fflush ( stdout );
                     handle_error_en ( code, "pthread_create" );
                     }
                  #ifdef __APPLE__
                  pthread_yield_np();
                  #else
                  pthread_yield();
                  #endif
                  }
               }

            // 7. create monitor thread that would kill all num machine thread
            printf ( "@ [t %ld] (CrawlerServer::run) START TO create monitor thread for send url\n", pthread_self() );
            fflush ( stdout );

            pthread_t monitorThreadHelper_t2;
            TPIntVector monitorThreadHelper_param2 ( this, 2, &sendUrlThreadsContainer );
            code = pthread_create ( &monitorThreadHelper_t2, nullptr, monitorThreadHelper, ( void* ) &monitorThreadHelper_param2 );
            if ( code != 0 )
               {
               printf ( "E [t %ld] (CrawlerServer::run) (send url) pthread_create monitor thread failed\n", pthread_self() );
               fflush ( stdout );
               handle_error_en ( code, "pthread_create" );
               }

            // 8. wait num machine thread to finihs by join
            printf ( "@ [t %ld] (CrawlerServer::run) START TO wait join num machine thread \n", pthread_self() );
            fflush ( stdout );

            for ( size_t i = 0; i < numMachines; ++i )
               {
               printf ( "@ [t %ld] (CrawlerServer::run) (send url) try to join thread `%ld`\n", pthread_self(), i );
               fflush ( stdout );

               if ( i != currMachineId)
                  pthread_join ( sendUrlThreadsContainer.at ( i ).Thread_t, nullptr );

               printf ( "@ [t %ld] (CrawlerServer::run) (send url) finish join thread`\n", pthread_self() );
               fflush ( stdout );
               }

            // 9. end the monitor threads
            printf ( "@ [t %ld] (CrawlerServer::run) START TO kill monitor threads for send url\n", pthread_self() );
            fflush ( stdout );

            pthread_cancel ( monitorThreadHelper_t2 );

            printf ( "@ [t %ld] (CrawlerServer::run) pthread_cancel called on monitor finish\n", pthread_self() );
            fflush ( stdout );

            // 10. decided if reload hostnames
            refreshMachineHostnameCounter++;
            if ( refreshMachineHostnameCounter % 100 )
               {
               refreshMachineHostnameCounter = 0;
               loadMachinesHostnames();
               }

            // 11. print end signal
            printf ( "@ [t %ld] (CrawlerServer::processThread) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!FINISH WHOLE K (%zu) THREADS!!!!!!!!!\n!!!!!!!!!!!!!!!!!!!!!!!!\n\n", pthread_self(), numConcurrentK );
            fflush ( stdout );
            makeHeartBeat();
            } // end of while loop


         printf ( "@ [t %ld] (CrawlerServer::run) finish all process thread, wait for listen url & signal thread\n", pthread_self() );

         pthread_join ( listenMsgThreadMainHelper_t, nullptr );
         pthread_join ( listenSignalThreadHelper_t, nullptr );

         makeHeartBeat();

         printf ( "* [t %ld] (CrawlerServer::run) @@@@@@@CRAWLER FINISH@@@@@@@\n", pthread_self() );
         }// end of run
   };
