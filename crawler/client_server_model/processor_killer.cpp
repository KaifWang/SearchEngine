#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <vector>
#include <cstdio>

using namespace std;

vector<pthread_t> all_process;

void* processor ( void* arg )
   {
   int* time_sleep_ptr = ( int* ) arg;
   int time_sleep = *time_sleep_ptr;

   printf ( "# [%ld] processor with time sleep %d start\n", pthread_self(), time_sleep );
   fflush ( stdout );
   for ( int i = 0; i < time_sleep; ++i )
      sleep ( 1 );

   printf ( "# [%ld] processor with time sleep %d end normally\n", pthread_self(), time_sleep );
   fflush ( stdout );
   }

void* killer ( void* arg )
   {
   int* time_sleep_ptr = ( int* ) arg;
   int time_sleep = *time_sleep_ptr;

   printf ( "# [%ld] killer start to sleep for %d\n", pthread_self(), time_sleep );
   fflush ( stdout );

   for ( int i = 0; i < time_sleep; ++i )
      sleep ( 1 );

   printf ( "# [%ld] killer finish to sleep for %d, start to kill other threads\n", pthread_self(), time_sleep );
   fflush ( stdout );

   for ( size_t i = 0; i < all_process.size(); ++i )
      {
      printf ( "# [%ld] thread i `%zu` \n", pthread_self(), i );
      fflush ( stdout );

      pthread_cancel ( all_process[i] );

      /*
      if ( pthread_kill ( all_process[ i ], 0 ) == ESRCH  )
         {
         printf ( "# [%ld] thread %zu have end, don't need to kill it\n", pthread_self(), i );
         fflush ( stdout );
         }
      else
         {
         printf ( "# [%ld] thread %zu have not yet end, try kill it\n", pthread_self(), i );
         fflush ( stdout );
         pthread_cancel(all_process[i]);
         printf ( "# [%ld] thread %zu have not yet end, finish kill it\n", pthread_self(), i );
         fflush ( stdout );
         }
      */
      }

   printf ( "# [%ld] killer thread end %d\n", pthread_self(), time_sleep );
   fflush ( stdout );
   }

int main()
   {
   vector<int> param{5, 10, 20, 25, 15};
   all_process.reserve ( 10 );

   int i = 0;
   while ( i < 2 )
      {
      printf ( "@ iteration %d\n", i );
      all_process.clear();

      pthread_t thread_t_1;
      pthread_create ( &thread_t_1, nullptr, &processor, ( void* ) &param.at ( 0 ) );
      all_process.emplace_back ( thread_t_1 );

      pthread_t thread_t_2;
      pthread_create ( &thread_t_2, nullptr, &processor, ( void* ) &param.at ( 1 ) );
      all_process.emplace_back ( thread_t_2 );

      pthread_t thread_t_3;
      pthread_create ( &thread_t_3, nullptr, &processor, ( void* ) &param.at ( 2 ) );
      all_process.emplace_back ( thread_t_3 );

      pthread_t thread_t_4;
      pthread_create ( &thread_t_4, nullptr, &processor, ( void* ) &param.at ( 3 ) );
      all_process.emplace_back ( thread_t_4 );

      pthread_t thread_t_killer;
      pthread_create ( &thread_t_killer, nullptr, &killer, ( void* ) &param.at ( 4 )  );

      for ( int i = 0; i < all_process.size(); ++i )
         {
         printf ( "@ [%ld] try to join `%d`\n", pthread_self(), i );

         pthread_join ( all_process.at ( i ), nullptr );

         /*
         if ( pthread_kill ( all_process[ i ], 0 ) == ESRCH  )
            printf ( "# [%ld] thread %d already finish, no need to join\n", pthread_self(), i );
         else
            {
            int code = pthread_join ( all_process.at ( i ), nullptr );
            printf ( "# [%ld] thread %d return code %d\n", pthread_self(), i, code );
            }
         */
         } // end of for loop

      printf ( "# [%ld] main join finish\n", pthread_self() );
      fflush ( stdout );

      pthread_kill ( thread_t_killer, SIGKILL );

      i++;
      } // end of while loop

   printf ( "@ main end\n" );
   }