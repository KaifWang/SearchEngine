// Search Engine Ranker
//
// Created by Kai Wang (kaifan@umich.edu)
#pragma once

//Debug output
#ifdef DEBUG
 #define dbgprintf(fmt, ...)  printf("DEBUG: " fmt, ##__VA_ARGS__)
#else
 #define dbgprintf(fmt, ...)
#endif

#include <iostream>
#include <dirent.h>
#include <string>
#include <chrono>
#include "../configs/config.h"
#include "../constraintSolver/isr.h"

using namespace std;
using namespace std::chrono;

// ----------Define Constraints-------------

#define SHORT_TITLE_LIMIT 6
#define SHORT_URL_LIMIT 40

// 1. Short Span if length < numWords * MULTIPLIER
#define SHORT_SPAN_MULTIPLIER 3
// 2. Max number of unmatched locations between NearDouble/NearTriple
#define MAX_SKIPPED_LOCATION 3 
// 3. A word is frequent in a document if 
// Occurance > Expected occurance（numPosts/numDocuments) * TIMES_EXPECTED_TO_BE_FREQUENT 
#define TIMES_EXPECTED_TO_BE_FREQUENT 2
//#define TIMES_EXPECTED_TO_BE_TOO_FREQUENT 10
// Define the boundry of near top
#define NEAR_TOP_BOUNDRY 100
// Define Rare words: estimated frequency (total words/this words) * MULTIPLIER
// Define Common words: estimated frequency (total words/this words) * MULTIPLIER
#define RARE_MULTIPLIER 100000
#define COMMON_MULTIPLIER 200



// --------------Define Ranking Metrics and Weights------------

// Weights between static and dynamic
#define STATIC_WEIGHT 10
#define DYNAMIC_WEIGHT 10
#define DYNAMIC_WEIGHT_WITH_SOME_RARE_WORDS_SEARCH 12
#define DYNAMIC_WEIGHT_WITH_MOST_RARE_WORDS_SEARCH 15
#define DYNAMIC_WEIGHT_WITH_SOME_COMMON_WORDS_SEARCH 8
#define DYNAMIC_WEIGHT_WITH_MOST_COMMON_WORDS_SEARCH 5

// Static Rank Weight
#define SHORT_TITLE_WEIGHT 30
#define SHORT_URL_WEIGHT 30
#define EDU_WEIGHT 20
#define GOV_WEIGHT 20
#define COM_WEIGHT 10

// Dynamic Rank Weight
#define SHORT_SPAN_WEIGHT 2 //* count
#define ORDER_SPAN_WEIGHT 2 //* count
#define EXACT_PHRASE_WEIGHT 7//* count
#define NEAR_THE_TOP_WEIGHT 3 //* count
#define NEAR_DOUBLE_WEIGHT 2 //* count
#define NEAR_TRIPLE_WEIGHT 5 //* count
#define ALL_WORDS_FREQUENT_WEIGHT 15
#define MOST_WORDS_FREQUENT_WEIGHT 10
#define SOME_WORDS_FREQUENT_WEIGHT 5
// Define MOST and SOME
#define MIN_PERCENT_TO_BE_MOST 0.6
#define MIN_PERCENT_TO_BE_SOME 0.3


// Title Weight
#define TITLE_CONTAIN_FEW_WEIGHT 10  // 10% of title words are query
#define TITLE_CONTAIN_SOME_WEIGHT 35 // 25% of title words are query
#define TITLE_CONTAIN_MANY_WEIGHT 70 // 50% of title words are query
#define TITLE_CONTAIN_MOST_WEIGHT 90 // 75% of title words are query
#define TITLE_CONTAIN_ALL_WEIGHT 250
#define MIN_PERCENT_TITLE_TO_BE_FEW 0.05
#define MIN_PERCENT_TITLE_TO_BE_SOME 0.24
#define MIN_PERCENT_TITLE_TO_BE_MANY 0.49
#define MIN_PERCENT_TITLE_TO_BE_MOST 0.74
#define MIN_PERCENT_TITLE_TO_BE_ALL 1

