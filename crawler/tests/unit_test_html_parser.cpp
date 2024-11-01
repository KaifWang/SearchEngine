//
// init_test_html_parser.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Unit test for html parser
//
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "../html_parser.hpp"
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;

char* ReadFile ( char* filename, size_t& fileSize )
   {
   // Read the file into memory.
   // You'll soon learn a much more efficient way to do this.

   // Attempt to Create an istream and seek to the end
   // to get the size.
   ifstream ifs ( filename, ios::ate | ios::binary );
   if ( !ifs.is_open( ) )
      return nullptr;
   fileSize = ifs.tellg( );

   // Allocate a block of memory big enough to hold it.
   char* buffer = new char[ fileSize ];

   // Seek back to the beginning of the file, read it into
   // the buffer, then return the buffer.
   ifs.seekg ( 0 );
   ifs.read ( buffer, fileSize );
   return buffer;
   }

int main ( int argc, char** argv )
   {
   if ( argc != 2 )
      {
      printf ( "Useage: ./test_html_parser website.html\n" );
      exit ( 1 );
      }

   size_t fileSize = 0;
   char* buffer = ReadFile ( argv[ 1 ], fileSize );
   if ( !buffer )
      {
      printf ( "Could not open the file.\n" );
      exit ( 1 );
      }

   HtmlParser parser ( buffer, fileSize );
   parser.parse();
   delete [ ] buffer;
   }
