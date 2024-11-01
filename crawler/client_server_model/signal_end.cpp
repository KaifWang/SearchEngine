//
// signal_end.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Send end signal to host:port
//
#include "../configs/crawler_cfg.hpp"
#include "../network_utility.hpp"

int main ( int argc, const char** argv )
   {
   if ( argc != 4 )
      {
      printf ( "Useage : ./signal_end host port_url port_signal\n" );
      exit ( 1 );
      }

   if ( tcp_connect_send ( argv[1], argv[3], UMSE_CRAWLER_END_SIGNAL ) < 0 )
      {
      printf ( "@ unable to send end msg to signal socket, end\n" );
      //exit ( 1 );
      }

   if ( tcp_connect_send ( argv[1], argv[2], UMSE_CRAWLER_END_SIGNAL ) < 0 )
      {
      printf ( "@ unable to send end msg to url socket, end\n" );
      //exit ( 1 );
      }

   printf ( "* signal_end end\n" );
   }