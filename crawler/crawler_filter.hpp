//
// filter.h
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Wenxuan Zhao zwenxuan@umich.edu
// Xiao Song xiaosx@umich.edu
//
// Filter (Seen Set, Robot Txt)
//
#pragma once

#include <pthread.h> // concurrent for lock, etc
#include <cmath>     // for log
#include <openssl/md5.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cxxabi.h>  // abi::__forced_unwind& 

#include "HashTable.h"      // delete when fixing config document
#include "configs/crawler_cfg.hpp" // configuration
#include "url_parser.hpp"          // parse url
#include "network_utility.hpp"     // network related helper funciton
#include "raii_fh.hpp"             // RAII for Fh
#include "raii_thread_mtx.hpp"     // RAII for Mtx

struct pair {
   uint64_t first;
   uint64_t second;
};

// VERY IMPORTANT: seenset.txt must be created manually;
//                 can not relay on system to create
class BloomFilter
   {
   public:
      //Add any private member variables that may be neccessary
      long long int m; // size of container
      string seenSetCheckpointFilename;
      int count_bf = 0;
      //int fd;     // file pointer
      //bool* addr; // mmap
      vector<bool> seenset;
      long long int k;

      pair hash ( const std::string& datum )
         {
         unsigned char md[MD5_DIGEST_LENGTH];
         size_t s = datum.length();
         MD5 ( ( const unsigned char* ) datum.c_str(), s, md );
         uint64_t first;
         uint64_t second;

         memcpy ( &first, md, sizeof ( first ) );
         memcpy ( &second, md + 8, sizeof ( second ) );
         pair p;
         p.first = first;
         p.second = second;
         return p;
         }

   public:
      BloomFilter ( int num_objects, double false_positive_rate )
         {
         count_bf = 0;
         seenSetCheckpointFilename = string ( UMSE_CRAWLER_SEENSET_CHECKPOINT );
         m = - ( num_objects * log ( false_positive_rate ) ) / ( log ( 2 ) * log ( 2 ) );
         int fd = open ( seenSetCheckpointFilename.c_str(), O_RDWR );

         if ( fd == -1 )
            {
            #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
            printf ( "E [t %ld] (BloomFilter::BloomFilter) Can not open file %s\n", pthread_self(), seenSetCheckpointFilename.c_str() );
            #endif
            exit ( -1 );
            }
         struct stat sb;
         fstat ( fd, &sb );
         if ( sb.st_size == 0 )
            {
            seenset.resize ( m, false );
            #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
            printf ( "* [t %ld]seenset size: %ld\n", pthread_self(), seenset.size() );
            #endif
            }
         else
            {
            loadBF();
            #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
            printf ( "* [t %ld]seenset size: %ld \n", pthread_self(), seenset.size() );
            #endif
            }

         k = m / num_objects * log ( 2 );
         }

      ~BloomFilter()
         {
         saveBF();
         }
      void loadBF()
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (Filter::loadfilter) start\n", pthread_self() );
         fflush ( stdout );
         #endif
         seenset.reserve ( m );
         int file_d = open(seenSetCheckpointFilename.c_str(), O_RDWR, 0);
         if ( file_d == -1 )
            {
            printf ( "E [t %ld] (CrawlerServer::loadbloomfilter) unable to open bf file\n", pthread_self() );
            fflush ( stdout );
            return;
            }
         char content;
         int c = 0;
         while ( c < m )
            {
            read(file_d, content, 1);
            if ( content == '1' )
               seenset.push_back ( true );
            else
               seenset.push_back ( false );
            c++;
            }
         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::loadbloomfilter) load %s finish, \n", pthread_self(),  seenSetCheckpointFilename.c_str() );
         fflush ( stdout );
         #endif
         close(file_d);
         return;
         }

      void saveBF()
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (Filter::savefilter) start\n", pthread_self() );
         fflush ( stdout );
         #endif
         int file_d = open(seenSetCheckpointFilename.c_str(), O_RDWR, 0);
         if ( file_d == -1 )
            {
            #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
            printf ( "E [t %ld] (Frontier::savebloomfilter) open file %s failed\n", pthread_self(), seenSetCheckpointFilename.c_str()  );
            #endif
            return;
            }
         char content;
         for ( size_t i = 0; i < seenset.size(); i++ )
            {
            if ( seenset[i] == false )
               content = '0';
            else
               content = '1';
            write(file_d, &content, 1);
            }
         close(file_d);

         #ifdef _UMSE_CRAWLER_FRONTIER_LOG
         printf ( "* [t %ld] (Frontier::savebloomfilter) save to file %s\n", pthread_self(), seenSetCheckpointFilename.c_str() );
         fflush ( stdout );
         #endif

         return;

         }

      void insert ( const std::string& s )
         {
         //Hash the string into two unique hashes
         pair result = hash ( s );
         for ( int i = 1; i < k + 1; i++ )
            {
            int h0 = ( result.first + i * result.second ) % m;
            seenset[h0] = true;
            }
         count_bf ++;
         if ( count_bf == 3000 )
            {
            saveBF();
            count_bf = 0;
            }
         }

      bool contains ( const std::string& s )
         {
         pair result = hash ( s );
         for ( int i = 1; i < k + 1; i++ )
            {
            int h0 = ( result.first + i * result.second ) % m;
            if ( seenset[h0] == false )
               return false;
            }
         return true;
         }
   };

