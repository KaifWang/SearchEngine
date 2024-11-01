#include "DeltaConverter.h"

int main()
{
    size_t offset = 28709;
    cout << "numBytes: " << IndicatedLength(offset) << endl;
    ByteStruct temp[ 8 ];
    const ByteStruct* head = temp;
    EncodeByteStruct( temp, offset );
    size_t delta = DecodeByteStruct(head);
    cout << "delta: " << delta;
}