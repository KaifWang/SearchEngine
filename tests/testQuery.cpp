// queryDriver.cpp
// Driver for the query parser program

// Patrick Su patsu@umich.edu

#include <iostream>

#include "../queryParser/queryParser.h"

 // The constructor for any plugin should set Plugin = this so that
 // LinuxTinyServer knows it exists and can call it.

static vector<DocumentScore> searchIndex(string query)
{
   char queryWords[MAX_QUERY_SIZE];
   memset(queryWords, 0, sizeof(queryWords));
   strncpy(queryWords, query.c_str(), query.length());
   vector< vector < DocumentScore > > allDocsScore;
   vector< vector < DebugScore > > allDebugInfo;
   string dirName = "../index/" + string(FINAL_INDEX);
   DIR* handle = opendir( dirName.c_str() );
   if ( handle )
   {
      struct dirent * entry;
      while ( ( entry = readdir( handle ) ) )
      {
         struct stat statbuf;
         string fileName = dirName + '/' + entry->d_name;
         if ( stat( fileName.c_str( ), &statbuf ) )
         {
            cerr << "stat of " << dirName << " failed, errono = " << errno << endl;
         }
         if( IndexFile ( fileName.c_str( ) ) )
         {
            HashFile indexChunk(fileName.c_str());
            const HashBlob *hashblob = indexChunk.Blob();
            QueryParser parser(queryWords, &indexChunk);
            vector<string> flatternWords = parser.GetFlattenedVector();
            ISROr* parsedISRQuery = parser.getParsedISRQuery();
			   Ranker ranker(parsedISRQuery, flatternWords, &indexChunk);
            ranker.RankDocuments();
            allDocsScore.push_back(ranker.getTopDocuments());
            allDebugInfo.push_back(ranker.getDebugInfo());
         }
      }
   }
   vector<DocumentScore> combinedTopDocs(5);
   vector<DebugScore> combinedInfo(5);
   for( size_t i = 0; i < allDocsScore.size(); i ++ )
   {
      for( size_t j = 0; j < allDocsScore[i].size() ; j++ )
      {
         InsertionSort(allDocsScore[i][j], combinedTopDocs);
         InsertionSort(allDebugInfo[i][j], combinedInfo);
      }
   }
   for (size_t i = 0; i < combinedInfo.size(); i ++ )
    {
       combinedInfo[i].print();
    }
   return combinedTopDocs;
}

int main( )
{
    vector<DocumentScore> result = searchIndex("university of michigan");
    for (size_t i = 0; i < result.size(); i ++ )
    {
        cout << result[i].URL << endl;  
        cout << result[i].title << endl;
        cout << result[i].score << endl;
    }
}
