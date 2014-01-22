#include "common.h"

#ifdef INFORMATION

This file covers routines that create and access a "dictionary entry" (WORDP) and the "meaning" of words (MEANING).

The dictionary acts as the central hash mechanism for accessing various kinds of data.

The dictionary consists of data imported from WORDNET 3.0 (copyright notice at end of file) + augmentations + system and script pseudo-words.

A word also gets the WordNet meaning ontology (->meanings & ->meaningCount). The definition of meaning in WordNet 
is words that are synonyms in some particular context. Such a collection in WordNet is called a synset. 

Since words can have multiple meanings (and be multiple parts of speech), the flags of a word are a summary
of all of the properties it might have and it has a list of entries called "meanings". Each entry is a MEANING 
and points to the circular list, one of which marks the word you land at as the synset head. 
 This is referred to as the "master" meaning and has the gloss (definition) of the meaning. The meaning list of a master node points back to 
all the real words which comprise it.

Since WordNet has an ontology, its synsets are hooked to other synsets in various relations, particular that
of parent and child. ChatScript represents these as facts. The hierarchy relation uses the verb "is" and
has the child as subject and the parent as object. Such a fact runs from the master entries, not any of the actual 
word entries. So to see if "dog" is an "animal", you could walk every meaning list of the word animal and
mark the master nodes they point at. Then you would search every meaning of dog, jumping to the master nodes,
then look at facts with the master node as subject and the verb "is" and walk up to the object. If the object is 
marked, you are there. Otherwise you take that object node as subject and continue walking up. Eventually you arrive at
a marked node or run out at the top of the tree.

Some words DO NOT have a master node. Their meaning is defined to be themselves (things like pronouns or determiners), so
their meaning value for a meaning merely points to themselves.
The meaning system is established by building the dictionary and NEVER changes thereafter by chatbot execution.
New words can transiently enter the dictionary for various purposes, but they will not have "meanings".

A MEANING is a reference to a specific meaning of a specific word. It is an index into the dictionary 
(identifying the word) and an index into that words meaning list (identifying the specific meaning).
An meaning list index of 0 refers to all meanings of the word. A meaning index of 0 can also be type restricted
so that it only refers to noun, verb, adjective, or adverb meanings of the word.

Since there are only two words in WordNet with more than 63 meanings (break and cut) we limit all words to having no
more than 63 meanings by discarding the excess meanings. Since meanings are stored most important first,
these are no big loss. This leaves room for the 5 essential type flags used for restricting a generic meaning.

Space for dictionary words, strings, and the meanings of words come from a common pool. Dictionary words are
allocated linearly forward in the pool, while strings and meanings are allocated linearly backwards. Thus
all dictionary entries are indexable as a giant array.

The dictionary separately stores uppercase and lowercase forms of the same words (since they have different meanings).
There is only one uppercase form stored, so United and UnItED would be saved as one entry. The system will have
to decide which case a user intended, since they may not have bothered to capitalize a proper noun, or they 
may have shouted a lowercase noun, and a noun at the start of the sentence could be either proper or not.

Dictionary words are hashed as lower case, but if the word has an upper case letter it will be stored
in the adjacent higher bucket. Words of the basic system are stored in their appropriate hash bucket.
After the basic system is read in, the dictionary is frozen. This means it remembers the spots the allocation
pointers are at for the dictionary and string space and is using mark-release memory management.

Words created on the fly (after a freeze) by interacting with a user are always stored in bucket 0. 
This allows the system to instantly discard them when the interaction has been processed merey by 
zeroing bucket 0. The string space and dictionary space allocated to those on-the-fly words are merely 
"released" back to the values at the time of dictionary freeze.

We mark sysnet entries with the word & meaning number & POS of word in the dictionary entry. The POS not used explicitly by lots of the system
but is needed when seeing the dictionary definitions (:word) and if one wants to use pos-restricted meanings in a match or in keywords.

#endif

#define HASH_EXTRA		2					// +1 for being 1-based and +1 for having uppercase bump

bool buildDictionary = false;				// indicate when building a dictionary
static char dictionaryTimeStamp[30];		// indicate when dictionary was built
char language[40];							// indicate current language used

static unsigned char* writePtr;				// used for binary dictionary writes

// memory data
#define MAX_STRING_SPACE 30000000  // transient string space
unsigned long maxHashBuckets = MAX_HASH_BUCKETS;
unsigned long maxDictEntries = MAX_ENTRIES;
unsigned long maxStringBytes = MAX_STRING_SPACE;
unsigned int userTopicStoreSize,userTableSize; // memory used which we will display

CharCharMap canonicalMap;				// canonical form of words read in from LIVEDATA canonical
MEANING posMeanings[64];				// concept associated with propertyFlags of WORDs
MEANING sysMeanings[64];				// concept associated with systemFlags of WORDs

bool fullDictionary = true;				// we have a big master dictionary, not a mini dictionary

// start and ends of space allocations
WORDP dictionaryBase = 0;			// base of allocated space that encompasses dictionary, string space, and meanings
WORDP dictionaryFree;				// current next dict space available going forward (not a valid entry)
char* stringBase;					// start of string space (runs backward)
char* stringFree;					// current free string ptr

// return-to values after WordNet loaded, before topic system loaded
WORDP dictionaryPreBuild0;
static char* stringSpacePreBuild0 = 0;
// wordnetFacts

// return-to values after build0 topic data loaded, before build1 topic data loaded
WORDP dictionaryPreBuild1;
static char* stringSpacePrebuild1;
// build0Facts 
	
// return-to values after build1 loaded, before user is loaded
static WORDP dictionaryLocked;
static FACT* factLocked;
static char* stringLocked;

// format of word looked up
uint64 verbFormat;
uint64 nounFormat;
uint64 adjectiveFormat;
uint64 adverbFormat;

// dictionary ptrs for these words
WORDP Dplacenumber;
WORDP Dpropername;
MEANING Mphrase;
MEANING MabsolutePhrase;
MEANING MtimePhrase;
WORDP Dclause;
WORDP Dverbal;
WORDP Dbasicform;
WORDP Dmalename,Dfemalename,Dhumanname;
WORDP Dtime;
WORDP Dunknown;
WORDP Dchild,Dadult;
MEANING Mchatoutput;
MEANING Mburst;
MEANING Mpending;
MEANING Mkeywordtopics;
MEANING Mmoney;
MEANING Mintersect;
MEANING MconceptComment;
MEANING MgambitTopics;
MEANING MadjectiveNoun;
MEANING Mnumber;
WORDP Dpronoun;
WORDP DunknownWord;

// language indirect code so can implement different languages
char* (*P_GetAdjectiveBase)(char*, bool) = NULL;   
char* (*P_GetAdverbBase)(char*,bool) = NULL;
char* (*P_GetPastTense)(char*) = NULL;
char* (*P_GetPastParticiple)(char*) = NULL;
char* (*P_GetPresentParticiple)(char*) = NULL;
char* (*P_GetThirdPerson)(char*) = NULL;
char* (*P_GetInfinitive)(char*,bool) = NULL;
char* (*P_GetSingularNoun)(char*,bool,bool) = NULL;
char* (*P_GetPluralNoun)(WORDP) = NULL;
void (*P_TagIt)() = NULL;
void (*P_SetTense)() = NULL;
	
static char* predefinedSets[] = //  some internally mapped concepts not including emotions from LIVEDATA/interjections
{
	"~mainsubject","~mainverb","~mainobject","~mainindirectobject",
	"~subject2","~verb2","~object2","~indirectobject2",
	"~appositive","~subjectcomplement","~objectcomplement","~address",
    "~repeatme","~repeatinput1","~repeatinput2","~repeatinput3","~repeatinput4","~repeatinput5","~repeatinput6",
    NULL
};

char* GetCanonical(char* word)
{
	return (!word) ? NULL : (char*) canonicalMap[word];
}

char* GetCanonical(WORDP D)
{
	return (!D) ? NULL : (char*) canonicalMap[D->word];
}


char* UseDictionaryFile(char* name)
{
	static char junk[100];
	if (!*language) sprintf(junk,"DICT/%s",name);
	else if (!name) sprintf(junk,"DICT/%s",language);
	else sprintf(junk,"DICT/%s/%s",language,name);
	return junk;
}

unsigned char BitCount(uint64 n)  
{  
	unsigned char count = 0;  
    while (n)  
	{  
       count++;  
       n &= (n - 1);  
    }  
    return count;  
 } 

WORDP GetSubstitute(WORDP D)
{
	return (D && D->systemFlags & SUBSTITUTE)  ?  D->w.substitutes : 0;
}

WORDP GetComparison(WORDP D)
{
	return (D && IsAlpha(*D->word) && !(D->systemFlags & SUBSTITUTE))  ? D->comparison : 0;
}

void BuildShortDictionaryBase();

void BuildDictionary(char* label)
{
	buildDictionary = true;
	int miniDict = 0;
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(label,word);
	if (!stricmp(word,"wordnet")) // the FULL wordnet dictionary w/o synset removal
	{
		miniDict = -1;
		ReadCompiledWord(ptr,word);
	}
	else if (!stricmp(word,"short")) // a mini dictionary
	{
		miniDict = 1;
		ReadCompiledWord(ptr,word);
		if (!stricmp(word,"init")) BuildShortDictionaryBase();
	}
	if (*word) MakeUpperCopy(language,word); // localized version of dict. otherwise its of the current language

	// insure language subdirectory exists
#ifdef WIN32
	_mkdir(UseDictionaryFile(NULL));
#else 
	mkdir(UseDictionaryFile(NULL), 0777); 
#endif
		
	NoteLanguage(); // set default is most recent built dictionary

	InitFacts(); 
	InitDictionary();
	AcquireDefines("src/dictionarySystem.h"); 
	LoadRawDictionary(miniDict); 
 	remove(UseDictionaryFile("dict.bin")); // invalidate cache of dictionary, forcing binary rebuild later
    WriteFacts(FopenUTF8Write(UseDictionaryFile("facts.txt")),factBase); 
	sprintf(logFilename,"USERS/build_log.txt"); // all data logged here by default
	FILE* out = FopenUTF8Write(logFilename);
	if (out) fclose(out);
	buildDictionary = false;
	CreateSystem();
}

