#include <stdio.h>
#include <iostream>
#include <string>
#include "DeltaConverter.h"
#include "index.h"
#include "HashTable.h"
#include "isr.h"
#include "ranker.h"
using namespace std;

class RankerTest
{
    private:
        const PostingList* w1;
        const PostingList* w2;
        const PostingList* w3;
        const PostingList* w4;
        const PostingList* docEnd;
        HashFile indexChunk;
        vector<string> searchWords;
        vector<DocumentData> documentData;
    public:
        RankerTest(const PostingList* _w1, const PostingList* _w2, 
        const PostingList* _w3, const PostingList* _w4, const PostingList* _docEnd, HashFile _indexChunk,
        vector<string> _searchWords, vector<DocumentData> _documentData)
         : w1(_w1), w2(_w2), w3(_w3), w4(_w4), docEnd(_docEnd), indexChunk(_indexChunk),
         searchWords(_searchWords), documentData(_documentData)
        {
        }
        ~RankerTest()
        {
        }
        
        void TestISRWord()
        {
            cout << "-------------Test Word--------------" << endl;
            ISRWord isr1(w1);
            while(isr1.Next())
                cout << isr1.getStartLocation() << endl;
        }

        // Fail on urlIndex = 0
        void TestISREndDoc()
        {
            cout << "-------------Test EndDoc--------------" << endl;
            ISREndDoc endDocISR(docEnd);
            while(endDocISR.Next()){
                cout << "Location:" << endDocISR.getStartLocation() << endl;
                cout << "doc length:" << endDocISR.getDocLength() << endl;
                cout << "Url: " << documentData[endDocISR.getUrlIndex()].URL << endl;
            }
        }

        void findMatchingDocuments()
        {
            cout << "-------------Find maching Documents--------------" << endl;
            ISRWord isr1(w1);
            ISRWord isr2(w2);
            ISRWord isr3(w3);
            ISRWord isr4(w4);
            ISREndDoc endDocISR(docEnd);
            ISR* andArray[4];
            andArray[0] = &isr1;
            andArray[1] = &isr2;
            andArray[2] = &isr3;
            andArray[3] = &isr4;
            ISRAnd andISR(andArray, &endDocISR, 4 );
            ISR* orArray[1];
            orArray[0] = &andISR;
            ISROr orISR(orArray, &endDocISR, 1);
            while(orISR.NextDocument()){
                cout << "url: " << documentData[orISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }
        void Rank()
        {
            cout << "-------------Test Ranker--------------" << endl;
            ISRWord isr1(w1);
            ISRWord isr2(w2);
            ISRWord isr3(w3);
            ISRWord isr4(w4);
            ISREndDoc endDocISR(docEnd);
            ISR* andArray[4];
            andArray[0] = &isr1;
            andArray[1] = &isr2;
            andArray[2] = &isr3;
            andArray[3] = &isr4;
            ISRAnd andISR(andArray, &endDocISR, 4 );
            ISR* orArray[1];
            orArray[0] = &andISR;
            ISROr orISR(orArray, &endDocISR, 1);
            vector<DocumentScore> top10(10);
            RankDocuments(indexChunk, &orISR, searchWords, top10);
            cout << endl << "--------------Result----------------" << endl << endl;
            for(size_t i = 0; i < top10.size(); i++)
            {
                cout << top10[i].url << endl;
                cout << top10[i].score << endl;
            }
        }

};




// map a hashFile representing dictionary back to working memory for Finding token
void DictToMem( vector<string> &fileNames )
{
   for(string fileName : fileNames)
   {
      HashFile hashfile(fileName.c_str());
      vector<DocumentData> documentData = hashfile.get_DocumentData();
      const HashBlob *hashblob = hashfile.Blob();
      cout << "numDocs: " << documentData.size() << endl;
      cout << "numWords: " << hashblob->NumberOfUniqueWords << endl;
      cout << "numPosts: " << hashblob->NumberOfPosts << endl;
      vector<string> searchWords = {"is", "american", "next", "insurgency"};
      const SerialTuple* docValue = hashblob->Find("##EndDoc");
      const SerialTuple* w1Value = hashblob->Find("is");
      const SerialTuple* w2Value = hashblob->Find("american");
      const SerialTuple* w3Value = hashblob->Find("next");
      const SerialTuple* w4Value = hashblob->Find("insurgency");
      const PostingList* docPL = docValue ? docValue->GetPostingListValue() : nullptr;
      const PostingList* w1PL = w1Value ? w1Value->GetPostingListValue() : nullptr;
      const PostingList* w2PL = w2Value ? w2Value->GetPostingListValue() : nullptr;
      const PostingList* w3PL = w3Value ? w3Value->GetPostingListValue() : nullptr;
      const PostingList* w4PL = w4Value ? w4Value->GetPostingListValue() : nullptr;
      RankerTest rankTest(w1PL, w2PL, w3PL, w4PL, docPL, hashfile, searchWords, documentData);
      rankTest.Rank();
      //vector<ByteStruct> post = PL->GetPostValue();
   }

}

int main(){
    vector<string> inputs = {"hashfile1.bin"};
//  vector<string> inputs = {"hashfile1.bin", "hashfile2.bin", "hashfile3.bin", "hashfile4.bin", "hashfile5.bin"};
   DictToMem(inputs);
}