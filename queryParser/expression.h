// expression.h
// Class declarations for expression

// Patrick Su patsu@umich.edu

#ifndef EXPRESSION_H_
#define EXPRESSION_H_

//#include "string.h"
#include <vector>
#include <string>
#include "../constraintSolver/isr.h"

// A simple expression
class Expression
{
public:
    virtual ~Expression( );
    
    virtual ISR *Compile( const HashBlob *hashblob,
                         const PostingList* docEnd);
};

// A SearchWord, like a number in math expression
class SearchWord : public Expression
	{
	protected:
		std::string word;
	
	public :
		// the return value of compile: a dynamic pointer
		ISRWord *compileResult;

		~SearchWord( );
		
		SearchWord ( std::string &w );
		
		ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
	};

// The Constraint
class Constraint : public Expression
	{
	public:
		// A vector of BaseConstraints
		std::vector <Expression *> terms;
		// Array of ISR pointers
		ISR **result;
		// EndDoc ISR for potentially the orISR
		ISREndDoc *endDocISR;
		// the return value of compile: a dynamic pointer
		ISROr *compileResult;

		~Constraint( );

		ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
	};

// The BaseConstraint
class BaseConstraint : public Expression
{
public:
	// A vector of SimpleConstraints
	std::vector <Expression *> terms;
	// Array of ISR pointers
	ISR **result;
	// EndDoc ISR for potentially the andISR
	ISREndDoc* endDocISR;
	// the return value of compile
	ISRAnd *compileResult;
	
	~BaseConstraint( );

	ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
};

class SimpleConstraint : public Expression
{
public:
	// A vector of Phrases, NestedConstraints, or SearchWords
	std::vector <Expression *> terms;
	// Array of ISR pointers

	~SimpleConstraint( );

	ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
};

class Phrase : public Expression
{
public:
	// A vector of SearchWords
	std::vector <Expression *> terms;
	// Array of ISR pointers
	ISR **result;
	// EndDoc ISR for potentially the phraseISR
	ISREndDoc* endDocISR;
	// the return value of compile
	ISRPhrase *compileResult;

	~Phrase( );

	ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
};

class NestedConstraint : public Expression
{
public:
	// A vector of Constraints
	std::vector <Expression *> terms;
	// Array of ISR pointers

	~NestedConstraint( );

	ISR *Compile( const HashBlob *hashblob, const PostingList* docEnd );
};

#endif /* EXPRESSION_H_ */
