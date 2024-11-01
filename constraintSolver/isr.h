// isr.h
// Input Stream Reader(ISR) that iterates through the index
//
// Kai Wang (kaifan@umich.edu)


#pragma once


#include "../index/index.h"

typedef size_t Location;
typedef size_t Offset;

static size_t MAX = 0xFFFFFFFF;
// using namespace std;

class Post
   {
   private:
      Location start;
      Location end;
   public:
      Post( ) : start( 0 ), end ( 0 ){ };
      Post( Location _start, Location _end ) : start( _start ), end( _end ) { };
      Location getStartLocation( ){ return start; }
      Location getEndLocation( ) { return end; }
   };

class ISR
   {
   public:
      Post* currentPost;
      ISR( )  : currentPost ( new Post ( 0, 0 ) ){ }
      ~ISR( ) { delete currentPost; }
      Post* setCurrentPost ( Post* _post ) 
         {
         delete currentPost;
         currentPost = _post;
         return currentPost;
         }
      Post* getCurrentPost() { return currentPost; }
      virtual Post *Next( ) = 0;
      virtual Post *Seek( Location target ) = 0;
      virtual Location getStartLocation( ) = 0;
      virtual Location getEndLocation( ) = 0;
   };

class ISRWord : public ISR
   {
   public:
      const PostingList* postingList;
      const ByteStruct* post_itr; //current posting of the ISR

      ISRWord( const PostingList* _postingList ) : postingList( _postingList ), 
            post_itr( _postingList->posts )
         {
            if( _postingList )
               post_itr = _postingList -> posts;
            else
               {
               post_itr = nullptr;
               setCurrentPost(nullptr);
               }
         }

      Post *Next( )
         {
         // Decode the delta from byte stream, 
         // byteOffset should be increment to the next delta
         if( !getCurrentPost ( ) ) { return nullptr; }
         size_t delta = DecodeByteStruct( post_itr );
         if ( delta == 0 ) 
            return setCurrentPost( nullptr );// Reaches end of the list
         Location nextLocation = currentPost -> getStartLocation( ) + delta;
         return setCurrentPost( new Post( nextLocation, nextLocation ) );
         }

      Post *Seek( Location target )
         {
         if( !getCurrentPost( ) ) { return nullptr; }
         //cout << "synIndex: " << highBit(target) << endl;
         SynchronizationData data = postingList->syncTable[ highBit ( target ) ];
         if( data.actualLocation )
            {
            post_itr = postingList->posts + data.indexOffset;
            // IndexOffset still points to current delta
            // Move the iterator to the next delta
            // Should not reache the end(return 0)
            assert( DecodeByteStruct ( post_itr ) );
            setCurrentPost( new Post ( data.actualLocation, data.actualLocation ) );
            }
         // Read Forward from the syn point until pass target or reach the end
         while( getStartLocation( ) < target )
            {
            if( !Next( ) )
               return setCurrentPost( nullptr );// Reaches end of the list
            }
         return getCurrentPost( );
         }

      Location getStartLocation( )
         {
         if( !getCurrentPost( ) )
            return MAX;
         return getCurrentPost( )->getStartLocation( );
         }
      Location getEndLocation()
         {
         if( !getCurrentPost( ) )
            return MAX;
         return getCurrentPost( )->getEndLocation( );
         }
   };

class ISREndDoc : public ISRWord
   {
   private:
      size_t currentUrlIndex;
      size_t docLength;
   public:
      ISREndDoc( const PostingList* _postingList ) 
            : ISRWord( _postingList ), currentUrlIndex( 0 ), docLength( 0 ){}

      Post *Seek( Location target )
         {
         if( !getCurrentPost( ) ) { return nullptr; }
         SynchronizationData data = postingList->syncTable[ highBit ( target ) ];
         if( data.actualLocation ){
            post_itr = postingList->posts + data.indexOffset ;
            docLength = DecodeByteStruct( post_itr );
            currentUrlIndex = DecodeByteStruct( post_itr );
            assert( docLength );
            setCurrentPost( new Post( data.actualLocation, data.actualLocation ) );
         }
         // Read Forward from the syn point until pass target or reach the end
         while( getStartLocation( ) < target )
            {
            if( !Next( ) )
               return setCurrentPost( nullptr );// Reaches end of the list
            }
         return getCurrentPost( );
         }

      Post *Next( )
         {
         if( !getCurrentPost( ) ) { return nullptr; }
         size_t delta = DecodeByteStruct( post_itr );
         if ( delta == 0 ) 
            return setCurrentPost( nullptr );// Reaches end of the list
         docLength = delta - 1; // Delta from previous post - 1 is exaclty the docLength
         currentUrlIndex = DecodeByteStruct( post_itr );
         Location nextLocation = getCurrentPost( )->getStartLocation( ) + delta;
         return setCurrentPost( new Post( nextLocation, nextLocation ) );
         }
      size_t getUrlIndex( ) { return currentUrlIndex; } 
      size_t getDocLength( ) { return docLength; }
};

