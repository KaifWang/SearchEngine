#include <string>

using namespace std;

int main()
   {
   string content{"https://helloworld https://thisissecond https://thisisthird "};

   int start_idx = 0;
   int end_idx = 0;
   for ( ; end_idx < content.size(); end_idx++ )
      {
      if ( content.at ( end_idx ) == ' ' )
         {
         string url ( content.c_str() + start_idx, end_idx - start_idx );
         printf ( "url `%s`\n", url.c_str() );
         start_idx = end_idx + 1;
         end_idx++;
         }
      }
   }