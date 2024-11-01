//
// valid_url.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Wenxuan Zhao zwenxuan@umich.edu
//
#pragma once

#include <pthread.h> // concurrent
#include "configs/crawler_cfg.hpp" // configuration

bool valid_url ( string& url )
   {
   if ( url == "" )
      return false;
   if ( url[0] == '\n' )
      return false;
   else if ( strstr(url.c_str(), "1") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "2") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "3") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "4") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "5") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "6") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "7") != nullptr)
      return false;
   else if (strstr(url.c_str(), "pornhub" ) != nullptr)
	 return false;
   else if ( strstr(url.c_str(), "8") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "9") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "0") != nullptr)
      return false;
   else if ( strstr ( url.c_str(), " " ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "?" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "#" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".pdf" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".md" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".exe" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".word" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".mp4" ) != nullptr )
       return false;
   else if ( strstr ( url.c_str(), ".ppt" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "*" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "&" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "$" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "@" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "http://" ) != nullptr )
      return false;
   else if ( strstr ( url.c_str(), "." ) == nullptr )
      return false;
   else if ( strstr ( url.c_str(), ".ru" ) != nullptr )
      return false;
   else if ( strstr(url.c_str(), "utsa.edu") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "uth.edu") != nullptr) 
      return false;
	else if ( strstr(url.c_str(), "porn") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "tufts.edu") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "github") != nullptr) 
      return false;
   else if (strstr(url.c_str(), "adactio.com") != nullptr)
	   	return false;
   else if ( strstr(url.c_str(), "usnews") != nullptr)
       return false;
   else if (strstr(url.c_str(), ".cn") != nullptr)
	   return false;
  	else if  (strstr(url.c_str(), ".ru") != nullptr)
		return false;
   else if ( strstr(url.c_str(), "tw") != nullptr)
       return false;
   else if ( strstr(url.c_str(), "vk.com") != nullptr)
       return false;
   else if ( strstr(url.c_str(), "Vk.com") != nullptr)
       return false;
   else if ( strstr(url.c_str(), "gitlab") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "archieve") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "58.com" ) != nullptr)
      return false;
   else if ( strstr(url.c_str(), "vndb.org" ) != nullptr)
      return false;
   else if ( strstr(url.c_str(), "/v/") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "/person/") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "/pages/") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "pixy.org") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "commit") != nullptr) 
      return false;
   else if ( strstr(url.c_str(), "colorgg.com") != nullptr)
      return false;
   else if ( strstr(url.c_str(), "/node") != nullptr)
      return false;
   else if ( strstr ( url.c_str(), "https://" ) == nullptr )
      {
      url = "https://" + url;
      return true;
      }
   else
      return true;
   }