struct RobotTxtRules
   {
   vector<string> allow;
   vector<string> disallow;
   };

class RobotTxt
   {
   public:
      // checkpoint filename
      string robotTxtCheckpointFilename;
      // map hostname to its corresbonding robot.txt content
      HashTable<const char*, RobotTxtRules*>* robotTxtS;
      // robots.txt downloader

   public:
      // Download and process hostname's robots.txt
      // return:
      //      1 : if successfully download & process the data
      //      -1 : otherwise
      // ASSUME: there's no redirection happened
      int downloadAndProcessHelper ( const char* hostname )
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (RobotTxt::downloadAndProcessHelper) start download & process robots.txt corr to %s \n", pthread_self(), hostname );
         #endif

         /* (1) set the robots.txt filename */
         // We're using https urls only and thus can hardcode it here
         char* robotTxtFilename = new char[8 + strlen ( hostname ) + 11 + 1];
         memset ( robotTxtFilename, '\0', sizeof ( robotTxtFilename[0] ) );
         strcpy ( robotTxtFilename, "https://" );
         strcpy ( robotTxtFilename + 8, hostname );
         strcpy ( robotTxtFilename + 8 + strlen ( hostname ), "/robots.txt" );

         /* (2) try to download robots.txt */
         // use try catch because download from internet are highly likely to fail
         string content;
         try
            {
            #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
            printf ( "# [t %ld] (RobotTxt::downloadAndProcessHelper) [%s] DOWNLOAD START\n", pthread_self(), robotTxtFilename );
            #endif

            switch ( download_webpage_ssl ( robotTxtFilename, content ) )
               {
               case DOWNLOAD_WEBPAGE_FAIL:
                  {
                  #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
                  printf ( "E [t %ld] (RobotTxt::downloadAndProcessHelper) error when try to download url `%s`\n", pthread_self(), robotTxtFilename );
                  #endif
                  RobotTxtRules* robotTxtRules = new RobotTxtRules();
                  robotTxtS->Find ( hostname, robotTxtRules );

                  return -1;
                  }
               case DOWNLOAD_WEBPAGE_URL_REDIRECT:
                  {
                  #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
                  printf ( "@ [t %ld] (RobotTxt::downloadAndProcessHelper) redirection happen\n", pthread_self()  );
                  #endif
                  RobotTxtRules* robotTxtRules = new RobotTxtRules();
                  robotTxtS->Find ( hostname, robotTxtRules );

                  return  -1;
                  }

               case DOWNLOAD_WEBPAGE_SUCCESS:
                  break;
               }

            #ifdef _UMSE_CRAWLER_CRAWLERCORE_LOG
            printf ( "# [t %ld] (RobotTxt::downloadAndProcessHelper) [%s] DOWNLOAD FINISH\n", pthread_self(), robotTxtFilename  );
            #endif
            }
         #ifndef __APPLE__
         catch ( abi::__forced_unwind& )
            {
            printf ( "E [t %ld] (RobotTxt::downloadAndProcessHelper) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!LISTEN SIGNAL THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
            fflush ( stdout );
            throw;
            }
         #endif
         catch ( ... )
            {
            printf ( "E [t %ld] (RobotTxt::downloadAndProcessHelper) exception when try to download url `%s`\n", pthread_self(), robotTxtFilename );
            fflush ( stdout );
            return -1;
            }

         /* (3) try to proces robots.txt */
         processRulesHelper ( content.c_str(), hostname );
         return 1;
         }

      // Process all the rules inside the html content
      // Idea:
      //    1. 找到第一个 User-agent: * 的地方
      //    2. 储存所有 Disallow 的部分
      //    3. 储存所有 Allow 的部分
      //    4. 找到下一个 User-agent: 的部分 （因为不是*所以暂停
      // Note: seprate process rules to seprate function to help unit test the functionality
      // Assume: lock is hold & is currenlty locked
      void processRulesHelper ( const char* htmlContent, const char* hostname )
         {
         if ( htmlContent == nullptr || hostname == nullptr )
            return;
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (RobotTxt::processRulesHelper) start on robots.txt of %s\n", pthread_self(), hostname );
         #endif

         /* (1) add entry (contain all rules) to the mapping */
         RobotTxtRules* robotTxtRules = new RobotTxtRules();
         robotTxtS->Find ( hostname, robotTxtRules );

         /* (2) add all rules */
         // TODO you can improve below part to use two pointer, current implementation is kind of slow
         // extract all lines
         const char* beginPtr = htmlContent;
         const char* endPtr = strstr ( beginPtr + 1, "\n" );
         vector<string> lines;
         while ( *beginPtr && endPtr != nullptr )
            {
            lines.push_back ( string ( beginPtr, endPtr - beginPtr ) );
            beginPtr = endPtr + 1;
            if ( beginPtr == nullptr )
               break;
            endPtr = strstr ( beginPtr + 1, "\n" );
            }

         // assume all we follow * rule
         string uas = "User-agent: *";
         string ua = "User-agent: ";
         string all = "Allow: ";
         string dis = "Disallow: ";
         size_t i = 0;
         bool flag = false;
         while ( i < lines.size() )
            {
            // ignore comments
            if ( lines[i].size() < 1 )
               i++;
            else if ( lines[i][0] == '#' )
               i++;
            else if ( strstr ( lines[i].c_str(), ua.c_str() ) != nullptr )
               {
               if ( strstr ( lines[i].c_str(), uas.c_str() ) != nullptr )
                  flag = true;
               else
                  flag = false;
               i++;
               }
            else if ( strstr ( lines[i].c_str(), all.c_str() ) != nullptr && flag )
               {
               char* push = ( char* ) lines[i].c_str() + 7;

               #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
               printf ( "@ [t %ld] (RobotTxt::processRulesHelper) allow %s\n", pthread_self(), push );
               #endif

               robotTxtRules->allow.push_back ( string ( push ) );
               i++;
               }
            else if ( strstr ( lines[i].c_str(), dis.c_str() ) != nullptr && flag )
               {
               char* push = ( char* ) lines[i].c_str() + 10;
               if ( push == nullptr )
                  i++;
               else
                  {
                  #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
                  printf ( "@ [t %ld] (RobotTxt::processRulesHelper) disallow %s\n", pthread_self(), push );
                  #endif

                  robotTxtRules->disallow.push_back ( string ( push ) );

                  i++;
                  }
               }
            else
               i++;
            }
         }

      // check if path obey the given rule
      bool obeyRuleHelper ( const char* path, const char* rule )
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (RobotTxt::obeyRuleHelper) check if %s obey the rules %s\n", pthread_self(), path, rule );
         #endif
         if ( rule == nullptr || path == nullptr )
            return true;

         int size = strlen ( path );
         if ( size + 1 > 4900 )
            return false;
         int pos[5000];
         pos[0] = 0;
         int num = 1;
         for ( const char* i = rule; *i != '\0'; i++ )
            {
            if ( *i == '$' && * ( i + 1 ) == '\0' )
               return ( pos[num - 1] == size );
            else if ( *i == '*' )
               {
               num = size - pos[0] + 1;
               for ( int k = 1; k < num; k++ )
                  pos[k] = pos[k - 1] + 1;
               }
            else
               {
               int newnum = 0;
               for ( int k = 0; k < num; k++ )
                  {
                  if ( pos[k] < size && * ( path + pos[k] ) == *i )
                     pos[newnum++] = pos[k] + 1;
                  }
               num = newnum;
               if ( num == 0 )
                  return false;
               }
            }
         return true;
         }

      //
      // Assume: lock is hold & is currently locked
      bool obeyRulesHelper ( const char* host, const char* path )
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (RobotTxt::obeyRulesHelper) check if %s%s obey the rules\n", pthread_self(), host, path );
         #endif


         RobotTxtRules* robotTxtRules;
         if ( robotTxtS->Find ( host ) == nullptr )
            return false;
         else
            robotTxtRules = robotTxtS->Find ( host )->value;
         // first check all the allowed rules
         for ( auto allowRuleI : robotTxtRules->allow )
            {
            if ( obeyRuleHelper ( path, allowRuleI.c_str() ) )
               return true;
            }

         // second check all the disallowed rules
         for ( auto disallowRulei : robotTxtRules->disallow )
            {

            if ( obeyRuleHelper ( path, disallowRulei.c_str() ) )
               {
               #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
               printf ( "# [t %ld] (RobotTxt::obeyRulesHelper) %s match disallow %s\n", pthread_self(), path, disallowRulei.c_str() );
               #endif
               return false;
               }
            }

         // if no rule match, then the url is allowed
         return true;
         }

   public:
      RobotTxt()
         {
         robotTxtS = new HashTable<const char*, RobotTxtRules*>();
         robotTxtCheckpointFilename = string ( UMSE_CRAELER_ROBOTTXT_CEHCKPOINT );
         }

      ~RobotTxt()
         {
         // free dynamically allocated memory
         for ( auto iter = robotTxtS->begin(); iter != robotTxtS->end(); ++iter )
            delete iter->value;
         delete robotTxtS;
         }

      // Check if given url obey the robot.txt restriction
      // return:
      //      return true if follow robots.txt & can add to frontier
      //      return false if not obey robots.txt & can not add to frontier
      bool obeyRobotTxtRules ( const string& url )
         {
         #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
         printf ( "# [t %ld] (RobotTxt::obeyRobotTxtRules) start to check if %s obey robot.txt rule\n", pthread_self(), url.c_str() );
         #endif

         /* (1) parse the url*/
         // this would make a local copy of the url
         ParsedUrl parsedUrl ( url.c_str() );

         /* (2) if hostname's corresbonding robots.txt not download before, first download */
         if ( robotTxtS->Find ( parsedUrl.Host ) == nullptr )
            {
            #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
            printf ( "# [t %ld] (RobotTxt::obeyRobotTxtRules) this hostname never crawl robot.txt of %s \n", pthread_self(), url.c_str() );
            #endif

            // if download corresbonding robot.txt file failed, consider the url as valid url
            if ( downloadAndProcessHelper ( parsedUrl.Host ) < 0 )
               {
               #ifdef _UMSE_CRAWLER_ROBOTXT_LOG
               printf ( "E [t %ld] (RobotTxt::obeyRobotTxtRules) download robot.txt of %s failed, consider url as valid by default\n", pthread_self(), url.c_str() );
               #endif
               return true;
               }
            }

         /* (3) check if url obey the robots.txt rules */
         // add a / to the start of path
         string newPath = "/" + string ( parsedUrl.Path );
         bool obeyRules = obeyRulesHelper ( parsedUrl.Host, newPath.c_str() );
         return obeyRules;
         }
   };

