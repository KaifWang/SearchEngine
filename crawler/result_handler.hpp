//
// result_handler.hpp
//
// University of Michigan - Ann Arbor, EECS 440
// Copyright @ 2021 mark. All rights reserved.
//
// Xiao Song xiaosx@umich.edu
//
// Handle the parsed result
//
#pragma once
#include <pthread.h> // concurrent
#include <unistd.h>                // write, read, open
#include <sys/stat.h>
#include <fcntl.h>                 // O_RDONLY
#include "url_parser.hpp" // ParsedUrl
#include "configs/crawler_cfg.hpp" // configuration

class HtmlParser;

void reverse(char str[], int length) 
   {
      int start = 0;
      int end = length - 1;
      while (start < end) 
         {
         char tmp = * (str + start);
         str[start] = str[end];
         str[end] = tmp;
         start++;
         end--;
         }
   }
char * itoa(int num)
   {  
      char str[100];
      int i = 0;
      if (num == 0)
         {
            str[i++] = '0';
            str[i] = '\0';
            return str;
         }
      while ( num != 0 )
         {
            int rem = num % 10;
            str[i++] = ( rem > 9 ) ? ( rem - 10 ) + 'a' : rem  + '0';
            num = num / 10;
         }
      str[i] = '\0';
      reverse(str, i);
      return str;
   }

class ResultHandler
   {
   public:
      int saveHtmlResultToFile ( const string& content, const char* url, const string& output_dir )
         {
         #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
         printf ( "# [t %ld] (ResultHandler::saveHtmlResultToFile) TRY TO SAVE HTML %s RESULT\n", pthread_self(), url );
         #endif

         /* (1) get the path&filename to store the parsed result */
         ParsedUrl parsedUrl ( url );
         string filename ( output_dir ); // output folder
         filename += string ( parsedUrl.Service );
         filename += "@";
         filename += string ( parsedUrl.Host );
         filename += "@";
         filename += string ( parsedUrl.Port );
         filename += "@";
         for ( size_t i = 0; i < strlen ( parsedUrl.Path ); ++i )
            {
            if ( parsedUrl.Path[i] == '/' || parsedUrl.Path[i] == '~' )
               parsedUrl.Path[i] = '&';
            }
         filename += string ( parsedUrl.Path );
         filename += ".html";

         int file_d = open(filename.c_str(), O_CREAT | O_RDWR, S_IRWXO);
         if ( file_d == -1 )
            {
            #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
            printf ( "E [t %ld] (ResultHandler::saveHtmlResultToFile) open file %s failed\n", pthread_self(), filename.c_str()  );
            #endif

            /* (4) return error status code */
            return -1;
            }

         write(file_d, content.c_str(), sizeof(content.c_str()) - 1);
         close(file_d);

         #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
         printf ( "* [t %ld] (ResultHandler::saveHtmlResultToFile) SUCCESS SAVE %s TO %s\n", pthread_self(), url, filename.c_str() );
         #endif
         return 1;
         }

      // save parsed result to file
      // return -1 if saved to file failed
      // return 1 if saved to file success
      int saveParseResultToFile ( const HtmlParser& parse, const char* url, const string& output_dir )
         {
         #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
         printf ( "# [t %ld] (ResultHandler::saveParseResultToFile) TRY TO SAVE %s PARSED RESULT\n", pthread_self(), url );
         #endif

         /* (1) get the path&filename to store the parsed result */
         ParsedUrl parsedUrl ( url );
         string filename ( output_dir ); // output folder
         filename += string ( parsedUrl.Service );
         filename += "@";
         filename += string ( parsedUrl.Host );
         filename += "@";
         filename += string ( parsedUrl.Port );
         filename += "@";
         for ( size_t i = 0; i < strlen ( parsedUrl.Path ); ++i )
            {
            if ( parsedUrl.Path[i] == '/' || parsedUrl.Path[i] == '~' )
               parsedUrl.Path[i] = '&';
            }
         filename += string ( parsedUrl.Path );
         filename += ".txt";

         // TODO : should replace below file stream implementation with mmap
         /* (2) try to open file */
         int file_d = open(filename.c_str(), O_CREAT | O_RDWR, S_IRWXO);
         if ( file_d == -1 )
            {
            #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
            printf ( "E [t %ld] (ResultHandler::saveParseResultToFile) open file %s failed\n", pthread_self(), filename.c_str()  );
            #endif

            /* (4) return error status code */
            return -1;
            }

         /* (3) write content to file */
         // 3.1 write current url
         string newline = "\n";
         write(file_d, url, strlen(url) - 1);
         lseek(file_d, 0, SEEK_CUR);
         write(file_d, newline.c_str(), 1);

         // 3.2 number of words
         lseek(file_d, 0, SEEK_CUR);
         char * sizeTl = itoa(parse.titleWords.size());
         write(file_d, sizeTl, strlen(sizeTl) - 1);
         lseek(file_d, 0, SEEK_CUR);
         write(file_d, newline.c_str(), 1);
         // number of title
         lseek(file_d, 0, SEEK_CUR);
         char * sizeW = itoa(parse.titleWords.size());
         write(file_d, sizeW, strlen(sizeW) - 1);
         lseek(file_d, 0, SEEK_CUR);
         write(file_d, newline.c_str(), 1);

         // 3.4 write title
         for ( auto titleWord : parse.titleWords )
            {
            string hashtag = "#";
            lseek(file_d, 0, SEEK_CUR);
            write(file_d, hashtag.c_str(), 1);
            lseek(file_d, 0, SEEK_CUR);
            write(file_d, titleWord.c_str(), strlen(titleWord.c_str()) - 1 );
            lseek(file_d, 0, SEEK_CUR);
            write(file_d, newline.c_str(), 1);
            }

         // 3.3 write words
         for ( auto word : parse.words )
            {
            lseek(file_d, 0, SEEK_CUR);
            write(file_d, word.c_str(), strlen(word.c_str()) - 1 );
            lseek(file_d, 0, SEEK_CUR);
            write(file_d, newline.c_str(), 1);
            }

         lseek(file_d, 0, SEEK_CUR);
         write(file_d, newline.c_str(), 1);
         /* (4) return ok status code */
         close(file_d);

         #ifdef _UMSE_CRAWLER_RESULT_HANDLER_LOG
         printf ( "* [t %ld] (ResultHandler::saveParseResultToFile) SUCCESS SAVE PARSED RESULT %s TO %s\n", pthread_self(), url, filename.c_str() );
         #endif

         return 1;
         }
   };