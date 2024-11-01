//
// frontier.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Wenxuan Zhao zwenxuan@umich.edu
//
// evaluate urls, give it a priority
//
#pragma once

#include <pthread.h>
#include <cstring>

class EvaluateUrl
   {
   public:
      // return a score for evaluation
      // Example heuristics that might be used in ranking of URLs on the frontier to decide what to crawl next:
      // 1. Some top-level domains are  better than others.  .gov / .edu >  .org > .com > .biz / .ru / etc.
      // 2. Short URLs and short domains are better.
      // 3. Domains with dashes and underscores are not as good.
      // 4. A URL with a lot of anchor text or links to it is better.
      int eval ( const string& url )
         {
         return eval ( url.c_str() );
         }

      int eval ( const char* url )
         {
         int score = 0;
         if ( strstr ( url, ".edu" ) != NULL )
            score += 850;
         else if ( strstr ( url, ".gov" ) != NULL )
            score += 850;
         else if ( strstr ( url, ".com" ) != NULL )
            score += 850;
         else if ( strstr ( url, ".org" ) != NULL )
            score += 850;
         else if ( strstr ( url, ".net" ) != NULL )
            score += 750;
         else if ( strstr ( url, ".biz" ) != NULL )
            score += 700;
         else
            score += 650;
         const char* begin = strstr ( url, "://" );
         if ( begin == nullptr )
            {
            printf ( "E [t %ld] (EvaluateUrl::eval) encounter nullptr\n", pthread_self() );
            return 10;
            }
         //cout << (begin == nullptr) << endl;
         const char* end = strstr ( begin + 2, "/" );
         string hostname = string ( begin, end );
         score -= ( hostname.length() * 2 );
         if ( strstr ( hostname.c_str(), "-" ) != nullptr || strstr ( hostname.c_str(), "_" ) )
            score -= 100;
         score -= strlen ( url );

         return score;
         }
   };