void InitDictionary()
{
	dictionaryLocked = 0;
	userTopicStoreSize = userCacheCount * userCacheSize; //  minimum cache spot
	userTopicStoreSize /= 64;
	userTopicStoreSize = (userTopicStoreSize * 64) + 64;
	
	//   dictionary and meanings and strings share space, running from opposite ends of a common pool
	size_t size = sizeof(WORDENTRY) * maxDictEntries;
	size += maxStringBytes;
	size /= sizeof(WORDENTRY);
	size = (size * sizeof(WORDENTRY)) + sizeof(WORDENTRY);
	size /= 64;
	size = (size * 64) + 64; // 64 bit align both ends
	// on FUTURE startups (not 1st) the userCacheCount has been preserved while the rest of the system is reloaded
	if ( dictionaryBase == 0) // 1st startup allocation -- not needed on a reload
	{
		userTableSize = userCacheCount * 3 * sizeof(unsigned int);
		userTableSize /= 64;
		userTableSize = (userTableSize * 64) + 64; // 64 bit align both ends
		InitCache((unsigned int)size);
		dictionaryBase = (WORDP) (cacheBase + userTopicStoreSize + userTableSize);
	}
	memset(dictionaryBase,0,size);
	dictionaryFree =  dictionaryBase + maxHashBuckets + HASH_EXTRA ;		//   prededicate hash space within the dictionary itself
	stringBase = stringFree = ((char*)dictionaryBase) + size;						//   the end of the allocated space

	//   The bucket list is threaded thru WORDP nodes, and consists of indexes, not addresses.

	dictionaryPreBuild0 = 0;				// in initial dictionary
	build0Facts = wordnetFacts = factFree;	// last fact in dictionary 
	canonicalMap.clear();
}

char* AllocateString(char* word,size_t len,bool align64,bool clear)
{ //   string allocation moves BACKWARDS from end of dictionary space (as do meanings)
	if (len == 0) len = strlen(word);
	if (word) ++len;	// null terminate string

	//   always allocate on word boundary (since we share with meaning space)
	unsigned int allocate = ((len + 3) / 4) * 4;

	stringFree -= allocate;
 	if (align64) // force 64bit alignment alignment
	{
		uint64 base = (uint64) stringFree;
		base &= 0xFFFFFFFFFFFFFFC0ULL;
		stringFree = (char*) base;
	}
	char* newword =  stringFree;
    if (stringFree <= (char*) dictionaryFree) 
    {
		stringFree += allocate - 4; 
        ReportBug("Out of transient string space")
        len = 2;
#ifndef WIN32
		myexit("no more transient string space");
#endif
    }
    if (word) 
	{
		memcpy(newword,word,--len);
		newword[len] = 0;
	}
	else if (clear) memset(newword,0,len);
    return newword;
}

static void SetIdiomHeader(WORDP D)
{
	//   internal use, do not allow idioms on words from #defines or user variables or  sets.. but allow substitutes to do it?
	unsigned int n = BurstWord(D->word);
	if (n != 1) 
	{
		D = StoreWord(JoinWords(1));		// create the 1-word header
		if (n > GETMULTIWORDHEADER(D)) SETMULTIWORDHEADER(D,n);	//   mark it can go this far for an idiom
	}
}

void AddInternalFlag(WORDP D, unsigned int flag)
{
	D->internalBits |= flag;
}

void RemoveInternalFlag(WORDP D,unsigned int flag)
{
	D->internalBits &= -1 ^ flag;
}

void AddSystemFlag(WORDP D, uint64 flag)
{
	D->systemFlags |= flag;
}

void RemoveSystemFlag(WORDP D, uint64 flags)
{
	D->systemFlags &= -1LL ^ flags;
}

void AddProperty(WORDP D, uint64 flag)
{
	// if (flag & FOREIGN_WORD && D->properties & PART_OF_SPEECH) return; // dont care about it being a foreign word
	if (flag & PART_OF_SPEECH && *D->word != '~'  && *D->word != '^') SetIdiomHeader(D); //   make composite real words auto find in input -- dont header a concept name
	D->properties |= flag;
}

void RemoveProperty(WORDP D, uint64 flags)
{
	D->properties &= -1LL ^ flags;
}

WORDP FindWord(const char* word, int len,unsigned int caseAllowed) 
{
	if (word == NULL || *word == 0) return NULL;
	if (len == 0) len = (int) strlen(word);
	bool hasUpperCharacters = false;
	bool hasUTF8Characters = false;
	uint64 fullhash = Hashit((unsigned char*) word,len,hasUpperCharacters,hasUTF8Characters); //   sets hasUpperCharacters and hasUTF8Characters 
	unsigned int hash  = (fullhash % maxHashBuckets) + 1; // mod by the size of the table
	if (caseAllowed & LOWERCASE_LOOKUP){;}
	else if (hasUpperCharacters || (caseAllowed & UPPERCASE_LOOKUP)) ++hash;

	//   normal or fixed case bucket
	WORDP D;
	if (caseAllowed & (PRIMARY_CASE_ALLOWED|LOWERCASE_LOOKUP|UPPERCASE_LOOKUP))
	{
		D = dictionaryBase + hash;
		while (D != dictionaryBase)
		{
			if (fullhash == D->hash) 
				return D;
			D = dictionaryBase + GETNEXTNODE(D);
		}
	}

    //    alternate case bucket
	if (caseAllowed & SECONDARY_CASE_ALLOWED)
	{
		D = dictionaryBase + hash + ((hasUpperCharacters) ? -1 : 1);
		while (D != dictionaryBase)
		{
			if (fullhash == D->hash)  
				return D;
			D = dictionaryBase + GETNEXTNODE(D);
		}
	}

    return NULL;
}

static WORDP AllocateEntry()
{
	WORDP  D = dictionaryFree++; 
	if (Word2Index(D) >= maxDictEntries)
	{
		ReportBug("used up all dict nodes\r\n")
		myexit("used up all dict nodes");
	}
    memset(D,0,sizeof(WORDENTRY));
	return D;
}

WORDP StoreWord(int val) // create a number word
{
	char value[MAX_WORD_SIZE];
	sprintf(value,"%d",val);
	return StoreWord(value);
}

WORDP StoreWord(char* word, uint64 properties, uint64 flags)
{
	WORDP D = StoreWord(word,properties);
	AddSystemFlag(D,flags);
	return D;
}

WORDP StoreWord(char* word, uint64 properties)
{
	if (!server) // servers dont want long lists of bugs from strange inputs
	{
		if (!*word) //   we require something 
		{
			ReportBug("entering null word to dictionary")
			return StoreWord("badword");
		}
	}
	if (!*word) return StoreWord("badword"); //   we require something 
			
	//   make all words normalized with no blanks in them.
	if (*word == '"' || *word == '_'); // dont change any quoted things or things beginning with _ (we use them in facts for a "missing" value
	else if (!(properties & AS_IS)) word = JoinWords(BurstWord(word,0)); //   when reading in the dictionary, BurstWord depends on it already being in, so just use the literal text here
	properties &= -1 ^ AS_IS;
	size_t len = strlen(word);
	bool hasUpperCharacters = false;
	bool hasUTF8Characters = false;
	uint64 fullhash = Hashit((unsigned char*)word,len,hasUpperCharacters,hasUTF8Characters); //   this sets hasUpperCharacters as well if needed
	unsigned int hash = (fullhash % maxHashBuckets) + 1; //   mod the size of the table (saving 0 to mean no pointer and reserving an end upper case bucket)
	if (hasUpperCharacters)  ++hash;
	WORDP base = dictionaryBase + hash;
 
	//   locate spot existing entry goes
    WORDP D = base; 
	while (D != dictionaryBase)
    {
 		if (fullhash == D->hash)
		{
			AddProperty(D,properties);
			return D;
		}
		D = dictionaryBase + GETNEXTNODE(D);
    }  

    //   not found, add entry 
	if (base->word == 0 && !dictionaryPreBuild0) D = base; // add into hash zone initial dictionary entries (nothing allocated here yet)
	else  
	{
		D = AllocateEntry();
		if ((char*)D >= stringFree) 
		{
			ReportBug("Out of dictionary nodes")
			myexit("out of dictionary space");
		}
		D->nextNode = GETNEXTNODE(base);
		base->nextNode &= MULTIHEADERBITS;
		base->nextNode |= D - dictionaryBase;
	}
    D->word = AllocateString(word,len); 
    AddProperty(D,properties);
	if (hasUTF8Characters) D->internalBits |= UTF8;
	if (hasUpperCharacters) 
	{
		D->internalBits |= UPPERCASE_HASH;
		if (D->word[1]  && IsUpperCase(*D->word)) // auto add gender from a proper name, not the pronoun I
		{
			char* sep = strrchr(D->word,'_');  // get last underscore (last name)
			if (sep && IsUpperCase(sep[1])) // 1st and last are capitalized. Not a phrase as only has 2 words. (we only attempt gender on 2-word names)
			{
				sep = strchr(D->word,'_');
				WORDP X = FindWord(D->word,sep-D->word);
				if (!X){;}
				else if (X->properties & NOUN_HE) AddProperty(D,NOUN_HE|NOUN_HUMAN|NOUN_PROPER_SINGULAR);
				else if (X->properties & NOUN_SHE) AddProperty(D,NOUN_SHE|NOUN_HUMAN|NOUN_PROPER_SINGULAR);
			}
		}
	}
	D->hash = fullhash;
	D->y.length = (unsigned short) len;
    return D;
}

//   insert entry into a circular list, initializing if need be
void AddCircularEntry(WORDP base, void* field,WORDP entry)
{
	if (!base) return;
	unsigned int offset = ((char*)field - (char*)base) / sizeof(WORDP);	// spot in entry is field as word index
	WORDP* setBase = (WORDP*)base;		// pretend to be collection of fields
	WORDP* setEntry = (WORDP*)entry;		// pretend to be collection of fields

	//   verify item to be added not already in circular list of this kind - dont add if it is
	if (!setEntry[offset]) 
	{
		if (!setBase[offset]) setBase[offset] = base; // if set base not initialized, make it loop to self
		setEntry[offset] = setBase[offset]; 
		setBase[offset] = entry;
	}
	else printf("%s already on circular list of %s\r\n",entry->word, base->word);
}

void WalkDictionary(DICTIONARY_FUNCTION func,uint64 data)
{
    for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
	{
		if (D->word) (func)(D,data); 
	}
}

void DeleteDictionaryEntry(WORDP D)
{
	unsigned int hash = (D->hash % maxHashBuckets) + 1; 
	if (D->internalBits & UPPERCASE_HASH) ++hash;
	WORDP base = dictionaryBase + hash;
	base->nextNode &= MULTIHEADERBITS;
	base->nextNode |= GETNEXTNODE(D); //   remove entry from buckets
}

void ReturnDictionaryToFreeze() 
{ 
	while (dictionaryFree > dictionaryLocked) DeleteDictionaryEntry(--dictionaryFree); //   remove entry from buckets
    stringFree = stringLocked; 
	while (factFree > factLocked) FreeFact(factFree);
	// system doesnt track changes to properties after freeze
}

