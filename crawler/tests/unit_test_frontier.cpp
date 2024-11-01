//
// unit_test_frontier.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Unit test for frontier
//

/*
 * let one of the two child thread consumer all urls
 * both child thread should stop
 * let the adder thread add urls
 * child thread should continue
 */

#include <iostream>
#include <pthread.h> // concurrent for lock, etc
#include "../configs/crawler_cfg.hpp" // configuration
#include "../crawler_frontier.hpp"

using std::cout;
using std::endl;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct thread_param
   {
   Frontier* frontierPtr;
   string threadName;

   thread_param ( Frontier* frontier, const char* name )
      {
      frontierPtr = frontier;
      threadName = name;
      }
   };

void* consumeFrontier ( void* x )
   {

   thread_param* thread_param_ptr = ( thread_param* ) x;
   string threadName = thread_param_ptr->threadName;
   Frontier* frontier = thread_param_ptr->frontierPtr;
   printf ( "# [t %ld] thread %s start\n", pthread_self(), threadName.c_str() );


   printf ( "@ [t %ld] %s try to get 2 items when %lu urls left, this will clear data\n", pthread_self(), threadName.c_str(), frontier->size() );
   vector<string> topKUrls;
   frontier->getTopKUrls ( topKUrls );
   printf ( "@ [t %ld] %s get # %lu url \n", pthread_self(), threadName.c_str(), topKUrls.size() );
   for ( auto url : topKUrls )
      printf ( "@ [t %ld] %s get url %s\n", pthread_self(), threadName.c_str(), url.c_str() );

   pthread_yield_np();

   printf ( "@ [t %ld] %s end\n", pthread_self(), threadName.c_str() );

   return ( void* ) 1;
   }

void* addFrontier ( void* x )
   {

   thread_param* thread_param_ptr = ( thread_param* ) x;
   string threadName = thread_param_ptr->threadName;
   Frontier* frontier = thread_param_ptr->frontierPtr;
   printf ( "# [t %ld] thread %s start\n", pthread_self(), threadName.c_str() );

   // https://man7.org/linux/man-pages/man3/pthread_yield.3.html
   // https://www.linuxquestions.org/questions/programming-9/pthread_yield-vs-pthread_yield_np-469283/

   printf ( "@ [t %ld] %s let the child run first\n", pthread_self(), threadName.c_str() );
   pthread_yield_np();
   printf ( "@ [t %ld] %s let the child run first\n", pthread_self(), threadName.c_str() );
   pthread_yield_np();

   printf ( "@ [t %ld] %s try to add urls\n", pthread_self(), threadName.c_str() );
   printf ( "@ [t %ld] %s check current urls size is %lu\n", pthread_self(), threadName.c_str(), frontier->size() );
   vector<string> newUrls;
   newUrls.emplace_back ( "https://www.baidu.com/" );
   newUrls.emplace_back ( "https://web.eecs.edu/rl" );
   newUrls.emplace_back ( "https://web.eecs.edu/rl4" );
   newUrls.emplace_back ( "https://web.eecs.edu/r/wer/s" );
   newUrls.emplace_back ( "https://web.eecs.gov/rl" );

   frontier->AddUrl ( newUrls );
   printf ( "@ [t %ld] %s 1 time add url\n", pthread_self(), threadName.c_str() );
   pthread_yield_np();


   printf ( "@ [t %ld] %s try to add urls\n", pthread_self(), threadName.c_str()  );
   printf ( "@ [t %ld] %s check current urls size is %lu\n", pthread_self(), threadName.c_str(), frontier->size() );
   newUrls.clear();
   newUrls.emplace_back ( "https://youtube.com/" );
   newUrls.emplace_back ( "https://www.baidu.com/wer" );
   newUrls.emplace_back ( "https://www.wenxuan.com/" );
   newUrls.emplace_back ( "https://hit.com/" );
   newUrls.emplace_back ( "https://hi-t.com/newurl" );
   newUrls.emplace_back ( "https://hi_t.com/newurl" );
   newUrls.emplace_back ( "https://twitter.com/new_url" );
   newUrls.emplace_back ( "https://hit.org/newurl" );
   newUrls.emplace_back ( "https://hit.net/newurl" );

   frontier->AddUrl ( newUrls );
   printf ( "@ [t %ld] %s 2 time add url\n", pthread_self(), threadName.c_str() );
   pthread_yield_np();


   printf ( "@ [t %ld] %s try to add urls\n", pthread_self(), threadName.c_str()  );
   printf ( "@ [t %ld] %s check current urls size is %lu\n", pthread_self(), threadName.c_str(), frontier->size() );
   newUrls.clear();
   newUrls.emplace_back ( "https://hi_t.com/newurl" );
   newUrls.emplace_back ( "https://twitter.com/new_url" );
   newUrls.emplace_back ( "https://hit.org/newurl" );
   newUrls.emplace_back ( "https://hit.net/newurl" );
   newUrls.emplace_back ( "https://hit.to/newurl" );

   frontier->AddUrl ( newUrls );
   printf ( "@ [t %ld] %s 3 time add url\n", pthread_self(), threadName.c_str() );
   printf ( "@ [t %ld] %s end\n", pthread_self(), threadName.c_str() );

   return ( void* ) 1;
   }


int main()
   {
   pthread_t tconsume1;
   pthread_t tconsume2;
   pthread_t tconsume3;

   pthread_t tadd;

   Frontier frontier;
   frontier.set_k ( 2 );
   frontier.set_r ( 2 );
   // https://man7.org/linux/man-pages/man3/pthread_create.3.html
   thread_param tp1 ( &frontier, "consumer1" );
   thread_param tp2 ( &frontier, "consumer2" );
   thread_param tp4 ( &frontier, "consumer3" );

   thread_param tp3 ( &frontier, "adder" );
   pthread_create ( &tconsume1, nullptr, consumeFrontier, ( void* ) &tp1 );
   pthread_create ( &tconsume2, nullptr, consumeFrontier, ( void* ) &tp2 );
   pthread_create ( &tconsume3, nullptr, consumeFrontier, ( void* ) &tp4 );

   pthread_create ( &tadd, nullptr, addFrontier, ( void* ) &tp3 );

   // https://www.man7.org/linux/man-pages/man3/pthread_join.3.html
   pthread_join ( tconsume1, nullptr );
   pthread_join ( tconsume2, nullptr );
   pthread_join ( tconsume3, nullptr );

   pthread_join ( tadd, nullptr );

   }