// Dynamic URL WEIGHT
#define URL_CONTAIN_QUERY_WORD_WEIGHT 70
#define URL_CONTAIN_QUERY_WORD_AND_HOME_PAGE_WEIGHT 250

// Score and data for each document
struct DocumentScore
   {
   string title;
   string abstract;
   string URL;
   size_t score;
   };

// InsertionSort Score one document into the topN list
static void InsertionSort( DocumentScore data, vector< DocumentScore > &topN )
   {
      //Return if value is smaller than the smallest value in the array
      if ( topN[ topN.size( ) - 1 ].score > data.score )
         return;

      // Return if url already exists
      for( size_t i = 0; i < topN.size( ); i ++)
         {
         // Remove duplicate urls
         if ( topN[ i ].URL == data.URL ) 
            { 
            if( data.URL != "" && !data.URL.empty( ) )
               dbgprintf( "Exact same url found: %s\n", data.URL.c_str( ) );
            return; 
            }
         // Remove similar websites
         if(topN[ i ].title == data.title && topN[ i ].score == data.score )
            {
            dbgprintf( "Similar website found: %s and %s.\n", 
                  data.URL.c_str( ), topN[ i ].URL.c_str( ) );
            return; 
            }
         }  

      // Sort while inserting.
      // top10 initialize with 0s
      // [ 3 0 0 0 0 ] insert 3
      // [ 3 2 0 0 0 ] insert 2
      // [ 5 3 2 0 0 ] insert 5
      // [ 5 4 3 2 0 ] insert 4
      // [ 5 4 3 2 1 ] insert 1
      // Iterate through the array to find the correct position to insert
      for ( size_t i = 0; i < topN.size( ) ; i ++ )
         {
         // If reach the end of array, put the element at the end
         if ( !topN[ i ].score )
            {
            topN[ i ] = data;
            break;
            }
         // Insert at the right position,
         // while pushing subsequent elements to a later position
         if ( data.score > topN[ i ].score )
            {
            for ( size_t j = topN.size( ) - 1 ; j > i; j -- )
               {
               // If exist
               if( topN[ j - 1 ].score )
                  topN[ j ] = topN[ j - 1 ];
               }
            topN[ i ] = data;
            break;
            }
         }
   }

// For the convinience of print out scoring details for each url
// Only used in DEBUG mode
struct DebugScore
   {
   DocumentScore doc;
   size_t totalScore;
   size_t staticScore;
   size_t dynamicScore;
   double dynmicNormFactor;
   bool isEDU = false;
   bool isGov = false;
   bool isCom = false;
   bool isShortURL = false;
   bool isShortTitle = false;
   size_t urlScore;
   size_t titleWordPercentage;
   size_t frequentWordPercentage;
   size_t numShortSpan;
   size_t numOrderSpan;
   size_t numNearTopSpan;
   size_t numExactPhrase;
   size_t numNearTriple;
   size_t numNearDouble;

   void print( ) const
      {
      dbgprintf( "-------------------------------------\n" );
      dbgprintf( "URL: %s\n", doc.URL.c_str( ) );
      dbgprintf( "---> Static score: %lu\n", staticScore );
      dbgprintf( "---> Dynamic score: %lu\n", dynamicScore );
      dbgprintf( "Dynamic Norm factor: %f\n\n", dynmicNormFactor );

      dbgprintf( "---> Total score: %lu\n\n", totalScore );

      if( isEDU )
         dbgprintf( "EDU Domain: +%i\n", EDU_WEIGHT );
      else if( isGov )
         dbgprintf( "GOV Domain: +%i\n", GOV_WEIGHT );
      else if( isCom )
         dbgprintf( "COM Domain: +%i\n", COM_WEIGHT );
      if( isShortURL )
         dbgprintf( "URL Length less than %i: +%i\n", SHORT_URL_LIMIT, SHORT_URL_WEIGHT );
      if( isShortTitle )
         dbgprintf( "Title length less than %i: +%i\n", SHORT_TITLE_LIMIT, SHORT_TITLE_WEIGHT );
      dbgprintf( "Url Score: %lu\n", urlScore );
      dbgprintf( "Title words percentage: %lu percent\n", titleWordPercentage );
      dbgprintf(" Frequent words percentage: %lu percent\n", frequentWordPercentage );
      dbgprintf(" shortSpan: %lu\n", numShortSpan );
      dbgprintf( "orderSpan: %lu\n", numOrderSpan );
      dbgprintf( "nearTopSpan: %lu\n", numNearTopSpan );
      dbgprintf( "exactPhrase: %lu\n", numExactPhrase );
      dbgprintf( "nearTriple: %lu\n", numNearTriple );
      dbgprintf( "nearDouble: %lu\n", numNearDouble );
      dbgprintf( "-------------------------------------\n" );
      }
   };

