#pragma once

// HashBlob, a serialization of a HashTable into one contiguous
// block of memory, possibly memory-mapped to a HashFile.

// Leo Yan leoyan@umich.edu

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include "../utility/HashTable.h"
#include "PostingList.h"

static const size_t max_length = 16;

using namespace std;
using Hash_blob = HashTable< const char *,  PostingListInMem>;
using Pair_blob = Tuple< const char *, PostingListInMem>;
using HashBucket = Bucket< const char *, PostingListInMem>;

static size_t RoundUp( size_t length, size_t boundary )
   {
   // Round up to the next multiple of the boundary, which
   // must be a power of 2.

   static const size_t oneless = boundary - 1,
      mask = ~( oneless );
   return ( length + oneless ) & mask;
   }


struct SerialTuple
   {

   public:

      // SerialTupleLength = 0 is a sentinel indicating
      // this is the last SerialTuple chained in this list.
      // (Actual length is not given but not needed.)

      size_t Length;
      uint32_t HashValue;
      // The Key will be a C-string of whatever length.
      char KeyValue[Unknown];

      // Calculate the bytes required to encode a HashBucket as a
      // SerialTuple.

      static size_t BytesRequired( const HashBucket *b )
         {
         // Your code here.
         if ( !b )
            // when b is nullptr, return the size of a sentinel SerialTuple
         {
            return RoundUp( sizeof( size_t ) + sizeof( uint32_t ) + sizeof( KeyValue ), sizeof( size_t ) );
         }
         size_t originalSize = sizeof( size_t ) + sizeof ( uint32_t ) +
            ( strlen( b->tuple.key ) + 1 );
         originalSize += RoundUp( originalSize, max( sizeof( size_t ), ( strlen( b->tuple.key ) + 1  ) ) );
         size_t headerSize = sizeof(b->tuple.value) - sizeof(vector<ByteStruct>) - sizeof(b->tuple.value.lastActualLocation) - sizeof(b->tuple.value.curPostIndex) - sizeof(b->tuple.value.lastOccurDocId);
         size_t totalSize = originalSize + headerSize + b->tuple.value.postLength * sizeof(ByteStruct);
         return totalSize;
         }

      // Write the HashBucket out as a SerialTuple in the buffer,
      // returning a pointer to one past the last character written.
      
      // The SerialTuple::Write( ) routine should cast the char *buffer to
      // a SerialTuple *, fill in the values, including the key as a C-string,
      // round up to the next 8-boundary and return the location where the next
      // SerialTuple is to be written.

      static char *Write( char *buffer, char *bufferEnd,
            const HashBucket *b )
         {
         // Your code here.
         size_t bytesRequired = BytesRequired( b );
         SerialTuple* bufferSerialTuple = reinterpret_cast<SerialTuple*>( buffer );
         // is !b, meaning we need to store a sentinel
         if ( !b )
         {
            bufferSerialTuple->Length = 0;
            return buffer + bytesRequired;
         }
         bufferSerialTuple->Length = bytesRequired;
         bufferSerialTuple->HashValue = b->hashValue;
         size_t length = strlen(b->tuple.key) + 1;
         std::memcpy(bufferSerialTuple->KeyValue, &(b->tuple.key[0]), length);
         //add padding of 0 if key length is smaller than max_length which is set to be 16
         if( length < max_length )
         {
            for( size_t i = length; i < max_length; ++i ) 
               bufferSerialTuple->KeyValue[i] = '0';
            
         }
         size_t headerSize = sizeof( b->tuple.value ) - sizeof( vector<ByteStruct> ) 
                              - sizeof( b->tuple.value.lastActualLocation ) 
                              - sizeof( b->tuple.value.curPostIndex ) 
                              - sizeof( b->tuple.value.lastOccurDocId );
         std::memcpy( bufferSerialTuple->KeyValue + max_length, &b->tuple.value, headerSize );
         std::memcpy( bufferSerialTuple->KeyValue + max_length + headerSize, &( b->tuple.value.posts[0] ), 
         b->tuple.value.postLength * sizeof( ByteStruct ) );
         return buffer + bytesRequired;
         }
       
       //Get the PostingList from the SerialTuple
       const PostingList* GetPostingListValue( ) const 
         {
         const PostingList* post = reinterpret_cast<const PostingList*> ( KeyValue + max_length );
         return post;
         }

       const char* getKey( ) const
         {
         const char* key = reinterpret_cast<const char*> ( KeyValue );
         return key;
         }
  };


