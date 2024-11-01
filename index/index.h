// Index.h
// Build the index chunk from a directory of parsed HTML

// Created by Zhicheng Huang (skyhuang@umich.edu)
// Edited by Kai Wang (kaifan@umich.edu)

#pragma once
#include <stdlib.h>

#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "DeltaConverter.h"
#include "PostingList.h"
#include "HashBlob.h"



using HashInMem = HashTable< const char *, PostingListInMem >;
using PairInMem = Tuple< const char *, PostingListInMem >;
using HashURL = HashTable< const char*, size_t >;
using PairURL = Tuple< const char*, size_t >;

static void memcopy(void *dest, void *src, size_t n)
   {
      char *s = (char *)src;
      char *d = (char *)dest;

      for(int i = 0; i < n; ++i)
         d[i] = s[i];
   }
// Obtain high bits of a location to index into the sycTable
static size_t highBit(size_t location){ return location >> 20;}


// Take in a word and offset and indexURL (default to be 0: non-zero => EndDoc token)
// Find the PostingListInMem corresponds to the word and push the
// encoded offset in PostingListInMem->posts
static void DictionaryInMemInsert( HashInMem* d, const char* token,
                            size_t actualLocation, size_t DocId, size_t indexURL)
{
   // MARK: first occurance of this token sets up a bucket & returns nullptr
   // MARK: therefore it needs a second find to get the reference to bucket's tuple
   PairInMem* pair = d->Find(token, PostingListInMem( ));
   if(!pair){
      pair = d->Find(token);
   }
   PostingListInMem* p = &( pair->value );
   size_t offset = actualLocation - p->lastActualLocation;
   p->lastActualLocation = actualLocation;
   p->numberOfPosts++;
   PostingListInMem::insert( p, offset );
   // update lastOccurDocId
   if ( p->lastOccurDocId < DocId )
      {
         p->lastOccurDocId = DocId;
         p->numberOfDocuments++;
      }

   // check if it's time to write to syncTable by checking if SynchronizationData.actualLocation is 0
   if ( !p->syncTable[ highBit(actualLocation) ].actualLocation )
      {
         //indexOffset will store the index for this delta
         p->syncTable[ highBit(actualLocation) ].indexOffset = p->curPostIndex;
         p->syncTable[ highBit(actualLocation) ].actualLocation = actualLocation;
      }
   // update curPostIndex
   p->curPostIndex += IndicatedLength( offset );
   // Insert indexURL for EndDoc postingList
   if(CompareEqual(token, "##EndDoc"))
   {
      PostingListInMem::insert( p, indexURL );
      p->curPostIndex += IndicatedLength( indexURL );
   }
}