//
#ifdef DEBUG
// InsertionSort Score into the topN list
static void InsertionSort( DebugScore data, vector< DebugScore > &topN )
   {
      //Return if value is smaller than the smallest value in the array
      if ( topN[ topN.size( ) - 1 ].doc.score > data.doc.score )
         return;

      // Return if url already exists
      for( size_t i = 0; i < topN.size( ); i ++ )
         {
         // Remove duplicate urls
         if ( topN[ i ].doc.URL == data.doc.URL ) 
            return; 
         // Remove similar websites
         if(topN[ i ].doc.title == data.doc.title && 
               topN[ i ].doc.score == data.doc.score)
            return; 
         }  

      // Sort while inserting.
      // top10 initialize with 0s
      // [ 3 0 0 0 0 ] insert 3
      // [ 3 2 0 0 0 ] insert 2
      // [ 5 3 2 0 0 ] insert 5
      // [ 5 4 3 2 0 ] insert 4
      // [ 5 4 3 2 1 ] insert 1
      // Iterate through the array to find the correct position to insert
      for ( size_t i = 0; i < topN.size( ) ; i ++ )
         {
         // If reach the end of array, put the element at the end
         if ( !topN[ i ].doc.score )
            {
            topN[ i ] = data;
            break;
            }
         // Insert at the right position,
         // while pushing subsequent elements to a later position
         if ( data.doc.score > topN[ i ].doc.score )
            {
            for ( size_t j = topN.size( ) - 1 ; j > i; j --)
               {
               // If exist
               if( topN[ j - 1 ].doc.score )
                  topN[ j ] = topN[ j - 1 ];
               }
            topN[ i ] = data;
            break;
            }
         }
   }
#endif

// Takes an parsed Query ISR pointer and a list of search words from the queryParser.
// Searches through the indexChunk and returns the topNDocuments

