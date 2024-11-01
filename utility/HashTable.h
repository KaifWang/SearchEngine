// STL Hash Table Implementation

// Created by 
// Kai Wang (kaifan@umich.edu)
// Patrick Su (patsu@umich.edu)

#pragma once

#include <cassert>
#include <iomanip>
#include <cstdint>
using namespace std;

/* CRC lookup table per IEEE 1003.2-1992 E.4.9 */
static unsigned long crctab[] =
      {  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
         0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
         0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
         0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
         0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
         0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
         0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
         0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
         0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
         0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
         0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
         0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
         0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
         0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
         0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
         0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
         0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
         0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
         0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
         0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
         0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
         0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
         0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
         0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
         0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
         0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
         0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
         0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
         0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
         0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
         0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
         0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
         0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
         0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
         0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
         0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
         0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
         0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
         0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
         0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
         0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
         0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
         0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
         0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
         0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
         0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
         0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
         0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
         0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
         0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
         0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
         0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
         0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
         0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
         0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
         0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
         0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
         0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
         0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
         0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
         0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
         0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
         0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
         0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4  };


#define  UpdateCRC(s, c)      (((s) << 8) ^ crctab[((s) >> 24) ^ (c)])


// You may add additional members or helper functions.
// template< typename Key>
// size_t hash ( const Key &k );

// Compare C-strings, return true if they are the same.

static bool CompareEqual( const char *L, const char *R )
   {
      while ( *L && *R && *L == *R )
         L++, R++;
      return !( *L || *R );
   };
   
static bool CompareEqual(size_t left, size_t right)
{
   return left == right;
}

template< typename Key, typename Value > class Tuple
   {
   public:
      Key key;
      Value value;

      Tuple( const Key &k, const Value v ) : key( k ), value( v )
         {
         }
   };


template< typename Key, typename Value > class Bucket
   {
   public:
      Bucket *next;
      uint32_t hashValue;
      Tuple< Key, Value > tuple;

      Bucket( const Key &k, uint32_t h, const Value v ) :
            tuple( k, v ), next( nullptr ), hashValue( h )
         {
         }
   };


