// expression.cpp
// Class implementations for the expression functionality

// Patrick Su patsu@umich.edu

#include "expression.h"

Expression::~Expression( )
	{
	}

SearchWord::~SearchWord( )
	{
	// delete the compileResult
	delete compileResult;
	}

Constraint::~Constraint( )
	{
	// delete vector of expression pointers 
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		delete terms[ i ];

	delete[ ] result;

	// delete the endDocISR
	delete endDocISR;

	// delete the compileResult
	delete compileResult;
	}

BaseConstraint::~BaseConstraint( )
	{
	// delete vector of expression pointers 
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		delete terms[ i ];

	delete[ ] result;

	// delete the endDocISR
	delete endDocISR;

	// delete the compileResult
	delete compileResult;
	}

SimpleConstraint::~SimpleConstraint( )
	{
	// delete vector of expression pointers 
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		delete terms[ i ];
	}

Phrase::~Phrase( )
	{
	// delete vector of expression pointers 
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		delete terms[ i ];

	delete[ ] result;
	// delete the endDocISR
	delete endDocISR;

	// delete the compileResult
	delete compileResult;
	}

NestedConstraint::~NestedConstraint( )
	{
	// delete vector of expression pointers 
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		delete terms[ i ];
	}

ISR *Expression::Compile( const HashBlob *hashblob, const PostingList* docEnd )
	{
	return nullptr;
	}

SearchWord::SearchWord( std::string &w ) : word ( w )
	{
	}

ISR *SearchWord::Compile( const HashBlob *hashblob,
                         const PostingList* docEnd )
	{
	const PostingList* w
	= hashblob->Find( word.c_str( ) )->GetPostingListValue( );
	//ISRWord searchWordISR( w );
	compileResult = new ISRWord( w );
	return compileResult;
	}

ISR *Constraint::Compile( const HashBlob *hashblob, const PostingList* docEnd )
	{
	result = new ISR*[ terms.size( ) ];
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		result[ i ] = terms[ i ]->Compile( hashblob, docEnd );
	endDocISR = new ISREndDoc( docEnd );
	// ISROr orISR( result, &endDocISR, terms.size( ) );
	// store as compileResult: easy for destructor to free the memory
	compileResult = new ISROr( result, endDocISR, terms.size( ) );
	return compileResult;
	}

ISR *BaseConstraint::Compile( const HashBlob *hashblob,
                             const PostingList* docEnd )
	{
	result = new ISR*[ terms.size( ) ];
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		result[ i ] = terms[ i ]->Compile( hashblob, docEnd );
	endDocISR = new ISREndDoc( docEnd );
	// ISRAnd andISR( result, &endDocISR, terms.size( ) );
	// store as compileResult: easy for destructor to free the memory
	compileResult = new ISRAnd( result, endDocISR, terms.size( ) );
	return compileResult;
	}

ISR *SimpleConstraint::Compile( const HashBlob *hashblob,
                               const PostingList* docEnd )
	{
	// Pass up an WordISR/ PhraseISR/ OrISR depends on this SimpleConstraint
	return terms[ 0 ]->Compile( hashblob, docEnd );
	}

ISR *Phrase::Compile( const HashBlob *hashblob, const PostingList* docEnd )
	{
	result = new ISR*[ terms.size( ) ];
	for ( std::size_t i = 0; i < terms.size( );  ++i )
		result[ i ] = terms[ i ]->Compile( hashblob, docEnd );
	endDocISR = new ISREndDoc( docEnd );
	// ISRPhrase phraseISR( result, &endDocISR, terms.size( ) );
	compileResult = new ISRPhrase( result, endDocISR, terms.size( ) );
	return compileResult;
	}

ISR *NestedConstraint::Compile( const HashBlob *hashblob,
                               const PostingList* docEnd )
	{
	// Pass up an OrISR
	return terms[ 0 ]->Compile( hashblob, docEnd );
	}