class ISROr : public ISR
   {   
   private: 
      size_t nearestTerm;
      Location nearestStartLocation;
      Location nearestEndLocation;
   public:
      ISR **terms;
      ISREndDoc* documentEnd;
      size_t numberOfTerms;

      ISROr( ISR** _terms, ISREndDoc* _documentEnd, size_t _numberOfTerms )
            : terms( _terms ), documentEnd( _documentEnd ), 
            numberOfTerms( _numberOfTerms ), nearestTerm( 0 ), 
            nearestStartLocation( 0 ), nearestEndLocation( 0 ) { }

      Post *Seek(Location target)
         {
         Location currentNearest = MAX;
         for( size_t i = 0; i < numberOfTerms; i++ )
            {
            if( terms[ i ]->Seek( target ) )
               {
               // Find nearest locations among all ISRs
               if( terms[ i ]->getCurrentPost( ) && 
                     terms[ i ]->getStartLocation( ) < currentNearest )
                  {
                  currentNearest = terms[ i ]->getStartLocation( ); 
                  nearestTerm = i;        
                  nearestStartLocation = terms[ i ]->getStartLocation( );
                  nearestEndLocation = terms[ i ]->getEndLocation( );       
                  }
               }
            }
         //return null if no posts found
         if (currentNearest == MAX){ return setCurrentPost( nullptr ); }
         documentEnd->Seek( nearestStartLocation );
         return setCurrentPost( new Post( nearestStartLocation, nearestEndLocation ) );
         }

      Post *Next( )
         {
         terms[ nearestTerm ]->Next( );
         Location currentNearest = MAX;
         for( size_t i = 0; i < numberOfTerms; i++ )
            {         
            // If this term not reaching the end         
            if( terms[ i ]->getCurrentPost( ) )
               {
               if(terms[ i ]->getStartLocation( ) == 0 )
                  terms[ i ]->Next( );

               // Find nearest locations among all ISRs
               if( terms[i]->getStartLocation() < currentNearest)
                  {
                  currentNearest = terms[i]->getStartLocation(); 
                  nearestTerm = i;        
                  nearestStartLocation = terms[i]->getStartLocation();
                  nearestEndLocation = terms[i]->getEndLocation();
                  }
               }
            }
         if ( currentNearest == MAX ){ return setCurrentPost( nullptr ); }
         documentEnd->Seek( nearestStartLocation );
         return setCurrentPost( new Post( nearestStartLocation, nearestEndLocation ) );
         }

      Post* NextDocument( )
         {
         if( documentEnd->getCurrentPost( ) )
            return Seek( documentEnd->getStartLocation( ) + 1 );
         return setCurrentPost( nullptr );
         }

      Location getStartLocation( )
         {
         return nearestStartLocation;
         }
      Location getEndLocation( )
         {
         return nearestEndLocation;
         }

      ISREndDoc* getDocumentEnd( )
         {
         return documentEnd;
         }

   };

class ISRAnd : public ISR
   {
   private:
      size_t nearestTerm, farthestTerm;
      Location nearestStartLocation, nearestEndLocation;
   public:
      ISR **terms;
      ISREndDoc *documentEnd;
      size_t numberOfTerms;

      ISRAnd( ISR** _terms, ISREndDoc* _documentEnd, size_t _numberOfTerms )
            : terms( _terms ), documentEnd( _documentEnd ), 
            numberOfTerms( _numberOfTerms ), nearestTerm( 0 ), 
            farthestTerm( 0 ), nearestStartLocation( 0 ), 
            nearestEndLocation( 0 ) { }

      Post *Seek( Location target )
         {
         Location currentFarthest = 0;
         //1. Seek all the ISRs to first occurance starting at the Target Location
         for( size_t i = 0; i < numberOfTerms; i++ )
            {
            if( !terms[ i ]->Seek( target ) )
               return setCurrentPost( nullptr );
            // Find farthest locations among all ISRs
            if( terms[ i ]->getStartLocation( ) > currentFarthest )
               {
               currentFarthest = terms[ i ]->getStartLocation( ); 
               farthestTerm = i;
               }
            }
         //Loop until find match
         while( true )
            {
            // A flag to quickly move to next doc
            bool moveToNextDoc = false;
            // 2. Move docEndISR to just past the farthest.
            // Return if reaching the end
            if( !documentEnd->Seek( currentFarthest ) )
               return nullptr;
            //3. Seek all the other terms to just pass the document beginning.
            Location seekTarget = documentEnd->getStartLocation( ) 
                  - documentEnd->getDocLength( );
            for( size_t i = 0; i < numberOfTerms; i++ )
               {
               if(i != farthestTerm)
                  {
                  //If any ISR reaches the end, there is no match
                  if( !terms[ i ]->Seek( seekTarget ) ) 
                     return setCurrentPost( nullptr );
                  //4. If any term is past the DocEnd, return to step 2
                  if( terms[ i ]->getStartLocation( ) > 
                        documentEnd->getStartLocation( ) )
                     {
                     currentFarthest = terms[ i ]->getStartLocation( ); 
                     farthestTerm = i; 
                     moveToNextDoc = true;
                     continue;     
                     }
                  }
               }
            if( moveToNextDoc ) { continue; }
            //At this poinst, we have found a matching document
            //We want to find nearestTerms
            Location currentNearest = currentFarthest;
            for( size_t i = 0; i < numberOfTerms; i++ )
               {
               // Find nearest locations among all ISRs
               if( terms[ i ]->getStartLocation( ) <= currentNearest )
                  {
                  currentNearest = terms[ i ]->getStartLocation( ); 
                  nearestTerm = i;        
                  nearestStartLocation = terms[ i ]->getStartLocation( );
                  nearestEndLocation = terms[ i ]->getEndLocation( );       
                  }
               }
            return setCurrentPost( new Post( nearestStartLocation, nearestEndLocation ) );
            }
         }

      Post *Next( )
         {
         return Seek( nearestStartLocation + 1 );
         }

      Post *NextDocument( )
         {
         if( documentEnd->getCurrentPost( ) )
            return Seek( documentEnd->getStartLocation( ) + 1 );
         return setCurrentPost( nullptr );
         }

      Location getStartLocation( )
         {
         return nearestStartLocation;
         }
      Location getEndLocation( )
         {
         return nearestEndLocation;
         }
      ISREndDoc* getDocumentEnd( )
         {
         return documentEnd;
         }
   };

