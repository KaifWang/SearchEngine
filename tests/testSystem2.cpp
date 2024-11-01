//
//  Build Index from sample_test/ directory and test with ISR
//
//  Created by Kai Wang(kaifan@umich.edu).
//

#include <stdio.h>
#include <iostream>
#include "DeltaConverter.h"
#include "index.h"
#include "HashTable.h"
#include "isr.h"
using namespace std;
class ISRTest
{
    private:
        const PostingList* quick;
        const PostingList* brown;
        const PostingList* fox;
        const PostingList* docEnd;
        vector<DocumentData> documentData;
    public:
        ISRTest(const PostingList* _quick, const PostingList* _brown, 
        const PostingList* _fox, const PostingList* _docEnd, vector<DocumentData> _documentData)
         : quick(_quick), brown(_brown), fox(_fox), docEnd(_docEnd), documentData(_documentData)
        {
        }
        ~ISRTest()
        {
        }
        
        void TestISRWord()
        {
            cout << "-------------Test Word--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            // ISRWord foxISR(fox);
            cout << "First Word:" << endl << endl;
            while(quickISR.Next())
                cout << quickISR.getStartLocation() << endl;
            cout << "Second Word:" << endl << endl;
            while(brownISR.Next())
                cout << brownISR.getStartLocation() << endl;
            // while(foxISR.Next())
            //     cout << foxISR.getStartLocation() << endl;
            //ISRWord quickISR2(quick);
            //Test Seek
            // cout << "Test Seek" << endl;
            // quickISR2.Seek(500);
            // cout << quickISR2.getStartLocation() << endl;
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
            //Test Seek
            ISREndDoc endDocISR2(docEnd);
            // endDocISR2.Seek(160);
            // cout << "Test SEEK" << endl;
            // cout << "Location:" << endDocISR2.getStartLocation() << endl;
            // cout << "Url Index:" << endDocISR2.getUrlIndex() << endl;
            // cout << "doc length:" << endDocISR2.getDocLength() << endl;
        }

