//
//  front_end
//
//  Created by Leo Yan
//

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include "PostingList.h"
#include <algorithm>
using namespace std;



vector<DocumentData> getDD()
{
   vector<DocumentData> DD;
   DD.resize(50);
   for(size_t i = 0; i < 50; ++i){
       DD[i].numTitleWords = 2;
       DD[i].URL = "umich.edu";
       string title = "title " + to_string(i);
       string abstract = "abstract " + to_string(i);
       strcpy(DD[i].title, title.c_str());
       strcpy(DD[i].abstract, abstract.c_str());
   }

   return DD;
}