class SeenSet
   {
   private:
      string seenSetCheckpointFilename;
      BloomFilter* seenUrls = nullptr;

   public:
      SeenSet()
         {
         seenUrls = new BloomFilter ( 3000000, 0.001 );
         }

      ~SeenSet()
         {
         assert ( seenUrls );
         delete seenUrls;
         }

      // Check if given url have seen before
      // return:
      //      return true if given url not seen before & can add to frontier
      //      return false if given url have seen before & can not add to frontier
      bool notSeenBefore ( const string& url )
         {
         #ifdef _UMSE_CRAWLER_SEENSET_LOG
         printf ( "# [t %ld] (SeenSet::notSeenBefore) start to check %s\n", pthread_self(), url.c_str() );
         #endif

         if ( seenUrls->contains ( url ) )
            {
            #ifdef _UMSE_CRAWLER_SEENSET_LOG
            printf ( "# [t %ld] (SeenSet::notSeenBefore) url %s seen before, don't add to frontier\n", pthread_self(), url.c_str() );
            #endif

            return false;
            }

         #ifdef _UMSE_CRAWLER_SEENSET_LOG
         printf ( "@ [t %ld] (SeenSet::notSeenBefore) url %s not seen before, try add to seen set\n", pthread_self(), url.c_str() );
         #endif

         seenUrls->insert ( url );

         #ifdef _UMSE_CRAWLER_SEENSET_LOG
         printf ( "@ [t %ld] (SeenSet::notSeenBefore) url %s add to seen set success\n", pthread_self(), url.c_str() );
         #endif

         return true;
         }
   };

