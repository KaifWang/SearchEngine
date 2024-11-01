#include <iostream>
#include "../configs/crawler_cfg.hpp"
#include "../network_utility.hpp"

int main ( int argc, char** argv )
   {
   if ( argc != 2 )
      {
      printf ( "Useage ./a.out url\n" );
      exit ( 1 );
      }

   string content;
   string redirected_url;
   try
      {
      switch ( download_webpage_ssl ( argv[1], content ) )
         {
         case DOWNLOAD_WEBPAGE_FAIL:
            printf ( "Download Error url `%s`\n", argv[1]  );
            fflush ( stdout );
            break;

         case DOWNLOAD_WEBPAGE_URL_REDIRECT:
            printf ( "Redirection happen\n" );
            redirected_url = handle_redirect_ssl ( argv[1] );
            if ( redirected_url.empty() )
               printf ( "redirection failed\n" );
            else
               printf ( "redirect %s to %s\n", argv[1], redirected_url.c_str() );
            break;

         case DOWNLOAD_WEBPAGE_SUCCESS:
            printf ( "Download Success, Content is \n%s", content.c_str() );
            break;
         }
      }
   catch ( ... )
      {
      printf ( "encounter error, use catch to avoid end \n%s", content.c_str() );
      }
   printf ( "@ continue execution\n" );
   }