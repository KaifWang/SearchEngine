// tokenstream.cpp
// Implementation of tokenstream.h

// Patrick Su patsu@umich.edu

#include "tokenstream.h"
#include <cstring>

TokenStream::TokenStream( )
	{
	}

TokenStream::TokenStream( char *in ) :
   	input( in ), end( in + strlen( in ) )
	{
   }

bool TokenStream::Match( char c )
	{
	// Different than a math expression, we want to skip the unnecessary
	// whitespace during the matching. For example, consider the tokenstream
	// || \"hello world\"
	// After parsing the || we currently have one space before the " char
	// which will cause unwanted behavior if we don't skip the whitespace
	while ( *input == ' ' && input < end )
		++input;
	if ( input >= end )
    	return false;
	if ( *input == c )
		{
		++input;
		return true;
		}
	return false;
   }

bool TokenStream::FindAndOp( )
	{
	if ( input >= end )
		return false;
	// skip the whitespace
	while ( *input == ' ' && input < end )
		++input;
	// Parsing a string using a while loop, not stop until we reach a whitespace
	char *curr = input;
	// start parsing the next SearchWord
	// note that ')' and '"' has special meaning and should not be included in
	// our search word
	while ( *curr != ' ' && *curr != ')' && *curr != '"' && curr < end )
		++curr;
	std::string word = std::string( input, curr - input );
	// Only move the input if the searchWord is the And operation
	// <AndOp> ::= 'AND' | '&' | '&&'
	if ( word == "AND" || word == "&" || word == "&&" )
		input = curr;
	// Can be a counter-intuitive step
	// We should return false when we reach an or operation
	// The reason is this is or marked the end of a BaseConstraint
	// c.f. 3*5+1 we stop the calculation of 3*5 at the + sign
	// Otherwise, we will treat it as a hidden and explained below
	if ( word == "OR" || word == "|" || word == "||" )
		return false;
	// Another edge case is for nestedConstraint
	// Move the check in FindNestedConstraint to here
	if ( Match( ')' ) )
		return false;
	// Handle the "hidden" and operation
	// Ex. Input = Apollo Moon Landing "Hello Earth"
	// Base Constraints = Apollo <AND> Moon <AND> Landing <AND> "Hello Earth"
	// Simple Constraints = Apollo, Moon, Landing, "Hello Earth"
	// The way to add the hidden AND is to not move the input pointer but return
	// true. Then we findBaseConstraints will assume we hit an AND and start
	// looking for the next SimpleConstraint.
	return true;
	}

bool TokenStream::FindOrOp( )
	{
	if ( input >= end )
		return false;
	// skip the whitespace
	while ( *input == ' ' && input < end )
		++input;
	// Parsing a string using a while loop, not stop until we reach a whitespace
	char *curr = input;
	// start parsing the next SearchWord
	// note that ')' and '"' has special meaning and should not be included in
	// our search word
	while ( *curr != ' ' && *curr != ')' && *curr != '"' && curr < end )
		++curr;
	std::string word = std::string( input, curr - input );
	// Only move the input if the searchword is the OrOp
	// <OrOp> ::= 'OR' | '|' | '||'
	if ( word == "OR" || word == "|" || word == "||" )
		{
		input = curr;
		return true;
		}
	// Do nothing otherwise
	return false;
	}

bool TokenStream::AllConsumed( ) const
   {
   return input >= end;
   }

SearchWord *TokenStream::ParseSearchWord( )
   {
	if ( input >= end )
		return nullptr;
	// skip the whitespace
	while ( *input == ' ' && input < end )
		++input;
	// Parsing a string using a while loop, not stop until we reach a whitespace
	char *curr = input;
	// start parsing the SearchWord
	// note that ')' and '"' has special meaning and should not be included in
	// our search word
	while ( *curr != ' ' && *curr != ')' && *curr != '"' && curr < end )
		++curr;
	std::string word = std::string( input, curr - input );
	input = curr;
	tokens.push_back ( word );
	return new SearchWord( word );
	}

std::vector<std::string> TokenStream::getTokens( )
	{
	return tokens;
	}