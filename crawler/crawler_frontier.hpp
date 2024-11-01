//
// dummy_frontier.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
// Wenxuan Zhao zwenxuan@umich.edu
//
#pragma once

#include <cstdio>                  // printf
#include <unistd.h>                // write, read, open
#include <sys/stat.h>
#include <fcntl.h>                 // O_RDONLY
#include <pthread.h>               // Multithread
#include <random>
#include <cxxabi.h>  // abi::__forced_unwind& for MacOS

#include "configs/crawler_cfg.hpp" // configuration
#include "url_evaluator.hpp"       // evaluate url
#include "valid_url.hpp"
#include "raii_thread_mtx.hpp"     // RAII for Mtx
#include "raii_fh.hpp"             // RAII for Fh

struct UrlPack
   {
   int Score;
   string Url;

   UrlPack ( int score, const string& url ) : Score ( score ), Url ( url ) {}
   UrlPack ( const string& url, int score ) : Score ( score ), Url ( url ) {}
   UrlPack ( int score, const char* url ) : Score ( score ), Url ( url ) {}
   UrlPack ( const char* url, int score ) : Score ( score ), Url ( url ) {}
   UrlPack ( const UrlPack& other ) : Score ( other.Score ), Url ( other.Url ) {}
   };

class UrlPackCmp
   {
   public:
      bool operator() ( const UrlPack& lhs, const UrlPack& rhs )
         {
         return lhs.Score < rhs.Score;
         }
   };

template <class T, class S, class C>
S& Container ( priority_queue<T, S, C>& q )
   {
   struct HackedQueue : private priority_queue<T, S, C>
      {
      static S& Container ( priority_queue<T, S, C>& q )
         {
         return q.*& HackedQueue::c;
         }
      };
   return HackedQueue::Container ( q );
   };