// Take in a HashInMem*, a filename, a vector & of urls, & actualLocation and & indexURL
// read from the file, keep track of offsets, and insert <token, offset> in HashInMem
static void InputReader( HashInMem* d, const char* filename,
                  size_t &actualLocation, size_t &indexURL, size_t DocId,
                  vector<DocumentData>& DocData, vector<char*> &keyManager)
{
   int fd = open( filename, O_RDONLY );
   struct stat fileInfo;
   fstat( fd, &fileInfo );
   size_t fSize = fileInfo.st_size;
   char *map = ( char* )mmap( ( caddr_t )0, fSize, PROT_READ, MAP_SHARED, fd, 0 );
   // TODO: fault tolerence

   // each parser output file must have three lines: url, numTitleWords, numBodyWords
   char *cur = map;
   char *newLine = ( char* ) memchr( cur, '\n', fSize );
   if ( !newLine )
      return;
   string url( cur, newLine - cur );
   cur = newLine + 1;
   newLine = ( char* ) memchr( cur, '\n', fSize );
   if ( !newLine )
      return;
   string numTitleWordsStr( cur, newLine - cur );
   // cout << "@Parsing url: " << url << endl;
   int numTitleWords = stoi( numTitleWordsStr );
   cur = newLine + 1;
   newLine = ( char* ) memchr( cur, '\n', fSize );
   if ( !newLine )
      return;
   string numBodyWordsStr( cur, newLine - cur );
   int numBodyWords = stoi( numBodyWordsStr );
   // DocumentData
   DocumentData dd;
   dd.numTitleWords = numTitleWords;
   dd.URL = url;
   // read each title word token
   int curTitleLength = 0;
   bool canAddtoTitle = true;
   memset( dd.title, 0, sizeof( dd.title ) );
   for ( int i = 0; i < numTitleWords; ++i )
   {
      cur = newLine + 1;
      newLine = ( char* ) memchr( cur, '\n', fSize );
      string token( cur, newLine - cur );
      if (token.length() >= TOKEN_MAX_SIZE) {continue;}
      if( canAddtoTitle )
      {
         if ( curTitleLength + token.length( ) - 1 < MAX_TITLE_SIZE )
         {
            strncpy( dd.title + curTitleLength, token.c_str() + 1, token.length() - 1);
            curTitleLength += token.length() - 1;
            dd.title[curTitleLength] = ' ';
            curTitleLength ++;
         }
         else
            canAddtoTitle = false;
      }
      char* temp = new char[ TOKEN_MAX_SIZE ];
      keyManager.push_back(temp);
      memset(temp, 0, TOKEN_MAX_SIZE);
      strncpy( temp, token.c_str( ), token.length() );
      DictionaryInMemInsert( d, temp, actualLocation++, DocId, indexURL );
   }
   // read each body words token
   int curAbstractLength = 0;
   bool canAddToAbstract = true;
   memset( dd.abstract, 0, sizeof( dd.abstract ) );
   for ( int i = 0; i < numBodyWords; ++i )
   {
      cur = newLine + 1;
      newLine = ( char* ) memchr( cur, '\n', fSize );
      string token( cur, newLine - cur );
      if (token.length() >= TOKEN_MAX_SIZE) { continue; }
      if ( canAddToAbstract )
      {
         if ( curAbstractLength + token.length( ) < MAX_ABSTRACT_SIZE )
         {
            if ( i >= 5 )
            {
               strncpy( dd.abstract + curAbstractLength, token.c_str(), token.length() );
               curAbstractLength += token.length();
               dd.abstract[curAbstractLength] = ' ';
               curAbstractLength ++;
            }
         }
         else
            canAddToAbstract = false;
      }
      char* temp = new char[TOKEN_MAX_SIZE];
      keyManager.push_back(temp);
      memset(temp, 0, TOKEN_MAX_SIZE);
      strncpy( temp, token.c_str( ), token.length() );
      DictionaryInMemInsert( d, temp, actualLocation++, DocId, indexURL );
   }

   
   // Overwrite the space at the end
   if(curTitleLength > 1)
      dd.title[ curTitleLength - 1 ] = '\0';
   if(curAbstractLength > 1)
      dd.abstract[ curAbstractLength - 1 ] = '\0';
   DocData.push_back( dd );
   
   // add to endDoc posting list at the end
   // MARK: endDoc token = ##EndDoc
   DictionaryInMemInsert( d, "##EndDoc", actualLocation++, DocId, indexURL++ );
   // close
   close( fd );
   munmap( map, fSize );
   return;
}

// Add a sentinel at the end of each PostingListInMem.posts
// Also update postLength;
static void AddSentinel( HashInMem* d )
   {
   for ( auto it = d->begin( ); it != d->end( ); it++ )
      {
      it->value.posts.push_back( ByteStruct( ) );
      it->value.postLength = it->value.posts.size( );
      }
   }

using Hash = HashTable< const char *, PostingList >;
using Pair = Tuple< const char *, PostingList >;


// Return true if name ends with .txt
static bool TextFile( const char* name )
   {
   size_t length = strlen( name );
   return ( name[ length - 1 ] == 't' && name[ length - 2 ] == 'x'
       && name[ length - 3 ] == 't' && name[ length - 4 ] == '.');
   }

