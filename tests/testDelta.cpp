//
//  TestDelta.cpp
//  DeltaConverter
//
//  Created by Zhicheng Huang on 3/31/21.
//

#include <stdio.h>
#include <iostream>
#include "DeltaConverter.h"
using namespace std;



 
int main( )
{
   cout << "**********test IndicatedLength()*********\n";
   ByteStruct* bb = new ByteStruct;
   bb->byte = 0b00000001; // Bit pattern 0000 0001
   cout << "num1 length: " << IndicatedLength(bb) << endl;
   
   ByteStruct* bb2 = new ByteStruct;
   bb2->byte = 0b01100001; // Bit pattern 0110 0001
   cout << "num2 length: " << IndicatedLength(bb2) << endl;
   
   cout << "**********test EncodeByteStruct() and DecodeByteStruct()*********\n";
   ByteStruct bb3[ 10 ];
   size_t offset3 = 0b00000001;
   ByteStruct* next3 = EncodeByteStruct( bb3, offset3 );
   size_t length3 = IndicatedLength( bb3 );
   cout << "offset: " << offset3 << endl;
   cout << "encode: ";
   for ( int i = 0; i < length3; ++i )
   {
      cout << bb3[ i ].b7 << bb3[ i ].b6 << bb3[ i ].b5 << bb3[ i ].b4 << " "
      << bb3[ i ].b3 << bb3[ i ].b2 << bb3[ i ].b1 << bb3[ i ].b0 << " ";
   }
   cout << endl;
   cout << "decode: " << DecodeByteStruct( bb3 );
   cout << "\n\n";
   
   ByteStruct bb4[ 10 ];
   size_t offset4 = 0b00010001;
   ByteStruct* next4 = EncodeByteStruct( bb4, offset4 );
   size_t length4 = IndicatedLength( bb4 );
   cout << "offset: " << offset4 << endl;
   cout << "encode: ";
   for ( int i = 0; i < length4; ++i )
   {
      cout << bb4[ i ].b7 << bb4[ i ].b6 << bb4[ i ].b5 << bb4[ i ].b4 << " "
      << bb4[ i ].b3 << bb4[ i ].b2 << bb4[ i ].b1 << bb4[ i ].b0 << " ";
   }
   cout << endl;
   cout << "decode: " << DecodeByteStruct( bb4 );
   cout << "\n\n";
   
   ByteStruct bb5[ 10 ];
   size_t offset5 = 0b01010001;
   ByteStruct* next5 = EncodeByteStruct( bb5, offset5 );
   size_t length5 = IndicatedLength( bb5 );
   cout << "offset: " << offset5 << endl;
   cout << "encode: ";
   for ( int i = 0; i < length5; ++i )
   {
      cout << bb5[ i ].b7 << bb5[ i ].b6 << bb5[ i ].b5 << bb5[ i ].b4 << " "
      << bb5[ i ].b3 << bb5[ i ].b2 << bb5[ i ].b1 << bb5[ i ].b0 << " ";
   }
   cout << endl;
   cout << "decode: " << DecodeByteStruct( bb5 );
   cout << "\n\n";
   
   ByteStruct bb6[ 10 ];
   size_t offset6 = 0x1FFFFFFFFFFFFFFF;
   ByteStruct* next6 = EncodeByteStruct( bb6, offset6 );
   size_t length6 = IndicatedLength( bb6 );
   cout << "offset: " << offset6 << endl;
   cout << "encode: ";
   for ( int i = 0; i < length6; ++i )
   {
      cout << bb6[ i ].b7 << bb6[ i ].b6 << bb6[ i ].b5 << bb6[ i ].b4 << " "
      << bb6[ i ].b3 << bb6[ i ].b2 << bb6[ i ].b1 << bb6[ i ].b0 << " ";
   }
   cout << endl;
   cout << "decode: " << DecodeByteStruct( bb6 );
   cout << "\n\n";
   
   ByteStruct bb7[ 2 ];
   ByteStruct* next7 = EncodeByteStruct( bb7, 1 );
   ByteStruct* next77 = EncodeByteStruct( next7, 0 ); // sentinel
   cout << "test sentinel: \n";
   cout << DecodeByteStruct(next7 );
   cout << endl;
   
   
}
/*
int main() {
  // size_t offset1 = 0xFFFFFFFFFFFFFF;
  // Delta d1 ( offset1 );
  // cout << d1.Value;
   
   cout << "****************************" << endl;
   size_t offset7 = 175; // 0b1010 1111
   cout << "input offset: " << offset7 << endl;
   DeltaConverter dc1 (offset7);
   cout << "size in bits: " << dc1.deltaSize << endl << "delta:    ";
   for (int i = 0; i < dc1.deltaSize; ++i) {
      cout << dc1.Delta[i];
   }
   cout << "\nexpected: " << "1100000010101111" << endl;
   cout << strlen(dc1.Delta);
   for (int i = 0; i < strlen(dc1.Delta); ++i) {
      cout << dc1.Delta[i];
   }
   
}
 */