void WordnetLockDictionary() // dictionary before build0 layer 
{
    dictionaryPreBuild0 = dictionaryFree;		
	stringSpacePreBuild0 = stringFree;		//   mark point for mark release
	chatbotFacts = topicFacts = build0Facts = wordnetFacts = factFree;

#ifndef DISCARDSCRIPTCOMPILER
	// memorize dictionary values for backup to pre build locations :build0 operations (reseting word to dictionary state)
	FILE* out1 = fopen("TMP/prebuild0","wb");
	if (!out1)  ReportBug("Cant generate tmp backups of dict values. Guessing folder isn't writeable.")
	else
	{
		for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
		{
			Write64(D->properties,out1);
			Write64(D->systemFlags,out1);
			unsigned char head = GETMULTIWORDHEADER(D);
			fwrite(&head,1,1,out1);
		}
		fclose(out1);
	}
#endif
}

void ReturnDictionaryToWordNet() // drop all non-fact memeory allocated after the wordnet freeze
{
	while (factFree > wordnetFacts) FreeFact(factFree--); // restore to end of dictionary load
	while (dictionaryFree > dictionaryPreBuild0) DeleteDictionaryEntry(--dictionaryFree); // remove entry from buckets
    stringFree = stringSpacePreBuild0;
#ifndef DISCARDSCRIPTCOMPILER
	FILE* in = fopen("TMP/prebuild0","rb");
	if (in)
	{
		for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
		{
			D->properties = Read64(in);
			D->systemFlags = Read64(in);
			unsigned char c;
			int n = fread(&c,1,1,in);
			if (n != 1)
			{
				ReportBug("Bad return to wordnet")
				myexit("bad return to wordnet");
			}
			SETMULTIWORDHEADER(D,c);
		}
		fclose(in);
	}
#endif
	canonicalMap.clear();
}

void Build0LockDictionary() // dictionary after build0 and before build1 layers 
{
    dictionaryPreBuild1 = dictionaryFree;		
	stringSpacePrebuild1 = stringFree;	
    chatbotFacts = topicFacts = build0Facts = factFree; 
#ifndef DISCARDSCRIPTCOMPILER
	FILE* out = fopen("TMP/prebuild1","wb");
	if (out)
	{
		for (WORDP D = dictionaryBase+1; D < dictionaryPreBuild1; ++D) 
		{
			fwrite(&D->properties,1,8,out); 
			fwrite(&D->systemFlags,1,8,out); 
			unsigned char head = GETMULTIWORDHEADER(D);
			fwrite(&head,1,1,out);
		}
		fclose(out);
	}
#endif
}

void ReturnDictionaryToBuild0() 
{
	while (factFree > build0Facts) FreeFact(factFree--); //   restore back to facts alone
	while (dictionaryFree > dictionaryPreBuild1) DeleteDictionaryEntry(--dictionaryFree); //   remove entry from buckets
    stringFree = stringSpacePrebuild1;
#ifndef DISCARDSCRIPTCOMPILER
	FILE* in = fopen("TMP/prebuild1","rb");
	if (in)
	{
		for (WORDP D = dictionaryBase+1; D < dictionaryFree; ++D) 
		{
			fread(&D->properties,1,8,in); 
			fread(&D->systemFlags,1,8,in); 
			unsigned char c;
			int n = fread(&c,1,1,in);
			if (n != 1)
			{
				ReportBug("Bad return to build0")
				myexit("bad return to build0");
			}
			SETMULTIWORDHEADER(D,c);
		}
		fclose(in);
	}
#endif

	// canonical map in layer 1 is now garbage- 
	canonicalMap.clear();
}

void FreezeDictionary()
{
    dictionaryLocked = dictionaryFree;		
	stringLocked = stringFree;		
    factLocked = chatbotFacts = topicFacts = factFree; 
}

void CloseDictionary()
{
	CloseTextUtilities();
	dictionaryBase = NULL;
	CloseCache(); // actual memory space of the dictionary
}

static void Write8(unsigned int val, FILE* out)
{
	unsigned char x[1];
	x[0] = val & 0x000000ff;
	if (out) fwrite(x,1,1,out);
	else *writePtr++ = *x;
}

static void Write16(unsigned int val, FILE* out)
{
	unsigned char x[2];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	if (out) fwrite(x,1,2,out);
	else 
	{
		memcpy(writePtr,(unsigned char*)x,2);
		writePtr += 2;
	}
}

void Write24(unsigned int val, FILE* out)
{
	unsigned char x[3];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	if (out) fwrite(x,1,3,out);
	else 
	{
		memcpy(writePtr,(unsigned char*)x,3);
		writePtr += 3;
	}
}

void Write32(unsigned int val, FILE* out)
{
	unsigned char x[4];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	x[3] = (val >> 24) & 0x000000ff;
	if (out) fwrite(x,1,4,out);
	else 
	{
		memcpy(writePtr,(unsigned char*)x,4);
		writePtr += 4;
	}
}

void Write64(uint64 val, FILE* out)
{
	unsigned char x[8];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	x[3] = (val >> 24) & 0x000000ff;
	x[4] = (val >> 32) & 0x000000ff;
	x[5] = (val >> 40) & 0x000000ff;
	x[6] = (val >> 48) & 0x000000ff;
	x[7] = (val >> 56) & 0x000000ff;
	if (out) fwrite(x,1,8,out);
	else 
	{
		memcpy(writePtr,(unsigned char*)x,8);
		writePtr += 8;
	}
}

void WriteDWord(WORDP ptr, FILE* out)
{
	unsigned int val = (ptr) ? Word2Index(ptr) : 0;
	unsigned char x[3];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	if (out) fwrite(x,1,3,out);
	else 
	{
		memcpy(writePtr,(unsigned char*)x,3);
		writePtr += 3;
	}
}

static void WriteString(char* str, FILE* out)
{
	if (!str || !*str) Write16(0,out);
	else
	{
		size_t len = strlen(str);
		Write16(len,out);
		if (out) fwrite(str,1,len+1,out);
		else
		{
			memcpy(writePtr,(unsigned char*)str,len+1);
			writePtr += len+1;
		}
	}
}

static void WriteBinaryEntry(WORDP D, FILE* out)
{
	unsigned char c;
	writePtr = (unsigned char*)(readBuffer+2); // reserve size space
	if (!D->word) // empty entry
	{
		c = 0;
		WriteString((char*)&c,0);
		unsigned int len = writePtr-(unsigned char*)readBuffer;
		*readBuffer = (unsigned char)(len >> 8);
		readBuffer[1] = (unsigned char)(len & 0x00ff);
		fwrite(readBuffer,1,len,out);
		return;
	}

	WriteString(D->word,0);
	unsigned int bits = 0;
	if (GETMULTIWORDHEADER(D)) bits |= 1 << 0;
	if (D->conjugation) bits |= 1 << 1;
	if (D->plurality) bits |= 1 << 2;
	if (GetComparison(D)) bits |= 1 << 3;
	if (GetMeaningCount(D)) bits |= 1 << 4;
	if (GetGlossCount(D)) bits |= 1 << 5;
	if (D->systemFlags) bits |= 1 << 6;
	if (D->properties) bits |= 1 << 7;
	Write8(bits,0);

	if (D->properties) Write64(D->properties,0);
	if (D->systemFlags) Write64(D->systemFlags,0);
	Write24(D->nextNode,0); 

	if (GETMULTIWORDHEADER(D))
	{
		c = (unsigned char)GETMULTIWORDHEADER(D);
		Write8(c,0); //   limit 255 no problem
	}
	if (D->conjugation) WriteDWord(D->conjugation,0);
	if (D->plurality) WriteDWord(D->plurality,0);
	if (GetComparison(D)) WriteDWord(GetComparison(D),0);
	if (GetMeaningCount(D)) 
	{
		unsigned char c = (unsigned char)GetMeaningCount(D);
		Write8(c,0);  //   limit 255 no problem
		for (unsigned int i = 1; i <= GetMeaningCount(D); ++i) Write32(D->meanings[i],0);
	}
	if (GetGlossCount(D)) 
	{
		unsigned char c = (unsigned char)GetGlossCount(D);
		Write8(c,0); //   limit 255 no problem
		for (unsigned int i = 1; i <= GetGlossCount(D); ++i) 
		{
			Write8(D->glosses[i] >> 24,0);
			WriteString(Index2String(D->glosses[i] & 0x00ffffff),0);
		}
	}
	Write8('0',0);
	unsigned int len = writePtr - (unsigned char*) readBuffer;
	*readBuffer = (unsigned char)(len >> 8);
	readBuffer[1] = (unsigned char)(len & 0x00ff);
	fwrite(readBuffer,1,len,out);
}

void WriteBinaryDictionary()
{
	FILE* out = fopen(UseDictionaryFile("dict.bin"),"wb");
	if (!out) return;
	Write32(maxHashBuckets,out); // bucket size used
	WORDP D = dictionaryBase;
	while (++D < dictionaryFree) WriteBinaryEntry(D,out);
	char x[2];
	x[0] = x[1] = 0;
	fwrite(x,1,2,out); //   end marker for synchronization
	// add datestamp
	strcpy(dictionaryTimeStamp, GetTimeInfo()+4);
	fwrite(dictionaryTimeStamp,1,20,out);
	fclose(out);
	printf("binary dictionary %ld written\r\n",(long int)(dictionaryFree - dictionaryBase));
}

static unsigned char Read8(FILE* in) 
{
	if (in)
	{
		unsigned char x[1];
		return (fread(x,1,1,in) != 1) ? 0 : (*x);
	}
	else return *writePtr++;
}

static unsigned short Read16(FILE* in) 
{
	if (in)
	{
		unsigned char x[2];
		return (fread(x,1,2,in) != 2) ? 0 : ((*x) | (x[1]<<8));
	}
	else 
	{
		unsigned int n = *writePtr++;
		return (unsigned short)(n | (*writePtr++ << 8));
	}
}

static unsigned int Read24(FILE* in)
{
	if (in)
	{
		unsigned char x[3];
		if (fread(x,1,3,in) != 3) return 0;
		return (*x) | ((x[1]<<8) | (x[2]<<16));
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		return n | (*writePtr++ << 16);
	}
}
  
unsigned int Read32(FILE* in)
{
	if (in)
	{
		unsigned char x[4];
		if (fread(x,1,4,in) != 4) return 0;
		unsigned int x3 = x[3];
		x3 <<= 24;
		return (*x) | (x[1]<<8) | (x[2]<<16) | x3 ;
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		return n | (*writePtr++ << 24);
	}
}

