
#define MAX_SYNLOOP	60

#define MAX_DICTIONARY	 0x000fffff  //   1M word vocabulary limit (doubling this FAILS on amazon server)

#define MAX_HASH_BUCKETS 0x10000 
#ifdef WIN32
#define MAX_ENTRIES      0x000fffff 
#else
#define MAX_ENTRIES      (0x000fffff/3)
#endif

#define NODEBITS 0x00ffffff
#define GETNEXTNODE(D) (D->nextNode & NODEBITS)		// top byte is the length of joined phrases of which this is header
#define MULTIHEADERBITS 0xFF000000
#define GETMULTIWORDHEADER(D)  (D->nextNode >> 24)
#define SETMULTIWORDHEADER(D,n) (  D->nextNode &= NODEBITS, D->nextNode |= n << 24 )


// system internal bits on dictionary entries internalBits

// various livedata substitions allowed
#define ESSENTIALS_FILE			0x00000001 
#define SUBSTITUTIONS_FILE		0x00000002
#define CONTRACTIONS_FILE		0x00000004
#define INTERJECTIONS_FILE		0x00000008
#define BRITISH_FILE			0x00000010 
#define SPELLING_FILE			0x00000020 
#define TEXTING_FILE			0x00000040 
// 80
// 100
// 200
// 400
// 800

#define UTF8					0x00001000		// word has utf8 char in it
#define UPPERCASE_HASH			0x00002000		// word has upper case English character in it
#define VAR_CHANGED				0x00004000		// $variable has changed value this volley
#define WORDNET_ID				0x00008000		// a wordnet synset header node (MASTER w gloss )
#define INTERNAL_MARK			0x00010000		// transient marker for Intersect coding and Country testing in :trim and deleted words in dictionary build and  allowed to write out w/o checksum in script compile
#define MARKED_WORD				0x00020000		// transient word marker

///   DEFINITION OF A MEANING 
#define TYPE_RESTRICTION	BASIC_POS  //  0xf0000000
#define SYNSET_MARKER		0x08000000  // this meaning is a synset head - on keyword import, its quote flag for binary read

#define INDEX_BITS          0x03F00000  //   7 bits of ontology indexing ability  63 possible meanings allowed
#define INDEX_OFFSET        20          //   shift for ontoindex  (rang 0..63)  
#define MAX_MEANING			63			// limit

#define MEANING_BASE		0x000fffff	//   the index of the dictionary item

#define STDMEANING ( INDEX_BITS | MEANING_BASE | TYPE_RESTRICTION ) // no synset marker
#define SIMPLEMEANING ( INDEX_BITS | MEANING_BASE ) // simple meaning, no type

//   codes for BurstWord argument
#define SIMPLE 0
#define STDBURST 0		// normal burst behavior
#define POSSESSIVES 1
#define CONTRACTIONS 2
#define HYPHENS 4
#define COMPILEDBURST 8  // prepare argument as though it were output script		
#define NOBURST 16		// dont burst (like for a quoted text string)

#define FUNCTIONSTRING '^'

//   values for FindWord lookup
#define PRIMARY_CASE_ALLOWED 1
#define SECONDARY_CASE_ALLOWED 2
#define STANDARD_LOOKUP (PRIMARY_CASE_ALLOWED |  SECONDARY_CASE_ALLOWED )
#define LOWERCASE_LOOKUP 4
#define UPPERCASE_LOOKUP 8

#define Index2Word(n) (dictionaryBase+n)
#define Word2Index(D) ((unsigned int) (D-dictionaryBase))
#define GetPlural(D) (D->plurality)
#define GetTense(D) (D->conjugation)
#define GetMeanings(D) (D->meanings)
#define GetMeaningsFromMeaning(T) (Meaning2Word(T)->meanings)
#define Meaning2Index(x) ((unsigned int)((x & INDEX_BITS) >> INDEX_OFFSET)) //   which dict entry meaning

// memory data
extern WORDP dictionaryBase;
extern char* stringBase;
extern char* stringFree;
extern unsigned long maxDictEntries;
extern unsigned long maxStringBytes;
extern unsigned int userTopicStoreSize;
extern unsigned int userTableSize;
extern unsigned long maxHashBuckets;

extern bool fullDictionary;

extern CharCharMap canonicalMap;
extern uint64 verbFormat;
extern uint64 nounFormat;
extern uint64 adjectiveFormat;
extern uint64 adverbFormat;
extern MEANING posMeanings[64];
extern MEANING sysMeanings[64];
extern bool buildDictionary;
extern char language[40];

extern WORDP dictionaryPreBuild0;
extern WORDP dictionaryPreBuild1;
extern WORDP dictionaryFree;

// internal references to defined words
extern WORDP Dplacenumber;
extern WORDP Dpropername;
extern MEANING Mphrase;
extern MEANING MabsolutePhrase;
extern MEANING MtimePhrase;
extern WORDP Dclause;
extern WORDP Dverbal;
extern WORDP Dbasicform;
extern WORDP Dmalename,Dfemalename,Dhumanname;
extern WORDP Dtime;
extern WORDP Dunknown;
extern WORDP DunknownWord;
extern WORDP Dpronoun;
extern WORDP Dchild,Dadult;
extern MEANING Mmoney;
extern MEANING Mchatoutput;
extern MEANING Mburst;
extern MEANING Mpending;
extern MEANING Mkeywordtopics;
extern MEANING Mintersect;
extern MEANING MconceptComment;
extern MEANING MgambitTopics;
extern MEANING MadjectiveNoun;
extern MEANING Mnumber;

