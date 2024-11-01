#include <iostream>
#include "../crawler_filter.hpp"
#include <vector>
#include <map>
using std::cout;
using std::endl;

int main()
   {
   Filter f;
   cout << "should be 1(true)" << f.canAddToFrontier ( "https://web.eecs.umich.edu" );
   cout << "should return 0(false)" << f.canAddToFrontier ( "https://web.eecs.umich.edu/etc/" );
   cout << "should return 0(false)" << f.canAddToFrontier ( "https://web.eecs.umich.edu/eecs/etc/calendar" );
   cout << "shoudl return 0(false) " << f.canAddToFrontier ( "https://www.nytimes.com/wirecutter/123.zip" );
   cout << "shoudl return 0(false) " << f.canAddToFrontier ( "https://www.nytimes.com/wirecutter/123.zip" );
   cout << "shoudl return 0(false) " << f.canAddToFrontier ( "https://www.nytimes.com/wirecutter/123?23query=11" );
   cout << "shoudl return 1(true) " << f.canAddToFrontier ( "https://www.nytimes.com/wirecutter/12323query=1" );
   return 0;
   }
