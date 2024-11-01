//
// Test ISR by manually creating PostingLists
// Depricated(PostingList constructor )
// Created by Kai Wang (kaifan@umich.edu)
//

#include "isr.h"

// Each DocEnd post has (delta, urlIndex)


class ISRTest
{
    private:
        PostingList* quick;
        PostingList* brown;
        PostingList* fox;
        PostingList* docEnd;
    public:
        ISRTest(vector<size_t> _quick, vector<size_t> _brown, vector<size_t> _fox, vector<size_t> _docEnd)
        {
            vector<ByteStruct> quickEncoded = EncodeVector(_quick);
            vector<ByteStruct> brownEncoded = EncodeVector(_brown);
            vector<ByteStruct> foxEncoded = EncodeVector(_fox);
            vector<ByteStruct> docEndEncoded = EncodeVector(_docEnd);
            quick = new PostingList(quickEncoded);
            brown = new PostingList(brownEncoded);
            fox = new PostingList(foxEncoded);
            docEnd = new PostingList(docEndEncoded);
        }
        ~ISRTest()
        {
            delete quick;
            delete brown;
            delete fox;
            delete docEnd;
        }
        
        void TestISRWord()
        {
            cout << "-------------Test Word--------------" << endl;
            ISRWord quickISR(quick);
            ISRWord brownISR(brown);
            ISRWord foxISR(fox);
            while(quickISR.Next())
                cout << quickISR.getStartLocation() << endl;
            while(brownISR.Next())
                cout << brownISR.getStartLocation() << endl;
            while(foxISR.Next())
                cout << foxISR.getStartLocation() << endl;
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
                cout << "Url Index:" << endDocISR.getUrlIndex() << endl;
                cout << "doc length:" << endDocISR.getDocLength() << endl;
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
                //cout << orISR.getStartLocation() << endl;
                cout << "document: " << endDocISR.getStartLocation() << endl;
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
                cout << "document: " << andISR.getDocumentEnd()->getStartLocation()<< endl;
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
                cout << "document: " << phraseISR.getDocumentEnd()->getStartLocation()<< endl;
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
            }
        }
        void TestAll()
        {
            cout << "___________________TEST START_________________________" << endl;
            // TestISRWord();
            // TestISREndDoc();
            TestISROr();
            TestISRAnd();
            TestISRPhrase();
            Test3And();
        }
};

int main( )
    {

        // Testcase 1
        // Actual location
        // quick  15  27    105
        // fox    87  104   515
        // DocEnd 10  100   200

        // Posting List
        // quick  15   12   78
        // fox    87   17   411
        // DocEnd 10   1    90   2  100   3
        // vector<size_t> quick{15, 12, 78};
        // vector<size_t> fox{87, 17, 411};
        // vector<size_t> docEnd{10, 1, 90, 2, 100, 3};
        // ISRTest test1(quick, fox, docEnd);
        // test1.TestAll();



        // TestCase 2
        // Actual location
        // quick 10   27   105          513   518  520                           1820
        // brown 28   50   62   70      514                 790
        // fox   87   106               515   550                     1200       1900
        // #DocEnd                 112                  570     1006        1704           2004

        // Posting List
        // quick 10   17   78   408   5  2   1300
        // brown 28   22   12   8   444   276
        // fox   87   19   409   35   650   700
        // #DocEnd 112  1  458  2  436  3  698  4  300  5

        // TestCase 3
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

        vector<size_t> quick2{1, 2, 8, 3, 4, 1, 7};
        vector<size_t> brown2{4, 1, 1, 1, 8, 7};
        vector<size_t> fox2{9, 3, 4, 4, 4, 4};
        vector<size_t> docEnd2{13, 1, 8, 2, 2, 3, 2, 4, 4, 5};
        ISRTest test2(quick2, fox2, brown2, docEnd2);
        // ISRTest test3(quick2, brown2, fox2, docEnd2);
        // ISRTest test4(brown2, fox2, quick2, docEnd2);
        test2.TestAll();
        // test3.TestAll();
        // test4.TestAll();
    }
