// Define PosttingList struct.
// PostingListInMem helps insert easier. Transformed into PostingList
// when writting to the disk

// Zhicheng Huang (skyhuang@umich.edu)
// Leo Yan (leoyan@umich.edu)
// Kai Wang (kaifan@umich.edu)

#pragma once

#include <vector>
#include "../configs/config.h"
#include "DeltaConverter.h"

using namespace std;

static const size_t Unknown = 0;

struct SynchronizationData
{
   size_t indexOffset; //index in posts, an array/vector of ByteStruct
   size_t actualLocation;
};

struct DocumentData
{
   size_t numTitleWords;
   char title[MAX_TITLE_SIZE];
   char abstract[MAX_ABSTRACT_SIZE];
   string URL;
};

class PostingListInMem
{
   public:
      
      size_t numberOfDocuments;
      size_t numberOfPosts;
      //this is the size of the vector<Bystruct> post
      size_t postLength; 
      // actualLocation of the previous occurence of a token in posting list
      SynchronizationData syncTable[ 128 ];
      vector<ByteStruct> posts;
      size_t lastActualLocation;
      size_t curPostIndex;
      size_t lastOccurDocId; // initialized to be 0, meaning never occured
   
      PostingListInMem( ) :
         numberOfDocuments( 0 ), numberOfPosts( 0 ), lastActualLocation( 0 ),
         curPostIndex( 0 ), lastOccurDocId( 0 ), postLength(0)
         {
         // initialize syncTable to be 0, 0
         for ( int i = 0; i < 128; ++i )
            syncTable[ i ] = SynchronizationData{ };
         }

      // Take in a size_t offset, push it at the end of PostingListInMem->posts
      static void insert( PostingListInMem*& p, size_t offset )
         {
         ByteStruct temp[ 8 ];
         ByteStruct* head = temp;
         EncodeByteStruct( temp, offset );
         size_t numBytes = IndicatedLength( head );
         for ( int i = 0; i < numBytes; ++i )
            p->posts.push_back( head[ i ] );
         }
};

class PostingList
{
   public:
      //char type; // End-Doc/word/title
      size_t numberOfDocuments;
      size_t numberOfPosts;
      //this is the size of the vector<Bystruct> post
      size_t postLength;
      SynchronizationData syncTable[ 128 ];
      ByteStruct posts[ Unknown ];
};
