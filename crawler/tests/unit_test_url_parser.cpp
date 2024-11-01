#include <iostream>
#include "../url_parser.hpp"

//using std::cout; using std::endl;

using std::cout;
using std::endl;

int main ( int argc, char** argv )
   {
   if ( argc  != 2 )
      {
      cout << "Useage ./a.out https://www.baidu.com" << endl;
      exit ( 1 );
      }

   ParsedUrl parsedUrl ( argv[1] );
   cout << "Server " << parsedUrl.Service << endl;
   cout << "Host " << parsedUrl.Host << endl;
   cout << "Port " << parsedUrl.Port << endl;
   cout << "Path " << parsedUrl.Path << endl;

   }
