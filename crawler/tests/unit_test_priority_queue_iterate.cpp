//
// unit_test_priority_queue_iterate.cpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//

#include <string>
#include <vector>
#include <queue>
#include <iostream>
using std::priority_queue;
using std::string;
using std::vector;
using std::endl;
using std::cout;

struct UrlPack
   {
   int Score;
   string Url;

   UrlPack ( int score, const string& url ) : Score ( score ), Url ( url ) {}
   UrlPack ( const string& url, int score ) : Score ( score ), Url ( url ) {}
   UrlPack ( int score, const char* url ) : Score ( score ), Url ( url ) {}
   UrlPack ( const char* url, int score ) : Score ( score ), Url ( url ) {}
   UrlPack ( const UrlPack& other ) : Score ( other.Score ), Url ( other.Url ) {}
   };

class UrlPackCmp
   {
   public:
      bool operator() ( const UrlPack& lhs, const UrlPack& rhs )
         {
         return lhs.Score < rhs.Score;
         }
   };

template <class T, class S, class C>
S& Container ( priority_queue<T, S, C>& q )
   {
   struct HackedQueue : private priority_queue<T, S, C>
      {
      static S& Container ( priority_queue<T, S, C>& q )
         {
         return q.*& HackedQueue::c;
         }
      };
   return HackedQueue::Container ( q );
   };

int main()
   {
   priority_queue<UrlPack, vector<UrlPack>, UrlPackCmp> urls;
   vector<UrlPack>& urls_iterable = Container ( urls );

   urls.emplace ( "msg1", 10 );
   urls.emplace ( "msg2", 20 );
   urls.emplace ( "msg3", 30 );
   urls.emplace ( "msg4", 40 );
   urls.emplace ( "msg5", 50 );

   cout << "before pop & add " << endl;
   for ( auto iter = urls_iterable.begin(); iter != urls_iterable.end(); ++iter )
      cout << "#" << iter->Url << endl;

   urls.pop ( );
   urls.emplace ( "aaa", 60 );
   urls.emplace ( "bbb", 70 );

   cout << "after pop & add " << endl;
   for ( auto iter = urls_iterable.begin(); iter != urls_iterable.end(); ++iter )
      cout << "#" << iter->Url << endl;

   }