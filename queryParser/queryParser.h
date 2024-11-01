//  queryParser.h
//  Query Parser that supports query language grammar
//  Read input from the front end and send the output to the constraint solver

//  Patrick Su patsu@umich.edu

//  Overview of the grammar
//  <Constraint>        ::= <BaseConstraint> { <OrOp> <BaseConstraint> }
//
//  <OrOp>              ::= 'OR' | '|' | '||'
//
//  <BaseConstraint>    ::= <SimpleConstraint> { [ <AndOp> ] <SimpleConstraint> }
//
//  <AndOp>             ::= 'AND' | '&' | '&&'
//
//  <SimpleConstraint>  ::= <Phrase> | <NestedConstraint> | <SearchWord>
//
//  <Phrase>            ::= '"' { <SearchWord> } '"'
//
//  <NestedConstraint>  ::= '(' <Constraint> ')'

#ifndef QUERYPARSER_H_
#define QUERYPARSER_H_

#include <dirent.h>
#include "../configs/config.h"
#include "tokenstream.h"
#include "../ranker/ranker.h"

// The actual query parser
// input :  query string, directory of index chunk
// output : vector <documentData>
class QueryParser
	{
	HashFile* indexChunk;

	// Stream of tokens to consume input from
	TokenStream ts;

	// Final "constraint" 
	Expression *parsedExpression;
	
	// Find the appropriate nonterminal
	Expression *FindConstraint( );

	Expression *FindBaseConstraint( );

	Expression *FindSimpleConstraint( );

	Expression *FindPhrase( );

	Expression *FindNestedConstraint( );

	Expression *FindSearchWord( );

	public:
		QueryParser( );

		// Constructor that takes in query from front end
		QueryParser( char *query, HashFile* indexChunk );

		// Destructor of Parser
		~QueryParser( );

		// Get a flattened vector of searhc result
		vector<string> GetFlattenedVector( );

		// Get ISROr*
		ISROr* getParsedISRQuery( );
      
		vector<DocumentScore> TopNDocuments( );
   };

#endif /* QUERYPARSER_H_ */