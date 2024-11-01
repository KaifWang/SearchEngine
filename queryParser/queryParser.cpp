// queryParser.cpp
// Implement the functions in queryParser.h

// Patrick Su patsu@umich.edu

#include "expression.h"
#include "queryParser.h"


Expression *QueryParser::FindSearchWord( )
	{
	return ts.ParseSearchWord( );
	}

//  <Constraint> ::= <BaseConstraint> { <OrOp> <BaseConstraint> }
Expression *QueryParser::FindConstraint( ) 
	{
	Expression *left = FindBaseConstraint( );
	if ( left )
		{
		Constraint *self = new Constraint( );
		self->terms.push_back( left );
		while ( ts.FindOrOp( ) )
			{
			left = FindBaseConstraint( );
			// Different than the math expression, do minimal syntax handling
			// User want to see an output
			if ( left )
				self->terms.push_back( left );
			}
		return self;
		}
	return nullptr;
	}

//  <BaseConstraint> ::= <SimpleConstraint> { [ <AndOp> ] <SimpleConstraint> }
Expression *QueryParser::FindBaseConstraint( ) 
	{
	Expression *left = FindSimpleConstraint( );
	if ( left )
		{
		BaseConstraint *self = new BaseConstraint( );
		self->terms.push_back( left );
		while ( ts.FindAndOp( ) )
			{
			left = FindSimpleConstraint( );
			// Different than the math expression, do minimal syntax handling
			// User want to see an output
			if ( left )
				self->terms.push_back( left );
			}
		return self;
		}
	return nullptr;
	}

//  <SimpleConstraint> ::= <Phrase> | <NestedConstraint> | <SearchWord>
Expression *QueryParser::FindSimpleConstraint( ) 
	{
	SimpleConstraint *self = new SimpleConstraint( );
	// check which form the SimpleConstraint is in
	Expression *phrase = FindPhrase( );
	if ( phrase )
		{
		self->terms.push_back( phrase );
		return self;
		}
	Expression *nestedConstraint = FindNestedConstraint( );
	if ( nestedConstraint )
		{
		self->terms.push_back( nestedConstraint );
		return self;
		}
	Expression *searchWord = FindSearchWord( );
	if ( searchWord )
		{
		self->terms.push_back( searchWord );
		return self;
		}
	return nullptr;
	}

//  <Phrase> ::= '"' { <SearchWord> } '"'
Expression *QueryParser::FindPhrase( )
	{
	if ( ts.Match( '"' ) )
		{
		Phrase *self = new Phrase( );
		Expression *left = FindSearchWord( );
		if ( left )
			self->terms.push_back( left );
		while ( !ts.Match( '"' ) && !ts.AllConsumed( ) )
			{
			Expression *left = FindSearchWord( );
			// Different than the math expression, do minimal syntax handling
			// User want to see an output
			if ( left )
				self->terms.push_back( left );
			}
		return self;
		}
	return nullptr;
	}

//  <NestedConstraint> ::= '(' <Constraint> ')'
Expression *QueryParser::FindNestedConstraint( )
	{
	if ( ts.Match( '(' ) )
		{
		Expression *constraint = FindConstraint( );
		// parse out ')' is handled in findAnd
		return constraint;
		}
	return nullptr;
	}

// default constructor for a query parser
QueryParser::QueryParser( )
	{
	}

// Constructor of a query parser
QueryParser::QueryParser( char *query, HashFile* _indexChunk )
	{
	// filtered the query first (moved to fronend)
	// filteredQuery = filteredWord( query );
   indexChunk = _indexChunk;
	ts = query;
	parsedExpression = FindConstraint( );
	}

// Destructor of the parser
QueryParser::~QueryParser( )
	{
	// Include delete results in segFault (double delete)
	delete parsedExpression;
	}

// Get the flattened vector of search words for ranker
vector<string> QueryParser::GetFlattenedVector( )
	{
	return ts.getTokens( );
	}

ISROr* QueryParser::getParsedISRQuery()
	{
   const HashBlob *hashblob = indexChunk->Blob();
   const PostingList* endDoc
               = hashblob->Find( "##EndDoc" )->GetPostingListValue( );
   ISR *result = parsedExpression->Compile( hashblob, endDoc );
   ISROr *parsedQuery = result ? dynamic_cast<ISROr*>( result ) : nullptr;
   return parsedQuery;
	}
