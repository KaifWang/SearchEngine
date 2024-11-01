//
// parsed_url.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Taken from hw4 instructor code
//
// Parse URL into hostname, path, etc
//
#pragma once

#include <cstring> // strcmp

#include "configs/crawler_cfg.hpp" // configutaiton

// Useage:
//    ParsedUrl url(char* urlCStr);
//    print(url.Host);
class ParsedUrl
   {
   public:
      //char* OriginalUrl;
      char* Service, *Host, *Port, *Path;
      bool isHttps;

   private:
      void ctr_helper ( const char* url )
         {
         #ifdef _UMSE_CRAWLER_PARSEDURL_LOG
         printf ( "# (ParsedUrl::ParsedUrl) start to parse url %s\n", url );
         #endif

         // no service provided, use https as default service
         // e.g. www.umich.edu
         if ( strncmp ( url, "http", 4 ) != 0 )
            {
            #ifdef _UMSE_CRAWLER_PARSEDURL_LOG
            printf ( "@ (ParsedUrl::ParsedUrl) url %s have no service, use http by default\n", url );
            #endif

            pathBuffer = new char[ strlen ( url ) + 1 + 8 ];
            strcpy ( pathBuffer, "https://" );
            strcpy ( pathBuffer + 8, url );
            }
         else
            {
            pathBuffer = new char[ strlen ( url ) + 1 ];
            strcpy ( pathBuffer, url );
            }

         // extract service
         Service = pathBuffer;

         const char Colon = ':', Slash = '/';
         char* p;
         for ( p = pathBuffer; *p && *p != Colon; p++ )
            ;

         if ( *p )
            {
            // Mark the end of the Service.
            *p++ = 0;

            if ( *p == Slash )
               p++;
            if ( *p == Slash )
               p++;

            Host = p;

            for ( ; *p && *p != Slash && *p != Colon; p++ )
               ;

            if ( *p == Colon )
               {
               // Port specified.  Skip over the colon and
               // the port number.
               *p++ = 0;
               Port = +p;
               for ( ; *p && *p != Slash; p++ )
                  ;
               }
            else
               Port = p;

            if ( *p )
               // Mark the end of the Host and Port.
               *p++ = 0;

            // Whatever remains is the Path.
            Path = p;
            }
         else
            Host = Path = p;

         // check what's the type
         if ( strcmp ( Service, "https" ) == 0 )
            isHttps = true;
         else
            isHttps = false;

         #ifdef _UMSE_CRAWLER_PARSEDURL_LOG
         printf ( "@ (ParsedUrl::ParsedUrl) url `%s` service `%s`\n", url, Service );
         printf ( "@ (ParsedUrl::ParsedUrl) url `%s` Host `%s`\n", url, Host );
         printf ( "@ (ParsedUrl::ParsedUrl) url `%s` Port `%s`\n", url, Port );
         printf ( "@ (ParsedUrl::ParsedUrl) url `%s` Path `%s`\n", url, Path );
         #endif
         }

   public:
      explicit ParsedUrl ( const string& url )
         {
         ctr_helper ( url.c_str() );
         }

      explicit ParsedUrl ( const char* url )
         {
         ctr_helper ( url );
         }

      ~ParsedUrl( )
         {
         delete[ ] pathBuffer;
         //delete[ ] OriginalUrl;
         }

   private:
      char* pathBuffer;
   };
