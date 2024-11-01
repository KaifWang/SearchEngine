//
// crawler.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
// Wenxuan Zhao zwenxuan@umich.edu
//
// Main function to run crawler
//

#include <cstring>
#include <signal.h>

#include "crawler_core.hpp"

int main(int argc, const char** argv )
   {
   if ( argc != 2 )
      {
         printf("Useage: ./crawler.o machine_number\n");
         exit(1);
      }
   printf("# version 1\n");

   signal(SIGPIPE, SIG_IGN);
   signal(SIGSEGV, SIG_IGN);

   int machineId = std::atoi(argv[1]);

   CrawlerServer crawler(machineId);
   crawler.run();
   }