class Ranker
   {
   private:
      ISROr* parsedQuery;
      vector< string > searchWords;
      HashFile* indexChunk;
      vector< DocumentScore > topDocuments;
      vector< DebugScore > debugInfo;
      size_t dynamicRankWeight;

      // Simplify URL
      string SimplifyUrl( string longUrl )
         {
         // 1.https://www.umich.edu -> www.umich.edu
         string result = string( longUrl, 8, string::npos );

         // 2. www.umich.edu -> umich.edu
         if( string( result, 0, 4 ) == "www." )
            result = string( result, 4, string::npos );
         return result;
         }

      // Retrieve the domain of an url
      string GetDomainSuffix( string url )
         {
         // Domain is right before the first '/'
         // or at the end if no '/' exists
         // ex. umich.edu/activity
         // ex. umich.edu
         for( size_t i = 0; i < url.length( ); i ++ )
            {
            if( url[ i ] == '/' )
               return string( url, i - 4, 4 );
            }
         if( url.length( ) >= 4 )
            return string( url, url.length( ) - 4 , string::npos );
         return "";
         }


      // Extract domain name between frist . and second .
      // ex. umich.edu
      // ex. lib.umich.edu
      string getDomainName( string url )
         {
         size_t firstDot = 0;
         size_t i = 0;
         //locate the first dot position
         while( i < url.length( ) && url[ i ] != '.' )
            i++;
         firstDot = i;
         i++;
         // locate the second dot position
         // if exists, retrieve string between 2 dots
         // if not, retrieve string before the first dot (url start at i=9)
         while( i < url.length( ) && url[ i ] != '.' )
            i++;
         if( i != url.length( ) )
            {
            size_t secondDot = i;
            return string( url, firstDot + 1, secondDot - firstDot - 1 );
            }
         else
            return string( url, 0, firstDot );
         }

      // Check if the url is likely a home page
      // umich.edu -> yes
      // lib.umich.edu -> no
      // lib.umich.edu/activity/... -> no
      bool isHomePage( string url )
         {
         size_t dotCount = 0;
         size_t i = 0 ; 
         for( ; i < url.length( ) && url[ i ] != '/' ; i ++ )
            {
            if ( url[ i ] == '.' )
               dotCount ++;
            }
         if ( dotCount > 1 ) { return false; }
         if( i != url.length( ) ) { return false; }
         return true;
         }


      // Helper function
      // Set the weight of dynamic ranking based on the query
      // The more rare words, the more weight on dynamic ranking
      bool SetDynamicRankWeight( )
         {
         const HashBlob* blob = indexChunk->Blob( );
         double numRare = 0;
         for( size_t i = 0; i < searchWords.size( ); i++ )
            {
            const SerialTuple* tuple = blob->Find( searchWords[ i ].c_str( ) );
            if( !tuple )
               {
               dbgprintf( "The word %s is not in the index\n", searchWords[ i ].c_str( ) );
               //cout << "The word " << searchWords[i] << " is not in the index" << endl;
               return false;
               }
            const PostingList* pl = tuple->GetPostingListValue( );
            // The word is rare if this words occur less than 1/100000 of total words
            if( blob->NumberOfPosts / pl->numberOfPosts > RARE_MULTIPLIER )
               numRare ++ ;
            }
         double rareWordPrecentage = numRare/static_cast<double>(searchWords.size( ) );
         if( rareWordPrecentage > MIN_PERCENT_TO_BE_MOST )
            dynamicRankWeight = DYNAMIC_WEIGHT_WITH_MOST_RARE_WORDS_SEARCH;
         else if( rareWordPrecentage > MIN_PERCENT_TO_BE_SOME )
            dynamicRankWeight = DYNAMIC_WEIGHT_WITH_SOME_RARE_WORDS_SEARCH;
         dbgprintf( "Rare word percentage: %f\n", rareWordPrecentage );
         dbgprintf( "Set dynmaic weight to be %lu times static\n", dynamicRankWeight );
         dbgprintf( "-----------------------------------------------------\n\n" );
         // cout << "rareWordPercentage: " << rareWordPrecentage << endl;
         // cout << "set dynmaic Weight to be " << dynamicRankWeight << endl;
         // cout << "-------------------------------------" << endl;
         return true;
         }

      // Helper 
      // Calculate the static rank given a document
      size_t StaticRank( DocumentData &docDota, DebugScore &infoScore )
         {
         size_t score = 0;
         string domainName = getDomainName( docDota.URL );
         if( !domainName.compare( ".edu" ) )
            {
            infoScore.isEDU = true;
            score += EDU_WEIGHT;
            }
         else if( !domainName.compare( ".gov" ) )
            {
            infoScore.isGov = true;
            score += GOV_WEIGHT;
            }
         else if( !domainName.compare( ".com" ) )
            {
            infoScore.isCom = true;
            score += COM_WEIGHT;
            }

         if( docDota.URL.length( ) < SHORT_URL_LIMIT )
            {
            infoScore.isShortURL = true;
            score += SHORT_URL_WEIGHT;
            }

         if( docDota.numTitleWords < SHORT_TITLE_LIMIT )
            {
            infoScore.isShortTitle = true;
            score += SHORT_TITLE_WEIGHT;
            }

         // Maybe add doc length? What is a good doc length?
         return score;
         }

      // Helper
      // Calculate the dynamic rank of the document body
      size_t DynamicBodyRank( ISRWord** terms, size_t expectedFrequency[], 
         size_t numTerms, size_t rarestIndex, Location docStart, Location docEnd,
            DebugScore &infoScore )
         {
         // Initialize counts for heuristics
         size_t shortSpanCount = 0;
         size_t orderSpanCount = 0;
         size_t nearTopSpanCount = 0;
         size_t phraseCount = 0;
         size_t nearDoubleCount = 0;
         size_t nearTripleCount = 0;
         size_t numOccurance[numTerms];
         for( size_t i = 0; i < numTerms; i++ )
            numOccurance[ i ] = 0;
         // 1. Start each word at the start of the document
         for( size_t i = 0; i < numTerms; i ++ )
            terms[ i ]->Seek( docStart );

         // 2. Initilize current span to be the first location of each word in this document.
         Location currentSpan[numTerms];
         for( size_t i = 0; i < numTerms; i++ )
            currentSpan[ i ] = terms[ i ]->getStartLocation( );
            
         Location rarestLocation = terms[ rarestIndex ]->getStartLocation( );
         // 2. Anchor at the rarest word and moves all ISRs forward
         while( rarestLocation < docEnd )
            {
            // Move each isr to the closest location to rarest
            for( size_t i = 0; i < numTerms; i ++ )
               {
               if( i != rarestIndex )
                  {
                  Location currentLoc = terms[ i ]->getStartLocation( );
                  // If passed the documentEnd, keep this location the same
                  // currentSpan[i] = currentSpan[i] in the previous wround
                  if( currentLoc > docEnd || !terms[ i ]->getCurrentPost( ) )
                     continue;
                  else
                     {
                     numOccurance[ i ] ++;
                     terms[ i ]->Next( );
                     Location nextLoc = terms[ i ] -> getStartLocation( );
                     if( nextLoc > docEnd || !terms[ i ]->getCurrentPost( ) )
                        continue;
                     numOccurance[ i ] ++;
                     bool isReachingEnd = false;
                     while( nextLoc < rarestLocation )
                        {
                        terms[ i ]->Next( );
                        currentLoc = nextLoc;
                        nextLoc = terms[ i ] -> getStartLocation( );
                        if( nextLoc > docEnd || !terms[ i ]->getCurrentPost( ) )
                           {
                           isReachingEnd = true;
                           break;
                           }
                        numOccurance[ i ] ++ ;
                        }
                     if( isReachingEnd )
                        continue;
                     if( rarestLocation - currentLoc < nextLoc - rarestLocation )
                        currentSpan[ i ] = currentLoc;
                     else 
                        currentSpan[ i ] = nextLoc;
                     }
                  }
               }
            // 3. count hits for each heuristic
            CountSpanHit( currentSpan, numTerms, docStart,
               shortSpanCount, orderSpanCount, nearTopSpanCount, 
               phraseCount, nearTripleCount, nearDoubleCount );
            terms[ rarestIndex ] -> Next( );
            rarestLocation = terms[ rarestIndex ]->getStartLocation( );
            currentSpan[ rarestIndex ] = rarestLocation;
            }

         // Limit order span, short span, and near double to avoid spamming websites
         shortSpanCount = shortSpanCount > 20 ? 20 : shortSpanCount;
         orderSpanCount = orderSpanCount > 20 ? 20 : orderSpanCount;
         nearDoubleCount = nearDoubleCount > 20 ? 20 : nearDoubleCount;
         // 4. Calculate Score
         size_t score = 0;

         // 4.a. Find percentage of frequent words in this document
         size_t numFrequentTerms = 0;
         for( size_t i = 0 ; i < numTerms; i++ )
            {
            if( numOccurance[ i ] > expectedFrequency[ i ] * TIMES_EXPECTED_TO_BE_FREQUENT )
               numFrequentTerms ++;
            }
         double frequentWordPercentage = static_cast< double >( numFrequentTerms ) 
               / static_cast< double >( numTerms );
         if( numFrequentTerms == numTerms )
            score += ALL_WORDS_FREQUENT_WEIGHT;
         else if( frequentWordPercentage > MIN_PERCENT_TO_BE_MOST )
               score += MOST_WORDS_FREQUENT_WEIGHT;
         else if( frequentWordPercentage > MIN_PERCENT_TO_BE_SOME )
               score += SOME_WORDS_FREQUENT_WEIGHT;

         // 4.b Linear Combination of heuristics
         score += shortSpanCount * SHORT_SPAN_WEIGHT + orderSpanCount * ORDER_SPAN_WEIGHT
            + nearTopSpanCount * NEAR_THE_TOP_WEIGHT + phraseCount * EXACT_PHRASE_WEIGHT
            + nearTripleCount * NEAR_TRIPLE_WEIGHT + nearDoubleCount * NEAR_DOUBLE_WEIGHT;
         
         // If it is 1 word search, there will be a lot of short spans 
         // Need to normalize score
         if ( numTerms == 1 ) { score /= 3; }

         // Debugging info for tuning the heruistic
         infoScore.frequentWordPercentage = frequentWordPercentage * 100;
         infoScore.numShortSpan = shortSpanCount;
         infoScore.numOrderSpan = orderSpanCount;
         infoScore.numExactPhrase = phraseCount;
         infoScore.numNearTopSpan = nearTopSpanCount;
         infoScore.numNearTriple = nearTripleCount;
         infoScore.numNearDouble = nearDoubleCount;
         return score;
         }

      // Helper function
      // Count heuristic hits for each span
      void CountSpanHit( const Location span[ ], size_t numTerms, size_t docStart,
            size_t &shortSpanCount, size_t &orderSpanCount, 
            size_t &nearTopSpanCount, size_t &phraseCount, 
            size_t &nearTripleCount, size_t &nearDoubleCount )
         {
         // 1. Check if it is a short span
         Location minLoc = span[ 0 ];
         Location maxLoc = span[ 0 ];
         for( size_t i = 0 ; i < numTerms; i ++ )
            {
            if( span[ i ] < minLoc )
               minLoc = span[ i ];
            if( span[ i ] > maxLoc )
               maxLoc = span[ i ];
            }
         if( maxLoc - minLoc < numTerms * SHORT_SPAN_MULTIPLIER )
            shortSpanCount ++;
      
         // 2. Check if the span is near the top
         if( maxLoc < docStart + NEAR_TOP_BOUNDRY )
            nearTopSpanCount ++;

         // Check the rest if it is not a single word query
         if ( numTerms > 1 )
            {
            // 3. Check if it is a order span
            bool isOrderSpan = true;
            for( size_t i = 0 ; i < numTerms - 1; i++ )
               {
               if( span[ i + 1 ] < span[ i ] )
                  isOrderSpan = false;
               }
            if( isOrderSpan )
               orderSpanCount ++;

            // 4. Check if it is a exact phrase
            bool isPhrase = true;
            for( size_t i = 0 ; i < numTerms - 1; i++ )
               {
               if( span[ i + 1 ] != span[ i ] + 1 )
                  isPhrase = false;
               }
            if( isPhrase )
               phraseCount ++;

            // 5. Check if there is a near triple if it is not a phrase
            bool isNearTriple = false;
            if ( !isPhrase )
               {
               for( size_t i = 0 ; i < numTerms; i++ )
                  {
                  size_t count = 0;
                  for( size_t j =  0; j < numTerms; j ++ )
                     {
                     if( i != j )
                        {
                        if( span[ j ] <= span[ i ] + MAX_SKIPPED_LOCATION && 
                           span[ j ] >= span[ i ] - MAX_SKIPPED_LOCATION )
                           count ++;
                        }
                     }
                  if( count >= 2 )
                     {
                     isNearTriple = true;
                     break;
                     }
                  }
               if( isNearTriple )
                  nearTripleCount ++;
               }


            // 6. Check if there is a near double if it is not a triple
            if( !isNearTriple )
               {
               bool isNearDouble = false;
               for( size_t i = 0 ; i < numTerms; i++ )
                  {
                  for( size_t j =  i + 1; j < numTerms; j ++ )
                     {
                     if( span[ j ] <= span[ i ] + MAX_SKIPPED_LOCATION &&
                        span[ j ] >= span[ i ] - MAX_SKIPPED_LOCATION )
                        {
                        isNearDouble = true;
                        break;
                        }
                     }
                  if( isNearDouble )
                     break;
                  }
               if( isNearDouble )
                  nearDoubleCount ++;
               }
            }
         }

      // Helper 
      // Calculate the dynmaic rank of the document title
      size_t DynamicTitleRank( ISRWord** terms, size_t numTerms, Location docStart, 
            Location docEnd, DocumentData docData, DebugScore &infoScore )
         {
         double count = 0;
         // Seek all title words to the start
         for( size_t i = 0; i < numTerms; i ++ )
            {
            if( terms[ i ]->getCurrentPost( ) )
               {
               terms[ i ]->Seek( docStart );
               if( terms[ i ]->getStartLocation( ) < docEnd )
                  count ++;
               }

            }
         double frequency = count / static_cast< double >( docData.numTitleWords );
         infoScore.titleWordPercentage = frequency * 100;
         if( frequency >= MIN_PERCENT_TITLE_TO_BE_ALL )
            return TITLE_CONTAIN_ALL_WEIGHT;
         if( frequency >= MIN_PERCENT_TITLE_TO_BE_MOST )
            return TITLE_CONTAIN_MOST_WEIGHT;
         if( frequency >= MIN_PERCENT_TITLE_TO_BE_MANY )
            return TITLE_CONTAIN_MANY_WEIGHT;
         if( frequency >= MIN_PERCENT_TITLE_TO_BE_SOME )
            return TITLE_CONTAIN_SOME_WEIGHT;
         if( frequency >= MIN_PERCENT_TITLE_TO_BE_FEW )
            return TITLE_CONTAIN_FEW_WEIGHT;
         return 0;
         }

      // Helper 
      // Calculate the dynmaic rank of the URL
      size_t DynamicURLRank( string url, vector< string > searchWords, 
            size_t rarestWordIndex)
         {
         size_t score = 0;
         for( size_t i = 0; i < searchWords.size( ); i++ )
            {
            if( getDomainName( url ) == searchWords[ i ] )
               {
               if( isHomePage( url ) )
                  score += URL_CONTAIN_QUERY_WORD_AND_HOME_PAGE_WEIGHT;
               else 
                  score += URL_CONTAIN_QUERY_WORD_WEIGHT;
               if( i == rarestWordIndex )
                  score *= 2;
               return score;
               }
            }
         return score;
         }

   public:
      Ranker( ISROr* _parsedQuery, vector<string> _searchWords, HashFile* _indexChunk )
         : parsedQuery ( _parsedQuery ), searchWords( _searchWords ), 
         indexChunk( _indexChunk ), dynamicRankWeight( DYNAMIC_WEIGHT ) 
            {
            topDocuments.resize( NUM_TOP_DOCUMENTS_RETURNED );
            debugInfo.resize( NUM_TOP_DOCUMENTS_RETURNED );
            };

      // Take in a ISROr and a list of search words(flattern query for ranker)
      // from query parser, find and rank documents in the given indexChunk
      void RankDocuments( )
         {
         // If any of the search word is not found in the index, return empty vector
         if( !SetDynamicRankWeight( ) )
            return;
         size_t numWords = searchWords.size( );
         ISRWord* terms[ numWords ];
         ISRWord* titleTerms[ numWords ];
         size_t expectedFrequency[ numWords ];
         size_t rarestWordIndex = 0;
         size_t rarestWordOccurance = MAX;
         for( size_t i = 0; i < numWords; i ++ )
            {
            // Generate Body ISRs
            const SerialTuple* tuple = indexChunk->Blob( )->
                  Find( searchWords[ i ].c_str( ) );
            const PostingList* pl = tuple ? tuple->GetPostingListValue( ) : nullptr;
            terms[ i ] = new ISRWord( pl );
            expectedFrequency[ i ] = pl->numberOfPosts / pl->numberOfDocuments;
            // Find rarest Words
            if( pl->numberOfPosts < rarestWordOccurance )
            {
               rarestWordOccurance = pl->numberOfPosts;
               rarestWordIndex = i;
            }
            // Generate Title ISRs
            char title[ searchWords[ i ].length( ) + 2 ];
            memset( title, 0, sizeof( title ) );
            strcpy( title, "#" );
            strcpy( title + 1, searchWords[ i ].c_str( ) );
            const SerialTuple* titleTuple = indexChunk->Blob( )->Find( title );
            const PostingList* titlePl = titleTuple ? titleTuple->
                  GetPostingListValue( ) : nullptr;
            titleTerms[ i ] = new ISRWord( titlePl );
            }
         // For each mathcing page, pass it to the ranker and rank the document
         size_t numDocsFound = 0;
         time_point< std::chrono::steady_clock > start, end;
         start = steady_clock::now( );
    //Do no send the result back to front end if timeout
         while( parsedQuery->NextDocument( ) && 
               numDocsFound++ <= MAX_DOCUMENT_PER_INDEX_PER_QUERY )
            {
            DebugScore infoScore;
            if( numDocsFound >= MAX_DOCUMENT_PER_INDEX_PER_QUERY )
               {
               dbgprintf( "Stop! More than %lu docs found in the index.\n",
                     numDocsFound );
               break;
               }
            // Limit each index search time at 3 seconds
            end = steady_clock::now( );
            duration< double > elapsed_seconds = end - start;
            if( elapsed_seconds.count( ) > 3 )
               {
               dbgprintf("Stop! Search on index takes more than 3s.\n");
               break;
               }
            Location docEnd = parsedQuery->getDocumentEnd( )->getStartLocation( );
            Location docStart = parsedQuery->getDocumentEnd( )->
                  getStartLocation( ) - parsedQuery->getDocumentEnd( )->
                  getDocLength( );
            DocumentData docData = indexChunk->
                  get_DocumentData( )[ parsedQuery->
                  getDocumentEnd( )->getUrlIndex( ) ];
            if ( docData.URL.length( ) < 10 ) 
               {
               cout << "Detect malformed Url: " << docData.URL << endl;
               continue;
               }

            string url = SimplifyUrl( docData.URL );
            size_t staticScore = StaticRank( docData, infoScore );
            infoScore.staticScore = staticScore;

            size_t dynamicURLScore = DynamicURLRank( url, searchWords, 
                  rarestWordIndex );
            infoScore.urlScore = dynamicURLScore;

            size_t dynamicTitleScore = DynamicTitleRank(titleTerms, numWords, 
                  docStart, docEnd, docData, infoScore );
            size_t dynamicBodyScore = DynamicBodyRank( terms, expectedFrequency, 
             numWords, rarestWordIndex, docStart, docEnd, infoScore ); 
            // Need to normalize dynamic weight for the body because very long
            // documents tend to have more spans and therefore higher dynamic weight
            // Normalize if length is 10x average
            double averageDocLength = 
                  static_cast< double >( indexChunk->Blob()->NumberOfPosts) / 
                  static_cast< double >( NUM_DOCS_IN_DICT );
            double normFactor = parsedQuery->getDocumentEnd( )->getDocLength( )
                  / averageDocLength / 10;
            infoScore.dynmicNormFactor = normFactor;
            if( normFactor > 1 )
            {
               dynamicBodyScore = static_cast<double>( dynamicBodyScore ) / 
                     normFactor;
            }
            size_t dynamicScore = dynamicBodyScore + dynamicTitleScore +
                  dynamicURLScore;
            infoScore.dynamicScore = dynamicScore;

            size_t totalScore = STATIC_WEIGHT * staticScore + 
                  dynamicRankWeight * dynamicScore;
            infoScore.totalScore = totalScore;
            DocumentScore docScore{ string( docData.title ), 
                  string( docData.abstract ), docData.URL, totalScore };
            infoScore.doc = docScore;

            InsertionSort( docScore, topDocuments );
            #ifdef DEBUG
            InsertionSort(infoScore, debugInfo);
            #endif
            }
         for( size_t i = 0; i < numWords; i++ )
            {
            delete terms[ i ];
            delete titleTerms[ i ];
            }
         }

      vector< DocumentScore > getTopDocuments( )
         {
         return topDocuments;
         }

      vector< DebugScore > getDebugInfo( )
         {
         return debugInfo;
         }
   };
