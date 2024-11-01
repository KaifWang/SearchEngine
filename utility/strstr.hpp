#pragma once

#include <cassert>

//namespace umse
{
size_t strLen ( const char* str )
   {
   const char* str_ptr = str;
   while ( *str_ptr )
      str_ptr++;
   return ( size_t ) ( str_ptr - str );
   }

const char* strStr ( const char* str, const char* sub )
   {
   assert ( str != nullptr );
   assert ( sub != nullptr );

   const char* str_ptr = str;
   const char* sub_ptr = sub;

   while ( *str_ptr )
      {
      if ( *str_ptr == *sub_ptr )
         {
         while ( *str_ptr == *sub_ptr && *sub_ptr )
            {
            str_ptr++;
            sub_ptr++;
            }

         if ( !*sub_ptr )
            return str_ptr;
         sub_ptr = sub;
         }
      else
         str_ptr++;
      }
   return nullptr;
   }
}
