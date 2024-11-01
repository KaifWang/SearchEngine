//
// client.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Send msg (new urls, etc) to server
//
// Useage: ./client localhost message
//
#include "../configs/crawler_cfg.hpp"
#include "../network_utility.hpp"

int main ( int argc, const char** argv )
   {
   if ( argc != 4 )
      {
      printf ( "Useage : ./client host port message\n" );
      exit ( 1 );
      }

   if ( tcp_connect_send ( argv[1], argv[2], argv[3] ) < 0 )
      {
      printf ( "@ unable to send msg %s to host %s port %s, end\n", argv[3], argv[1], argv[2] );
      exit ( 1 );
      }

   printf ( "* client end\n" );
   }