struct SerialTupleDoc{
    public:
        size_t Length;
        char KeyValue[ Unknown ];
    
    static size_t BytesRequired( const DocumentData *d )
    {
      if( !d ) 
         return RoundUp( sizeof( size_t ) + sizeof( KeyValue ), sizeof( size_t ) );
      size_t originalSize = sizeof( size_t ) + sizeof( d -> numTitleWords ) + sizeof(char) * (MAX_ABSTRACT_SIZE + MAX_TITLE_SIZE);
      size_t URLSize = d->URL.size( ) + 1;
      return originalSize + URLSize;
    }

    static char *Write( char *buffer, char *bufferEnd, const DocumentData *d )
      {
      size_t bytesRequired = BytesRequired( d );
      SerialTupleDoc* bufferSTD = reinterpret_cast<SerialTupleDoc*> ( buffer );
      if( !d )
         {
         bufferSTD -> Length = 0;
         return buffer + bytesRequired;
         }
      bufferSTD->Length = bytesRequired;
      size_t counter = 0;
      std::memcpy( bufferSTD->KeyValue, &( d->numTitleWords ), sizeof( d->numTitleWords ) );
      counter += sizeof( d->numTitleWords );
      std::memcpy( bufferSTD->KeyValue + counter, d->title, sizeof( char ) * MAX_TITLE_SIZE );
      counter += sizeof( char ) * MAX_TITLE_SIZE;
      std::memcpy( bufferSTD->KeyValue + counter, d->abstract, sizeof( char ) * MAX_ABSTRACT_SIZE );
      counter += sizeof( char ) * MAX_ABSTRACT_SIZE;
      std::memcpy( bufferSTD->KeyValue + counter, &( d->URL[ 0 ] ), d->URL.size( ) + 1);
      return buffer + bytesRequired;
      }
    
    //Get the Value which is a DocumentData Struct
    DocumentData GetDocData( ){
        auto *numTitleWords = reinterpret_cast<const size_t*>( KeyValue );
        auto *title = reinterpret_cast<const char*>( KeyValue + sizeof(size_t) );
        auto *abstract = reinterpret_cast<const char*>( KeyValue + sizeof(size_t) + MAX_TITLE_SIZE );
        auto *Url = reinterpret_cast<const char*>( KeyValue + sizeof(size_t) + MAX_TITLE_SIZE + MAX_ABSTRACT_SIZE );
        //char titleArray[MAX_TITLE_SIZE];
        //char abs[MAX_ABSTRACT_SIZE];
        //std::memcpy(titleArray, &title[0], MAX_TITLE_SIZE);
        //std::memcpy(abs, &abstract[0], MAX_ABSTRACT_SIZE);
        DocumentData tmp;
        tmp.numTitleWords = *numTitleWords;
        std::memcpy(tmp.title, &title[0], MAX_TITLE_SIZE);
        std::memcpy(tmp.abstract, &abstract[0], MAX_ABSTRACT_SIZE);
        tmp.URL = Url;
        return tmp;
    }
};