// Check if the file is a index chunk
// Return true if name ends with .bin
static bool IndexFile( const char* name )
   {
   size_t length = strlen( name );
   return ( name[ length - 1 ] == 'n' && name[ length - 2 ] == 'i'
       && name[ length - 3 ] == 'b' && name[ length - 4 ] == '.');
   }

// reset variables for a Dictionary
static void ResetNewDictionary( size_t& actualLocation, size_t &indexURL, size_t &DocId,
                        vector<DocumentData>& DocData )
   {
   // MARK: actualLocation and indexURL are both initialized to be 1
   // MARK: (need to subtract 1 when indexing into URL vector)
   // MARK: because ByteStruct 0 is used as sentinel
   actualLocation = 1;
   indexURL = 0;
   DocId = 1; // for numberOfDocuments
   while ( DocData.size( ) != 0 )
      DocData.pop_back( );
   }

// Once we have added all tokens to d and u
// 0) Optimize and addSentinel
// 1) Turn d and u into HashFiles
// 2) Delete dynamic keys
static void FinishUpDictionary( HashInMem* d, vector<DocumentData>& DocData,
      int DictID, vector<char*> &keyManager)
   {
   // Optimize and addSentinel
   d->Optimize( );
   // cout << d->Find("fox")->key << endl;
   AddSentinel( d );
   
   // Turn hashTable into HashBlob
   char fileName[ 100 ];
   char bufferDictID[ sizeof( int ) * 8 + 1 ];
   sprintf( fileName, "%s/hashfile%d.bin", INDEX_DIRECTORY, DictID );
   HashFileWrite hashfile(fileName, d, DocData);
   for(size_t i = 0; i < keyManager.size(); i++)
      delete[] keyManager[i];
   keyManager = vector<char*>(0);
   }

// the function that does the following job
// 1) take in a directory path
// 2) BFS and open each file output from HTMLParser
// 3) read each file, calculate offset as we go, and push to a hashtable (DictionaryInMem)
// 4) convert DictionaryInMem to Dictionary
// 5) convert Dictionary to Hashblob with correct headers (***THIS IS "THE" DICTIONARY***)
static void ParserToDictionary ( const string dirName )
   {
   // TODO: implement at the end
   // MARK: actualLocation and indexURL are both initialized to be 1
   // MARK: (need to subtract 1 when indexing into URL vector)
   // MARK: because ByteStruct 0 is used as sentinel
   size_t actualLocation = 1;
   size_t indexURL = 0;
   size_t DocId = 1; // for numberOfDocuments
   HashInMem* dic = new HashInMem( );
   
   vector<DocumentData> DocData;
   vector<char*> keyManager;
   // FIXME: loop over each file in directory
   DIR* handle = opendir( dirName.c_str( ) );
   if ( handle )
      {
      struct dirent * entry;
      size_t DictID = 1;
      size_t counter = 1;
      while ( ( entry = readdir( handle ) ) )
         {
         struct stat statbuf;
         string fileName = dirName + '/' + entry->d_name;
         if ( stat( fileName.c_str( ), &statbuf ) )
            cerr << "stat of " << dirName << " failed, errono = " << errno << endl;
         else
            {
            // if file extension is .txt
            if ( TextFile( fileName.c_str( ) ) )
               {
               InputReader( dic, fileName.c_str( ), actualLocation, indexURL, DocId, DocData, keyManager );
               DocId++;
               if ( counter++ >= NUM_DOCS_IN_DICT )
                  {
                  FinishUpDictionary(dic, DocData, ( int )DictID, keyManager);
                  ResetNewDictionary(actualLocation, indexURL, DocId, DocData);
                  delete dic;
                  dic = new HashInMem();
                  counter = 1;
                  DictID++;
                  }
               }
            }
         }
         // FinishUp the last dictionary
         if ( counter > 1 )
            {
            FinishUpDictionary( dic, DocData, ( int )DictID, keyManager);
            delete dic;
            }
   closedir( handle );
   }
         
   else
      cerr << "open handle failed, errno = " << errno;
   return;
   }
