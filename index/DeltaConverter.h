//
//  DeltaConverter.h
//  DeltaConverter
//
//  Created by Zhicheng Huang on 3/31/21.
//
#pragma once
#ifndef DeltaConverter_h
#define DeltaConverter_h


#endif /* DeltaConverter_h */
#include <iostream>
#include <vector>
#include <assert.h>
using namespace std;



/*
  Posting list should be an array of ByteStructs instead of array of char
  because
      1) if storing the byte (type char) within each ByteStructs into
  an array in encode stage, it can't be reverted to a ByteStruct in decode stage;
      2) to achieve variable length array in C++, we need to initialize it to
         have length zero. If it's a char[0], it appends a '\0' at the end,
         adding an unnecessary byte at the end.
 */
typedef union
   {
   char byte;
   struct
      {
      // Bits start with bit 0 (the low bit).
      uint b0:1;
      uint b1:1;
      uint b2:1;
      uint b3:1;
      uint b4:1;
      uint b5:1;
      uint b6:1;
      uint b7:1;
      };

   void print(ostream &os) const
   {
      os << b7 << b6 << b5 << b4 << b3 << b2 << b1 << b0 << " ";
   }
   } ByteStruct;

/*
Big Idea: use a maximum of 8 bytes to represent a size_t offset
1) The top 3 bits of the first byte is used to indicate the number of following
   bytes (from 0 to 7), and the remaining 5 bits are used to store numbers
2) The next seven (potentially) bytes are used to store the actual value of the offset
 
 Therefore, the max value we can convert is 2^61
 0b - 5b = [0 - 32] => 1 Byte
 6b - 13b = (32 - 8192) => 2 Bytes
 14b - 21b = (8192 - 2097152) => 3 Bytes
 22b - 29b = (2097152 - 536870912) => 4 Bytes
 30b - 37b = (536870912 - 1.37*10^11) => 5 Bytes
 the rest are trivial
*/

const size_t BytesCheckOne = 0x1F; // num <= BytesCheckOne needs 1 Byte to store
const size_t BytesCheckTwo = 0x1FFF;
const size_t BytesCheckThree = 0x1FFFFF;
const size_t BytesCheckFour = 0x1FFFFFFF;
const size_t BytesCheckFive = 0x1FFFFFFFFF;
const size_t BytesCheckSix = 0x1FFFFFFFFFFF;
const size_t BytesCheckSeven = 0x1FFFFFFFFFFFFF;
const size_t BytesCheckEight = 0x1FFFFFFFFFFFFFFF;


// IndicatedLength looks at the first byte of a ByteStruct sequence
// and determines the expected length.
static size_t IndicatedLength( const ByteStruct* bs )
   {
   return 1 + ( ( bs->b7 ) << 2 ) + (( bs->b6 ) << 1 ) + (bs->b5);
   }


// Encode offset into ByteStruct sequence, write it in the buffer at bs
// Then return the pointer points to 1 pass the latest ByteStruct sequence
static ByteStruct* EncodeByteStruct( ByteStruct* bs, size_t offset )
   {
   assert( offset <= BytesCheckEight );
   assert( offset >= 0 ); 
   size_t numFollowBytes = 0;
   if ( offset <= BytesCheckOne )
      {
      numFollowBytes = 0;
      // set first 3 bits of first byte to be 0b000
      bs->b7 = 0;
      bs->b6 = 0;
      bs->b5 = 0;
      }
   else if ( offset <= BytesCheckTwo )
      {
      numFollowBytes = 1;
      // set first 3 bits of first byte
      bs->b7 = 0;
      bs->b6 = 0;
      bs->b5 = 1;
      }
   else if ( offset <= BytesCheckThree )
      {
      numFollowBytes = 2;
      // set first 3 bits of first byte
      bs->b7 = 0;
      bs->b6 = 1;
      bs->b5 = 0;
      }
   else if ( offset <= BytesCheckFour )
      {
      numFollowBytes = 3;
      // set first 3 bits of first byte
      bs->b7 = 0;
      bs->b6 = 1;
      bs->b5 = 1;
      }
   else if ( offset <= BytesCheckFive )
      {
      numFollowBytes = 4;
      // set first 3 bits of first byte
      bs->b7 = 1;
      bs->b6 = 0;
      bs->b5 = 0;
      }
   else if ( offset <= BytesCheckSix )
      {
      numFollowBytes = 5;
      // set first 3 bits of first byte
      bs->b7 = 1;
      bs->b6 = 0;
      bs->b5 = 1;
      }
   else if ( offset <= BytesCheckSeven )
      {
      numFollowBytes = 6;
      // set first 3 bits of first byte
      bs->b7 = 1;
      bs->b6 = 1;
      bs->b5 = 0;
      }
   else
      {
      // offset <= BytesCheckEight
      numFollowBytes = 7;
      // set first 3 bits of first byte
      bs->b7 = 1;
      bs->b6 = 1;
      bs->b5 = 1;
      }

   // set the other 5 bits of first byte to the highest 5 bits of offset
   size_t masker = 1;
   size_t temp = offset >> ( ( numFollowBytes ) * 8 );
   bs->b0 = temp & masker;
   bs->b1 = ( temp >> 1 ) & masker;
   bs->b2 = ( temp >> 2 ) & masker;
   bs->b3 = ( temp >> 3 ) & masker;
   bs->b4 = ( temp >> 4 ) & masker;
   bs++;
   
   // set the other following bytes
   for ( int i = 0; i < numFollowBytes; i++ )
      {
      temp = offset >> ( ( numFollowBytes - 1 - i) * 8 );
      bs->b0 = temp & masker;
      bs->b1 = ( temp >> 1 ) & masker;
      bs->b2 = ( temp >> 2 ) & masker;
      bs->b3 = ( temp >> 3 ) & masker;
      bs->b4 = ( temp >> 4 ) & masker;
      bs->b5 = ( temp >> 5 ) & masker;
      bs->b6 = ( temp >> 6 ) & masker;
      bs->b7 = ( temp >> 7 ) & masker;
      bs++;
      }
   
   return bs;
   }



// Take in a ByteStruct*
// Return the size_t representing the original offset before encoding
// Return 0 if reaches the end of the posting list signaled by 0x00
static size_t DecodeByteStruct( const ByteStruct *&bs )
   {
   if ( !bs->byte ) 
   {
      bs++;
      return 0;
   }
   size_t numFollowBytes = IndicatedLength( bs ) - 1;
   size_t result = 0;
   // 5 low bits of first byte
   result = bs->b0 + ( bs->b1 << 1 ) + ( bs->b2 << 2 ) + ( bs->b3 << 3 ) + ( bs->b4 << 4 );
   bs++;
   // other follow bytes
   for ( int i = 0; i < numFollowBytes; ++i )
      {
      result = result << 8;
      result += bs->b0 + ( bs->b1 << 1 ) + ( bs->b2 << 2 ) + ( bs->b3 << 3 ) + ( bs->b4 << 4 )
               + ( bs->b5 << 5 ) + ( bs->b6 << 6 ) + ( bs->b7 << 7 );
      bs++;
      }
   return result;
   }



// IndicatedLength looks at the first byte of a ByteStruct sequence
// and determines the expected length.
static size_t IndicatedLength( const size_t offset )
   {
   ByteStruct temp[9];
   ByteStruct* head = temp;
   EncodeByteStruct( temp, offset );
   return IndicatedLength( head );
   }