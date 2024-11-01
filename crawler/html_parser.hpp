//
// html_parser.h
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// HTML Parser Interface
//

#pragma once

#include <cstring> // strstr
#include <cassert> // asssert
#include <pthread.h> // concurrent
#include <cxxabi.h>  // abi::__forced_unwind& 

#include "configs/crawler_cfg.hpp" // configuration
#include "html_tags.hpp"

#define UMSE_FFLUSH

// This is a simple HTML parser class.  Given a text buffer containing
// a presumed HTML page, the constructor will parse the text to create
// lists of words, title words and outgoing links found on the page.  It
// does not attempt to parse the entire the document structure.
//
// The strategy is to word-break at whitespace and HTML tags and discard
// most HTML tags.  Three tags require discarding everything between
// the opening and closing tag. Four tags require special processing.
//
// We will use the list of possible HTML element names found at
// https://developer.mozilla.org/en-US/docs/Web/HTML/Element +
// !-- (comment), !DOCTYPE and svg, stored as a table in HtmlTags.h.

// Here are the rules for recognizing HTML tags.
//
// 1. An HTML tag starts with either < if it's an opening tag or </ if
//    it's closing token.  If it starts with < and ends with /> it is both.
//
// 2. The name of the tag must follow the < or </ immediately.  There can't
//    be any whitespace.
//
// 3. The name is terminated by whitespace, > or / and is case-insensitive.
//
// 4. If it is terminated by whitepace, arbitrary text representing various
//    arguments may follow, terminated by a > or />.
//
// 5. If the name isn't on the list we recognize, we assume it's the whole
//    is just ordinary text.
//
// 6. Every token is taken as a word-break.
//
// 7. Most opening or closing tokens can simply be discarded.
//
// 8. <script>, <style>, and <svg> require discarding everything between the
//    opening and closing tag.  Unmatched closing tags are discarded.
//
// 9. <title>, <a>, <base> and <embed> require special processing.
//
//      <title> should cause all the words between the opening and closing
//          tags to be added to the titleWords vector rather than the default
//          words vector.  A closing </title> without an opening <title> is discarded.
//
//      <a> is expected to contain an href="...url..."> argument with the
//          URL inside the double quotes that should be added to the list
//          of links.  All the words in between the opening and closing tags
//          should be collected as anchor text associated with the link
//          in addition to being added to the words or titleWords vector,
//          as appropriate.  A closing </a> without an opening <a> is
//          discarded.
//
//     <base> may contain an href="...url..." parameter.  If present, it should
//          be captured as the base variable.  Only the first is recognized; any
//          others are discarded.
//
//     <embed> may contain a src="...url..." parameter.  If present, it should be
//          added to the links with no anchor text.

class Link
   {
   public:
      string URL;
      vector<string> anchorText;

      Link ( const char* start, size_t num ) : URL ( start, num ) {}
      Link ( const string& url ) : URL ( url ) {}
   };

