#pragma once

#include <cstddef> // for size_t
#include <iostream> //for ostream

// Seach Us Team String
// Our own implmentation of STL string
class string
   {
   public:
      // Default Constructor
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Creates an empty string
      string( )
         {
         len = 0;
         data = new char[ 1 ];
         data[ 0 ] = '\0';
         }

      // string Literal / C string Constructor
      // REQUIRES: cstr is a null terminated C style string
      // MODIFIES: *this
      // EFFECTS: Creates a string with equivalent contents to cstr
      string (const char* cstr )
         {
         len = 0;
         const char* cstr_ptr = cstr;
         while( *cstr_ptr++ )
            len ++;
         data = new char[len + 1];
         for ( size_t i = 0 ; i < len; i ++ )
            data[ i ] = cstr[ i ];
         data[ len ] = '\0';
         }
       
      // construct string with first {length} of chars from cstr
      // if encounter a \0 before {length}, stop there
      string( const char* cstr, size_t length )
         {
         if ( !length )
            string( );
         len = 0;
         data = new char[ length + 1 ];
         for ( size_t i = 0; i < length; ++i )
            {
            if ( !cstr[ i ] )
               {
               data[ i ] = '\0';
               len = i;
               return;
               }
            data[ i ] = cstr[ i ];
            ++len;
            }
         data[ length + 1 ] = '\0';
         }

      // Copy the input string from a location of input length
      string ( string str, size_t start, size_t length )
         {
            if ( start >= str.length( ) )
               {
                  string();
                  return;
               }
            size_t acutal_length = length;
            if(length > str.length() - start )
               acutal_length = str.length() - start;
            data = new char[ acutal_length + 1 ];
            for( size_t i = 0; i < acutal_length; i++ )
               data[ 0 ] = str[ start + i ]; 
            data[ acutal_length ] = '\0';
         }
      
      // Destructor
      ~string ()
         {
         delete[] data;
         data = nullptr;
         }

      // Size
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns the number of characters in the string
      size_t length ( ) const
            {
            return len;
            }

      // C string Conversion
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a pointer to a null terminated C string of *this
      const char* c_str ( ) const
            {
            return data;
            }

      // Iterator Begin
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to the start of the string
      const char* begin ( ) const
            {
            return data;
            }

      // Iterator End
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns a random access iterator to the end of the string
      const char* end ( ) const
            {
            return data + len;
            }

      // Element Access
      // REQUIRES: 0 <= i < size()
      // MODIFIES: Allows modification of the i'th element
      // EFFECTS: Returns the i'th character of the string
      const char& operator [ ] ( size_t i ) const
            {
            return data[ i ];
            }

      // string Append
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Appends the contents of other to *this, resizing any
      //      memory at most once
      void operator+= ( const string& other )
         {
         size_t new_length = len + other.len;
         char* new_data = new char[ new_length + 1 ];
         for( size_t i = 0; i < len; i ++ )
            new_data[ i ] = data[ i ];
         for( size_t i = 0; i < other.len; i ++ )
            new_data[ i + len ] = other.data[ i ];
         new_data[ new_length ] = '\0';
         delete[ ] data;
         data = new_data;
         len = new_length;
         }
      
      // string Append
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Appends the contents of other to *this, resizing any
      //      memory at most once
      void append( const string& other )
         {
         *this += other;
         }
      
      // Push Back
      // REQUIRES: Nothing
      // MODIFIES: *this
      // EFFECTS: Appends c to the string
      void pushBack ( char c )
         {
         char* new_data = new char[ len + 2 ];
         for ( size_t i = 0; i < len; i ++ )
            new_data[ i ] = data[ i ];
         new_data[ len ] = c;
         new_data[ len + 1 ] = '\0';
         delete[ ] data;
         data = new_data;
         len++;
         }

      // Pop Back
      // REQUIRES: string is not empty
      // MODIFIES: *this
      // EFFECTS: Removes the last charater of the string
      void popBack ( )
         {
         len --;
         char* new_data = new char[ len + 1 ];
         for ( size_t i = 0; i < len ; i ++ )
            new_data[ i ] = data[ i ];
         new_data[ len ] = '\0';
         delete[ ] data;
         data = new_data;
         }


      // Equality Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether all the contents of *this
      //    and other are equal
      
      bool operator== ( const string& other ) const
         {
         if (len != other.len) { return false; }
         for( size_t i = 0; i < len; i ++ )
            {
            if( *( data + i ) != *( other.data + i ) )
               return false;
            }
         return true;
         }

      // Not-Equality Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether at least one character differs between
      //    *this and other
      bool operator!= ( const string& other ) const
         {
         return ! ( *this == other );
         }

      // Less Than Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less than other
      bool operator< ( const string& other ) const
         {
         char* this_ptr = data;
         char* other_ptr = other.data;
         while( *this_ptr == *other_ptr )
         {
         if( !( *this_ptr++ ) )
            return * ( other_ptr + 1 );
         if( !( *other_ptr++ )  )
            return false;
         }
         return *this_ptr < *other_ptr;
         }

      // Greater Than Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically greater than other
      bool operator> ( const string& other ) const
         {
         return !( *this == other ) && !( *this < other );
         }

      // Less Than Or Equal Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less or equal to other
      bool operator<= ( const string& other ) const
         {
         return ! ( *this > other );
         }

      // Greater Than Or Equal Operator
      // REQUIRES: Nothing
      // MODIFIES: Nothing
      // EFFECTS: Returns whether *this is lexigraphically less or equal to other
      bool operator>= ( const string& other ) const
         {
         return ! ( *this < other );
         }
       
       //find the index of given substring
       //string is a single char
       size_t find( const string str, size_t start = 0 )
            {
                for( size_t i = start; i < len; ++i )
                {
                    if( data[i] == str[0])
                        return i;
                }
                return npos;
            }
       
       void replace ( size_t pos, size_t spanLen, const string str ){
           size_t insertLength = str.length( );
           len = len - spanLen + insertLength;
           char* new_data = new char[ len + 1 ];
           for ( size_t i = 0; i < pos; ++i )
                new_data[ i ] = data[ i ];
           size_t begin = 0;
           for ( size_t i = pos; i < pos + insertLength; ++i)
           {
                new_data[ i ] = str[ begin ];
                ++begin;
           }
           for ( size_t i = pos + insertLength; i < len; ++i)
           {
                new_data[ i ] = data[ pos ];
                ++pos;
           }
           new_data[ len + 1 ] = '\0';
           delete[] data;
           data = new_data;
       }
       
       static const size_t npos = -1;

   private:
      size_t len;
      char* data;
   };

std::ostream& operator<< ( std::ostream& os, const string& s )
   {
      for( size_t i = 0; i < s.length( ) ; i++ )
         os << *( s.begin( ) + i );
      return os;
   }

// return string representation of num
// MAX = 2147483647
string toString( const int num )
   {
   assert( num <= 2147483647 && num >= -2147483648 );
   char buffer[ 15 ];
   int n = sprintf( buffer, "%d", num );
   return string( buffer, n );
   }