template< typename Key, typename Value > class HashTable
   {
   private:

      // Hash using CRC
      static uint32_t hash ( const Key k )
      // uint32_t hash ( const Key k ) const
         {
         const uint8_t *ptr = reinterpret_cast<const uint8_t*> ( k );
         uint32_t result = 0;
         while ( *ptr )
            {
            result = UpdateCRC( result, *ptr );
            ++ptr;
            }
         return result;
         }
   
      // Alternative way of hashing. Compare to see which one is better
      // uint32_t hash ( const Key k) const
      // {
      //    static const size_t FnvOffsetBasis = 146959810393466560;
      //    static const size_t FnvPrime = 1099511628211ul;
      //    size_t result = FnvOffsetBasis;
      //    const char *ptr = reinterpret_cast<const char*> (k);
      //    while (*ptr)
      //    {
      //       result *= FnvPrime;
      //       result ^= *ptr;
      //       ++ptr;
      //    }
      //    return result;
      // }

      Bucket< Key, Value > **buckets;
      size_t numberOfBuckets;
      size_t numberOfWords;

      friend class Iterator;
      friend class HashBlob;
      friend class HashBlobURL;

   public:

      Tuple< Key, Value > *Find( const Key k, const Value initialValue )
         {
         // Search for the key k and return a pointer to the
         // ( key, value ) entry.  If the key is not already
         // in the hash, add it with the initial value.

         // Your code here.
         uint32_t inputHashValue = hash ( k );
         size_t index = inputHashValue % numberOfBuckets;
         Bucket<Key, Value> *currentBucket = buckets[ index ];

         // search the key in the corresponding chain
         while ( currentBucket )
            {
               if ( currentBucket->hashValue == inputHashValue && 
                     CompareEqual( currentBucket->tuple.key, k ) )
                  return &( currentBucket->tuple );
               // Break at the last element of the chain
               // So we can use the variable currentBucket to store next word
               if ( !( currentBucket->next ) )
                  break;
               currentBucket = currentBucket->next;
            }
         // if not found, add the initial value to the end of the chain
         // since the earlier we add, the more common it is for searching
         Bucket<Key, Value> *newBucket = 
               new Bucket<Key, Value>( k, inputHashValue, initialValue );
         numberOfWords++;
         // If it is the first element at this index
         if ( !buckets[ index ] )
            buckets[ index ] = newBucket;
         else
            currentBucket->next = newBucket; 
         return nullptr;
         }

      Tuple< Key, Value > *Find( const Key k ) const
         {
         // Search for the key k and return a pointer to the
         // ( key, value ) enty.  If the key is not already
         // in the hash, return nullptr.

         // Your code here.
         uint32_t inputHashValue = hash ( k );
         size_t index = inputHashValue % numberOfBuckets;
         Bucket<Key, Value> *currentBucket = buckets[ index ];

         // search the key in the corresponding chain
         while ( currentBucket )
            {
            if ( currentBucket->hashValue == inputHashValue && 
                  CompareEqual( currentBucket->tuple.key, k ) )
               return &( currentBucket->tuple );
            currentBucket = currentBucket->next;
            }
         return nullptr;
         }

      void Optimize( double loading = 1.5 )
         {
         // Modify or rebuild the hash table as you see fit
         // to improve its performance now that you know
         // nothing more is to be added.
         size_t newNumberOfBuckets = numberOfWords / loading;
         newNumberOfBuckets = ( newNumberOfBuckets )? newNumberOfBuckets : 1;
         auto newBuckets = new Bucket<Key, Value> *[ newNumberOfBuckets ];
         for ( int i = 0;  i < newNumberOfBuckets;  ++i )
            newBuckets[ i ] = nullptr;
         //Iterate through each element and insert it in the newBuckets
         for ( auto itr = begin( );  itr != end( ) ;  ++itr )
            {
            Key k = itr->key;
            Value v = itr->value;
            uint32_t newHashValue = hash( k );
            size_t index = newHashValue % newNumberOfBuckets;
            Bucket<Key, Value> *newBucket = 
               new Bucket<Key, Value>( k, newHashValue, v );
            Bucket<Key, Value> *currentBucket = newBuckets[ index ];
            // If it is the first element in that position
            if ( !currentBucket )
               newBuckets[ index ] = newBucket;
            else
               {
               // Find next available position in that index
               // Note: Different from a find function, each bucket 
               // has a unique key during rebuilding
               while ( currentBucket->next )
                  currentBucket = currentBucket->next;
               currentBucket->next = newBucket;
               }
            }
         // Clean up memory for the old hashtable
         for ( int i = 0;  i < numberOfBuckets;  ++i )
            {
            Bucket<Key, Value> *currentBucket = buckets[ i ];
            while ( currentBucket )
               {
               Bucket<Key, Value> *tmp = currentBucket;
               currentBucket = currentBucket->next;
               delete tmp;
               }
            }
         delete [ ] buckets;
         // Reset the parameters
         numberOfBuckets = newNumberOfBuckets;
         buckets = newBuckets;
         }


      // Your constructor may take as many default arguments
      // as you like.

      HashTable( )
         {
         // Your code here.
         numberOfBuckets = 20000;
         numberOfWords = 0;
         buckets = new Bucket<Key, Value> *[ 20000 ];
         for ( int i = 0;  i < numberOfBuckets;  ++i )
            buckets[ i ] = nullptr;
         }

      ~HashTable( )
         {
         for ( int i = 0;  i < numberOfBuckets;  ++i )
            {
            Bucket<Key, Value> *currentBucket = buckets[ i ];
            while ( currentBucket )
               {
               Bucket<Key, Value> *tmp = currentBucket;
               currentBucket = currentBucket->next;
               delete tmp;
               }
            }
         delete [ ] buckets;
         }


      class Iterator
         {
         private:

            friend class HashTable;
            friend class HashBlob;
            friend class HashBlobURL;

            // Your code here.
            HashTable *hashTable;
            size_t bucketIndex;
            Bucket<Key, Value> *currentBucket;

            Iterator( HashTable *table, size_t bucket, Bucket<Key, Value> *b ):
                  hashTable( table ), bucketIndex( bucket ), currentBucket( b )
               {
               }

         public:

            Iterator( ) : Iterator( nullptr, 0, nullptr )
               {
               }

            ~Iterator( )
               {
               }

            Tuple< Key, Value > &operator*( )
               {
                  return currentBucket->tuple;
               }

            Tuple< Key, Value > *operator->( ) const
               {
                  return &( currentBucket->tuple );
               }

            // Prefix ++
            Iterator &operator++( )
               {
               currentBucket = currentBucket->next;
               // If reach the end of the collision chain
               // jump to the start of pointer to the next chain of buckets
               // Find the next position in the table containing a bucket
               while ( !currentBucket )
                  {
                  bucketIndex++;
                  // Out of bound
                  if ( bucketIndex >= hashTable->numberOfBuckets )
                     {
                     hashTable = nullptr;
                     bucketIndex = 0;
                     currentBucket = nullptr;
                     return *this;
                     }
                  currentBucket = hashTable->buckets[ bucketIndex ] ;
                  }
               return *this;
               }

            // Postfix ++
            Iterator operator++( int )
               {
               // Your code here.
               Iterator tmp = *this;
               ++*this;
               return tmp;
               }

            bool operator==( const Iterator &rhs ) const
               {
               // Your code here.
               return currentBucket == rhs.currentBucket;
               }

            bool operator!=( const Iterator &rhs ) const
               {
               // Your code here.
               return currentBucket != rhs.currentBucket;
               }
         };

      Iterator begin( )
         {
         // if ( buckets == nullptr )
         //    return Iterator( );
         Bucket <Key, Value> *currentBucket = *buckets;
         size_t currentIndex = 0;
         //Find the first Bucket that contains the word
         while ( !currentBucket )
            {
            currentIndex ++;
            if ( currentIndex >= numberOfBuckets )
               return Iterator( );
            currentBucket = buckets[ currentIndex ] ;
            }
         return Iterator( this, currentIndex, currentBucket );
         }
      
      Iterator cbegin( ) const
         {
         // if ( buckets == nullptr )
         //    return Iterator( );
         Bucket <Key, Value> *currentBucket = *buckets;
         size_t currentIndex = 0;
         //Find the first Bucket that contains the word
         while ( !currentBucket )
            {
            currentIndex ++;
            if ( currentIndex >= numberOfBuckets )
               return Iterator( );
            currentBucket = buckets[ currentIndex ] ;
            }
            Iterator itr = Iterator( );
            itr.hashTable = const_cast< HashTable*>(this);
            itr.bucketIndex = currentIndex;
            itr.currentBucket = currentBucket;
            return itr;
         //return Iterator( this, currentIndex, currentBucket );
         }

      Iterator end( )
         {
         return Iterator( );
         }
      
      Iterator cend( ) const
         {
         return Iterator( );
         }
   };


