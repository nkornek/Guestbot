#ifndef _TEXTUTILITIESH_
#define _TEXTUTILITIESH_

#ifdef INFORMATION
Copyright (C) 2011-2013 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

// end of word status on ptr
#define TOKEN_INCLUSIVE 1
#define TOKEN_EXCLUSIVE 2
#define TOKEN_INCOMPLETE 4

#define SPACES 1			//   \t \r \n 
#define PUNCTUATIONS 2      //    , | -  (see also ENDERS )
#define ENDERS	4			//   . ; : ? ! -
#define BRACKETS 8			//   () [ ] { } < >
#define ARITHMETICS 16		//    % * + - ^ = / .  (but / can be part of a word)
#define SYMBOLS 32			//    $ # @ ~  ($ and # can preceed, & is stand alone)
#define CONVERTERS 64		//   & `
#define QUOTERS 128			//   " ' *

#define UNINIT -1
typedef struct NUMBERDECODE
{
    const char* word;				//   word of a number
    int value;				//   value of word
	unsigned int length;	//   length of word
} NUMBERDECODE;

#define SHOUT 1
#define ABBREVIATION 2

#define NOT_A_NUMBER 2147483646

// accesses to these arrays MUST use unsigned char in the face of UTF8 strings
extern unsigned char punctuation[];
extern unsigned char toLowercaseData[];
extern unsigned char toUppercaseData[];
extern unsigned char isVowelData[];
extern unsigned char isAlphabeticDigitData[];
extern unsigned char isComparatorData[];
extern unsigned char legalNaming[256];
extern unsigned char realPunctuation[256];
extern signed char nestingData[];

extern bool showBadUTF;
extern char* userRecordSourceBuffer;
extern char tmpWord[MAX_WORD_SIZE];
extern time_t docTime;
extern bool singleSource;
extern bool echoDocument;

#define IsWhiteSpace(c) (punctuation[c] == SPACES)
#define IsWordTerminator(c) (punctuation[c] == SPACES || c == 0)
#define IsVowel(c) (isVowelData[c] != 0)
#define IsAlphabeticDigitNumeric(c) (isAlphabeticDigitData[c] != 0)
#define IsUpperCase(c) (isAlphabeticDigitData[c] == 3)
#define IsLowerCase(c) (isAlphabeticDigitData[c] == 4)
#define IsAlpha(c) (isAlphabeticDigitData[c] >= 3)
#define IsLegalNameCharacter(c)  legalNaming[c] 
#define IsDigit(c) (isAlphabeticDigitData[c] == 2)
#define IsAlphaOrDigit(c) (isAlphabeticDigitData[c] >= 2)
#define IsNonDigitNumberStarter(c) (isAlphabeticDigitData[c] == 1)
#define IsNumberStarter(c) (isAlphabeticDigitData[c] && isAlphabeticDigitData[c] <= 2)
#define IsComparison(c) (isComparatorData[c])


void AcquireDefines(char* fileName);
char* FindNameByValue(uint64 val); // properties
uint64 FindValueByName(char* name);
char* FindName2ByValue(uint64 val); // flags
uint64 FindValue2ByName(char* name);
void CloseTextUtilities();

void BOMAccess(int &BOMvalue, char &oldc, int &oldCurrentLine);

// boolean style tests
bool IsSentenceStart(unsigned int i);
bool IsArithmeticOperator(char* word);
char* IsUTF8(char* x);
bool IsNumber(char* word,bool placeAllowed = true);
bool IsPlaceNumber(char* word);
bool IsDigitWord(char* word);
bool IsUrl(char* word, char* end);
unsigned int IsMadeOfInitials(char * word,char* end);
bool IsNumericDate(char* word,char* end);
bool IsFloat(char* word, char* end);
char GetTemperatureLetter (char* ptr);
bool IsLegalName(char* name);
char* GetCurrency(char* ptr,char* &number);

// conversion reoutines
void MakeLowerCase(char* ptr);
void MakeUpperCase(char* ptr);
char* MakeLowerCopy(char* to,char* from);
char* MakeUpperCopy(char* to,char* from);
void UpcaseStarters(char* ptr);
void Convert2Underscores(char* buffer,bool upcase,bool removeClasses=true,bool removeBlanks=false);
void Convert2Blanks(char* output);
void ForceUnderscores(char* ptr);
char* TrimSpaces(char* msg,bool start = true);
char* UTF2ExtendedAscii(char* bufferfrom);

// startup
void InitTextUtilities();
bool ReadDocument(char* inBuffer,FILE* sourceFile);

// reading functions
char* ReadFlags(char* ptr,uint64& flags);
char* ReadHex(char* ptr, uint64 & value);
char* ReadInt(char* ptr, unsigned int & value);
char* ReadInt64(char* ptr, int64 & w);
char* ReadQuote(char* ptr, char* buffer,bool backslash = false, bool noblank = true);
char* ReadArgument(char* ptr, char* buffer);
char* ReadCompiledWord(char* ptr, char* word);
char* ReadALine(char* buf,FILE* file,unsigned int limit = maxBufferSize);
char* SkipWhitespace(char* ptr);
char* BalanceParen(char* ptr,bool within=true);
int64 NumberPower(char* number);
int64 Convert2Integer(char* word);

#endif