uint64 Read64(FILE* in)
{
	if (in)
	{
		unsigned char x[8];
		if (fread(x,1,8,in) != 8) return 0;
		unsigned int x1,x2,x3,x4,x5,x6,x7,x8;
		x1 = x[0]; 
		x2 = x[1];
		x3 = x[2];
		x4 = x[3];
		x5 = x[4];
		x6 = x[5];
		x7 = x[6];
		x8 = x[7];
		uint64 a = x1 | (x2<<8) | (x3<<16) | (x4<<24);
		uint64 b = x5 | (x6<<8) | (x7<<16) | (x8<<24);
		b <<= 16;
		b <<= 16;
		a |= b;
		return a;
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		n |= (*writePtr++ << 24);

		unsigned int n1 = *writePtr++;
		n1 |= (*writePtr++ << 8);
		n1 |= (*writePtr++ << 16);
		n1 |= (*writePtr++ << 24);

		uint64 ans = n1;
		ans <<= 16;
		ans <<= 16;
		return n | ans ;
	}
}

WORDP ReadDWord(FILE* in)
{
	if (in)
	{
		unsigned char x[3];
		if (fread(x,1,3,in) != 3) return 0;
		return Index2Word((x[0]) + (x[1]<<8) + (x[2]<<16));
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		return Index2Word(n);
	}
}

static char* ReadString(FILE* in)
{
	unsigned int len = Read16(in);
	if (!len) return NULL;
	char* str;
	if (in)
	{
		char* buffer = AllocateBuffer();
		if (fread(buffer,1,len+1,in) != len+1) return NULL;
		str = AllocateString(buffer,len);
		FreeBuffer();
	}
	else 
	{
		str = AllocateString((char*)writePtr,len);
		writePtr += len + 1;
	}
	return str;
}

static WORDP ReadBinaryEntry(FILE* in)
{
	writePtr = (unsigned char*) readBuffer;
	unsigned int len = fread(writePtr,1,2,in);
	if (writePtr[0] == 0 && writePtr[1] == 0) return NULL;	//   normal ending in synch
	len = (writePtr[0] << 8 ) | writePtr[1];
	writePtr += 2;
	if (fread(writePtr,1,len-2,in) != len-2) myexit("bad binary dict entry"); // swallow entry

	unsigned int nameLen = *writePtr | (writePtr[1] << 8); // peek ahead
	WORDP D = AllocateEntry();
	char* name = ReadString(0);
	if (!name)  return D;

	D->word = name;
	D->y.length = (unsigned short)nameLen;
	echo = true;
	unsigned int bits = Read8(0);

    if (bits & ( 1 << 7)) D->properties = Read64(0);
	if (bits & ( 1 << 6)) D->systemFlags |= Read64(0);

	bool hasUpperCharacters;
	bool hasUTF8Characters;
	D->hash = Hashit((unsigned char*)name,nameLen,hasUpperCharacters,hasUTF8Characters);
	D->nextNode = Read24(0); 

	if (bits & (1 << 0)) 
	{
		unsigned char c = Read8(0);
		SETMULTIWORDHEADER(D,c);
	}
	if (bits & (1 << 1)) D->conjugation = ReadDWord(0);
	if (bits & (1 << 2)) D->plurality = ReadDWord(0);
	if (bits & (1 << 3)) D->comparison = ReadDWord(0);
	if (bits & (1 << 4)) 
	{
		unsigned char c = Read8(0);
		unsigned int size = (c+1) * sizeof(MEANING);
		MEANING* meanings = (MEANING*) AllocateString(NULL,size); 
		memset(meanings,0,size); 
		D->meanings =  meanings;
		D->meanings[0] = c;
		for (unsigned int i = 1; i <= c; ++i)  
		{
			D->meanings[i] = Read32(0);
			if (D->meanings[i] == 0)
			{
				ReportBug("binary entry meaning is null %s",name)
				myexit("null meaning for binary dict entry");
			}
		}
	}
	if (bits & (1 << 5)) // glosses
	{
		unsigned char c = Read8(0);
		unsigned int size = (c+1) * sizeof(MEANING);
		MEANING* glosses = (MEANING*) AllocateString(NULL,size); 
		memset(glosses,0,size); 
		glosses[0] = c;
		D->glosses =  glosses;
		for (unsigned int i = 1; i <= c; ++i)  
		{
			unsigned int index = Read8(0);
			char* string = ReadString(0);
			D->glosses[i] = String2Index(string) | (index << 24);
		}
	}
	if (Read8(0) != '0')
	{
		printf("Bad Binary Dictionary entry, rebuild the binary dictionary %s\r\n",name);
		ReportBug("Bad Binary Dictionary entry, rebuild the binary dictionary\r\n")
		myexit("bad binary entry rebuilding");
	}
	return D;
}

bool ReadBinaryDictionary() 
{
	FILE* in = FopenStaticReadOnly(UseDictionaryFile("dict.bin")); 
	if (!in) return false;
	unsigned int size = Read32(in); // bucket size used
	if (size != maxHashBuckets) // if size has changed, rewrite binary dictionary
	{
		ReportBug("Binary dictionary uses hash=%d but system is using %d -- rebuilding binary dictionary\r\n",size,maxHashBuckets)
		return false;
	}
	dictionaryFree = dictionaryBase + 1;
	while (ReadBinaryEntry(in));
	unsigned int len = fread(dictionaryTimeStamp,1,20,in);
	dictionaryTimeStamp[len] = 0;
	fclose(in);
	return true;
}

void WriteDictionaryFlags(WORDP D, FILE* out)
{
	uint64 properties = D->properties;
	uint64 bit = START_BIT;	
	while (properties)
	{
		if (properties & bit)
		{
			properties ^= bit;
			char* label = FindNameByValue(bit);
			fprintf(out,"%s ",label);
		}
		bit >>= 1;
	}

	properties = D->systemFlags;
	bit = START_BIT;
	while (properties)
	{
		if (properties & bit)
		{
			char* label = 0;
			if (bit & POSTAG_PRIORITY)
			{
				if (bit & NOUN) label = "posdefault:NOUN";
				else if (bit & VERB) label = "posdefault:VERB";
				else if (bit & ADJECTIVE) label = "posdefault:ADJECTIVE";
				else if (bit & ADVERB) label = "posdefault:ADVERB";
				else if (bit & PREPOSITION) label = "posdefault:PREPOSITION";
				else if (bit & CONJUNCTION) label = "posdefault:CONJUNCTION";
			}
			else if (bit & (BUILD0|BUILD1|TOPIC_NAME | PATTERN_WORD|PATTERN_ALLOWED)) label = 0;			// internal marker or transient script compiler flag
			else label = FindName2ByValue(bit);
			properties ^= bit;
			if (label) fprintf(out,"%s ",label);
		}
		bit >>= 1;
	}
}

char* GetGloss(WORDP D,unsigned int index)
{
	unsigned int count = GetGlossCount(D);
	if (!count) return NULL;
	MEANING* glosses = D->glosses;
	for (unsigned int i = 1; i <= count; ++i)
	{
		if (GlossIndex(glosses[i]) == index) return Index2String(glosses[i]);
	}
	return NULL;
}

static void WriteDictionaryReference(char* label,WORDP D,FILE* out)
{
    if (!D) return; 
	if (D->internalBits & INTERNAL_MARK) return;	// ignore bad links
    fprintf(out,"%s=%s ",label,D->word);
}

void WriteDictionary(WORDP D,uint64 data)
{
	if (D->internalBits & INTERNAL_MARK) return;
	if (*D->word == '$') return;	// var never and money never
	if (stricmp(D->word,"been")) RemoveInternalFlag(D,MARKED_WORD);	// MARKED_WORD used to note topic system request to store this word regardless  -- OR for "been" as AUX_BE
	RemoveSystemFlag(D,BUILD0|BUILD1|HAS_EXCLUDE); 

	// choose appropriate subfile
	char c = toLowercaseData[(unsigned char) *D->word]; 
	char name[40];
	if (IsDigit(c)) sprintf(name,"%c.txt",c); //   main real dictionary
    else if (!IsLowerCase(c)) sprintf(name,"other.txt"); //   main real dictionary
    else sprintf(name,"%c.txt",c);//   main real dictionary
    FILE* out = FopenUTF8WriteAppend(UseDictionaryFile(name));

	//   write out the basics (name meaningcount idiomcount)
	fprintf(out," %s ( ",D->word);
	if (GetMeaningCount(D)) fprintf(out,"meanings=%d ",GetMeaningCount(D));
	if (GetGlossCount(D)) fprintf(out,"glosses=%d ",GetGlossCount(D));

	//   now do the dictionary bits into english
	WriteDictionaryFlags(D,out);
 	fprintf(out,") ");

	//   these must have valuable ->properties on them
	WriteDictionaryReference("conjugate",D->conjugation,out);
	WriteDictionaryReference("plural",D->plurality,out);
	WriteDictionaryReference("comparative",GetComparison(D),out);

	//   show the meanings, with illustrative gloss
	unsigned int count = GetMeaningCount(D);
	
	fprintf(out,"\r\n");

	//   now dump the meanings and their glosses
	for (unsigned int i = 1; i <= count; ++i)
	{
		MEANING M = D->meanings[i];
		fprintf(out,"    %s ", WriteMeaning(M,true)); 
		if (M & SYNSET_MARKER) //   facts for this will be OUR facts
		{
			M = MakeMeaning(D,i) | (M & TYPE_RESTRICTION);
			FACT* F = GetSubjectHead(D);
			while (F)
			{
				if ( M == F->subject) // show up path as information only
				{
					fprintf(out,"(%s) ",WriteMeaning(F->object));
					break;
				}
				F = GetSubjectNext(F);
			}
			if (!F) 
			{
				if ((M & TYPE_RESTRICTION) != NOUN); // we dont believe wordnets other hierarchies.
				else if (Meaning2Word(D->meanings[i]) == D) fprintf(out,"(%s) ",WriteMeaning(D->meanings[i])); // was already a self master
				else fprintf(out,"() "); // show we dont recognize it...
			}
		}
		char* gloss =  GetGloss(D,i);
		if (gloss == NULL) gloss = "";
		fprintf(out,"%s\r\n",gloss);
	}
 
    fclose(out);
}

char* ReadDictionaryFlags(WORDP D, char* ptr,unsigned int* meaningcount, unsigned int * glosscount)
{
	char junk[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,junk);
	uint64 properties = 0;
	while (*junk && *junk != ')' )		//   read until closing paren
	{
		if (!strncmp(junk,"meanings=",9)) 
		{
			if (meaningcount) *meaningcount = atoi(junk+9);
		}
		else if (!strncmp(junk,"glosses=",8)) 
		{
			if (glosscount) *glosscount = atoi(junk+8);
		}
		else if (!strncmp(junk,"#=",2));
		else if (!strcmp(junk,"posdefault:NOUN")) D->systemFlags |= NOUN;
		else if (!strcmp(junk,"posdefault:VERB")) D->systemFlags |= VERB;
		else if (!strcmp(junk,"posdefault:ADJECTIVE")) D->systemFlags |= ADJECTIVE;
		else if (!strcmp(junk,"posdefault:ADVERB")) D->systemFlags |= ADVERB;
		else if (!strcmp(junk,"posdefault:PREPOSITION")) D->systemFlags |= PREPOSITION;
		else if (!strcmp(junk,"posdefault:CONJUNCTION")) D->systemFlags |= CONJUNCTION;
		else 
		{
			uint64 val = FindValueByName(junk);
			if (val) properties |= val;
			else
			{
				val = FindValue2ByName(junk);
				if (val) D->systemFlags |= val;
			}
		}
		ptr = ReadCompiledWord(ptr,junk);
	}
	AddProperty(D,properties);
	return ptr;
}

