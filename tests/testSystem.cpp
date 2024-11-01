//
//  Build Index from files/ directory and test with ISR
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
            ISRWord foxISR(fox);
            cout << "First Word:" << endl << endl;
            while(quickISR.Next())
                cout << quickISR.getStartLocation() << endl;
            cout << "Second Word:" << endl << endl;
            while(brownISR.Next())
                cout << brownISR.getStartLocation() << endl;
            cout << "Third Word:" << endl << endl;
            while(foxISR.Next())
                cout << foxISR.getStartLocation() << endl;
        }

        // Fail on urlIndex = 0
        void TestISREndDoc()
        {
            cout << "-------------Test EndDoc--------------" << endl;
            ISREndDoc endDocISR(docEnd);
            while(endDocISR.Next()){
                cout << "Location:" << endDocISR.getStartLocation() << endl;
                cout << "Url Index:" << endDocISR.getUrlIndex() << endl;
                cout << "doc length:" << endDocISR.getDocLength() << endl;
            }
            //Test Seek
            ISREndDoc endDocISR2(docEnd);
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
            ISREndDoc endDocISR1(docEnd);
            ISREndDoc endDocISR2(docEnd);
            ISR* quickOrBrown[2];
            quickOrBrown[0] = &quickISR;
            quickOrBrown[1] = &brownISR;
            ISRAnd andISR(quickOrBrown, &endDocISR1, 2);
            ISR* orArray[1];
            orArray[0] = &andISR;
            ISROr orISR(orArray, &endDocISR2, 1);
            while(orISR.NextDocument()){
                //cout << "document: " << andISR.getDocumentEnd()->getStartLocation() << endl;
                cout << "url: " << documentData[orISR.getDocumentEnd()->getUrlIndex()].URL << endl;
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
                //cout << "document: " << phraseISR.getDocumentEnd()->getStartLocation() << endl;
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
                cout << "url: " << documentData[andISR.getDocumentEnd()->getUrlIndex()].URL << endl;
            }
        }
        void TestAll()
        {
            cout << "TEST START" << endl << endl;;
            // TestISRWord();
            // TestISREndDoc();
            // TestISROr();
             TestISRAnd();
            // TestISRPhrase();
            // Test3And();
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
      cout << "numDocs: " << documentData.size() << endl;
      const HashBlob *hashblob = hashfile.Blob();
      const SerialTuple* docValue = hashblob->Find("##EndDoc");
      const SerialTuple* quickValue = hashblob->Find("quick");
      const SerialTuple* brownValue = hashblob->Find("brown");
      const SerialTuple* foxValue = hashblob->Find("fox");
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
   }

}

int main(){
   vector<string> inputs = {"hashfile2.bin"};
   DictToMem(inputs);
}