class Filter
   {
   private:
      RobotTxt robotTxt;
      SeenSet seenSet;

      HashTable<const char*, int> domainCount;

      pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

   public:
      Filter() = default;

      // Check if url can be add to frontier
      bool canAddToFrontier ( const string& url )
         {
         #ifdef _UMSE_CRAWLER_FILTER_LOG
         printf ( "# [t %ld] (Filter::canAddToFrontier) start, try to add %s\n", pthread_self(), url.c_str() );
         fflush ( stdout );
         #endif

         MtxRAII raii ( &mtx );

         if ( seenSet.notSeenBefore ( url ) && robotTxt.obeyRobotTxtRules ( url )  )
            {
            #ifdef _UMSE_CRAWLER_FILTER_LOG
            printf ( "@ [t %ld] (Filter::canAddToFrontier) url %s can add to frontier\n", pthread_self(), url.c_str() );
            fflush ( stdout );
            #endif

            ParsedUrl parsedUrl ( url );
            if ( domainCount.Find( parsedUrl.Host ) == nullptr)
                domainCount.Find( parsedUrl.Host, 1 );
            else if ( domainCount.Find ( parsedUrl.Host )->value > 10000 )
               {
               #ifdef _UMSE_CRAWLER_FILTER_LOG
               printf ( "@ [t %ld] (Filter::canAddToFrontier) domain reach max num url, ignore new url\n", pthread_self(), url.c_str() );
               #endif
               return false;
               }
            else
               {
               domainCount.Find( parsedUrl.Host )->value += 1;
               #ifdef _UMSE_CRAWLER_FILTER_LOG
               printf ( "@ [t %ld] (Filter::canAddToFrontier) domain not reach limit, add new url\n", pthread_self(), url.c_str() );
               #endif
               return true;
               }
            }

         #ifdef _UMSE_CRAWLER_FILTER_LOG
         printf ( "* [t %ld] (Filter::canAddToFrontier) url %s can not add to frontier\n", pthread_self(), url.c_str() );
         #endif

         return false;
         }
   };