MEANING AddGloss(WORDP D,char* glossy,unsigned int index)
{ 
	//   cannot add gloss to entries before the freeze (space will free up when transient space chopped back but pointers will be bad).
	//   If some dictionary words cannot add after the fact, none should
	if (dictionaryLocked) return 0;
#ifndef NOGLOSS
	MEANING gloss = String2Index(glossy) | (index << 24);
	MEANING* glosses = D->glosses;

	//   if we run out of room, reallocate gloss space double in size (ignore the hole in memory)
	unsigned int oldCount = GetGlossCount(D);

	//prove we dont already have this here
	for (unsigned int i = 1; i <= oldCount; ++i) if (glosses[i] == gloss) return gloss;

	unsigned int count = oldCount + 1;  
	if (!(count & oldCount)) //   new count has no bits in common with old count, is a new power of 2
	{
		unsigned int size =  (count<<1) * sizeof(MEANING);
		glosses = (MEANING*) AllocateString(NULL,size); 
		memset(glosses,0,size); //   just to be purist
		memcpy(glosses+1,D->glosses+1,oldCount * sizeof(MEANING));
		D->glosses =  glosses;
	}
	*glosses = count;
	return glosses[count] = gloss;
#else
	return 0;
#endif
}

MEANING AddMeaning(WORDP D,MEANING M)
{ //   worst case wordnet meaning count = 75 (break)
	//   meaning is 1-based (0 means generic)

	//   cannot add meaning to entries before the freeze (space will free up when transient space chopped back but pointers will be bad).
	//   If some dictionary words cannot add after the fact, none should
	//   Meanings disambiguate multiple POS per word. User not likely to be able to add words that have
	//   multiple parts of speech.
	if (dictionaryLocked) return 0;

	//   no meaning given, use self with meaning one
	if (!(((ulong_t)M) & MAX_DICTIONARY)) M |= MakeMeaning(D,(1 + GetMeaningCount(D))) | SYNSET_MARKER;

	//   meanings[0] is always the count of existing meanings
	//   Actual space available is always a power of 2.
	MEANING* meanings = D->meanings;

	//   if we run out of room, reallocate meaning space double in size (ignore the hole in memory)
	unsigned int oldCount = GetMeaningCount(D);
	if (oldCount == MAX_MEANING) return 0; // refuse more -- (break and cut)

	//prove we dont already have this here
	for (unsigned int i = 1; i <= oldCount; ++i) 
	{
		if (meanings[i] == M) return M;
		if (M & PREPOSITION && meanings[i] & PREPOSITION) return M;
	}

	unsigned int count = oldCount + 1;  
	if (!(count & oldCount)) //   new count has no bits in common with old count, is a new power of 2
	{
		unsigned int size =  (count<<1) * sizeof(MEANING);
		meanings = (MEANING*) AllocateString(NULL,size); 
		memset(meanings,0,size); //   just to be purist
		memcpy(meanings+1,D->meanings+1,oldCount * sizeof(MEANING));
		D->meanings =  meanings;
	}
	meanings[0] = count;
	return meanings[count] = M;
}

MEANING GetMaster(MEANING T)
{ //   for a specific meaning return node that is master or return general node if all fails.
	if (!T) return 0;
    WORDP D = Meaning2Word(T);
	if (!GetMeaningCount(D)) return MakeMeaning(D,0); // has none, all erased
    unsigned int index = Meaning2Index(T);
	if (index > GetMeaningCount(D))
	{
		ReportBug("Bad meaning index %s %d",D->word,index)
		return MakeMeaning(D,0);
	}
	if (index == 0) return T;
	MEANING old = T;
	MEANING at = GetMeanings(D)[index];
	unsigned int n = 0;
	while (!(at & SYNSET_MARKER)) // find the correct ptr to return. The marked ptr means OUR dict entry is master, not that the ptr points to.
	{
		old = at;
		WORDP X = Meaning2Word(at);
		unsigned int ind = Meaning2Index(at);
		if (ind > GetMeaningCount(X)) 
		{
			ReportBug("syset master failure %s",X->word)
			return old;
		}
		at = GetMeanings(X)[ind];
		if (++n >= 20) break; // force an end arbitrarily
	}
	old &= -1 ^ (SYNSET_MARKER | TYPE_RESTRICTION);	// never return the flags -- FindChild loop wont want them nor will fact creation.
    return old; // the marked synst value
}

void RemoveMeaning(MEANING M, MEANING M1)
{
	M1 &= STDMEANING; 
	
	//   remove meaning and keep only valid main POS values (may not have a synset ptr when its irregular conjugation or such)
	WORDP D = Meaning2Word(M);
	for (unsigned int i = 1; i <= GetMeaningCount(D); ++i)
	{
		MEANING master = D->meanings[i];
		if ((master & STDMEANING) == M1) D->meanings[i] = 0;
	}
}

MEANING ReadMeaning(char* word,bool create,bool precreated)
{// be wary of multiple deletes of same word in low-to-high-order
	char hold[MAX_WORD_SIZE];
	if (*word == '\\' && word[1] && !word[2]) 
	{
		strcpy(hold,word+1);	//   special single made safe, like \[  or \*
		word = hold;
	}

	unsigned int flags = 0;
	unsigned int index = 0;

	char* at = (*word != '~') ? strchr(word,'~') : NULL; 
	if (at && *word != '"' ) // beware of topics or other things, dont lose them. we want xxx~n (one character) or  xxx~digits  or xxx~23n
	{
		if (IsDigit(at[1]))  // number starter  at~3  or   at~3n
		{
			index = atoi(at+1);
			char* p = at;
			while (IsDigit(*++p)); // find end
			if (*p == 'n') flags = NOUN;
			else if (*p == 'v') flags = VERB;
			else if (*p == 'a') flags = ADJECTIVE;
			else if (*p == 'b') flags = ADVERB;
			if (*p == 'z' ||  (*p && p[1] == 'z')) flags |= SYNSET_MARKER; 
			*at = 0; // drop the tail
		}
		if (index == 0) //   at~nz
		{
			if (at[2] && at[2] != ' ' && at[2] != 'z'){;} // insure flag only the last character - write meaning can write multiple types, but only for internal display. externally only 1 type at a time is allowed to be input
			else if (at[1] == 'n') flags = NOUN;
			else if (at[1] == 'v') flags = VERB;
			else if (at[1] == 'a') flags = ADJECTIVE;
			else if (at[1] == 'b') flags = ADVERB;
			if (at[1] == 'z' || at[2] == 'z') flags |= SYNSET_MARKER;
			if (flags) *at = 0;
		}
	}
	if (*word == '"') 
	{
		if (!precreated) 
		{
			strcpy(hold,JoinWords(BurstWord(word,CONTRACTIONS)));
			word = hold;
		}
		else if (word[1] == FUNCTIONSTRING) {;} // compiled script string
		else // some other quoted thing, strip off the quotes, becomes raw text
		{
			if (!word[1]) return NULL;	 // just a " is bad
			strcpy(hold,word+1); // the system should already have this correct if reading a file. dont burst and rejoin
			size_t len = strlen(hold);
			hold[len-1] = 0;	// remove ending dq
			word = hold; // hereinafter, this fact will be written out as `xxxx` instead
		}
	}
	if (!*word) return 0;
	WORDP D = (create) ? StoreWord(word,AS_IS) : FindWord(word,0,PRIMARY_CASE_ALLOWED);
    return (!D)  ? 0 :  (MakeMeaning(D,index) | flags);
}

