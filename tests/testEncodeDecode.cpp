#include "../ranker/ranker.h"

static char* getNextToken(const char* &itr)
{
      const char* token_start = itr;
      while(*itr && *itr != DOCUMENT_MESSAGE_DELIMITER)
         itr ++;
      char* token = new char[ itr - token_start + 1 ];
      memset( token, 0, itr - token_start + 1 );
      strncpy( token, token_start, itr - token_start );
      itr ++;
      return token;
} 

// Define message format to send top Documents back to the front end
// Vector<DocumentScore>
// numDoc#url#title#abstract#score#url#tiltl#abstract#score ...
static vector<DocumentScore> decodeMsg(string msg, size_t &seqNum)
{
   const char* itr = msg.c_str();
   // Get sequence number
   seqNum = atoi(getNextToken(itr));
   //Read num of documents
   size_t numDoc = atoi(getNextToken(itr));
   vector<DocumentScore> docScores(numDoc);
   for(size_t i = 0 ; i < numDoc; i ++)
   {
      char* title = getNextToken(itr);
      char* abstract = getNextToken(itr);
      char* url = getNextToken(itr);
      char* scoreStream = getNextToken(itr);
      size_t score = atoi(scoreStream);
      DocumentScore docScore{string(title), string(abstract), string(url), score};
      docScores[i] = docScore;
      delete title;
      delete abstract;
      delete url;
      delete scoreStream;
   } 
   return docScores;
}
// Define message format to send top Documents back to the front end
// Vector<DocumentScore>
// seqNum@numDoc@url@title@abstract@score@url@tiltl@abstract@score ...
static string encodeMsg(vector<DocumentScore> &docScores, size_t seqNum)
{
   // Encode seq number
   char seqBuf[10];
   memset(seqBuf, 0, sizeof(seqBuf));
   sprintf(seqBuf, "%lu", seqNum);
   string msg( seqBuf );

   // Encode numDocs
   char numDocBuf[10];
   memset(numDocBuf, 0, sizeof(numDocBuf));
   sprintf(numDocBuf, "%lu", docScores.size());
   msg += DOCUMENT_MESSAGE_DELIMITER;
   msg += string(numDocBuf);

   for( size_t i = 0 ; i < docScores.size() ; i++ )
   {
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[i].title;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[i].abstract;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      msg += docScores[i].URL;
      msg += DOCUMENT_MESSAGE_DELIMITER;
      char scoreBuf[10];
      memset( scoreBuf, 0, sizeof(scoreBuf) );
      sprintf(scoreBuf, "%lu", docScores[i].score);
      msg += string(scoreBuf);
   }
   return msg;
}

int main()
{
    DocumentScore docScore1{string("title1"), string("abstract1"), string("url1"), 5};
    DocumentScore docScore2{string("title2"), string("abstract2"), string("url2"), 6};
    DocumentScore docScore3{string("title3"), string("abstract3"), string("url3"), 7};
    vector<DocumentScore> docScores;
    docScores.push_back(docScore1);
    docScores.push_back(docScore2);
    docScores.push_back(docScore3);
    string msg = encodeMsg(docScores, 5);
    cout << "encoded msg: " << msg << endl;
    size_t seqNum = 0;
    vector<DocumentScore> decoded = decodeMsg(msg, seqNum);
    cout << "decoded:" << endl;
    cout << decoded.size() << endl;
    cout << "seqNum "  << seqNum << endl;
    for(size_t i = 0; i < decoded.size(); i++)
    {
        cout << decoded[i].title << endl;
    }
 }