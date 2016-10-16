#include <math.h>
#include <vector>
#include <iostream>
#include <iomanip>

typedef unsigned int WORD;
/**
 * Number of bits per word, i.e. per unsigned integer, since the
 * bitmap will represented by an array of unsigned integers
 */
const unsigned int BITSPERWORD = sizeof(WORD)*8;

inline double log2(double x) {
        return log(x)/log(2);
}

const unsigned int SHIFT = static_cast<unsigned int>(ceil(log2(BITSPERWORD)));

/**
 * Returns the number of words needed to stored a bitmap
 * of size no_bits bits
 */
inline unsigned int numOfwords(unsigned int no_bits) {
	//return no_bits/BITSPERWORD + 1;
	return static_cast<unsigned int>(ceil(double(no_bits)/double(BITSPERWORD)));
}

inline WORD create_mask(){
        WORD MASK = 1;
        // turn on the #SHIFT LSBits
        for(int b = 1; b<SHIFT; b++){
                MASK |= (1 << b);
        }
        return MASK;
}

void set(int i, WORD* bmp);
void clr(int i, WORD* bmp);
int test(int i, WORD* bmp);

main() {
        cout << "give me bitmap: \n";
        char c;//[100]; //maximum bitmap
        //cin.getline(c,100);
        bit_vector bitv;

        while(cin.get(c) && c != '\n'){
                //cout << c <<endl;
                if(c == '0')
                        bitv.push_back(false);
                if(c == '1')
                        bitv.push_back(true);
        }

        cout<<"length of bitmap is "<<bitv.size()<<endl;

        //allocate new bitmap
        WORD* bmp = new WORD[numOfwords(bitv.size())];

        cout<<"BITSPERWORD : "<<BITSPERWORD<<endl;
        cout <<"number of words : "<< numOfwords(bitv.size())<<endl;
        cout<<"SHIFT : "<<SHIFT<<endl;

        //create MASK
        WORD MASK = create_mask();
        cout<<"MASK = "<<hex<<MASK<<endl;

        //reset bitmap
        //for(int i = 0; i<numOfwords(bitv.size()); i++){
        //        clr(i, bmp);
        //}

        //copy the bitmap
        int bit = 0;
        for(bit_vector::iterator iter = bitv.begin(); iter!=bitv.end(); iter++){
                (*iter == true) ? set(bit, bmp):clr(bit,bmp);
                bit++;
        }

        cout<<"Here is your bitmap : \n";
        for(int i =0; i<bitv.size(); i++) {
                (test(i, bmp)) ? cout<<"1" : cout<<"0";
        }
        cout<<endl;
}

void set(int i, WORD* bmp){
        //create MASK
        WORD MASK = create_mask();
        bmp[i>>SHIFT] |= (1<<(i & MASK));
}
void clr(int i, WORD* bmp){
        //create MASK
        WORD MASK = create_mask();
        bmp[i>>SHIFT] &= ~(1<<(i & MASK));
}

int test(int i, WORD* bmp){
        //create MASK
        WORD MASK = create_mask();
        return bmp[i>>SHIFT] & (1<<(i & MASK));
}