        void TestISROr()
        {
            cout << "-------------Test Or--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISREndDoc endDocISR(docEnd);
            ISR* quickOrBrown[2];
            quickOrBrown[0] = &quickISR;
            quickOrBrown[1] = &brownISR;
            ISROr orISR(quickOrBrown, &endDocISR, 2);
            while(orISR.NextDocument()){
                //cout << "document: " << orISR.getDocumentEnd()->getStartLocation() << endl;
                cout << "url: " << documentData[orISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
               
        }

        void TestISRAnd()
        {
            cout << "-------------Test AND--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISREndDoc endDocISR(docEnd);
            ISR* quickOrBrown[2];
            quickOrBrown[0] = &quickISR;
            quickOrBrown[1] = &brownISR;
            ISRAnd andISR(quickOrBrown, &endDocISR, 2);
            while(andISR.NextDocument()){
                //cout << "document: " << andISR.getDocumentEnd()->getStartLocation() << endl;
                cout << "url: " << documentData[andISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }

        void TestISRPhrase()
        {
            cout << "-------------Test Phrase--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISREndDoc endDocISR(docEnd);
            ISR* quickOrBrown[2];
            quickOrBrown[0] = &quickISR;
            quickOrBrown[1] = &brownISR;
            ISRPhrase phraseISR(quickOrBrown, &endDocISR, 2);
            while(phraseISR.NextDocument()){
                cout << "first at : " <<phraseISR.terms[0]->getStartLocation() << endl;
                cout << "second at : " <<phraseISR.terms[1]->getStartLocation() << endl;
                cout << "documentLength: " << phraseISR.getDocumentEnd()->getDocLength() << endl;
                cout << "document start: " << phraseISR.getDocumentEnd()->getStartLocation() - phraseISR.getDocumentEnd()->getDocLength() << endl;
                cout << "url: " << documentData[phraseISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }

        void Test3ISRPhrase()
        {
            cout << "-------------Test Phrase--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISRWord foxISR(fox);
            ISREndDoc endDocISR(docEnd);
            ISR* phrase[3];
            phrase[0] = &quickISR;
            phrase[1] = &brownISR;
            phrase[2] = &foxISR;
            ISRPhrase phraseISR(phrase, &endDocISR, 3);
            while(phraseISR.NextDocument()){
                cout << "first at : " <<phraseISR.terms[0]->getStartLocation() << endl;
                cout << "second at : " <<phraseISR.terms[1]->getStartLocation() << endl;
                cout << "documentLength: " << phraseISR.getDocumentEnd()->getDocLength() << endl;
                cout << "document start: " << phraseISR.getDocumentEnd()->getStartLocation() - phraseISR.getDocumentEnd()->getDocLength() << endl;
                cout << "url: " << documentData[phraseISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }

        void Test3And()
        {
            cout << "-------------Test AND 3 Terms--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISRWord foxISR(fox);
            ISREndDoc endDocISR(docEnd);
            ISR* quickBrownFox[3];
            quickBrownFox[0] = &quickISR;
            quickBrownFox[1] = &brownISR;
            quickBrownFox[2] = &foxISR;
            ISRAnd andISR(quickBrownFox, &endDocISR, 3);
            while(andISR.NextDocument()){
                cout << "document: " << andISR.getDocumentEnd()->getStartLocation()<< endl;
                cout << "url: " << documentData[andISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }
        void TestAll()
        {
            cout << "TEST START" << endl << endl;;
            // for(DocumentData data : documentData){
            //   cout << data.URL << endl;
            // }
            //TestISRWord();
            //TestISREndDoc();
            // TestISROr();
            // TestISRAnd();
            //TestISRPhrase();
            Test3ISRPhrase();
            //Test3And();
            cout << endl << endl;
        }
};

// map a hashFile representing dictionary back to working memory for Finding token
void DictToMem( vector<string> &fileNames )
{
   // TestCase 2
   // Actual location
   // quick 1   3   11         14   18  19                        26
   // brown 4   5   6   7      15                 22
   // fox   9   12             16   20                   24       28
   // #DocEnd             13                  21     23      25      29

   // Posting List
   // quick 1   2   8   3   4   1   7
   // brown 4   1   1   1   8   7
   // fox   9   3   4   4   4   4
   // #DocEnd 13  1  8  2  2  3  2  4  4  5 
   // sanity check
   for(string fileName : fileNames)
   {
      HashFile hashfile(fileName.c_str());
      vector<DocumentData> documentData = hashfile.get_DocumentData();
      const HashBlob *hashblob = hashfile.Blob();
     cout << "numDocs: " << documentData.size() << endl;
      cout << "numWords: " << hashblob->NumberOfUniqueWords << endl;
      cout << "numPosts: " << hashblob->NumberOfPosts << endl;
      const SerialTuple* docValue = hashblob->Find("##EndDoc");
      const SerialTuple* quickValue = hashblob->Find("food");
      const SerialTuple* brownValue = hashblob->Find("and");
      const SerialTuple* foxValue = hashblob->Find("drink");
      const PostingList* docPL = docValue ? docValue->GetPostingListValue() : nullptr;
      const PostingList* quickPL = quickValue ? quickValue->GetPostingListValue() : nullptr;
      const PostingList* brownPL = brownValue ? brownValue->GetPostingListValue() : nullptr;
      const PostingList* foxPL = foxValue ? foxValue->GetPostingListValue() : nullptr;
      ISRTest test1(quickPL, brownPL, foxPL, docPL, documentData);
      ISRTest test2(quickPL, foxPL, brownPL, docPL, documentData);
      ISRTest test3(brownPL, foxPL, quickPL, docPL, documentData);
      cout << "-------------quick and brown Testing------------------" << endl;
      test1.TestAll();
      cout << "-------------quick and fox Testing------------------" << endl;
      test2.TestAll();
      cout << "-------------brown and fox Testing------------------" << endl;
      test3.TestAll();
      //vector<ByteStruct> post = PL->GetPostValue();
   }

}

int main(){
    vector<string> inputs = {"hashfile3.bin"};
//  vector<string> inputs = {"hashfile1.bin", "hashfile2.bin", "hashfile3.bin", "hashfile4.bin", "hashfile5.bin"};
   DictToMem(inputs);
}
