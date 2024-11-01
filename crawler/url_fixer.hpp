//
// url_fixer.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Wenxuan Zhao zwenxuan@umich.edu
//
// Fix relative url to absolute url
//
#pragma once

#include "./configs/crawler_cfg.hpp"
#include "url_parser.hpp"

void fix_url_to_correct_format(string& url, const ParsedUrl& parsedUrl, const string& baseUrl)
   {
   if ( url.length() >= 2 && url[0] == '/' && url[1] == '/' )
      url = "https:" + url;
   else if ( url.length() >= 1 && url[0] == '/' )
      url = string ( parsedUrl.Service ) + "://" + string ( parsedUrl.Host ) + url;
   else if ( url.length() >= 1 && url[0] == '#' )
      url = baseUrl + url;
   else if ( url.length() >= 3 && url[0] == '.' && url[1] == '.' && url[2] == '/' )
      {
      int parent_count = 0;
      string URL_end = "";
      for ( size_t ind = 0; ind < strlen ( parsedUrl.Path ); ind++ )
         {
         if ( parsedUrl.Path[ind] == '/' )
            parent_count++;
         }
      size_t i = 0;
      while ( i < url.length() )
         {
         if ( url[i] == '.' && url[i + 1] == '.' && url[i + 2] == '/' )
            {
            parent_count--;
            i = i + 3;
            }
         else
            {
            URL_end += url[i];
            i++;
            }
         }
      url = string ( parsedUrl.Service ) + "://" + string ( parsedUrl.Host ) + "/";
      for ( size_t ind = 0; ind < strlen ( parsedUrl.Path ); ind++ )
         {
         if ( parent_count <= 0 )
            break;       
         if ( parsedUrl.Path[ind] == '/' )
            {
               url = url + parsedUrl.Path[ind];
               parent_count--;
            }
            
         else 
            url = url + parsedUrl.Path[ind];
         }
      url = url + URL_end;
      }
      else {
         return;
      }
   }