bool ReadDictionary(char* file)
{
	char junk[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	char* ptr;
    char* equal;
	FILE* in = FopenReadOnly(file); // text dictionary file
	if (!in) return false;
	while (ReadALine(readBuffer,in))
	{
		ptr = ReadCompiledWord(readBuffer,word); // word
		if (!*word) continue;
		ptr = ReadCompiledWord(ptr,junk);	//   read open paren
		if (*junk != '(') ReportBug("bad dictionary alignment")
		WORDP D = StoreWord(word,AS_IS);
		if (stricmp(D->word,word)) ReportBug("Dictionary read does not match original %s %s\r\n",D->word,word)
		unsigned int meaningCount = 0;
		unsigned int glossCount = 0;
		ptr = ReadDictionaryFlags(D,ptr,&meaningCount,&glossCount);

		//   precreate meanings...

		//   read cross-reference attribute ptrs
		while (*ptr)		//   read until closing paren
		{
			ptr = ReadCompiledWord(ptr,word);
			if (!*word) break;
			equal = strchr(word,'=');
			*equal++ = 0;
			if (!strcmp(word,"conjugate")) D->conjugation = StoreWord(equal);
			else if (!strcmp(word,"plural")) D->plurality = StoreWord(equal);
			else if (!strcmp(word,"comparative")) D->comparison = StoreWord(equal);
		}

		//   directly create meanings, since we know the size-- no meanings may be added after this
		if (meaningCount)
		{
			MEANING* meanings = (MEANING*) AllocateString(NULL,(meaningCount+1) * sizeof(MEANING),false,true); 
			meanings[0]= meaningCount;
			D->meanings =  meanings;

			unsigned int glossIndex = 0;
			//   directly create gloss space, since we know the size-- no glosses may be added after this
			if (glossCount)
			{
				MEANING* glosses = (MEANING*) AllocateString(NULL,(glossCount+1) * sizeof(MEANING),false,true); 
				glosses[0] = glossCount;
				D->glosses =  glosses;
			}
			for (unsigned int i = 1; i <= meaningCount; ++i) //   read each meaning
			{
				ReadALine(readBuffer,in);
				char* ptr = ReadCompiledWord(readBuffer,junk);
				D->meanings[i] = ReadMeaning(junk,true,true);
				if (*ptr == '(') ptr = strchr(ptr,')') + 2; // point after the )
				if (glossCount && *ptr && D->meanings[i] & SYNSET_MARKER) 
					D->glosses[++glossIndex] =  String2Index(AllocateString(ptr)) + (i << 24);
			}
			if (glossIndex != glossCount)
			{
				ReportBug("Gloss not matching count");
				myexit("bad gloss count");
			}
		}
	}
	fclose(in);
	return true;
}

MEANING MakeTypedMeaning(WORDP x, unsigned int y, unsigned int flags)
{
	return (!x) ? 0 : (((MEANING)(Word2Index(x) + (((unsigned int)y) << INDEX_OFFSET))) | flags);
}

MEANING MakeMeaning(WORDP x, unsigned int y) //   compose a meaning
{
    return (!x) ? 0 : (((MEANING)(Word2Index(x) + (((unsigned int)y) << INDEX_OFFSET))));
}

WORDP Meaning2Word(MEANING x) //   convert meaning to its dictionary entry
{
    WORDP D = (!x) ? NULL : Index2Word((((ulong_t)x) & MAX_DICTIONARY)); 
	return D;
}

unsigned int GetMeaningType(MEANING T)
{
    if (T == 0) return 0;
	WORDP D = Meaning2Word(T);
    unsigned int index = Meaning2Index(T);
	if (index) T = D->meanings[index]; //   change to synset head for specific meaning
	else if (T & TYPE_RESTRICTION) return T & TYPE_RESTRICTION; //   generic word type it must be
	D = Meaning2Word(T);
	return (unsigned int) (D->properties & PART_OF_SPEECH);
}

MEANING FindSynsetParent(MEANING T,unsigned int which) //  presume we are at the master, next wordnet
{
    WORDP D = Meaning2Word(T);
    unsigned int index = Meaning2Index(T);
    FACT* F = GetSubjectHead(D); //   facts involving word 
	unsigned int count = 0;
    while (F)
    {
        FACT* at = F;
        F = GetSubjectNext(F);
        if (at->verb == Mis) // wordnet meaning
		{
			//   prove indexes mate up
			if (index && index != Meaning2Index(at->subject)) continue; // must match generic or specific precisely
			if (count++ == which) return at->object; //   next set/class in line
		}
    }
    return 0;
}

MEANING FindSetParent(MEANING T,int n) //   next set parent
{
    WORDP D = Meaning2Word(T);
    unsigned int index = Meaning2Index(T);
    FACT* F = GetSubjectHead(D); //   facts involving word 
    while (F)
    {
        FACT* at = F;
		F = GetSubjectNext(F);
        if (!(at->verb == Mmember)) continue;
        
		//   prove indexes mate up
		unsigned int localIndex = Meaning2Index(at->subject); //   what fact says
        if (index != localIndex) continue; //   must match generic or specific precisely

        if (--n == 0)  return at->object; // next set/class in line
    }
    return 0;
}

char* WriteMeaning(MEANING T,bool withPos)
{
	if (!T) return "";
    WORDP D = Meaning2Word(T);

	if ((T & MEANING_BASE) == T) return D->word; 

	//   need to annotate the value
    static char mybuffer[150];
	strcpy(mybuffer,D->word); 
	char* at = mybuffer + strlen(mybuffer);
   
	//   index 
	unsigned int index = Meaning2Index(T);
	if (index > 9) 
	{
		*at++ = '~';
		*at++ = (char)((index / 10) + '0');
		*at++ = (char)((index % 10) + '0');
	}
	else if (index)
	{
		*at++ = '~';
		*at++ = (char)(index + '0');
	}

	if (withPos)
	{
		if ((T & TYPE_RESTRICTION) && !index) *at++ = '~';	// pos marker needed on generic
		if (T & NOUN) *at++ = 'n'; 
		else if (T & VERB) *at++ = 'v'; 
		else if (T & ADJECTIVE) *at++ = 'a';
		else if (T & ADVERB) *at++ = 'b';
	}
	if (T & SYNSET_MARKER) *at++ = 'z';
	*at = 0;
    return mybuffer;
}

void NoteLanguage()
{
	FILE* out = fopen("language.txt","wb"); 
	fprintf(out,"%s\r\n",language); // current default language
	fclose(out);
}

static void ReadSubstitutes(char* file,unsigned int fileFlag)
{
    char original[MAX_WORD_SIZE];
    char replacement[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(file); // substitutes
    if (!in) return;
    while (ReadALine(readBuffer,in) != 0) 
    {
        if (*readBuffer == '#' || *readBuffer == 0) continue;
        char* ptr = ReadCompiledWord(readBuffer,original); //   original phrase
		
        if (*original == 0 || *original == '#') continue;
		//   replacement can be multiple words joined by + and treated as a single entry.  
		ptr = ReadCompiledWord(ptr,replacement);    //   replacement phrase
		WORDP D = FindWord(original,0,PRIMARY_CASE_ALLOWED);	//   do we know original already?
		if (D && D->systemFlags & SUBSTITUTE)
		{
			Log(STDUSERLOG,"Currently have a substitute for %s in %s\r\n",original,readBuffer);
			continue;
		}
		D = StoreWord(original,AS_IS,SUBSTITUTE); //   original word
		D->internalBits |= fileFlag;
		unsigned int n = BurstWord(D->word);
		char wd[MAX_WORD_SIZE];
		strcpy(wd,JoinWords(1));
		// now determine the multiword headerness...
		char* word = wd;
		if (*word == '<') ++word;		// do not show the < starter for lookup
		size_t len = strlen(word);
		if (len > 1 && word[len-1] == '>')  word[len-1] = 0;	// do not show the > on the starter for lookup
		WORDP E = StoreWord(word);		// create the 1-word header
		if (n > GETMULTIWORDHEADER(E)) SETMULTIWORDHEADER(E,n);	//   mark it can go this far for an idiom

		WORDP S = NULL;
		if (replacement[0] != 0 && replacement[0] != '#') 	//   with no substitute, it will just erase itself
		{
			D->w.substitutes = S = StoreWord(replacement,AS_IS,SUBSTITUTE_RECIPIENT);  //   the valid word
	
			// for the emotions (like ~emoyes) we want to be able to reverse access, so make them a member of the set
			if (*S->word == '~') CreateFact(MakeMeaning(D),Mmember,MakeMeaning(S));
		}

        //   if original has hyphens, replace as single words also. Note burst form for initial marking will be the same
        bool hadHyphen = false;
		char copy[MAX_WORD_SIZE];
		strcpy(copy,original);
        ptr = copy;
        while (*++ptr) // replace all alphabetic hypens using _
        {
            if (*ptr == '-' && IsAlpha(ptr[1])) 
            {
                *ptr = '_';
                hadHyphen = true;
            }
        }
        if (hadHyphen) 
        {
			D = FindWord(copy);	//   do we know original already?
			if (D && D->systemFlags & SUBSTITUTE)
			{
				ReportBug("Already have a substitute for %s of %s",original,readBuffer)
				continue;
			}
	
			D = StoreWord(copy,0,SUBSTITUTE);
			D->internalBits |= fileFlag;
 			D->w.substitutes = S;
        }
	}
    fclose(in);
}

void ReadWordsOf(char* file,uint64 mark)
{
    char word[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(file); // scriptcompile nonwords allowed OR lowercase title words
    if (!in) return;
    while (ReadALine(readBuffer,in) != 0) 
    {
        ReadCompiledWord(readBuffer,word); 
        if (*word != 0 && *word != '#') StoreWord(word,mark); 
	}
    fclose(in);
}

static void ReadCanonicals(char* file)
{
    char original[MAX_WORD_SIZE];
    char replacement[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(file); // canonicals
    if (!in) return;
    while (ReadALine(readBuffer,in) != 0) 
    {
        if (*readBuffer == '#' || *readBuffer == 0) continue;

        char* ptr = ReadCompiledWord(readBuffer,original); //   original phrase
        if (*original == 0 || *original == '#') continue;
        ptr = ReadCompiledWord(ptr,replacement);    //   replacement word
		WORDP D = StoreWord(original);
		WORDP R = StoreWord(replacement);
		canonicalMap[D->word] = R->word;
	}
    fclose(in);
}

void ReadAbbreviations(char* file)
{
    char word[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(file);
    if (!in) return;
    while (ReadALine(readBuffer,in) != 0) 
    {
		ReadCompiledWord(readBuffer,word); 
		if (*word != 0 && *word != '#')  StoreWord(word,0,KINDERGARTEN);
	}
    fclose(in);
}

static void ReadQueryLabels(char* file)
{
    char word[MAX_WORD_SIZE];
    FILE* in = FopenReadOnly(file); // queries
    if (!in) return;
    while (ReadALine(readBuffer,in) != 0) 
    {
        if (*readBuffer == '#' ||  *readBuffer == 0) continue;
        char* ptr = ReadCompiledWord(readBuffer,word);    // the name
        if (*word == 0) continue;
		ptr = SkipWhitespace(ptr); // in case excess blanks before control string
        WORDP D = StoreWord(word,0,QUERY_KIND);
		char* at = strchr(ptr,' '); // in case has blanks after control string
		if (at) *at = 0;
 	    D->w.userValue = AllocateString(ptr);    
    }
    fclose(in);
}

void ReadLiveData()
{
	// read pos rules of langauge
	uint64 xdata[MAX_POS_RULES * MAX_TAG_FIELDS];
	char*  xcommentsData[MAX_POS_RULES];
	data = xdata;
	commentsData = xcommentsData;
	tagRuleCount = 0;
	char word[MAX_WORD_SIZE];
	sprintf(word,"LIVEDATA/%s",language);
	WalkDirectory(word,ReadPosPatterns,0);
	tags = (uint64*)AllocateString((char*) xdata,tagRuleCount * MAX_TAG_FIELDS * sizeof(uint64),true,false);
	comments = 0;
	bool haveComments = true;
#ifdef IOS // applications dont want comments
	haveComments = false;
#endif
#ifdef NOMAIN
	haveComments = false;
#endif
	if (haveComments) comments = (char**)AllocateString((char*) xcommentsData,tagRuleCount * sizeof(char*));
	ReadSubstitutes("LIVEDATA/expandabbreviations.txt",ESSENTIALS_FILE);
	ReadSubstitutes("LIVEDATA/systemessentials.txt",ESSENTIALS_FILE);
	ReadSubstitutes("LIVEDATA/substitutes.txt",SUBSTITUTIONS_FILE);
	ReadSubstitutes("LIVEDATA/contractions.txt",CONTRACTIONS_FILE);
	ReadSubstitutes("LIVEDATA/interjections.txt",INTERJECTIONS_FILE);
	ReadSubstitutes("LIVEDATA/british.txt",BRITISH_FILE);
	ReadSubstitutes("LIVEDATA/spellfix.txt",SPELLING_FILE);

	ReadCanonicals("LIVEDATA/canonical.txt");
	ReadQueryLabels("LIVEDATA/queries.txt");
	ReadWordsOf("LIVEDATA/lowercaseTitles.txt",LOWERCASE_TITLE);
}

static bool ReadAsciiDictionary()
{
    char buffer[50];
	unsigned int n = 0;
	bool found = false;
	for (char i = '0'; i <= '9'; ++i)
	{
		sprintf(buffer,"%c.txt",i);
		if (!ReadDictionary(UseDictionaryFile(buffer))) ++n;
		else found = true;
	}
	for (char i = 'a'; i <= 'z'; ++i)
	{
		sprintf(buffer,"%c.txt",i);
		if (!ReadDictionary(UseDictionaryFile(buffer))) ++n;
		else found = true;
	}
	if (!ReadDictionary(UseDictionaryFile("other.txt"))) ++n;
	else found = true;
	if (n) printf("Missing %d word files\r\n",n);

	return found;
}

void VerifyEntries(WORDP D,uint64 junk) // prove meanings have synset heads
{
	if (!D->word) return;	// just a hash bucket, no entry there
	if (D->internalBits & (INTERNAL_MARK|WORDNET_ID)) return;
	unsigned int count = GetMeaningCount(D);
	unsigned int synsetHeads;
	for (unsigned int i = 1; i <= count; ++i)
	{
		synsetHeads = 0;
		unsigned int counter = 0;
		MEANING M = D->meanings[i];
		WORDP X = Meaning2Word(M);
		unsigned int index = Meaning2Index(M);
		if (M & SYNSET_MARKER);
		else while (X != D) // run til we loop once back to this entry, counting synset heads we see
		{
			if (X->internalBits & INTERNAL_MARK)
			{
				ReportBug("Synset references dead entry %s word: %s meaning: %d\r\n",X->word,D->word,i)
				break;
			}
			if (M & SYNSET_MARKER) ++synsetHeads; // prior was a synset head
			index = Meaning2Index(M);
			if (index == 0) break; // generic pointer
			if (!X->meanings)
			{
				M = 0;
				ReportBug("Missing synsets for %s word: %s meaning: %d\r\n",X->word,D->word,i)
				break;
			}
			if (GetMeaningCount(X) < index)
			{
				ReportBug("Missing synset index %s %s\r\n",X->word,D->word)
				break;
			}
			M = X->meanings[index];
			X = Meaning2Word(M);
			if (++counter > MAX_SYNLOOP) break; // in case of trouble
		}
		if (M & SYNSET_MARKER) ++synsetHeads; // prior was a synset head
		if (synsetHeads != 1 || counter > MAX_SYNLOOP) ReportBug("Bad synset list %s heads: %d count: %d\r\n",D->word,synsetHeads,counter)
	}
	if (D->conjugation) 
	{
		WORDP E = D->conjugation;
		while (E != D)
		{
			if (!E)
			{
				ReportBug("Missing conjugation %s \r\n",D->word)
				break;
			}
			if (E->internalBits & INTERNAL_MARK)
			{
				ReportBug("Deleted conjucation %s %s \r\n",D->word,E->word)
				break;
			}
			E = E->conjugation;
		}
	}
	if (D->plurality) 
	{
		WORDP E = D->plurality;
		while (E != D)
		{
			if (!E)
			{
				ReportBug("Missing plurality %s \r\n",D->word)
				break;
			}
			if (E->internalBits & INTERNAL_MARK)
			{
				ReportBug("Deleted plurality %s %s \r\n",D->word,E->word)
				break;
			}
			E = E->plurality;
		}
	}
	if (GetComparison(D)) 
	{
		WORDP E = GetComparison(D);
		while (E != D)
		{
			if (!E)
			{
				ReportBug("Missing comparison %s \r\n",D->word)
				break;
			}
			if (E->internalBits & INTERNAL_MARK)
			{
				ReportBug("Deleted comparison %s %s \r\n",D->word,E->word)
				break;
			}
			E = GetComparison(E);
		}
	}
}

void LoadDictionary()
{
	if (!ReadBinaryDictionary()) //   if binary form not there or wrong hash, use text form (slower)
	{
		ReadAbbreviations("LIVEDATA/abbreviations.txt"); // needed for burst/tokenizing
		if (ReadAsciiDictionary())
		{
			*currentFilename = 0;
			WalkDictionary(VerifyEntries); // prove good before writeout
			remove(UseDictionaryFile("facts.bin")); // insure no erroneous binary of facts
			remove(UseDictionaryFile("dict.bin")); 
			WriteBinaryDictionary(); //   store the faster read form of dictionary
		}
	}
	else  ReadAbbreviations("LIVEDATA/abbreviations.txt"); // needed for burst/tokenizing - a change in this file will require rebuilds of topic data
	*currentFilename = 0;
	fullDictionary = (!stricmp(language,"ENGLISH")) || (dictionaryFree-dictionaryBase) > 170000; // a lot of words are defined, must be a full dictionary.
	InitFactWords(); 
}

#define BUILDCONCEPT(name,word) {name = StoreWord(word,0,CONCEPT);}

void ExtendDictionary()
{
	Mburst = MakeMeaning(StoreWord("^burst"));
	Mchatoutput = MakeMeaning(StoreWord("chatoutput"));
	MconceptComment = MakeMeaning(StoreWord("^concept"));
	MgambitTopics = MakeMeaning(StoreWord("^gambittopics"));
	Mintersect = MakeMeaning(StoreWord("^intersect"));
	Mkeywordtopics = MakeMeaning(StoreWord("^keywordtopics"));
 	Mmoney = MakeMeaning(StoreWord("~moneynumber",0,CONCEPT));
	Mnumber = MakeMeaning(StoreWord("~number",0,CONCEPT));
	MadjectiveNoun  = MakeMeaning(StoreWord("~adjectivenoun",0,CONCEPT));
	Mpending = MakeMeaning(StoreWord("^pending"));
	DunknownWord = StoreWord("unknown-word");

	// generic concepts the engine marks automatically
	BUILDCONCEPT(Dadult,"~adultword");
	BUILDCONCEPT(Dbasicform,"~basic_form"); 
	BUILDCONCEPT(Dchild,"~childword");
	BUILDCONCEPT(Dfemalename,"~femalename"); 
	BUILDCONCEPT(Dhumanname,"~humanname"); 
	BUILDCONCEPT(Dmalename,"~malename"); 
    BUILDCONCEPT(Dplacenumber,"~placenumber"); 
	BUILDCONCEPT(Dpronoun,"~pronoun");
	BUILDCONCEPT(Dpropername,"~propername"); 
	Mphrase = MakeMeaning(StoreWord("~phrase",0,CONCEPT));
	MabsolutePhrase = MakeMeaning(StoreWord("~absolutephrase",0,CONCEPT));
	MtimePhrase = MakeMeaning(StoreWord("~timeephrase",0,CONCEPT));
	BUILDCONCEPT(Dclause,"~clause"); 
	BUILDCONCEPT(Dverbal,"~verbal"); 
	BUILDCONCEPT(Dtime,"~timeword"); 
	BUILDCONCEPT(Dunknown,"~unknownword"); 
	// predefine builtin sets with no engine variables
	unsigned int i = 0;
	char* ptr;
	while ((ptr = predefinedSets[i++]) != 0) StoreWord(ptr,0,CONCEPT);
}

char* FindCanonical(char* word, unsigned int i,bool notNew)
{
    WORDP D = FindWord(word,0,PRIMARY_CASE_ALLOWED);
	if (i == 1)
	{
		WORDP S = FindWord(word,0,SECONDARY_CASE_ALLOWED);
		if (S && IsLowerCase(*S->word))  D = S;
	}
    if (D) 
	{
		char* answer = GetCanonical(D);
		if (answer) return answer; //   special canonical form (various pronouns typically)
	}

    //    numbers - use digit form
	char* number;
    if (IsNumber(word))
    {
        char word1[MAX_WORD_SIZE];
        if (strchr(word,'.') || strlen(word) > 9)  //   big numbers need float
        {
            float y;
			if (*word == '$') y = (float)atof(word+1);
			else y = (float)atof(word);
            int x = (int)y;
            if (((float) x) == y) sprintf(word1,"%d",x); //   use int where you can
            else sprintf(word1,"%.2f",atof(word)); 
        }
		else if (GetCurrency(word,number)) sprintf(word1,"%d",atoi(number));
#ifdef WIN32
        else sprintf(word1,"%I64d",Convert2Integer(word)); // integer
#else
        else sprintf(word1,"%lld",Convert2Integer(word)); // integer
#endif
        WORDP N = StoreWord(word1,ADJECTIVE|NOUN|ADJECTIVE_CARDINAL|NOUN_CARDINAL); // digit format cannot be ordinal
		return N->word;
    }
 
	// before seeing if canoncial can come from verb, see if it is from a known noun.  "cavities" shouldnt create cavity as a verb
	char* noun = NULL;
	size_t len = strlen(word);
	if (word[len-1] == 's') noun = GetSingularNoun(word,true,true); // do we already KNOW this as a an extension of a noun

    //   VERB
    char* verb = GetInfinitive(word,(noun) ? true : notNew);
    if (verb) 
    {
        WORDP V = FindWord(verb,0);
        verb = (V) ? V->word : NULL;
    }
	if (verb) return verb; //   we prefer verb base-- gerunds are nouns and verbs for which we want the verb

    //   NOUN
    noun = GetSingularNoun(word,true,notNew);
    if (noun) 
    {
        WORDP  N = FindWord(noun,0);
        noun = (N) ? N->word : NULL;
    }
	if (noun) return noun;
    
	//   ADJECTIVE
    char* adjective = GetAdjectiveBase(word,(noun) ? false : notNew);
    if (adjective) 
    {
        WORDP A = FindWord(adjective,0);
        adjective = (A) ? A->word : NULL;
    }
	if (adjective) return adjective;
 
	//   ADVERB
    char* adverb = GetAdverbBase(word,(noun) ? false : notNew);
    if (adverb) 
    {
        WORDP A = FindWord(adverb,0);
        adverb = (A) ? A->word : NULL;
    }
	if (adverb) return adverb;

	return (D && D->properties & PART_OF_SPEECH) ? D->word : NULL;
}

bool IsHelper(char* word)
{
    WORDP D = FindWord(word,0);
    return (D && D->properties & (AUX_VERB|AUX_VERB_BITS) );
}

bool IsFutureHelper(char* word)
{
	WORDP D = FindWord(word,0);
    return (D &&  D->properties & AUX_VERB_FUTURE);
}    
	
bool IsPresentHelper(char* word)
{
	WORDP D = FindWord(word,0);
	return (D && D->properties & AUX_VERB && D->properties & (VERB_PRESENT | VERB_PRESENT_3PS | AUX_VERB_PRESENT));
}

bool IsPastHelper(char* word)
{
	WORDP D = FindWord(word,0);
    return (D && D->properties & AUX_VERB && D->properties & (AUX_VERB_PAST | VERB_PAST));
}

void DumpDictionaryEntry(char* word,unsigned int limit)
{
	char name[MAX_WORD_SIZE];
	strcpy(name,word);
	MEANING M = ReadMeaning(word,false,true);
	unsigned int index = Meaning2Index(M);
	WORDP D = Meaning2Word(M);
	if (D > dictionaryLocked) D = 0;	// debugging may have forced this to store, its not in base system
	if (limit == 0) limit = 5; // default
	Log(STDUSERLOG,"\r\n%s: ",name);

	uint64 properties = (D) ? D->properties : 0;
	uint64 bit = START_BIT;	
	while (bit)
	{
		if (properties & bit) Log(STDUSERLOG,"%s ",FindNameByValue(bit));
		bit >>= 1;
	}
	properties = (D) ? D->systemFlags : 0;
	bit = START_BIT;
	bool once = true;
	while (bit)
	{
		if (properties & bit)
		{
			char word[MAX_WORD_SIZE];
			if (bit & POSTAG_PRIORITY) 
			{
				if (once) Log(STDUSERLOG," POS-tiebreak: ");
				once = false;
				Log(STDUSERLOG,"%s ",FindNameByValue(bit));
			}
			else Log(STDUSERLOG,"%s ",MakeLowerCopy(word,FindName2ByValue(bit)));
		}
		bit >>= 1;
	}

	WORDP entry = NULL;
	WORDP canonical = NULL;
	properties = (D) ? D->properties : 0;
	char* tilde = strchr(name+1,'~');
	if (tilde && IsDigit(tilde[1])) *tilde = 0;	// turn off specificity
	uint64 flags = (name[0] != '~') ? GetPosData(name,entry,canonical) : 0; 
	bit = START_BIT;
	bool extended = false;
	while (flags)
	{
		if (flags & bit)
		{
			flags ^= bit;
			if (!D || !(properties & bit)) // bits beyond what was directly known in dictionary
			{
				if (!extended) Log(STDUSERLOG," Implied: ");
				extended = true;
				char* label = FindNameByValue(bit);
				Log(STDUSERLOG,"%s ",label);
			}
		}
		bit >>= 1;
	}


	if (!D) D = entry;
	if (!D) 
	{
		if (canonical) Log(STDUSERLOG," cannonical: %s ",canonical->word);
		Log(STDUSERLOG,"\r\n    Not in the dictionary\r\n");
		return;
	}
	
	if (canonical) Log(STDUSERLOG," cannonical: %s ",canonical->word);
#ifndef DISCARDTESTING
	unsigned int basestamp = inferMark;
#endif
	NextinferMark();
#ifndef DISCARDTESTING
	if (D->systemFlags & CONCEPT  && !(D->systemFlags & TOPIC_NAME)) Log(STDUSERLOG,"concept (%d members) ",CountSet(D,basestamp));
#endif
	if (D->systemFlags & SUBSTITUTE) 
	{
		Log(STDUSERLOG,"substitute=");
		if (GetSubstitute(D)) Log(STDUSERLOG,"%s ",GetSubstitute(D)->word+1); // skip internal | on name
		else Log(STDUSERLOG,"  ");
	}
	if (D->systemFlags & FUNCTION_NAME) 
	{
		char* kind;
		if (D->systemFlags & IS_PATTERN_MACRO) kind = (char*)"pattern";
		else if (D->systemFlags & IS_TABLE_MACRO) kind = (char*) "table";
		else kind = (char*)"output";
		if (!D->x.codeIndex) Log(STDUSERLOG,"user %s function %s ",kind,D->w.fndefinition);
		else Log(STDUSERLOG,"systemfunction %d", D->x.codeIndex);
	}
	if (*D->word == '%') Log(STDUSERLOG,"systemvar ");
	if (*D->word == '$')
	{
		char* val = GetUserVariable(D->word);
		Log(STDUSERLOG,"VariableValue= \"%s\" ",val);
	}
	Log(STDUSERLOG,"\r\n");

	if (D->conjugation) 
	{
		Log(STDUSERLOG,"  conjugationLoop= ");
		WORDP E = D->conjugation;
		while (E != D)
		{
			Log(STDUSERLOG,"-> %s ",E->word);
			E = E->conjugation;
		}
		Log(STDUSERLOG,"\r\n");
	}
	if (D->plurality) 
	{
		Log(STDUSERLOG,"  pluralLoop= ");
		WORDP E = D->plurality;
		while (E != D)
		{
			Log(STDUSERLOG,"-> %s ",E->word);
			E = E->plurality;
		}
		Log(STDUSERLOG,"\r\n");
	}
	if (GetComparison(D)) 
	{
		Log(STDUSERLOG,"  comparativeLoop= ");
		WORDP E = GetComparison(D);
		while (E != D)
		{
			Log(STDUSERLOG,"-> %s ",E->word);
			E = GetComparison(E);
		}
		Log(STDUSERLOG,"\r\n");
	}
	
	// find special parser mark values of word
	WORDP X = FindWord("~special_english_attributes");
	if (X)
	{
		SetParserMarks(X,true);
		bool heading = false;

		// mark the words now -- they must be direct members of marked sets
		FACT* F = GetSubjectHead(D);
		while (F)
		{
			if (F->verb == Mmember)
			{
				WORDP Y = Meaning2Word(F->object);
				if (Y->properties & AS_IS) 
				{
					if (!heading)
					{
						Log(STDUSERLOG,"ParserMarks: ");
						heading = true;
					}
					Log(STDUSERLOG,"%s ",Y->word);
				}
			}
			F = GetSubjectNext(F);
		}
		if (heading) Log(STDUSERLOG,"\r\n\r\n");
		else Log(STDUSERLOG,"\r\n");
		SetParserMarks(X,false);
	}

	//   now dump the meanings
	unsigned int count = GetMeaningCount(D);
	for (unsigned int i = 1; i <= count; ++i)
	{
		if (index && i != index) continue;
		MEANING M = D->meanings[i];
		char* gloss;
		MEANING master = GetMaster(M);
		gloss = GetGloss(Meaning2Word(master),Meaning2Index(master));

		if (!gloss) gloss = "";
		Log(STDUSERLOG," %d: %s %s\r\n",i,WriteMeaning(M & STDMEANING,true),gloss);

		M = D->meanings[i] & STDMEANING;
		bool did = false;
		while (Meaning2Word(M) != D)
		{
			if (!did) 
			{
				did = true;
				Log(STDUSERLOG,"    synonyms: ");
			}
			MEANING next = Meaning2Word(M)->meanings[Meaning2Index(M)];
			if (next & SYNSET_MARKER) Log(STDUSERLOG," *%s ",WriteMeaning(M));	// mark us as master for display
			else Log(STDUSERLOG," %s ",WriteMeaning(M)); 
			M = next & STDMEANING;
		}
		if (!did) Log(STDUSERLOG,"    synonyms: ");
 		if (D->meanings[i] & SYNSET_MARKER) Log(STDUSERLOG," *%s ",WriteMeaning(M)); 
		else Log(STDUSERLOG," %s ",WriteMeaning(M)); 
		Log(STDUSERLOG,"\r\n"); //   header for this list
	}

	if (D->v.patternStamp) Log(STDUSERLOG,"Pstamp- %d\r\n",D->v.patternStamp);
	if (D->inferMark) Log(STDUSERLOG,"Istamp- %d\r\n",D->inferMark);
	if (GETMULTIWORDHEADER(D)) Log(STDUSERLOG,"MultiWordHeader length: %d\r\n",GETMULTIWORDHEADER(D));

	// show concept/topic members
	FACT* F = GetSubjectHead(D);
	bool did = false;
	Log(STDUSERLOG,"Direct Sets: ");
	while (F)
	{
		if (index && Meaning2Index(F->subject) && Meaning2Index(F->subject) != index ){;} // wrong path member
		if (F->verb == Mmember)
		{
			did = true;
			Log(STDUSERLOG,"%s ",Meaning2Word(F->object)->word);
		}
		F = GetSubjectNext(F);
	}
	Log(STDUSERLOG,"\r\n");

	char* buffer = AllocateBuffer();
	Log(STDUSERLOG,"Facts:\r\n");

	count = 0;
	F = GetSubjectHead(D);
	while (F)
	{
		if (F->verb != Mis && F->verb != Mmember) // non-structural facts
		{
			Log(STDUSERLOG,"  %s",WriteFact(F,false,buffer,false,true));
			if (++count >= limit) break;
		}
		F = GetSubjectNext(F);
	}

	F = GetVerbHead(D);
	count = limit;
	while (F)
	{
		if (F->verb != Mmember && F->verb != Mis)
		{
			Log(STDUSERLOG,"  %s",WriteFact(F,false,buffer,false,true));
			if (++count >= limit) break;
		}
		F = GetVerbNext(F);
	}
	
	F = GetObjectHead(D);
	count = 0;
	while (F)
	{
		if (F->verb != Mmember && F->verb != Mis)
		{
			Log(STDUSERLOG,"  %s\r\n",WriteFact(F,false,buffer,false,true));
			if (++count >= limit) break;
		}
		F = GetObjectNext(F);
	}
	Log(STDUSERLOG,"Index: %d\r\n",Word2Index(D));

	FreeBuffer();
}

#ifdef COPYRIGHT

Per use of the WordNet dictionary data....

 This software and database is being provided to you, the LICENSEE, by  
  2 Princeton University under the following license.  By obtaining, using  
  3 and/or copying this software and database, you agree that you have  
  4 read, understood, and will comply with these terms and conditions.:  
  5   
  6 Permission to use, copy, modify and distribute this software and  
  7 database and its documentation for any purpose and without fee or  
  8 royalty is hereby granted, provided that you agree to comply with  
  9 the following copyright notice and statements, including the disclaimer,  
  10 and that the same appear on ALL copies of the software, database and  
  11 documentation, including modifications that you make for internal  
  12 use or for distribution.  
  13   
  14 WordNet 3.0 Copyright 2006 by Princeton University.  All rights reserved.  
  15   
  16 THIS SOFTWARE AND DATABASE IS PROVIDED "AS IS" AND PRINCETON  
  17 UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR  
  18 IMPLIED.  BY WAY OF EXAMPLE, BUT NOT LIMITATION, PRINCETON  
  19 UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES OF MERCHANT-  
  20 ABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE  
  21 OF THE LICENSED SOFTWARE, DATABASE OR DOCUMENTATION WILL NOT  
  22 INFRINGE ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR  
  23 OTHER RIGHTS.  
  24   
  25 The name of Princeton University or Princeton may not be used in  
  26 advertising or publicity pertaining to distribution of the software  
  27 and/or database.  Title to copyright in this software, database and  
  28 any associated documentation shall at all times remain with  
  29 Princeton University and LICENSEE agrees to preserve same.  
#endif
