//
// raii_ssl.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
#pragma once

#include <unistd.h> // close
#include <stdio.h>  // printf
#include <pthread.h> // pthread_self()
#include <openssl/ssl.h> // ssl connection
#include <openssl/err.h> // ssl error

#include "./configs/crawler_cfg.hpp" // configuration

// Doc
// https://www.ibm.com/docs/en/ztpf/2020?topic=apis-ssl-shutdown
// https://blog.csdn.net/tianjian_blog/article/details/43671425
// https://stackoverflow.com/questions/29200797/close-ssl-connection-correctly-with-openssl
// SSH_shutdown https://www.openssl.org/docs/man1.0.2/man3/SSL_shutdown.html

class SSLRAII
   {
   public:
      SSL* ssl_ptr;

   public:
      explicit SSLRAII ( SSL* ssl ) : ssl_ptr ( ssl ) {}
      ~SSLRAII()
         {
         // Doc
         // SSH_shutdown https://www.openssl.org/docs/man1.0.2/man3/SSL_shutdown.html
         // SSL_free https://www.openssl.org/docs/man1.0.2/man3/SSL_free.html
         // StackOverflow similar problem https://stackoverflow.com/questions/29200797/close-ssl-connection-correctly-with-openssl
         if ( ssl_ptr != nullptr )
            {
            int count = 0;
            while ( SSL_shutdown ( ssl_ptr ) == 0 && count < 5 )
               {
               count++;
               }

            if ( count == 5 )
               printf ( "E [t %ld] (SSLRAII::~SSLRAII) SSL_shutdown success after retry %d times (safe to ignore)\n", pthread_self(), count );
            else
               printf ( "@ [t %ld] (SSLRAII::~SSLRAII) able to SSL_shutdown after retry %d\n", pthread_self(), count);
            /* SSL Free */
            SSL_free ( ssl_ptr );
            }
         }
   };