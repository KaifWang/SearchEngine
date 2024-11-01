//
// unit_test_frontier.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Unit test for frontier
//

/*
 * let one of the two child thread consumer all urls
 * both child thread should stop
 * let the adder thread add urls
 * child thread should continue
 */

#include <iostream>
#include "../configs/crawler_cfg.hpp" // configuration
#include "../crawler_frontier.hpp"

using std::cout;
using std::endl;

int main()
   {

   Frontier frontier;
   printf ( "# frontier init finish\n" );

   }