class HtmlParser
   {
   public:
      vector<string> words, titleWords;
      vector<Link> links;
      string base = "";

      //DesiredAction tagAction;
      bool inTitle;
      bool inAnchor;
      char* buffer;
      int length;

   private:
      // extract word and add to saved container
      inline void handleText ( int startIdx, int endIdx )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::handleText) handle text `%s` between %d and %d\n", pthread_self(), string ( buffer + startIdx, endIdx - startIdx ).c_str(), startIdx, endIdx );
         #endif

         int wordStartIdx = startIdx;
         int wordEndIdx = wordStartIdx;

         while ( wordStartIdx < endIdx )
            {
            // find first character not white space ( ` `, `\t`, `\n` )
            while ( wordStartIdx < endIdx && ( buffer[ wordStartIdx ] == ' '
                        || buffer[ wordStartIdx ] == '\t' || buffer[ wordStartIdx ] == '\n'
                        || buffer[ wordStartIdx ] == '\r' ) )
               wordStartIdx++;

            if ( wordStartIdx == endIdx )
               break;


            // find one pass last word idx
            wordEndIdx = wordStartIdx;
            while ( wordEndIdx < endIdx && ( buffer[ wordEndIdx ] != ' '
                        && buffer[ wordEndIdx ] != '\t' && buffer[ wordEndIdx ] != '\n'
                        && buffer[ wordEndIdx ] != '\r' ) )
               wordEndIdx++;


            string word ( buffer + wordStartIdx, wordEndIdx - wordStartIdx );

            // change from upper case to lower case
            // if word contain non alpha character, discard word
            for ( unsigned int i = 0; i < word.length(); ++i )
               {
               if ( std::isalpha ( word[i] ) )
                  word[i] = std::tolower ( word[i] );
               else
                  {
                  word = "";
                  break;
                  }
               }

            try
               {
               if ( word.length() != 0 )
                  {
                  //#ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
                  //printf ( "@ [t %ld] (parse) word `%s`\n", pthread_self(), word.c_str() );
                  //#endif

                  if ( inTitle )
                     titleWords.emplace_back ( word );
                  else
                     words.emplace_back ( word );

                  if ( inAnchor )
                     links.back().anchorText.emplace_back ( word );
                  }
               }
            #ifndef __APPLE__
            catch ( abi::__forced_unwind& )
               {
               printf ( "E [t %ld] (HtmlParser::handleText) \n\n!!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!LISTEN SIGNAL THREAD BEING KILLED!!!!!!!!!\n\n", pthread_self() );
               fflush ( stdout );
               throw;
               }
            #endif
            catch ( ... )
               {
               printf ( "@ [t %ld] add words to memory failed\n", pthread_self() );
               return;
               }

            wordStartIdx = wordEndIdx;
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::handleText) end\n", pthread_self() );
         #endif

         }


      // return one pass `>` if valid tag
      // return tagStartIdx if ordinary tag
      // return tagStartIdx if reach end of file (regardless of tagname)
      int handleOpenTag ( int tagStartIdx )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::handleOpenTag) start, next 40 \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(),
               string ( buffer + tagStartIdx,
                     ( tagStartIdx + 40 < length ? 40 : length - tagStartIdx ) ).c_str() );
         #endif

         #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
         assert ( buffer[tagStartIdx] == '<' );
         #endif

         //std::cout << "tagStartIdx: " << tagStartIdx << std::endl;
         int onePassTagNameEnd = findOnePassTagNameEnd ( tagStartIdx + 1 );
         //std::cout << "onePassTagNameEnd: " << onePassTagNameEnd << std::endl;

         #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
         assert ( onePassTagNameEnd > 0 );
         #endif

         if ( onePassTagNameEnd >= length )
            {
            //printf("Exception: onepasstagNameEnd return %d, more than %d", onePassTagNameEnd, length);
            return length;
            }
         int tagEndIdx = findTagEnd ( onePassTagNameEnd );
         /*
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "@ [t %ld] (HtmlParser::handleOpenTag) close tagname %s\n", pthread_self(),
               string ( buffer + tagStartIdx + 2,
                     onePassTagNameEnd - tagStartIdx - 1 ).c_str() );
         printf ( "@ [t %ld] (HtmlParser::handleOpenTag) whole close tag %s\n", pthread_self(),
               string ( buffer + tagStartIdx, tagEndIdx - tagStartIdx + 1 ).c_str() );
         #endif

         */

         // if cannot find tag end, discard rest of it
         if ( tagEndIdx < 0 )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) reach end of file, return length\n", pthread_self() );
            #endif


            return length;
            }
         //std::cout << "here" << std::endl;
         // [tag start, tag end)
         DesiredAction tagAction = LookupPossibleTag ( buffer + tagStartIdx + 1, buffer + onePassTagNameEnd );

         if ( tagAction == DesiredAction::Anchor )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) tag type = anchor\n", pthread_self() );
            #endif


            //int urlStartIdx = onePassTagNameEnd;
            //int urlEndIdx = onePassTagNameEnd;
            //extractURL ( onePassTagNameEnd, tagEndIdx, "href", urlStartIdx, urlEndIdx );
            string newURL = extractURL ( onePassTagNameEnd, tagEndIdx, "href" );

            if ( newURL.size() != 0 )
               {
               inAnchor = true;
               links.emplace_back ( newURL );
               }
            }
         else if ( tagAction == DesiredAction::Base )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) tag type = base\n", pthread_self() );
            #endif


            //int urlStartIdx = onePassTagNameEnd;
            //int urlEndIdx = onePassTagNameEnd;
            //extractURL ( onePassTagNameEnd, tagEndIdx, "href", urlStartIdx, urlEndIdx );
            string newURL = extractURL ( onePassTagNameEnd, tagEndIdx, "href" );

            if ( newURL.size() != 0 && base.empty( ) )
               base = newURL; //std::string ( buffer + urlStartIdx + 1, urlEndIdx - urlStartIdx - 1 );
            }
         else if ( tagAction == DesiredAction::Embed )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) tag type = embed\n", pthread_self() );
            #endif

            //int urlStartIdx = onePassTagNameEnd;
            //int urlEndIdx = onePassTagNameEnd;
            //extractURL ( onePassTagNameEnd, tagEndIdx, "href", urlStartIdx, urlEndIdx );
            string newURL = extractURL ( onePassTagNameEnd, tagEndIdx, "href" );

            if ( newURL.size() != 0 )
               links.emplace_back ( newURL );
            }
         else if ( tagAction == DesiredAction::DiscardSection )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "* [t %ld] (HtmlParser::handleOpenTag) tag type = discard section\n", pthread_self() );
            #endif
            if ( onePassTagNameEnd - tagStartIdx - 1 < 0 )
               return length;


            return findOnePassWholeDiscardSection ( tagEndIdx + 1,
                        std::string ( buffer + tagStartIdx + 1, onePassTagNameEnd - tagStartIdx - 1 ) );
            }
         else if ( tagAction == DesiredAction::Title )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) tag type = title\n", pthread_self() );
            #endif


            inTitle = true;
            }
         else if ( tagAction == DesiredAction::OrdinaryText )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "* [t %ld] (HtmlParser::handleOpenTag) tag type = ordinary text should not exist WRONG!\n", pthread_self() );
            #endif


            ;
            }
         else
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleOpenTag) tag type = discard\n", pthread_self() );
            #endif


            ;
            }
         //else if ( tagAction == DesiredAction::Discard )
         //   ;

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::handleOpenTag) finish\n", pthread_self() );
         #endif


         return tagEndIdx + 1;
         }

      // return one pass `>` if valid tag
      // return tagStartIdx if ordinary tag
      // return tagStartIdx if reach end of file (regardless of tagname)
      int handleCloseTag ( int tagStartIdx )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
         assert ( buffer[tagStartIdx] == '<' && buffer[tagStartIdx + 1] == '/' );
         #endif

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::handleCloseTag) start, next 40 \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(),
               string ( buffer + tagStartIdx,
                     ( tagStartIdx + 40 < length ? 40 : length - tagStartIdx ) )
               .c_str() );
         #endif

         int onePassTagNameEnd = findOnePassTagNameEnd ( tagStartIdx + 2 );

         #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
         assert ( onePassTagNameEnd > 0 );
         #endif

         int tagEndIdx = findTagEnd ( onePassTagNameEnd );

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "@ [t %ld] (HtmlParser::handleCloseTag) close tagname %s\n", pthread_self(),
               string ( buffer + tagStartIdx + 2,
                     onePassTagNameEnd - tagStartIdx - 1 )
               .c_str() );
         printf ( "@ [t %ld] (HtmlParser::handleCloseTag) whole close tag %s\n", pthread_self(),
               string ( buffer + tagStartIdx, tagEndIdx - tagStartIdx + 1 ).c_str() );
         #endif

         // if at end of the file, treat `<` to length as text
         if ( tagEndIdx < 0 )
            {
            //tagAction = DesiredAction::OrdinaryText;
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "* [t %ld] (HtmlParser::handleCloseTag) reach end of file, return length\n", pthread_self() );
            #endif

            return tagStartIdx;
            }


         DesiredAction tagAction = LookupPossibleTag ( buffer + tagStartIdx + 2, buffer + onePassTagNameEnd );

         if ( tagAction == DesiredAction::Title )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleCloseTag) tag type = title\n", pthread_self() );
            #endif

            inTitle = false;
            }
         else if ( tagAction == DesiredAction::Anchor )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleCloseTag) tag type = anchor\n", pthread_self() );
            #endif

            inAnchor = false;
            }
         else if ( tagAction == DesiredAction::OrdinaryText )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "* [t %ld] (HtmlParser::handleCloseTag) tag type = text\n", pthread_self() );
            #endif

            return tagStartIdx;
            }
         else
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "@ [t %ld] (HtmlParser::handleCloseTag) tag type = discard\n", pthread_self() );
            #endif
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::handleCloseTag) finish\n", pthread_self() );
         #endif

         return tagEndIdx + 1;
         }

      // find one pass index of corresponding discard section tag
      // return length (discard whole document) if no corresponding tag found
      inline int findOnePassWholeDiscardSection ( int startIdx,
            const std::string& tagName )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findOnePassWholeDiscardSection) start, tagname %s \n", pthread_self(),
               tagName.c_str() );
         #endif

         int numCloseTagToFind = 1;

         while ( numCloseTagToFind > 0 && startIdx < length )
            {
            int potenTagNameStartIdx = findStr ( tagName.c_str( ), startIdx );

            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "# [t %ld] (HtmlParser::findOnePassWholeDiscardSection) start, next 40 \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(),
                  string ( buffer + potenTagNameStartIdx,
                        ( potenTagNameStartIdx + 40 < length ? 40 : length -
                              potenTagNameStartIdx ) ).c_str() );
            #endif

            if ( potenTagNameStartIdx < 0 )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "* [t %ld] (HtmlParser::potenTagNameStartIdx) no corr tag, return length (1)\n", pthread_self() );
               #endif

               return length;
               }

            // find one close tag
            // NOTE: do not add < checking here
            // TODO this line may lead to segfault
            if ( buffer[ potenTagNameStartIdx - 1 ] == '/'
                  && buffer[ potenTagNameStartIdx - 2 ] == '<'  )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "@ [t %ld] (HtmlParser::potenTagNameStartIdx) find corr </)\n", pthread_self() );
               #endif

               numCloseTagToFind--;
               startIdx = findTagEnd ( potenTagNameStartIdx + tagName.size( ) ) + 1;
               if ( startIdx < 0 )
                  {
                  #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
                  printf ( "* [t %ld] (HtmlParser::potenTagNameStartIdx) find tagend fail (</) %d num close tag still need\n", pthread_self(),
                        numCloseTagToFind );
                  #endif

                  return length;
                  }
               }

            // find one open tag
            else if ( buffer[ potenTagNameStartIdx - 1 ] == '<' )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "@ [t %ld] (HtmlParser::potenTagNameStartIdx) find new open tag, %d num close tag still needed\n", pthread_self(), numCloseTagToFind + 1 );
               #endif

               numCloseTagToFind++;
               startIdx = findTagEnd ( potenTagNameStartIdx + tagName.size( )  ) + 1;
               if ( startIdx < 0 )
                  {
                  #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
                  printf ( "* [t %ld] (HtmlParser::potenTagNameStartIdx) no corr tag, return length (2)\n", pthread_self() );
                  #endif

                  return length;
                  }
               }

            // find text
            else
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "@ [t %ld] (HtmlParser::potenTagNameStartIdx) find text, %d num close tag still needed\n", pthread_self(),
                     numCloseTagToFind + 1 );
               #endif

               startIdx = potenTagNameStartIdx + tagName.size( );
               }
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::potenTagNameStartIdx) finish\n", pthread_self() );
         #endif

         return startIdx;
         }

      // find the idx of next valid tag
      // return length if no valid tag found (reach end & broken last tag)
      inline int findNextValidTagStartIdx ( int startIdx )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findNextValidTagStartIdx) start from %d\n", pthread_self(), startIdx );
         #endif

         int nextPotenTagStartIdx = startIdx;
         int nextPotenTagNameStart;
         while ( nextPotenTagStartIdx < length )
            {
            nextPotenTagStartIdx = findChar ( '<', nextPotenTagStartIdx );
            // reach end and no tag found, treat whole part as text
            if ( nextPotenTagStartIdx < 0 )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "* [t %ld] (HtmlParser::findNextValidTagStartIdx) failed to find next start tag, return length\n", pthread_self() );
               #endif
               return length;
               }

            // nextPotenTagNameStart = nextPotenTagStartIdx + 1;

            if ( nextPotenTagStartIdx + 1 < length
                  && buffer[ nextPotenTagStartIdx + 1 ] == '/' )
               nextPotenTagNameStart = nextPotenTagStartIdx + 2;
            else if ( nextPotenTagStartIdx + 1 >= length )
               break;
            else
               nextPotenTagNameStart = nextPotenTagStartIdx + 1;

            if ( nextPotenTagNameStart >= length )
               break;
            int nextPotentOnePassTagNameEnd = findOnePassTagNameEnd (
                        nextPotenTagNameStart );

            #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
            assert ( nextPotentOnePassTagNameEnd > 0 );
            #endif

            DesiredAction action = LookupPossibleTag ( buffer + nextPotenTagNameStart,
                        buffer + nextPotentOnePassTagNameEnd );

            if ( action != DesiredAction::OrdinaryText )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "* [t %ld] (HtmlParser::findNextValidTagStartIdx) success\n", pthread_self() );
               #endif

               return nextPotenTagStartIdx;
               }

            nextPotenTagStartIdx = nextPotentOnePassTagNameEnd;
            }

         // reach end and deprecated last tag, handle as text
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::findNextValidTagStartIdx) failed, return length (2)\n", pthread_self() );
         #endif

         return length;
         }

      // return idx of tag end `>` with more corner case handling
      //    by iterating to find pair of attributes. notice is this
      //    is more fault tolerence but takes more time to run
      // return -1 if unable to find tag end
      // e.g. `Email <a href="mailto:Nicole Hamilton <hamilton@hamiltonlabs.com>">hamilton@hamiltonlabs.com</a>`
      inline int findTagEnd ( int startIdx )
         {
         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findTagEnd) start\n", pthread_self() );
         #endif

         while ( startIdx < length )
            {
            if ( buffer[ startIdx ] == '"' )
               {
               int paramEnd = findChar ( '"', startIdx + 1 );
               if ( paramEnd < 0 )
                  return -1;
               startIdx = paramEnd + 1;
               }
            else if ( buffer[ startIdx ] == '>' )
               return startIdx;
            else
               startIdx++;
            }

         return -1;
         }

      // return one pass the last index of tagname
      // return length if finding failed (to let the caller function in charge of handling the rest)
      //    tag name immediately follow < or </ (funciton do not check this)
      //    tag name terminate by whitespace, >, or /
      // e.g.
      //    <tagname a=b> return the index of white space between `tagnema` and `a`
      //    </tagname a=b> return the index of white space between `tagnema` and `a`
      inline int findOnePassTagNameEnd ( int tagNameStartIdx )
         {

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findOnePassTagNameEnd) start\n", pthread_self() );
         #endif

         while ( tagNameStartIdx < length )
            {
            if ( buffer[ tagNameStartIdx] == ' ' || buffer[ tagNameStartIdx] == '>'
                  || buffer[ tagNameStartIdx] == '/' || buffer[ tagNameStartIdx] == '\n'
                  || buffer[ tagNameStartIdx] == '\t' || buffer[ tagNameStartIdx ] == '\r' )
               {
               #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
               printf ( "* [t %ld] (HtmlParser::findOnePassTagNameEnd) success\n", pthread_self() );
               #endif

               return tagNameStartIdx;
               }
            tagNameStartIdx++;
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::findOnePassTagNameEnd) failed, return length\n", pthread_self() );
         #endif

         return length;
         }


      // find first occurence of str from [start, length)
      // return -1 if not found
      // str: string to find
      inline int findStr ( const char* str, int startIdx )
         {

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findStr) start\n", pthread_self() );
         #endif

         while ( startIdx < length )
            {
            if ( buffer[ startIdx ] == *str )
               {
               const char* curr = str + 1;
               int idx = startIdx + 1;

               while ( buffer[ idx ] == *curr && *curr )
                  {
                  idx++;
                  curr++;
                  }

               if ( *curr == '\0' )
                  {
                  #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
                  printf ( "* [t %ld] (HtmlParser::findStr) success found\n", pthread_self() );
                  #endif

                  return startIdx;
                  }
               startIdx = idx;
               }
            else
               startIdx++;
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::findStr) not found\n", pthread_self() );
         #endif

         return -1;
         }

      // find first occurence of character `c` starting from [startIdx, length)
      // return -1 if character not found
      inline int findChar ( char c, int startIdx )
         {
         while ( startIdx < length )
            {
            if ( buffer[startIdx] == c )
               return startIdx;
            startIdx++;
            }

         return -1;
         }

      // extract url from [start, end)
      //    return std::string of url if found
      //    return empty string if no url attribute found / finding process failed
      //    attributeName is the attribute name (href / src) to search url
      // TODO can still optimize `const std::string& attributeName` to avoid calling constructor here
      // TODO change header here
      //inline void extractURL ( int startIdx, int endIdx, const std::string& attributeName, int& urlStartIdx, int& urlEndIdx )


      string extractURL ( int startIdx, int endIdx, const std::string& attributeName )
         {

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::extractURL) start, attribute %s, in [%d, %d] %s\n", pthread_self(), attributeName.c_str(), startIdx, endIdx,
               string ( buffer + startIdx, endIdx - startIdx ).c_str() );
         #endif

         int attributeStartIdx = findStr ( attributeName.c_str( ), startIdx );
         if ( attributeStartIdx < 0 || attributeStartIdx >= endIdx )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "E [t %ld] (HtmlParser::extractURL) attr not find or out of range\n", pthread_self() );
            #endif

            //urlStartIdx = -1;
            return string();
            }

         // start, end of url
         // e.g. href = "url"
         int urlStartIdx = findChar ( '"', attributeStartIdx + attributeName.size( ) );
         if ( urlStartIdx < 0 || urlStartIdx >= endIdx )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "E [t %ld] (HtmlParser::extractURL) first ' not find or out of rang\n", pthread_self() );
            #endif

            //urlStartIdx = -1;
            return string();
            }

         int urlEndIdx = findChar ( '"', urlStartIdx + 1 );
         if ( urlEndIdx < 0 || urlStartIdx >= endIdx )
            {
            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "E [t %ld] (HtmlParser::extractURL) second ' not find or out of rang\n", pthread_self() );
            #endif

            //urlEndIdx = -1;
            return string();
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
         assert ( urlEndIdx >= urlStartIdx + 1 );
         #endif


         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "* [t %ld] (HtmlParser::extractURL) end\n", pthread_self() );
         #endif

         return string ( buffer + urlStartIdx + 1, urlEndIdx - urlStartIdx - 1 );
         }


   public:
      // The constructor is given a buffer and length containing
      // presumed HTML.  It will parse the buffer, stripping out
      // all the HTML tags and producing the list of words in body,
      // words in title, and links found on the page.

      // NOTE: HtmlParser may change the input uffer, this help faster the speed
      HtmlParser ( const char* buffer, size_t length )
         : inTitle ( false ), inAnchor ( false ), buffer ( ( char* ) buffer ), length ( length )
         {

         }

      HtmlParser ( string& htmlContent )
         : inTitle ( false ), inAnchor ( false ), buffer ( ( char* ) htmlContent.c_str() ),
           length ( htmlContent.size() )
         {
         }

      // main parse function
      void parse()
         {
         int nextTagStartIdx;
         int onePassTagEndIdx;
         int idx = 0;
         int count = 0;
         while ( idx < length )
            {
            count++;

            if ( count == 200 )
               {
               printf ( "@ [t %ld] (HtmlParser::parse) parsing... count %d\n", pthread_self(), count );
               count = 0;
               }

            #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
            printf ( "# [t %ld] (HtmlParser::parse) current idx %d (status %s) next 40 \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", pthread_self(),
                  idx, inTitle ? "in title" : ( inAnchor ? "in anchor" : "text" ),
                  string ( buffer + idx, ( idx + 40 < length ? 40 : length - idx ) ).c_str() );
            printf ( "# [t %ld] (HtmlParser::parse) count %d\n", pthread_self(), count );
            #endif

            nextTagStartIdx = findNextValidTagStartIdx ( idx );
            //std::cout << "length: " << length << "nextTagS: " << nextTagStartIdx << "idx: " << idx << std::endl;

            #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
            assert ( nextTagStartIdx >= idx );
            #endif

            handleText ( idx, nextTagStartIdx );

            idx = nextTagStartIdx;
            if ( idx >= length )
               return;

            if ( idx + 1 < length && buffer[ idx + 1 ] == '/' )
               onePassTagEndIdx = handleCloseTag ( idx );
            else
               onePassTagEndIdx = handleOpenTag ( idx );

            #ifdef _UMSE_CRAWLER_HTMLPARSER_ASSERT
            assert ( onePassTagEndIdx > 0 );
            #endif

            //std::cout << "onePassTag: " << onePassTagEndIdx << std::endl;
            //std::cout << "word: " << words.size() << "titile: " << titleWords.size() << "links: " << links.size() << "length" << length << std::endl;
            idx = onePassTagEndIdx;
            if ( idx >= length )
               break;
            }

         #ifdef _UMSE_CRAWLER_HTMLPARSER_LOG
         printf ( "# [t %ld] (HtmlParser::findNextValidTagStartIdx) parse end\n", pthread_self() );
         #endif
         }

   };
