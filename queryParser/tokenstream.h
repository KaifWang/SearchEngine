// tokenstream.h
// Declaration of a stream of tokens that we can read from
// Main functionality includes find and/or opertors and the search word

// Patrick Su patsu@umich.edu

#ifndef TOKENSTREAM_H_
#define TOKENSTREAM_H_

#include "expression.h"

class TokenStream
	{
	// The input we receive
	char* input;
	char* end;
	std::vector<std::string> tokens;

	public:
		// Default constructor
		TokenStream( );

		// Construct a token stream that uses a copy of the input
		TokenStream( char *in );

		// Attempt to match and consume a specific character
		// Returns true if the char was matched and consumed
		bool Match( char c );

		// Attempt to match and consume an "and" operator
		// <AndOp> ::= 'AND' | '&' | '&&'
		// Returns true if the opeartor was matched and consumed
		bool FindAndOp( );

		// Attempt to match and consume an "or" operator
		// <OrOp> ::= 'OR' | '|' | '||'
		// Returns true if the opeartor was matched and consumed
		bool FindOrOp( );

		// Check whether all input are consumed
		bool AllConsumed( ) const;

		// Attempt to match and consume a whole word
		// Return a dynamically allocated SearchWord if successful
		SearchWord *ParseSearchWord( );

		// Return the tokens we have parsed
		std::vector<std::string> getTokens( );
	};

#endif /* TOKENSTREAM_H_ */
