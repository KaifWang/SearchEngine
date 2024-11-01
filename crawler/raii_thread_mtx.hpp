//
// raii_thread_mtx.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#pragma once

#include <pthread.h>
#include "./configs/crawler_cfg.hpp" // configuration

class MtxRAII
   {
   public:
      pthread_mutex_t* Mtx = nullptr;

   public:
      explicit MtxRAII ( pthread_mutex_t* mtx )
         {
         Mtx = mtx;
         int code = pthread_mutex_lock ( Mtx );
         if ( code != 0 )
            {
            #ifdef _UMSE_RAII_LOG
            printf ( "E [t %ld] (MtxRAII::MtxRAII) UNABEL TO ACQUIRE LOCK, error code is %d\n", pthread_self(), code );
            fflush ( stdout );
            #endif
            ;
            }
         }

      ~MtxRAII()
         {
         // Doc
         // pthread_mutex_unlock https://linux.die.net/man/3/pthread_mutex_lock
         int code = pthread_mutex_unlock ( Mtx );
         if ( code != 0 )
            {
            #ifdef _UMSE_RAII_LOG
            printf ( "E [t %ld] (MtxRAII::~MtxRAII) UNABEL TO UNLOCK LOCK, error code is %d\n", pthread_self(), code );
            fflush ( stdout );
            #endif
            ;
            }
         }
   };