class ISRPhrase : public ISR
   {
   private:
      size_t farthestTerm;
      Location nearestStartLocation, nearestEndLocation;
   public:
      ISR **terms;
      ISREndDoc *documentEnd;
      size_t numberOfTerms;

      // Terms should passed in exactly as the order of the phrase
      // e.x For phrase "quick brown fox", 
      // terms[0] = quick, terms[1] = brown, terms[2] = fox
      ISRPhrase( ISR** _terms, ISREndDoc* _documentEnd, size_t _numberOfTerms )
            : terms( _terms ), documentEnd( _documentEnd ), 
            numberOfTerms( _numberOfTerms ), farthestTerm( 0 ), 
            nearestStartLocation( 0 ), nearestEndLocation( 0 ) { }

      Post *Seek( Location target )
         {
         Location currentFarthest = 0;
         //1. Seek all the ISRs to first occurance starting at the Target Location
         for( size_t i = 0; i < numberOfTerms; i++ )
            {
            if( !terms[ i ]->Seek( target ) )
               return setCurrentPost( nullptr );
            // Find farthest locations among all ISRs
            if( terms[ i ]->getStartLocation( ) > currentFarthest )
               {
               currentFarthest = terms[ i ]->getStartLocation( ); 
               farthestTerm = i;
               }
            }
         //Loop until find match
         while( true )
            {
            bool moveToNextDoc = false;
            // 2. Pick the Furthest Term and seek all others to the location
            // they should appear relative to the furtherest term
            // Return if reaching the end
            for( size_t i = 0; i < numberOfTerms; i++ )
               {
               if( i != farthestTerm )
                  {
                  // Seek to its corrent location for the phrase
                  Location desiredLocation = currentFarthest + i - farthestTerm;
                  // If any ISR reaches the end, there is no match
                  if( !terms[ i ]->Seek( desiredLocation ) ) 
                     return setCurrentPost( nullptr );
                  //4. If any term is past the DocEnd, return to step 2
                  if( terms[ i ]->getStartLocation( ) > desiredLocation )
                     {
                     currentFarthest = terms[ i ]->getStartLocation( ); 
                     farthestTerm = i; 
                     moveToNextDoc = true;
                     break;     
                     }
                  }
               }
            if( moveToNextDoc ) { continue; }
            // At this poinst, we have found a matching document
            // nearest start location is the first term location
            // nearest end location is the last term location
            nearestStartLocation = terms[ 0 ] -> getStartLocation( );
            nearestEndLocation = terms[ numberOfTerms - 1 ] -> getEndLocation( );
            documentEnd->Seek( nearestEndLocation );
            return setCurrentPost( new Post( nearestStartLocation, nearestEndLocation ) );
            }
         }

      Post *Next( )
         {
         return Seek( nearestStartLocation + 1 );
         }

      Post *NextDocument( )
         {
         if( documentEnd->getCurrentPost( ) )
            return Seek( documentEnd->getStartLocation( ) + 1 );
         return setCurrentPost( nullptr );
         }

      Location getStartLocation( )
         {
         return nearestStartLocation;
         }
      Location getEndLocation( )
         {
         return nearestEndLocation;
         }
      ISREndDoc* getDocumentEnd( )
         {
         return documentEnd;
         }
   };