class HashBlob
   {
   // This will be a hash specifically designed to hold an
   // entire hash table as a single contiguous blob of bytes.
   // Pointers are disallowed so that the blob can be
   // relocated to anywhere in memory

   // The basic structure should consist of some header
   // information including the number of buckets and other
   // details followed by a concatenated list of all the
   // individual lists of tuples within each bucket.

   public:

      // Define a MagicNumber and Version so you can validate
      // a HashBlob really is one of your HashBlobs.

      //char* URLS[Unknown];

      size_t NumberOfUniqueWords,
         NumberOfPosts,
         BlobSize,
         NumberOfBuckets,
         Buckets[ Unknown ];

      // The SerialTuples will follow immediately after.
      // SerialTuple[ Unknown ];


      const SerialTuple *Find( const char *key ) const
         {
         // Search for the key k and return a pointer to the
         // ( key, value ) entry.  If the key is not found,
         // return nullptr.
            
         // hash the key
         uint32_t inputHashValue = Hash_blob::hash( key );
         // find the bucket
         size_t index = inputHashValue % NumberOfBuckets;
         // look the hashValue up in the table of Buckets
         if ( !Buckets[ index ] )
            return nullptr;

         size_t offset = Buckets[index];
         // cast a ptr to the HashBlob to a char*
         char* hashBlobCharPtr = reinterpret_cast<char*>( const_cast<HashBlob*> ( this ) );
         // then add the offset to it
         hashBlobCharPtr = hashBlobCharPtr + offset;
         // cast the ptr to a SerialTuple*
         SerialTuple* serialTuplePtr = reinterpret_cast<SerialTuple*>( hashBlobCharPtr );
         // search through chain of SerialTuples until sentinel
         while ( serialTuplePtr->Length != 0 )
            {
            if ( serialTuplePtr->HashValue == inputHashValue )
               return serialTuplePtr;
            // increment to the next serialTuplePtr w/ the same index
               
            size_t serialTupleLength = serialTuplePtr->Length;
            char* temp = reinterpret_cast<char*>( serialTuplePtr );
            temp = temp + serialTupleLength;
            serialTuplePtr = reinterpret_cast<SerialTuple*>( temp );
            }
         return nullptr;

         }


      static size_t BytesRequired( const Hash_blob *hashTable)
         {
         // Calculate how much space it will take to
         // represent a HashTable as a HashBlob.
         // Need space for the header + buckets +
         // all the serialized tuples.
         // Your code here.
         size_t sizeHeader = sizeof( size_t ) * 4;
         size_t sizeBuckets = sizeof( size_t ) * hashTable->numberOfBuckets;
         size_t sizeSerialTuples = 0;
         Hash_blob::Iterator itr = hashTable->cbegin( );
         while ( itr != hashTable->cend( ) )
            {
            sizeSerialTuples += SerialTuple::BytesRequired( itr.currentBucket );
            // check end of chain of the same index for sentinel size
            if ( !itr.currentBucket->next )
               sizeSerialTuples += SerialTuple::BytesRequired( nullptr );
            itr++;
            }
         return sizeHeader + sizeBuckets + sizeSerialTuples;
         }

      // Write a HashBlob into a buffer, returning a
      // pointer to the blob.
      static HashBlob *Write( HashBlob *hb, size_t bytes,
            const Hash_blob *hashTable)
         {
         // Your code here.
         // header
         hb->NumberOfUniqueWords = hashTable->numberOfWords;
         hb->NumberOfPosts = 0;
         hb->BlobSize = bytes;
         hb->NumberOfBuckets = hashTable->numberOfBuckets;
         // write to buckets array and SerialTuples in one
         // iteration of hashTable.buckets instead of two
         // bufferSerialTuple = first byte following buckets array
         // currentOffset and totalLengthAtIndex are for buckets array
         size_t currentOffset = sizeof ( size_t ) * ( 4 + hb->NumberOfBuckets );
         char* bufferSerialTuple = reinterpret_cast<char*>( hb ) + sizeof ( size_t ) * ( 4 + hb->NumberOfBuckets );
         for ( size_t i = 0; i < hb->NumberOfBuckets; ++i )
            {
            size_t totalLengthAtIndex = 0;
            if ( !hashTable->buckets[ i ] )
               {
               hb->Buckets[ i ] = 0;
               continue;
               }
            // when hashTable->buckets[ i ] has at least one element
            hb->Buckets[ i ] = currentOffset;
            for ( HashBucket* b = hashTable->buckets[ i ]; b; b = b->next )
               {
               totalLengthAtIndex += SerialTuple::BytesRequired( b );
               bufferSerialTuple = SerialTuple::Write( bufferSerialTuple, reinterpret_cast<char*>( hb ) + bytes, b );
               hb->NumberOfPosts += b->tuple.value.numberOfPosts;
               }
            // add sentinel only when necessary
            totalLengthAtIndex += SerialTuple::BytesRequired( nullptr );
            // update currentOffset
            currentOffset += totalLengthAtIndex;
            bufferSerialTuple = SerialTuple::Write( bufferSerialTuple, reinterpret_cast<char*>( hb ) + bytes, nullptr );
            }
         return hb;
         }

      // Create allocates memory for a HashBlob of required size
      // and then converts the HashTable into a HashBlob.
      // Caller is responsible for discarding when done.

      // (No easy way to override the new operator to create a
      // variable sized object.)

      static HashBlob *Create( const Hash_blob *hashTable)
         {
         // Your code here.
         size_t bytesRequired = BytesRequired( hashTable);
         HashBlob* buffer = reinterpret_cast<HashBlob*>( malloc( bytesRequired ) );
         assert( buffer );
         Write( buffer, bytesRequired, hashTable);
         return buffer;
         }

      // Discard

      static void Discard( HashBlob *blob )
         {
         // Your code here.
         free( blob );
         }
   };