// language dependent routines
extern char* (*P_GetAdjectiveBase)(char*, bool);   
extern char* (*P_GetAdverbBase)(char*,bool);
extern void (*P_SetTense)();
extern char* (*P_GetPastTense)(char*);
extern char* (*P_GetPastParticiple)(char*);
extern char* (*P_GetPresentParticiple)(char*);
extern char* (*P_GetThirdPerson)(char*);
extern char* (*P_GetInfinitive)(char*,bool);
extern char* (*P_GetSingularNoun)(char*,bool,bool);
extern char* (*P_GetPluralNoun)(WORDP);
extern void (*P_TagIt)();

char* AllocateString(char* word,size_t len = 0,bool align64=false,bool clear = false);
WORDP StoreWord(int);
WORDP StoreWord(char* word, uint64 properties = 0);
WORDP StoreWord(char* word, uint64 properties, uint64 flags);
WORDP FindWord(const char* word, int len = 0,unsigned int caseAllowed = STANDARD_LOOKUP);
WORDP FullStore(char* word, uint64 properties, uint64 flags);
unsigned char BitCount(uint64 n);

char* UseDictionaryFile(char* name);
inline char* Index2String(unsigned int offset) { return stringBase - (offset & 0x00ffffff);}
inline unsigned int String2Index(char* str) {return stringBase- str;}
inline unsigned int GlossIndex(MEANING M) { return M >> 24;}
void ReadAbbreviations(char* file);
void Write32(unsigned int val, FILE* out);
unsigned int Read32(FILE* in);
void ReadLiveData();
WORDP GetSubstitute(WORDP D);
WORDP GetComparison(WORDP D);

// adjust data on a dictionary entry
void AddProperty(WORDP D, uint64 flag);
void RemoveProperty(WORDP D, uint64 flag);
void RemoveSystemFlag(WORDP D, uint64 flag);
void AddSystemFlag(WORDP D, uint64 flag);
void AddInternalFlag(WORDP DP, unsigned int flag);
void RemoveInternalFlag(WORDP D,unsigned int flag);
void WriteDWord(WORDP ptr, FILE* out);
WORDP ReadDWord(FILE* in);
void AddCircularEntry(WORDP base, void* field,WORDP entry);

inline unsigned int GetMeaningCount(WORDP D) { return (D->meanings) ? D->meanings[0] : 0;}
inline unsigned int GetGlossCount(WORDP D) 
{
	if (D->glosses)  return D->glosses[0];
	return 0;
}
char* GetCanonical(char* word);
char* GetCanonical(WORDP D);
char* GetGloss(WORDP D, unsigned int index);

// startup and shutdown routines
void InitDictionary();
void CloseDictionary();
void LoadDictionary();
void ExtendDictionary();
void WordnetLockDictionary();
void ReturnDictionaryToWordNet();
void Build0LockDictionary();
void ReturnDictionaryToBuild0();
void FreezeDictionary();
void ReturnDictionaryToFreeze();
void DeleteDictionaryEntry(WORDP D);
void BuildDictionary(char* junk);

// read and write dictionary or its entries
void WriteDictionary(WORDP D, uint64 data);
void DumpDictionaryEntry(char* word,unsigned int limit);
bool ReadDictionary(char* file);
char* ReadDictionaryFlags(WORDP D, char* ptr,unsigned int *meaningcount = NULL, unsigned int *glosscount = NULL);
void WriteDictionaryFlags(WORDP D, FILE* out);
void WriteBinaryDictionary();
bool ReadBinaryDictionary();
void Write64(uint64 val, FILE* out);
uint64 Read64(FILE* in);
void Write24(unsigned int val, FILE* out);

// utilities
void ReadWordsOf(char* file,uint64 mark);
void WalkDictionary(DICTIONARY_FUNCTION func,uint64 data = 0);
char* FindCanonical(char* word, unsigned int i, bool nonew = false);
void VerifyEntries(WORDP D,uint64 junk);
void NoteLanguage();


bool IsHelper(char* word);
bool IsFutureHelper(char* word);
bool IsPresentHelper(char* word);
bool IsPastHelper(char* word);


///   code to manipulate MEANINGs
MEANING MakeTypedMeaning(WORDP x, unsigned int y, unsigned int flags);
MEANING MakeMeaning(WORDP x, unsigned int y = 0);
WORDP Meaning2Word(MEANING x);
MEANING AddMeaning(WORDP D,MEANING M);
MEANING AddGloss(WORDP D,char* gloss,unsigned int index);
void RemoveMeaning(MEANING M, MEANING M1);
MEANING ReadMeaning(char* word,bool create=true,bool precreated = false);
char* WriteMeaning(MEANING T,bool withPOS = false);
MEANING GetMaster(MEANING T);
unsigned int GetMeaningType(MEANING T);
MEANING FindSynsetParent(MEANING T,unsigned int which = 0);
MEANING FindSetParent(MEANING T,int n);

