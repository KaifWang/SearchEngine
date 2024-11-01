//
// thread_ThreadSignal.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#pragma once

#include <pthread.h>
#include "raii_thread_mtx.hpp" // RAII

class ThreadSignal
   {
   private:
      bool alive;
      pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

   public:
      explicit ThreadSignal() : alive ( true ) {}
      void set_terminate()
         {
         MtxRAII raii(&mtx);
         alive = false;
         }

      bool is_alive()
         {
         MtxRAII raii(&mtx);
         return alive;
         }
   };