class HashFile
   {
   private:
      size_t DocDataSize;
      char *URL;
      HashBlob *blob;
      size_t bytes;
      size_t FileSize( int f )
         {
         struct stat fileInfo;
         fstat( f, &fileInfo );
         return fileInfo.st_size;
         }

   public:

      const HashBlob *Blob( )
         {
            return blob;
         }
   
       
       //Get the vector of all DocumentData in the chunk
       vector<DocumentData> get_DocumentData( ){
           vector<DocumentData> result;
           char* start_ptr = URL;
           for(size_t i = 0; i < DocDataSize;) 
           {
               SerialTupleDoc* tmp_s = reinterpret_cast<SerialTupleDoc*>( start_ptr );
               DocumentData tmp_d = tmp_s -> GetDocData( );
               string s = tmp_d.URL;
               result.push_back( tmp_d );
               start_ptr += tmp_s -> Length;
               i += tmp_s -> Length;
           }
           return result;
       }

      HashFile( const char *filename )
         {
         // Open the file for reading, map it, check the header,
         // and note the blob address.
         URL = new char[DOC_DATA_SPARSE_FILE_SIZE];
         int file_decipher = open( filename, O_RDONLY );
         bytes = FileSize( file_decipher );
         char *file_in_memory = ( char* )mmap( ( caddr_t )0, bytes, PROT_READ, MAP_SHARED, file_decipher, 0 );
         std::memcpy( &DocDataSize, file_in_memory, sizeof( size_t ) );
         std::memcpy( URL, file_in_memory + sizeof( size_t ), DocDataSize );
         blob = reinterpret_cast<HashBlob*>( file_in_memory + sizeof( DocDataSize )+ DocDataSize );
         close( file_decipher );
         }

      ~HashFile( )
         {
            // HashBlob::Discard(blob);
            munmap( blob, bytes );
            delete URL;
         }
   };



// Seperate Hashfile write process from read to avoid
// too much stack memory allocated for URL sparsed array (bus error)
class HashFileWrite
   {
   private:
      size_t DocDataSize;
      char URL[DOC_DATA_SPARSE_FILE_SIZE];
      HashBlob *blob;
      size_t bytes;
      size_t FileSize( int f )
         {
         struct stat fileInfo;
         fstat( f, &fileInfo );
         return fileInfo.st_size;
         }

   public:

      //write vector docs data to The big chunk of memory array
      void Write( vector<DocumentData> &Docs ){
         size_t totalLength = 0;
         char* bufferSerialSTD = URL;
         //cout << "URLs built into Index: " << endl;
         for ( size_t i = 0; i < Docs.size( ); ++i )
         {
            //cout << Docs[ i ].URL << endl; 
            totalLength += SerialTupleDoc::BytesRequired( &Docs[ i ] );
            bufferSerialSTD = SerialTupleDoc::Write( bufferSerialSTD, URL + DOC_DATA_SPARSE_FILE_SIZE - 1, &Docs[ i ] );
         }
         DocDataSize = totalLength;
      }
      
      HashFileWrite( const char *filename, const Hash_blob *hashtable, vector<DocumentData> &DOCS )
         {
         // Open the file for write, map it, write
         // the hashtable out as a HashBlob, and note
         // the blob address.
         Write( DOCS) ;//URL write
         bytes = HashBlob::BytesRequired( hashtable ) + DocDataSize;
         cout << "size of hashblob: " << bytes << endl;
         int file_decipher = open( filename, O_RDWR | O_CREAT | O_TRUNC,
            S_IRWXU );
         //size_t fsize = FileSize( file_decipher );
         char *file_in_memory = ( char* )mmap( ( caddr_t)0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, file_decipher, 0 );
         // seek to bytes + 1 and write 'a' first
         // to prevent build error
         lseek( file_decipher, bytes + 1, SEEK_SET );
         write( file_decipher, "a", 1 );

         memset( file_in_memory, 0, bytes );
         std::memcpy( file_in_memory, &DocDataSize, sizeof( DocDataSize ) );
         char *url = file_in_memory + sizeof( DocDataSize );
         std::memcpy( url, URL, DocDataSize );
         HashBlob *hb = reinterpret_cast<HashBlob*>( file_in_memory + sizeof( DocDataSize )+ DocDataSize);
         close( file_decipher );
         HashBlob::Write( hb, HashBlob::BytesRequired( hashtable ), hashtable);
         blob = hb;;
         }

      ~HashFileWrite( )
         {
            munmap( blob, bytes );
         }
   };