class Frontier
   {
   private:
      // Container that store all urls
      priority_queue< UrlPack, vector<UrlPack>, UrlPackCmp > urls;
      vector<UrlPack>& urls_iterable = Container ( urls );

      // Evaluator
      EvaluateUrl evaluator;

      // Number of urls to get each time
      size_t K;
      size_t R;

      // For multi-thread & lock
      bool Signal;
      pthread_mutex_t urlsMtx = PTHREAD_MUTEX_INITIALIZER;
      pthread_cond_t urlsCv = PTHREAD_COND_INITIALIZER;

      // Checkpoint
      string frontierCheckpointDir;
      int count;
      int periodic_save;

   public:
      Frontier()
         {
         frontierCheckpointDir = string ( UMSE_CRAWLER_FRONTIER_CHECKPOINT );
         K = UMSE_CRAWLER_NUM_CONCURRENT;
         R = UMSE_CRAWLER_FRONTIER_TOPK_RATIO;
         periodic_save = 500;
         Signal = true;
         count = 0;

         if ( loadFrontierCheckpoint() < 0 )
            {
            printf ( "E [t %ld] (Frontier::AddUrl) [frontier size %ld] unable to load checkpoint, quit\n", pthread_self(), urls.size() );
            exit ( -1 );
            }
         }

      ~Frontier()
         {
         saveFrontierCheckpoint();
         }

      void set_terminate()
         {
         MtxRAII raii ( &urlsMtx );
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::set_terminate) start\n", pthread_self() );
         #endif

         Signal = false;
         pthread_cond_signal ( &urlsCv );

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::set_terminate) end\n", pthread_self() );
         #endif
         }


      size_t size()
         {
         //printf ( "# [t %ld] (Frontier::size) start\n", pthread_self() );
         MtxRAII raii ( &urlsMtx );
         //printf ( "* [t %ld] (Frontier::size) end\n", pthread_self() );
         return urls.size();
         }

   public:
      void AddUrl ( const vector<string>& newUrls )
         {
         MtxRAII raii ( &urlsMtx );
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] start\n", pthread_self(), urls.size() );
         #endif

         // Frontier size limitation
         if ( ( urls.size() + newUrls.size() ) > UMSE_CRAWLER_FRONTIER_MAX_SIZE )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "* [t %ld] (Frontier::AddUrl) [frontier size %ld] frontier is full, don't add\n", pthread_self(), urls.size() );
            #endif

            return;
            }

         // Try to add
         try
            {
            for ( auto newUrl : newUrls )
               {
               urls.push ( UrlPack( newUrl, evaluator.eval ( newUrl ) ) );

               #ifdef _UMSE_CRAWLER_FRONTIER_LOG
               printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] add %s to frontier\n", pthread_self(), urls.size(), newUrl.c_str() );
               #endif
               }
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (Frontier::AddUrl) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!LISTEN SIGNAL THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "E [t %ld] (Frontier::AddUrl) [frontier size %ld] frontier add url crash, quit add\n", pthread_self(), urls.size() );
            fflush ( stdout );
            return;
            }

         // Signal waitings thread to continue
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] singal wait thread to continue\n", pthread_self(), urls.size() );
         #endif

         // Broadcast for listener thread
         if (urls.size() < 1000) 
            pthread_cond_broadcast ( &urlsCv );
            

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::AddUrl) [frontier size %ld] add url finish\n", pthread_self(), urls.size() );
         fflush ( stdout );
         #endif
         }

      void AddUrl ( const string& newUrl )
         {
         AddUrl ( newUrl.c_str() );
         }

      void AddUrl ( const char* newUrl )
         {
         MtxRAII raii ( &urlsMtx );
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] start\n", pthread_self(), urls.size() );
         #endif

         if ( ( urls.size() + 1 ) > UMSE_CRAWLER_FRONTIER_MAX_SIZE )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] frontier is full, don't add\n", pthread_self(), urls.size() );
            #endif

            return;
            }

         try
            {
            urls.push ( UrlPack( newUrl, evaluator.eval ( newUrl ) ) );

            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] add %s to frontier\n", pthread_self(), urls.size(), newUrl );
            #endif
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (Frontier::AddUrl) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!LISTEN SIGNAL THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] frontier add url crash, quit add\n", pthread_self(), urls.size() );
            return;
            }

         // Signal waitings
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::AddUrl) [frontier size %ld] singal wait thread to continue\n", pthread_self(), urls.size() );
         #endif

         if (urls.size() < 1000) 
            pthread_cond_broadcast ( &urlsCv );
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::AddUrl) [frontier size %ld] add url finish\n", pthread_self(), urls.size() );
         fflush ( stdout );
         #endif
         }

      void getTopKUrls ( vector<string>& container )
         {
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::getTopKUrls) [frontier size %ld] try to get lock\n", pthread_self(), urls.size() );
         #endif

         MtxRAII raii ( &urlsMtx );

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::getTopKUrls) [frontier size %ld] start\n", pthread_self(), urls.size() );
         #endif

         /* (1) Wait if there are less than K urls */
         while ( urls.size() < R * K && Signal )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "@ [t %ld] (Frontier::getTopKUrls) [frontier size %ld] need %ld, release lock & wait\n", pthread_self(), urls.size(), K * R );
            #endif

            pthread_cond_wait ( &urlsCv, &urlsMtx );

            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "@ [t %ld] (Frontier::getTopKUrls) [frontier size %ld] receive cv signal, off wait\n", pthread_self(), urls.size() );
            #endif
            }

         if ( !Signal )
            return;

         if ( urls.size() <  R * K )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "E [t %ld] (Frontier::getTopKUrls) [frontier size %ld] invalid frontier size, need %ld, still been signaled\n", pthread_self(), urls.size(), R * K );
            fflush ( stdout );
            #endif

            return;
            }

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::getTopKUrls) [frontier size %ld] start to get top %ld\n", pthread_self(), urls.size(), K );
         #endif

         /* (2) First create a copy of R * K where R is the ratio */
         vector<UrlPack> tmp;
         for ( size_t i = 0; i < R * K; ++i )
            {
            //assert( !urls.empty() );
            tmp.push_back ( UrlPack( urls.top() ) );
            urls.pop();
            }

         /* (3) Select one url for every R url */
         container.clear();
         std::random_device rd;  //Will be used to obtain a seed for the random number engine
         std::mt19937 gen ( rd() ); //Standard mersenne_twister_engine seeded with rd()
         std::uniform_int_distribution<> distrib ( 0, R - 1 );
         for ( size_t i = 0; i < K ; ++i )
            {
            int rdm = distrib ( gen );
            for ( size_t r = 0; r < R; r++ )
               {
               if ( ( i * R + r ) % R == ( size_t ) rdm )
                  container.push_back ( tmp.at ( i * R + r ).Url );
               else
                  urls.push ( UrlPack( tmp.at ( i * R + r ) ) );
               }
            }
         // Periodic save checkpoint
         count += K;
         if ( count >= periodic_save )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "* [t %ld] (Frontier::AddUrl) [frontier size %ld] add url finish\n", pthread_self(), urls.size() );
            fflush ( stdout );
            #endif
            saveFrontierCheckpoint();
            count = 0;
            if ( periodic_save <= 10000 )
               periodic_save = periodic_save + 2000;
            }

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::getTopKUrls) [frontier size %ld] finish\n", pthread_self(), urls.size() );
         fflush ( stdout );
         #endif
         }

      // Store Checkpoints
      // Return:
      //    1 on success
      //    -1 on fail
      // Note: do not add lock here!!!!!. Only called internally
      int saveFrontierCheckpoint()
         {
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::saveFrontierCheckpoint) start\n", pthread_self() );
         fflush ( stdout );
         #endif

         // If no more url, don't save
         if ( urls.empty() )
            {
            #ifdef _UMSE_CRAWLER_FRONTIER_LOG
            printf ( "* [t %ld] (Frontier::saveFrontierCheckpoint) empty frontier, no save\n", pthread_self() );
            fflush ( stdout );
            #endif
            return 1;
            }

         // Use a simple version of write
         string content;
         for ( auto iter = urls_iterable.begin(); iter != urls_iterable.end(); ++iter )
            {
            content += iter->Url;
            content += "\n";
            }

         int file_d = open(frontierCheckpointDir.c_str(), O_RDWR, 0);
         if ( file_d == -1)
            {
            #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
            printf ( "E [t %ld] (Frontier::saveFrontierCheckpoint) open file %s failed\n", pthread_self(), frontierCheckpointDir.c_str()  );
            #endif
            return -1;
            }
         lseek(file_d, 0, SEEK_CUR);
         write(file_d, content.c_str(), sizeof(content.c_str()) - 1);
         close(file_d);

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::saveFrontierCheckpoint) save to file %s\n", pthread_self(), frontierCheckpointDir.c_str() );
         fflush ( stdout );
         #endif

         return 1;
         }

      // Load Checkpoints
      // Return:
      //    1 on success
      //    -1 on fail
      // Checkpoint Format: all in single line, seprate by space, terminate by \n
      //    https://www.baidu.com https://umich.edu https://www.berkeley.edu
      int loadFrontierCheckpoint()
         {
         MtxRAII raii ( &urlsMtx );
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "# [t %ld] (Frontier::loadFrontierCheckpoint) load %s\n", pthread_self(), frontierCheckpointDir.c_str() );
         fflush ( stdout );
         #endif

         int file_d = open( frontierCheckpointDir.c_str(), O_RDWR, 0 );
         if ( file_d == -1 )
            {
            printf ( "E [t %ld] (CrawlerServer::loadFrontierCheckpoint) unable to open hostname file\n", pthread_self() );
            fflush ( stdout );
            return -1;
            }

         string line = "";
         char buf;
         while ( read(file_d, &buf, 1) > 0 )
            {
            if ( buf == '\n' ) 
               {
                  if ( line.empty() || line.size() == 1 )
                     continue;
                  if ( valid_url(line) )
                     urls.push ( UrlPack( line, evaluator.eval ( line ) ) );
               }
            else 
               line += buf;   
            }
         if ( line.empty() || line.size() == 1 )
             ;
         else if ( valid_url(line) )
            urls.push ( UrlPack( line, evaluator.eval ( line ) ) );
            

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::loadFrontierCheckpoint) load %s finish, FRONTIER SIZE %zu\n", pthread_self(), frontierCheckpointDir.c_str(), urls.size() );
         fflush ( stdout );
         #endif

         return 1;
         }
   };
