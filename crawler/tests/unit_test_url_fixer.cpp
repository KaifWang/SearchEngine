#include "../url_fixer.hpp"
#include "../url_parser.hpp"

// ./unit_test_url_fixer.o https://scripts.mit.edu/web ../start/
// ./unit_test_url_fixer.o https://scripts.mit.edu/web ../start
// ./unit_test_url_fixer.o https://scripts.mit.edu/web ../
// ./unit_test_url_fixer.o https://man7.org/linux/man-pages/man2/read.2.html  ../index.html
// ./unit_test_url_fixer.o https://man7.org/linux/man-pages/man2/read.2.html ../../../style.css

int main ( int argc, char** argv )
   {
   if ( argc != 3 )
      {
      printf ( "Useage: ./a.out base_url fixed_url\n" );
      exit ( -1 );
      }

    printf("@ fix `%s` on base `%s`\n", argv[2], argv[1]);

   ParsedUrl parsedUrl ( argv[1] );
   string base_url ( argv[1] );
   string fixed_url ( argv[2] );

   fix_url_to_correct_format ( fixed_url, parsedUrl, fixed_url );

   printf ( "@ fixed url %s\n", fixed_url.c_str() );
   printf ( "@ unit test finish\n" );

   return 1;
   }