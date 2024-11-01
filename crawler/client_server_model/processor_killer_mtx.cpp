#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <vector>
#include <cstdio>

/*
 * Idea successfully validated
 * check https://stackoverflow.com/questions/14268080/cancelling-a-thread-that-has-a-mutex-locked-does-not-unlock-the-mutex
 */

/*
@ iteration 0
# [thread 139907121923840] killer start to sleep for 5
# [thread 139907130316544] processor try to acquire lock
* [thread 139907130316544] (MtxRAII::MtxRAII) acquire lock success
# [thread 139907130316544] processor with time sleep 10 start

# [thread 139907121923840] killer finish to sleep for 5, start to kill other threads
# [thread 139907121923840] thread 0 have not yet end, try kill it
# [thread 139907121923840] thread 0 have not yet end, finish kill it
# [thread 139907121923840] killer thread end 5
* [thread 139907130316544] (MtxRAII::~MtxRAII) unlock lock success
# [139907130320704] thread 0 return code 0
# [139907130320704] main join finish
@ iteration 1
# [thread 139907113531136] killer start to sleep for 5
# [thread 139907130316544] processor try to acquire lock
* [thread 139907130316544] (MtxRAII::MtxRAII) acquire lock success
# [thread 139907130316544] processor with time sleep 10 start
# [thread 139907113531136] killer finish to sleep for 5, start to kill other threads
# [thread 139907113531136] thread 0 have not yet end, try kill it
# [thread 139907113531136] thread 0 have not yet end, finish kill it
# [thread 139907113531136] killer thread end 5
* [thread 139907130316544] (MtxRAII::~MtxRAII) unlock lock success
# [139907130320704] thread 0 return code 0
# [139907130320704] main join finish
@ main end
*/

using namespace std;
#define _UMSE_RAII_LOG

vector<pthread_t> all_process;

pthread_mutex_t* get_tcp_ssl_gloabl_mtx()
   {
   static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; /* ONLY RUN ONCE */
   return &mtx;
   }

struct MtxRAII
   {
   pthread_mutex_t* Mtx = nullptr;
   explicit MtxRAII ( pthread_mutex_t* mtx )
      {
      Mtx = mtx;
      int code = pthread_mutex_lock ( Mtx );
      if ( code != 0 )
         {
         printf ( "E [t %ld] (MtxRAII::MtxRAII) UNABEL TO ACQUIRE LOCK, error code is %d\n", pthread_self(), code );
         fflush ( stdout );
         }
      else
         {
         printf ( "* [t %ld] (MtxRAII::MtxRAII) acquire lock success\n", pthread_self() );
         fflush ( stdout );
         }
      }

   ~MtxRAII()
      {
      int code = pthread_mutex_unlock ( Mtx );
      if ( code != 0 )
         {
         printf ( "E [t %ld] (MtxRAII::~MtxRAII) UNABEL TO UNLOCK LOCK, error code is %d\n", pthread_self(), code );
         fflush ( stdout );
         }
      else
         {
         printf ( "* [t %ld] (MtxRAII::~MtxRAII) unlock lock success\n", pthread_self() );
         fflush ( stdout );
         }
      }
   };

void* processor ( void* arg )
   {
   printf ( "# [t %ld] processor try to acquire lock\n", pthread_self() );

   MtxRAII raii ( get_tcp_ssl_gloabl_mtx() );

   int* time_sleep_ptr = ( int* ) arg;
   int time_sleep = *time_sleep_ptr;

   printf ( "# [t %ld] processor with time sleep %d start\n", pthread_self(), time_sleep );
   fflush ( stdout );

   sleep ( time_sleep );

   printf ( "* [t %ld] processor with time sleep %d end normally\n", pthread_self(), time_sleep );
   fflush ( stdout );
   }

void* killer ( void* arg )
   {
   int* time_sleep_ptr = ( int* ) arg;
   int time_sleep = *time_sleep_ptr;

   printf ( "# [t %ld] killer start to sleep for %d\n", pthread_self(), time_sleep );
   fflush ( stdout );

   sleep ( time_sleep );

   printf ( "# [t %ld] killer finish to sleep for %d, start to kill other threads\n", pthread_self(), time_sleep );
   fflush ( stdout );

   for ( size_t i = 0; i < all_process.size(); ++i )
      {
      if ( pthread_kill ( all_process[ i ], 0 ) == ESRCH  )
         {
         printf ( "# [t %ld] thread %zu have end, don't need to kill it\n", pthread_self(), i );
         fflush ( stdout );
         }
      else
         {
         printf ( "# [t %ld] thread %zu have not yet end, try kill it\n", pthread_self(), i );
         fflush ( stdout );
         pthread_cancel ( all_process[i] );
         printf ( "# [t %ld] thread %zu have not yet end, finish kill it\n", pthread_self(), i );
         fflush ( stdout );
         }
      }

   printf ( "# [t %ld] killer thread end %d\n", pthread_self(), time_sleep );
   fflush ( stdout );
   }

int main()
   {
   vector<int> param{10, 5};
   all_process.reserve ( 10 );

   int i = 0;
   while ( i < 2 )
      {
      printf ( "@ iteration %d\n", i );
      all_process.clear();

      pthread_t thread_t_1;
      pthread_create ( &thread_t_1, nullptr, &processor, ( void* ) &param.at ( 0 ) );
      all_process.emplace_back ( thread_t_1 );

      pthread_t thread_t_killer;
      pthread_create ( &thread_t_killer, nullptr, &killer, ( void* ) &param.at ( 1 )  );

      for ( int i = 0; i < all_process.size(); ++i )
         {
         if ( pthread_kill ( all_process[ i ], 0 ) == ESRCH  )
            printf ( "# [%ld] thread %d already finish, no need to join\n", pthread_self(), i );
         else
            {
            int code = pthread_join ( all_process.at ( i ), nullptr );
            printf ( "# [%ld] thread %d return code %d\n", pthread_self(), i, code );
            }
         } // end of for loop

      printf ( "# [%ld] main join finish\n", pthread_self() );
      fflush ( stdout );

      pthread_kill ( thread_t_killer, SIGKILL );
      i++;
      } // end of while loop

   printf ( "@ main end\n" );
   }

