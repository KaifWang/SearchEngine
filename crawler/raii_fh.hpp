//
// raii_fh.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#pragma once

#include <unistd.h> // close
#include <stdio.h> // printf
#include <openssl/ssl.h> // ssl connection
#include <pthread.h> // pthread_self()
#include "./configs/crawler_cfg.hpp" // configuration

class FhRAII
   {
   public:
      int Fh;

   public:
      explicit FhRAII ( int fh ) : Fh ( fh ) {}
      ~FhRAII()
         {
         if ( Fh < 0 )
            ;
         else if ( close ( Fh ) < 0 )
            {
            printf ( "E [t %ld] (FhRAII::~FhRAII) UNABLE TO CLOSE FH %d\n", pthread_self(), Fh );
            }
         }
   };
