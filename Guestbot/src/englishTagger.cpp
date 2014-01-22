#include "common.h"

extern unsigned int tagRuleCount;

static bool reverseWords = false;
void SetRole(unsigned int i, unsigned int role,bool revise = false);
#define MAINLEVEL 1

static WORDP firstAux = NULL;

bool ApplyRules(bool guessAcceptable);

#ifdef INFORMATION
A rule consists of 4 64bit values, representing 4 comparator words (result uses one of them also), and a uint64 control word
The control word represents 6 bytes (describing how to interpret the 4 patterns and result), and a 1-byte offset locator to orient the pattern
The result is to either discard named bits or to restrict to named bits on the PRIMARY location, using the bits of the primary include...

A std rule has 4 tests it can consult, anchored around a starting word. A big rule uses a 2nd rule as a continuation, to see 4 more tests.
basic/0:  		6-CONTROL_OP	3-CONTROL_FLAGS		PART2_BIT		1-GUESS_BIT		2-RESULT_INDEX	48-PATTERN_BITS 
value1:  		6-CONTROL_OP	3-CONTROL_FLAGS		KEEP_BIT	    ----------------3-OFFSET_SHIFT	48-PATTERN_BITS
value2:  		6-CONTROL_OP	3-CONTROL_FLAGS		REVERSE_BIT 	1-GUESS_BIT		1-?		        48-PATTERN_BITS
value3: 		6-CONTROL_OP	3-CONTROL_FLAGS		TRACE_BIT		1-GUESS_BIT		2-?	PART1_BIT	48-PATTERN_BITS

The pattern component identies what std word properties are to be checked for from D->properties.
Result (1st field) indicates which test has the word whose bits we want to modify.
Offset_shift (2nd field) indicates where first test word is relative to base word (+ or -)
Control flags are: SKIP, STAY, NOT. 
The control_op  specifies what test to perform on that field.

Rules are executed in forward order only, so later rules can presume earlier rules have already processed appropriately.

# should cardinal adjectives be under DETERMINER and not ADJECTIVE?
# the word HOME is wonderfully overloaded as noun,verb,adjective,adverb for testing

# TRACE on start of a rule result allows you to watch it
# NOGUESS		# do no guessing
# INVERTWORDS	# test sentence words in opposite order
# ENABLE_STATS	# GUESS2 will use statistics to choose meanings
# ENABLE_BASE	# GUESS3 will use the default value of word types (not needed in our test suite yet)

# HAS = any bit matching (could be only bit it has)
# IS = bits from this collection match and no other bits are left over
# !IS will fail if result is ambiguous so it may or may not be.
# INCLUDE = has one or more of these bits AND has other bits -- DO NOT ! this, must be the ONLY field which also has * on it
# -- beware of using !IS (should use HAS) because it will match while still ambiguous
# A pattern should have only one "include", the bits you are deciding to keep or discard. Other places should use IS or HAS.

# SKIP takes a test and a value set.  But it AUTOMATICALLY skips over every phrase or clause already marked

# START, END check location of this word relative to sentence start and end
# ISORIGINAL = is this  word
# ISCANONICAL = is this root word
# ISMEMBER = is canonical word a direct member of this set
# HASPROPERTY = check for systemflag presence of OTHER_SINGULAR, OTHER_PLURAL. absence means nothing but presence is important so dont use ! (defined for determiner and pronoun)
# START = this is just before 1st word of sentence
# ENDSWITH - matches the characters given
# ISQUESTION aux or qword - sentence begins with possible aux verb or question word
# 0 = no bits
# ! inverts test
# STAY = dont move to next sentence word yet
# x means control/bits not used

# reverse means going backwards... Before the word still has offset -1 (means actually after the word)

# actions: DISCARD, KEEP, can also put GUESS, GUESS1, GUESS2, GUESS3 and TEST on the result or DISABLE
# the current position should always be tested as INCLUDE, because it should have too many bits. and this must be that fields first test (to set result bits)
# tests with HAS mean it may or may not have been fully resolved yet, you are making a heuristic guess

# adjectives cant end sentence UNLESS object of a be-type verb
# try to make rules self-standing, not merely a default happening after a prior rule fails to fire
# rules should be independent of each other and the order they are run in. Periodically test inverse order of rules by saying "INVERTRULES" before the first rule.

# top level parts of speech are: 
# PREDETERMINER DETERMINER NOUN_BITS VERB_TENSES AUX_VERB_BITS ADJECTIVE_BITS ADVERB_BITS PREPOSITION CONJUNCTION_COORDINATE CONJUNCTION_SUBORDINATE
# THERE_EXISTENTIAL TO_INFINITIVE PRONOUN_BITS POSSESSIVE_BITS COMMA PAREN PARTICLE NOUN_INFINITIVE

#endif

uint64* data;
char** commentsData;

// zone data is transient per call to assignroles
#define ZONE_LIMIT 25
static unsigned char zoneBoundary[ZONE_LIMIT];	 // where commas are
static unsigned char zoneSubject[ZONE_LIMIT];    // where 1st noun is in zone
static unsigned char zoneVerb[ZONE_LIMIT];		// where verb is in zone
static unsigned int	 zoneData[ZONE_LIMIT];		// what can be found in a zone - noun before verb, verb, noun after verb
static unsigned char zoneMember[MAX_SENTENCE_LENGTH];
static unsigned int zoneIndex;			
static int predicateZone; // where is main verb found
static unsigned int currentZone;


#ifdef JUNK
Subject complements are after linking verbs. We label noun complements as direct objects and adjective complements as subject_complement.
Object complements follow a direct object and is noun-like or adjective-like- "The convention named him President" -- not appositive?  verb takes object complement.???
	"The clown got the children *excited"
Verb complement is direct or indirect object of verb. 
	Additionally some verbs expect object complements which are directly nouns, eg FACTITIVE_NOUN_VERB "we elected him *president" which has an omitted "as"  - we elected him as president
	Some verbs expect object complements to be adjectives. eg FACTITIVE_ADJECTIVE_VERB  "we made him *happy"
	Some verbs expect object complements to be infinitive eg CAUSAL_TOINFINITIVE_VERB or DIRECTOBJECT_INFINITIVE_COMPLEMENT  "we want him to go"
			if (parseFlags[i] & (FACTITIVE_ADJECTIVE_VERB|FACTITIVE_NOUN_VERB))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; //

Two nouns in a row:
1. appositive (renaming a noun after - "my *dog *Bob") - can be on any noun
2. Adjectival noun ("*bank *teller") - can be on any noun
3. indirectobject/directobject ("I gave *Bob the *ball) - expected by verb
4. object object-complement ("the convention named *Bob *President" ) expected by verb 
5. omitted clause starter?

To Infinitives verbals can be nouns, postnominal adjectives, or adverb. Cannot be appositive.
1. postnominal adjective:  "her plan to subsidize him was sound"
2. object - "she wanted to raise taxes"
3. subject- "to watch is fun"
4. adverb - "he went to college to study math"

parentheitical infinitive:  to sum up, I worked

adjective_noun 

How do you tell postnominal adjective from adverb--  
can postnominal adjective NOT follow object of a phrase?  
But "it is time to go" or "that was a sight to see"

//  ~factitive_adjective_Verbs take object complement adjective after direct object noun
// ~factitive_noun_verbs take object complement noun after direct object noun
// ~adjectivecomplement_taking_noun_infinitive adjectives can take a noun to infinitive after them as adjective "I was able to go"
//  ~causal_to_infinitive_verbs  verb indirectobject "to infinitive maybe with to and maybe not" as direct object 

Basic main sentence requirements on verb are:
	mainverb
	mainverb subjectcomplement (linking verbs like "be" take noun or adjective as subject complement though we label noun as direct object and adjective as subject complement)
	mainverb directobject  (directobject can be noun, to-infinitve, infinitive, as well as clause, depending on verb)
	mainverb indirectobject directobject
	mainverb directobject objectcomplement (objectcomplement can be noun, infinitive, adjective depending on verb)
#endif

// parseFlags values
#define FACTITIVE_ADJECTIVE_VERB 1  
#define FACTITIVE_NOUN_VERB 2 
#define CAUSAL_TOINFINITIVE_VERB 4 
								//  // "I made john *run"  now under systemflag DIRECTOBJECT_INFINITIVE_COMPLEMENT (object_complement)
#define QUOTEABLE_VERB 8  // he said "I love you"
#define ADJECTIVE_TAKING_NOUN_INFINITIVE 16 // "he is *determined to go home" -- complement
#define OMITTABLE_THAT_VERB	32 // that in object clause might be omitted

static char* tagOps [] = 
{
	"?","HAS","IS","INCLUDE","ISORIGINAL","POSITION","ISCANONICAL","ISMEMBER","RESETLOCATION","HAS2VERBS",
	"CANONLYBE","HASPROPERTY","HASALLPROPERTIES","ENDSWITH","ORIGINALVALUE","HASCANONICALPROPERTY","ISQWORD","ISQUESTION","ISABSTRACT","NONEAFTER",
	"NONEBEFORE","POSSIBLEMASSNOUN","?","PRIORCANONICAL", // 23
	"?",
	"?",
};

unsigned char bitCounts[MAX_SENTENCE_LENGTH]; // number of tags still to resolve in this word position
int lastClause = 0; 
int lastVerbal = 0;
int lastPhrase = 0;
int startSentence = 0;
void InitEnglishParser();

static void InitPosData()
{
	firstRealWord = 1;
	if (wordStarts[firstRealWord][0] == '\'' || wordStarts[firstRealWord][0] == '"') ++firstRealWord;
	lastRealWord = wordCount;
	if (wordStarts[lastRealWord][0] == '\'' || wordStarts[lastRealWord][0] == '"') --lastRealWord;

	canSysFlags[0] = canSysFlags[wordCount+1] = lcSysFlags[0] = lcSysFlags[wordCount+1] = 0;
#ifndef DISCARDPARSER
	InitEnglishParser();
#endif

	memset(originalLower,0,sizeof(WORDP)*(wordCount+2));
	memset(originalUpper,0,sizeof(WORDP)*(wordCount+2));
	memset(canonicalLower,0,sizeof(WORDP)*(wordCount+2));
	memset(canonicalUpper,0,sizeof(uint64)*(wordCount+2));
	memset(posValues,0,sizeof(uint64)*(wordCount+2));
	memset(lcSysFlags,0,sizeof(uint64)*(wordCount+2));
	memset(canSysFlags,0,sizeof(uint64)*(wordCount+2));

	wordStarts[0] = wordStarts[wordCount+1] = "";
	knownWords = 0;

	// count words beginning lowercase
	lowercaseWords = 0;
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		if (IsLowerCase(*wordStarts[i])) ++lowercaseWords; 
	}

	// initialize the words with whatever pos they MIGHT be, from dictionary as well as what might be inferred
    for (unsigned int i = 1; i <= wordCount; ++i)
    {
		char* original =  wordStarts[i];

		// change case of word if appropriate
		WORDP D = FindWord(original,0,LOWERCASE_LOOKUP);
		if (D && !(D->properties & PART_OF_SPEECH)) D = NULL;
		WORDP E  = FindWord(original,0,UPPERCASE_LOOKUP);
		if (tokenControl & ONLY_LOWERCASE) E = NULL;
			
		if (E && !(E->properties & PART_OF_SPEECH)) E = NULL;
		// has no known uppercase meaning and was given as upper case, but we have a known lowercase meaning - convert it to lower case
		if (!E && IsUpperCase(*original) && D)
		{
			if (tokenControl & STRICT_CASING && i != firstRealWord){;} // not allowed on any other word but 1st or 1st word is quote and we do 2nd
			else original = wordStarts[i] = D->word;
		}
	
		WORDP entry;
		WORDP canonical;
		uint64 flags = GetPosData(original,entry,canonical);
		if (flags & POSSESSIVE && *original == '\'' && !original[1]) // is this a possessive quotemark or a QUOTE?
		{
			size_t len = (i > 1) ? strlen(wordStarts[i-1]) : 0;
			if (!len || wordStarts[i-1][len-1] != 's')  flags = QUOTE;	// change from possessive to QUOTE if word before it does not end in s
		}
		else if (flags & PRONOUN_BITS) // pronouns are lower case (except for "I")
		{
			originalLower[i] = entry;
			canonicalLower[i] = canonical;
		}
		else if (IsUpperCase(*original)) // he gave us the word in upper case. Was it "real" or just weird or sentence start
		{
			originalUpper[i] = entry;
			canonicalUpper[i] = canonical;

			if (original[1] == 0 && *original == 'I'); // dont lower case I
			else if ( canonical && !stricmp(canonical->word,"unknown-word")) // dont try to change
			{
				if (!entry) originalUpper[i] = StoreWord(original); // accept the word on faith
			}
			else if (lowercaseWords > 1 && i != 1 && !IsUpperCase(*original) ) flags &= NOUN_PROPER; // if he uses a lot of lower case words, he MEANS upper case unless he is shouting
			else if (IsSentenceStart(i)) // may be spurious sentence start
			{
				// if is title or name and next word is also proper, it was intended
				if (entry && entry->properties & (NOUN_TITLE_OF_ADDRESS|NOUN_FIRSTNAME)) {;}
				else // dont know it was intended
				{
					char word[MAX_WORD_SIZE];
					MakeLowerCopy(word,original);
					uint64 uflags = GetPosData(word,entry,canonical,false); // NOT first try
					if (!canonicalLower[i]) canonicalLower[i] = canonical;
					originalLower[i] = entry;
					flags |= uflags;
				}
			}
			else if (!entry) 
			{
				char word[MAX_WORD_SIZE];
				strcpy(word,original);
				*word = toLowercaseData[(unsigned char)*word];
				uint64 uflags = GetPosData(word,entry,canonical);
				if (uflags && canonical)  // IGNORE the upper case word...lest we have multiple noun flags (proper and non) and get confused
				{
					flags = uflags;
					originalUpper[i] = canonicalUpper[i]  = NULL;
				}
				if (!canonicalLower[i]) canonicalLower[i] = canonical;
				originalLower[i] = entry;
			}
		}
		else  // given as lower case - can we consider upper casing it?
		{
			originalLower[i] = entry;
			canonicalLower[i] = canonical;

			WORDP X;
			bool useUpper = false;
			bool allowUpper = true;
			if (tokenControl & NO_IMPERATIVE) allowUpper = false;
			else 
			{
				//allowUpper = entry && (entry->properties & NOUN) && (entry->properties & (VERB|ADJECTIVE|ADVERB|PREPOSITION)) == 0;
				allowUpper = true;
			}
			X = FindWord(original,0,UPPERCASE_LOOKUP);

			if (tokenControl & ONLY_LOWERCASE){;}
			else if (tokenControl & STRICT_CASING && i != 1) {;} // cannot change from lower to upper
			else if (allowUpper && X && (X->properties & NOUN))
			{
				// there is an uppercase interpreteation... see if we should be that -- Did superman do it
				if (!(posValues[i-1] & (ADJECTIVE_BITS|POSSESSIVE|PRONOUN_POSSESSIVE|DETERMINER|PREDETERMINER) )) 
				{
					useUpper = true;
					if (!canonicalLower[i])
					{
						strcpy(wordStarts[i],X->word);
						originalLower[i] = NULL;
					}
				}
			}

			if (tokenControl & ONLY_LOWERCASE) {;}
			else if (tokenControl & STRICT_CASING && i != 1) {;} // cannot change from lower to upper
			else if (E && entry == canonical) useUpper = true; // we dont know it as lower case but it was given in lower case and we do know an upper case of it. Force upper case
			
			if (useUpper)
			{
				uint64 uflags = GetPosData(X->word,entry,canonical);
				if (uflags) flags |= uflags;
				if (!canonicalUpper[i]) canonicalUpper[i] = canonical;
				originalUpper[i] = entry;
			}
		}
		posValues[i] = flags;
		if (originalLower[i]) lcSysFlags[i] = originalLower[i]->systemFlags; // from lower case

		if (FindWord(original)) ++knownWords; // known as lower or upper
	}

	 // get all possible flags first
	for (unsigned int i = 1; i <= wordCount; ++i)
	{
		if (canonicalLower[i]) canSysFlags[i] = canonicalLower[i]->systemFlags;
		allOriginalWordBits[i] = posValues[i];	// for dual verb to know what was possible
		posValues[i] &= TAG_TEST;  // only the bits we care about for pos tagging
		if (posValues[i] & FOREIGN_WORD) // words like "a" which are foreign and regular, we want regular if it has such a meaning since we are postagging english.
		{
			posValues[i] ^= FOREIGN_WORD;
			if (posValues[i] == 0) posValues[i] |= FOREIGN_WORD;
		}
		if (*wordStarts[i] == '~') posValues[i] = 0;	// interjection
		// generator's going down... rewrite here
		if (posValues[i] & POSSESSIVE && wordStarts[i][1] && posValues[i-1] & NOUN_SINGULAR && posValues[i+1] & VERB_PRESENT_PARTICIPLE)
		{
			wordStarts[i] = AllocateString("is");
			originalLower[i] = FindWord(wordStarts[i]);
			posValues[i] = AUX_VERB|AUX_VERB_PRESENT;
			canonicalLower[i] = FindWord("be");
		}
	}

	if (tokenControl & NO_IMPERATIVE && posValues[firstRealWord] & VERB_TENSES && posValues[firstRealWord+1] & NOUN_BITS)
	{
		WORDP imperative = FindWord("~legal_imperatives");
		unsigned int i = firstRealWord;
		if (!stricmp(wordStarts[firstRealWord],"please")) ++i;
		if (canonicalLower[i] && SetContains(MakeMeaning(imperative),MakeMeaning(canonicalLower[i]))){;}
		else posValues[i] &= -1 ^ (VERB_TENSES|VERB);	// no command sentence starts
	}
	if (tokenControl & NO_VERB)
	{
		WORDP imperative = FindWord("~legal_imperatives");
		// block other verbs as well..
		for (unsigned int i = firstRealWord+1; i <= lastRealWord; ++i)
		{
			if ( posValues[i] & VERB_TENSES && posValues[i] & NOUN_BITS && !(posValues[firstRealWord] & VERB_PRESENT_PARTICIPLE))
			{
				if (SetContains(MakeMeaning(imperative),MakeMeaning(canonicalLower[i])));
				else posValues[i] &= -1 ^ VERB_TENSES;	
			}
		}
	}
}

static void PerformPosTag()
{
	if (prepareMode == POS_MODE || prepareMode == POSVERIFY_MODE){;} // told to try regardless
	else if (wordCount <= 2 && !(posValues[firstRealWord] & (PRONOUN_BITS | DETERMINER | PREPOSITION | AUX_VERB_BITS | QWORD))) return; // short things are unreliable unless obvious how they start
	else if (tokenControl & DO_POSTAG ) {;} 
	else if (tokenControl & DO_CONDITIONAL_POSTAG && knownWords == wordCount)  {;} // only if we know all the words
	else return;

#ifdef WIN32
	unsigned int start = 0;
	if (prepareMode == POSTIME_MODE) start = GetTickCount();
#endif

	for (unsigned int i = 1; i <= wordCount; ++i) bitCounts[i] = BitCount(posValues[i]);
	if (trace & POS_TRACE && prepareMode != PREPARE_MODE) Log(STDUSERLOG,"%s",DumpAnalysis(posValues,"\r\nOriginal POS: ",true,true));

	// mark the values of the words
	WORDP D = FindWord("~special_english_attributes");
	if (D)
	{
		SetParserMarks(D,true);

		// mark the words now -- they must be direct members of marked sets
		for (unsigned int i = firstRealWord; i <= lastRealWord; ++i)
		{
			if (!canonicalLower[i]) continue;
			FACT* F = GetSubjectHead(canonicalLower[i]);
			while (F)
			{
				if (F->verb == Mmember)
				{
					WORDP X = Meaning2Word(F->object);
					if (X->properties & AS_IS) parseFlags[i] |= X->inferMark;
				}
				F = GetSubjectNext(F);
			}
		}

		SetParserMarks(D,false);
	}

	if (!ApplyRules(true)) 
		tokenFlags |= FAULTY_PARSE;
#ifdef WIN32
	if (prepareMode == POSTIME_MODE) posTiming += GetTickCount() - start;
#endif
}

static void SetCanonical()
{
	// now set canonical lowercase forms
	for (unsigned int i = 1; i <= wordCount; ++i)
    {
		char* original =  wordStarts[i];
		if (originalLower[i]) original = originalLower[i]->word;
		uint64 pos = posValues[i] & (TAG_TEST|PART_OF_SPEECH);
		if (!pos && !(*original == '~')) posValues[i] = pos = NOUN;		// default it back to something
		WORDP D = FindWord(original);
		char* canon =  GetCanonical(D);
		if (posValues[i] == DETERMINER && original[1] == 0)  // treat "a" as not a letter A
		{
			canon = NULL;
			canonicalLower[i] = originalLower[i];
		}
		// a word like "won" has noun, verb, adjective meanings. We prefer a canonical that's different from the original
		if (canon && IsUpperCase(*canon)) canonicalUpper[i] = FindWord(canon);
		else if (canon) canonicalLower[i] = FindWord(canon);
		else if (pos & NUMBER_BITS); // must occur before verbs and nouns, since "second" is a verb and a noun
		else if (pos & (VERB_TENSES | NOUN_GERUND | NOUN_INFINITIVE|ADJECTIVE_PARTICIPLE) ) 
		{
			canonicalLower[i] = FindWord(GetInfinitive(original,false));
		}
		else if (pos & (NOUN_BITS - NOUN_GERUND)  || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (pos & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && canonicalUpper[i] && canonicalUpper[i]->properties & NOUN) // can it be upper case interpretation?
			{
				// if ONLY upper case interpretation
				if (!(pos & (VERB_TENSES|NOUN_SINGULAR|NOUN_PLURAL|ADJECTIVE_NOUN)) && !canonicalLower[i]) 
				{
					original = wordStarts[i] = canonicalUpper[i]->word; // make it upper case
					originalLower[i] = canonicalLower[i] = 0;	// blow away any lower meaning
				}
			}
			if (canonicalLower[i] && canonicalLower[i]->properties & (DETERMINER|NUMBER_BITS));
			else if (IsAlpha(*original) &&  canonicalLower[i] && !strcmp(canonicalLower[i]->word,"unknown-word"));	// keep unknown-ness
			else if (pos & NOUN_BITS && !canonicalUpper[i]) canonicalLower[i] = FindWord(GetSingularNoun(original,true,false));
		}
		else if (pos & (ADJECTIVE_BITS - ADJECTIVE_PARTICIPLE - ADJECTIVE_NOUN) || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (canonicalLower[i] && canonicalLower[i]->properties & NUMBER_BITS);
			else canonicalLower[i] = FindWord(GetAdjectiveBase(original,false));

			// for adjectives that are verbs, like married, go canonical to the verb if adjective is unchanged
			if (canonicalLower[i] && !strcmp(canonicalLower[i]->word,original))
			{
				char* infinitive = GetInfinitive(original,false);
				if (infinitive) canonicalLower[i] = FindWord(infinitive);
			}
		}
		else if (pos & ADVERB_BITS || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (canonicalLower[i] && canonicalLower[i]->properties & NUMBER_BITS);
			else canonicalLower[i] = FindWord(GetAdverbBase(original,false));
			// for adverbs that are adjectives, like faster, go canonical to the adjective if adverb is unchanged
			if (canonicalLower[i] && !strcmp(canonicalLower[i]->word,original))
			{
				char* adjective = GetAdjectiveBase(original,false);
				if (adjective) canonicalLower[i] = FindWord(adjective);
			}
		}
		else if (pos & (PRONOUN_BITS|CONJUNCTION_BITS|PREPOSITION|DETERMINER|PUNCTUATION|COMMA|PAREN)) canonicalLower[i] = FindWord(original);
		else if (*original == '~') canonicalLower[i] = FindWord(original);
		else if (!IsAlpha(*original)) canonicalLower[i] = FindWord(original);
		
		if (canonicalLower[i]) wordCanonical[i] = canonicalLower[i]->word;
		else if (canonicalUpper[i]) wordCanonical[i] = canonicalUpper[i]->word;
		else wordCanonical[i] = wordStarts[i];
	}
	SetTense();
}

static void FinishTagging()
{
	unsigned int foreign = 0;
	for (unsigned int i = firstRealWord; i <= lastRealWord; ++i) if (posValues[i] & FOREIGN_WORD && !(posValues[i] & PART_OF_SPEECH)) ++foreign;
	bool noparse = false;
	if (((foreign * 10) / wordCount) >= 5) 
	{
		noparse = true;
		tokenFlags |= FOREIGN_TOKENS;
	}

	if (!noparse) SetCanonical(); // dont process with gesture data intruding
	else
	{
		for (unsigned int i = 1; i <= wordCount; ++i) 
		{
			wordCanonical[i] = wordStarts[i];
			canonicalUpper[i] = canonicalLower[i] = 0;
		}
	}
	if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) 
	{
		Log(STDUSERLOG,"%s",DumpAnalysis(posValues,"Tagged POS",false,true));
		DumpSentence(0);
	}
}

void English_TagIt() // get the set of all possible tags. Parse if one can to reduce this set and determine word roles
 {
	InitPosData();
	PerformPosTag();
	FinishTagging();
}

void ReadPosPatterns(char* file,uint64 junk)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadOnly(file);
	if (!in) return;
	uint64 val;
	uint64 flags = 0;
	uint64 v = 0;
	uint64 offsetValue;
	uint64* dataptr;
	char comment[MAX_WORD_SIZE];
	while (ReadALine(readBuffer,in)) // read new rule or comments
	{
		char* ptr = ReadCompiledWord(readBuffer,word);
		if (!*word) continue;
		if (*word == '#' ) 
		{
			strcpy(comment,readBuffer);
			continue;
		}
		if (!stricmp(word,"INVERTWORDS")) // run sentences backwards
		{
			reverseWords = true;
			continue;
		}

		int c = 0;
		int offset;
		bool reverse = false;
		unsigned int limit = MAX_TAG_FIELDS;
		bool big = false;

		dataptr = data + (tagRuleCount * MAX_TAG_FIELDS);
		uint64* base = dataptr;
		memset(base,0,sizeof(uint64) * 8);	// clear all potential rule info for a double rule
		bool skipped = false;

		if (!stricmp(word,"TRACE"))
		{
			base[3] |= TRACE_BIT;
			ptr = ReadCompiledWord(ptr,word);
		}
		bool backwards = false;

		if (!stricmp(word,"reverse"))
		{
			reverse = true;
			backwards = true;
			ptr = ReadCompiledWord(ptr,word);
			c = 0;
			if (!IsDigit(word[0]) && word[0] != '-')
			{
				printf("Missing reverse offset  %s rule: %d comment: %s\r\n",word,tagRuleCount,comment);
				return;
			}
			c = atoi(word);
			if ( c < -3 || c > 3) // 3 bits
			{
				printf("Bad offset (+-3 max)  %s rule: %d comment: %s\r\n",word,tagRuleCount,comment);
				return;
			}
		}
		else if (!IsDigit(word[0]) && word[0] != '-') continue; // not an offset start of rule
		else 
		{
			c = atoi(word);
			if ( c < -3 || c > 3) // 3 bits
			{
				printf("Bad offset (+-3 max)  %s rule: %d comment: %s\r\n",word,tagRuleCount,comment);
				return;
			}
		}
		offset = (reverse) ? (c + 1) : (c - 1); 
		offsetValue = (uint64) ((c+3) & 0x00000007); // 3 bits offset
		offsetValue <<= OFFSET_SHIFT;
		int resultIndex = -1;
		unsigned int includes = 0;
		bool doReverse = reverse;

		for (unsigned int i = 1; i <= (limit*2); ++i)
		{
			unsigned int kind = 0;
			if (reverse) 
			{
				reverse = false;
				--offset;
			}
			else ++offset;
			flags = 0;
resume:
			// read control for this field
			ptr = ReadCompiledWord(ptr,word);
			if (!*word || *word == '#')
			{
				ReadALine(readBuffer,in);
				ptr = readBuffer;
				--i;
				continue;
			}
			if (!stricmp(word,"debug"))  // just a debug label so we can stop here if we want to understand reading a rule
				goto resume;

			// LAST LINE OF PATTERN
			if (!stricmp(word,"GUESS") || !stricmp(word,"GUESS1") || !stricmp(word,"GUESS2") || !stricmp(word,"GUESS3") || 
				 !stricmp(word,"KEEP") || !stricmp(word,"DISCARD") || !stricmp(word,"DISABLE")) 
			{
				if (i < 5) while (i++ < 5) *dataptr++;  // use up blank fields
				else if (i > 5 &&  i < 9) while (i++ < 9) *dataptr++;  // use up blank fields
				ptr = readBuffer;
				break;
			}

			if ( i == 5) // we are moving into big pattern territory
			{
				big = true; // extended pattern
				*(dataptr-1) |= PART1_BIT; // mark last field as continuing to 8zone
			}

			if (!stricmp(word,"STAY")) 
			{ 
				if ( doReverse) ++offset;
				else --offset;
				kind |= STAY;
				goto resume;
			}
			if (!stricmp(word,"SKIP")) // cannot do wide negative offset and then SKIP to fill
			{ 
				skipped = true;
				if ( resultIndex == -1)
				{
					if (!reverse) printf("Cannot do SKIP before the primary field -- offsets are unreliable (need to use REVERSE) Rule: %d comment: %s at line %d in %s\r\n",tagRuleCount,comment,currentFileLine,currentFilename);
					else printf("Cannot do SKIP before the primary field -- offsets are unreliable Rule: %d comment: %s at line %d in %s\r\n",tagRuleCount,comment,currentFileLine,currentFilename);
					return;
				}

				if ( doReverse) ++offset;
				else --offset;
				kind |= SKIP;
				goto resume;
			}

			if (*word == 'x') val = 0; // no field to process
			else if (*word == '!')
			{
				if (!stricmp(word+1,"STAY"))
				{
					printf("Cannot do !STAY (move ! after STAY)  Rule: %d comment: %s at line %d in %s\r\n",tagRuleCount,comment,currentFileLine,currentFilename);
					return;
				}
				val = FindValueByName(word+1);
				if ( val == 0)
				{
					printf("Bad notted control word %s rule: %d comment: %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
					return;
				}

				if (!stricmp(word+1,"include") )
				{
					printf("Use !has instead of !include-  %s rule: %d comment: %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
					return;
				}
				kind |= NOTCONTROL;
				if (!stricmp(word+1,"start")) flags = 1;
				else if (!stricmp(word+1,"first")) flags = 1;
				else if (!stricmp(word+1,"end")) flags = 10000;
				else if (!stricmp(word+1,"last")) flags = 10000;
			}
			else
			{
				val = FindValueByName(word);
				if ( val == 0 || val > LASTCONTROL)
				{
					printf("Bad control word %s rule: %d comment: %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
					return;
				}
				if (!stricmp(word,"include")) ++includes;
				if (!stricmp(word,"start")) flags |= 1;
				else if (!stricmp(word,"first")) flags |= 1;
				else if (!stricmp(word,"end")) flags |= 10000;
				else if (!stricmp(word+1,"last")) flags = 10000;
			}
			flags |= val << OP_SHIFT;	// top byte
			flags |= ((uint64)kind) << CONTROL_SHIFT;
			if (i == ( MAX_TAG_FIELDS + 1)) flags |= PART2_BIT;	// big marker on 2nd
			if (flags) // there is something to test
			{
				// read flags for this field
				bool subtract = false;
				bool once = false;
				while (ALWAYS) 
				{
					ptr = ReadCompiledWord(ptr,word);
					if (!*word || *word == '#') break;	// end of flags
					uint64 baseval = flags >> OP_SHIFT;
					if (baseval == ISCANONICAL || baseval == ISORIGINAL || baseval == ENDSWITH || baseval == PRIORCANONICAL) 
					{
						if (FindValueByName(word) && stricmp(word,"not") && !once) 
						{
							printf("Did you intend to use ORIGINALVALUE for %s rule: %d %s at line %d in %s?\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
							once = true;
						}
						v = MakeMeaning(StoreWord(word));
					}
					else if (IsDigit(word[0])) v = atoi(word); // for POSITION 
					else if (!stricmp(word,"reverse")) v = 1;	// for RESETLOCATION
					else if (!stricmp(word,"ALL")) v = TAG_TEST;
					else if ( word[0] == '-')
					{
						subtract = true;
						v = 0;
					}
					else if ( *word == '*')
					{
						if (offset != 0)
						{
							printf("INCLUDE * must be centered at 0 rule: %d comment: %s at line %d in %s\r\n",tagRuleCount,comment,currentFileLine,currentFilename);
							return;
						}
						if ( resultIndex != -1)
						{
							printf("Already have pattern result bits %s rule: %d comment: %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
							return;
						}
						resultIndex = i-1;
						v = 0;
					}
					else if (baseval == ISMEMBER) 
					{
						v = MakeMeaning(FindWord(word));
						if (!v)
						{
							printf("Failed to find set %s - POS tagger incomplete because build 0 not yet done.\r\n",word);
						}
					}
					else if (baseval == ISQUESTION)
					{
						if (!stricmp(word,"aux")) v = AUXQUESTION;
						else if (!stricmp(word,"qword")) v = QWORDQUESTION;
						else printf("Bad ISQUESTION %s\r\n",word);
					}
					else
					{
						v = FindValueByName(word);
						if ( v == 0)
						{
							v = FindValue2ByName(word);
							if (!v)
							{
								printf("Bad flag word %s rule: %d %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
								return;
							}
						}
						if (v & BASIC_POS && baseval != HASALLPROPERTIES && baseval != HASPROPERTY)
						{
							printf("Bad flag word overlaps BASIC bits %s rule: %d %s at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
							return;
						}
						if ( subtract ) 
						{
							flags &= -1 ^ v;
							v = 0;
							subtract = false;
						}
					}
					flags |= v;
				}
			}
			if (includes > 1) 
			{
				printf("INCLUDE should only be on primary field - use HAS Rule: %d %s at line %d in %s\r\n",tagRuleCount,comment,currentFileLine,currentFilename);
				return;
			}
			if (i == 2) flags |= offsetValue;	// 2nd field gets offset shift
			*dataptr++ |= flags; 
	
			ReadALine(readBuffer,in);
			ptr = readBuffer;
		} // end of fields loop

		// now get results data
		ptr = ReadCompiledWord(ptr,word);
		if (!stricmp(word,"GUESS") || !stricmp(word,"GUESS1") || !stricmp(word,"GUESS2") || !stricmp(word,"GUESS3") || !stricmp(word,"KEEP") || !stricmp(word,"DISCARD") || !stricmp(word,"DISABLE")) {;}
		else 
		{
			printf("Too many fields before result %s: %d %s\r\n",word,tagRuleCount,comment);
			return;
		}
		if (doReverse) base[2] |=  REVERSE_BIT;
		while (ALWAYS)
		{
			if (!stricmp(word,"GUESS")) *base |= GUESS_BIT;
			else if (!stricmp(word,"GUESS1")) 
			{
				*base |= GUESS_BIT;
				base[2] |= GUESS_BIT;
			}
			else if (!stricmp(word,"GUESS2")) 
			{
				*base |= GUESS_BIT;
				base[3] |= GUESS_BIT;
			}
			else if (!stricmp(word,"GUESS3")) 
			{
				*base |= GUESS_BIT;
				base[2] |= GUESS_BIT;
				base[3] |= GUESS_BIT;
			}
			else if (!stricmp(word,"DISABLE")) base[1] =  ((uint64)HAS) << CONTROL_SHIFT; // 2nd pattern becomes HAS with 0 bits which cannot match
			else break;
			ptr = ReadCompiledWord(ptr,word);
		}
		if (!stricmp(word,"KEEP")) base[1] |= KEEP_BIT;
		else if (!stricmp(word,"DISCARD")) {;}
		else
		{
			printf("Too many fields before result? %s  rule: %d comment: %s  at line %d in %s\r\n",word,tagRuleCount,comment,currentFileLine,currentFilename);
			return;
		}
		if ( resultIndex == -1)
		{
			printf("Needs * on result bits %s rule: %d comment: %s\r\n",word,tagRuleCount,comment);
			return;
		}

		*base |= ((uint64)resultIndex) << RESULT_SHIFT;
		if (backwards && !skipped) printf("Running backwards w/o a skip? Use forwards with minus start. %d %s.   at line %d in %s \r\n",tagRuleCount,comment,currentFileLine,currentFilename);
		
		commentsData[tagRuleCount] = AllocateString(comment);
		++tagRuleCount;
		if (big) 
		{
			commentsData[tagRuleCount] = AllocateString(" ");
			++tagRuleCount;		// double-size rule
		}
	}
}

static char* PosBits(uint64 bits, char* buff)
{
	while (bits)  // shows lowest order bits first
	{  
		uint64 oldbits = bits;
		bits &= (bits - 1); 
  		strcat(buff," ");
		strcat(buff,FindNameByValue(oldbits ^ bits));
	}  
	return buff;
}

static char* PropertyBits(uint64 bits, char* buff)
{
	while (bits)  // shows lowest order bits first
	{  
		uint64 oldbits = bits;
		bits &= (bits - 1); 
  		strcat(buff," ");
		strcat(buff,FindName2ByValue(oldbits ^ bits));
	}  
	return buff;
}

static char* OpDecode(uint64 field)
{
	static char buff[500];
	unsigned int control = (unsigned int) ( (field >> CONTROL_SHIFT) &0x01ff);
	strcpy(buff, (control & NOTCONTROL) ? "!" : "");

	uint64 bits = field & PATTERN_BITS;
	control >>= CTRL_SHIFT; // get the op
	strcat(buff, tagOps[control]);
	if (control == HAS || control == IS  || control == ORIGINALVALUE || control == CANONLYBE) PosBits(bits,buff);
	else if (control == POSITION)
	{
		if (bits == 1) strcat(buff," START");
		else if (bits > 100) strcat(buff," END");
	}
	else if (control == HASCANONICALPROPERTY || control == HASPROPERTY || control == HASALLPROPERTIES) PropertyBits(bits,buff);
	else if (control == ISQUESTION)
	{
		if (bits & AUXQUESTION) strcat(buff," AUX");
		if (bits & QWORDQUESTION) strcat(buff," QWORD");	// may be a question started with a question word "how are you"
	}
	else if (control == ISCANONICAL || control == ISORIGINAL)
	{
		WORDP D = Meaning2Word((unsigned int)bits);
		strcat(buff," ");
		strcat(buff,D->word);
	}
	return buff;
}

static int TestTag(int &i, int control, uint64 bits,int direction,bool tracex)
{
	bool notflag = false;
	static bool endwrap = false;	// allow endwrap ONLY immediately after successful end test going forward
	if (control & NOTCONTROL) notflag = true;
	bool skip = (control & SKIP) ? true : false;
	control >>= CTRL_SHIFT; // get the op
	if ( i <= 0 || (unsigned int)i > lastRealWord) 
	{
		if (skip) return false;	// cannot stay any longer
		// endwrap allowed
		if ((unsigned int) i > lastRealWord && endwrap && direction == 1) i = firstRealWord;
		else if ( i < (int)firstRealWord  && endwrap && direction == -1) i = lastRealWord;
		else if (control == ISQUESTION) {;}
		else if (control == PRIORCANONICAL) i = lastRealWord;
		else return (notflag) ? true : false;
	}
	endwrap = false;
	int answer = false;
	switch(control)
	{
		case HAS: // the bit is among those present of which there are at least 2
			answer = (posValues[i] & bits) != 0;
			break;
		case IS: // The bit is what is given --  If word is ambiguous then if fails both IS and !IS.
			if (bitCounts[i] == 1) // no ambiguity
			{
				if (posValues[i] & bits) answer = true;
			}
			else notflag = false;  // ambiguous value will be false always
			break;
		case POSITION: // FIRST LAST START END
			if ( bits == 1 && IsSentenceStart(i)) 
			{
				answer = true;
				if (!notflag && direction == -1) endwrap = true;	// allowed to test wrap around end
			}
			else if ( bits > 100 && (unsigned int)i == lastRealWord) 
			{
				answer = true;
				if (!notflag && direction == 1) endwrap = true;	// allowed to test wrap around end
			}
			break;
		case ORIGINALVALUE:
			if  (allOriginalWordBits[i] & bits) answer = true;
			break;
		case ENDSWITH:
			{
				WORDP D = Meaning2Word((int)bits);
				int wordlen = strlen(wordStarts[i]) - strlen(D->word);
				answer = !stricmp(wordStarts[i] + wordlen,D->word);
			}
			break;
		case HASPROPERTY: // system properties of lower case (never care about upper case properties)
			answer = (lcSysFlags[i] & bits) != 0; 
			break;
		case HASALLPROPERTIES: // system properties of lower case (never care about upper case properties)
			answer = (lcSysFlags[i] & bits) == bits; 
			break;
		case HASCANONICALPROPERTY:
			answer = (canSysFlags[i] & bits) != 0;
			break;
		case ISABSTRACT:
			if (allOriginalWordBits[i] & NOUN_ABSTRACT) answer = true;
			break;
		case ISORIGINAL: // original word is this
		{
			WORDP X =  Meaning2Word((int)bits);
			answer = !stricmp(wordStarts[i],X->word);
			if (tracex) Log(STDUSERLOG," vs %s ",X->word);
		}
			break;
		case ISQWORD:
			if (originalLower[i] && originalLower[i]->properties & QWORD) answer = true;
			break;
		case ISCANONICAL: // canonical form of word is this (we never have canonical on upper that we care about)
		{
			WORDP X =  Meaning2Word((int)bits);
			answer =  canonicalLower[i] == X;
			if (tracex) 
				Log(STDUSERLOG," vs %s ",X->word);
		}
			break;
		case HAS2VERBS: // subord conjunct will require it
			{
				int n = 0;
				for (unsigned int i = firstRealWord; i <= lastRealWord; ++i)
				{
					if (posValues[i] & VERB_TENSES) ++n;
				}
				if (n > 1) answer = true;
			}
			break;
		case PRIORCANONICAL:
			{
				WORDP D = Meaning2Word((int)bits);
				unsigned int x = i;
				while (--x > 0)
				{
					if (canonicalLower[x] == D) 
					{
						answer = true;
						break;
					}
				}
			}
			break;
		case CANONLYBE: // has no bits we didn't mention
			if ((posValues[i] & (-1 ^ bits)) == 0) answer = true;
			break;
		case ISMEMBER: // direct member...
			{
				WORDP D = Meaning2Word((int)bits);
				FACT* F = (D) ? GetObjectHead(D) : NULL;
				MEANING M = MakeMeaning(canonicalLower[i]);
				while (F)
				{
					if (F->subject == M)
					{
						answer = true;
						break;
					}
					F = GetObjectNext(F);
				}
			}
			break;
		case INCLUDE:
			answer = posValues[i] & bits && posValues[i] & (-1LL ^ bits); // there are bits OTHER than what we are testing for
			break;
		case POSSIBLEMASSNOUN:
			if (allOriginalWordBits[i] & NOUN_MASS) answer = true;
			break;
		case NONEAFTER: // find any of these later? dont negate.. only meaningful if NOTHING can be found
			{
				answer = true;
				unsigned int x = i;
				while (++x <= lastRealWord)
				{
					if (posValues[x] & bits) 
					{
						answer = false;
						break;
					}
				}
			}
			break;
			case NONEBEFORE: // find any of these earier? dont negate.. only meaningful if NOTHING can be found
			{
				answer = true;
				unsigned int x = i;
				while (--x > 0)
				{
					if (posValues[x] & bits) 
					{
						answer = false;
						break;
					}
				}
			}
			break;
		case ISQUESTION: // COULD this be a question (does not insure that it is, only that it could be)
			{
				answer = false;
				if (bits & AUXQUESTION && posValues[firstRealWord] & AUX_VERB_BITS) answer = true;	// MAY BE A QUESTION started by aux "did you go"
				if (bits & QWORDQUESTION && allOriginalWordBits[firstRealWord] & QWORD) answer = true;	// may be a question started with a question word "how are you"
				else if (bits & QWORDQUESTION && allOriginalWordBits[firstRealWord+1] & QWORD && allOriginalWordBits[firstRealWord] & PREPOSITION) answer = true;	// may be a question started with a question word "for whom  are you"
			}
			break;
		default: // unknown control
			return 2;
	}
	if (notflag) answer = !answer;
	return answer;
}

static  char* BitLabels(uint64 properties)
{
	static char buffer[MAX_WORD_SIZE];
	*buffer = 0;
	char* ptr = buffer;
	uint64 bit = START_BIT;		//   starting bit
	while (properties)
	{
		if (properties & bit)
		{
			properties ^= bit;
			char* label = FindNameByValue(bit);
			sprintf(ptr,"%s+",label);
			ptr += strlen(ptr);
		}
		bit >>= 1;
	}
	if (ptr > buffer) *--ptr = 0;
	return buffer;
}

bool ApplyRules(bool guessAcceptable) // get the set of all possible tags.
{
	if (!tags) return true;
	

	unsigned int ambiguous = lastRealWord;
	bool keep;
	bool tracex;
	int offset;
	int start;
	int direction;
	uint64* data;
	int guessAllowed = 0;
	unsigned int pass = 0;
	unsigned int limit = 50;
	bool resolved = false;
	bool parsed = false;
	bool changed = true;
	while (changed && ambiguous) // while we have something different and something not resolved
	{
		parsed = false;
		ambiguous = lastRealWord;
		++pass;
		changed = false;
		if (--limit == 0) 
		{
			ReportBug("ApplyRules overran\r\n")
			return false;
		}

		if (trace & POS_TRACE) 
			Log(STDUSERLOG,"\r\n------------- POS rules pass %d: \r\n",pass);

		// test all items in the sentence
		for (unsigned int wordIndex =  firstRealWord; wordIndex <= lastRealWord; ++wordIndex)
		{
			unsigned int j = (reverseWords) ? (lastRealWord + 1 - wordIndex) : wordIndex; // test from rear of sentence forwards to prove rules are independent
			char* word = wordStarts[j];

			if (bitCounts[j] == 1) // known completely
			{
				--ambiguous;
				continue;			
			}

			//  do rules to reduce flags
			for (unsigned int i = 0; i < tagRuleCount; ++i)
			{
				data = tags + (i * MAX_TAG_FIELDS);
				uint64 basic = *data;

				// reasons this rule doesnt apply
				if (basic & PART2_BIT) continue; // skip 2nd part

				unsigned int resultOffset = (basic >> RESULT_SHIFT) & 0x0003; 
				uint64 resultBits = data[resultOffset] & PATTERN_BITS;	// we will want to change THIS FIELD
				if (!(posValues[j] & resultBits) || !(posValues[j] & (-1LL ^ resultBits)) ) continue;		// cannot help  this- no bits in common or all bits would be kept or erased
				if (basic & GUESS_BIT) // will we consider guesses
				{
					// does rule require guessing and is it allowed --  GUESS=001  GUESS1=011 GUESS2=101  GUESS3=111
					if (!guessAllowed  || !guessAcceptable)  continue; // ignore all guesses 
					if (guessAllowed == 1) // GUESS1 
					{
						if (data[3] & GUESS_BIT) continue;	// guess2 and guess3 rejected
					}
					else if (guessAllowed == 2) // guess2 or guess1 allowed
					{
						if (data[3] & GUESS_BIT && data[2] & GUESS_BIT) continue; // reject guess3
					}
				}

				char* comment = (comments) ? comments[i] : NULL;
				tracex = (data[3] & TRACE_BIT) && (trace & POS_TRACE); 
				if (tracex) 
					Log(STDUSERLOG,"   => Trace rule:%d   %s (%d) %s \r\n",i,word,j,comment);

				unsigned int limit = (data[3] & PART1_BIT) ? (MAX_TAG_FIELDS * 2) : MAX_TAG_FIELDS;
				offset = ((data[1] >> OFFSET_SHIFT) & 0x00000007) - 3; // where to start pattern match relative to current field (3 bits)
				keep =  (data[1] & KEEP_BIT) != 0;
				direction =  (data[2] & REVERSE_BIT) ? -1 : 1;
				offset *= direction;	
				start = j + offset - direction;
				unsigned int k;
				int result;
				for (k = 0; k < limit; ++k) // test rule fields
				{
					uint64 field = data[k];
					unsigned int control = (unsigned int) ( (field >> CONTROL_SHIFT) &0x01ff);
					if (!(control & STAY)) start += direction;
					if (k == resultOffset) result = 1; // the result include is known to match if we are here
					else if (control) // if we care
					{
						if (control & SKIP) // check for any number of matches until fails.
						{
							result = TestTag(start,control,field & PATTERN_BITS,direction,tracex); // might change start to end or start of PREPOSITIONAL_PHRASE if skipping
							if (tracex) Log(STDUSERLOG,"    =%d  SKIP %s(%d) op:%s result:%d\r\n",k,wordStarts[start],start, OpDecode(field),result);
							if ( result == 2)
							{
								ReportBug("unknown control %d) %s Rule #%d %s\r\n",j,word,i,comment)
								result = false;
							}
							while (result) 
							{
								start += direction;
								result = TestTag(start,control,field & PATTERN_BITS,direction,tracex);
								if (tracex ) Log(STDUSERLOG,"    =%d  SKIP %s(%d)  result:%d @%s(%d)\r\n",k,wordStarts[start],start,result,wordStarts[start],start);
							}
							start -= direction; // ends on non-skip, back up so will see again
						}
						else if ((control >> CTRL_SHIFT) == RESETLOCATION)  // reset to include offset
						{
							if (field & PATTERN_BITS) direction = - direction; // flip scan direction
							start = j; // be back on the word as field
							if (tracex) Log(STDUSERLOG,"    =%d  RESETLOCATION %d\r\n",k,direction);
						}
						else
						{
							result = TestTag(start,control,field & PATTERN_BITS,direction,tracex); // might change start to end or start of PREPOSITIONAL_PHRASE if skipping
							if (tracex) Log(STDUSERLOG,"    =%d  %s(%d) op:%s result:%d\r\n",k,wordStarts[start],start,OpDecode(field),result);
							if (result == 2)
							{
								ReportBug("unknown control1 %d) %s Rule #%d %s\r\n",j,word,i,comment)
								result = false;
							}
							if (!result) break;	// fails to match // if matches, move along. If not, just skip over this (used to align commas for example)
						}
					}
				} // end test on fields

				// pattern matched, apply change if we can
				if (k >= limit) 
				{
					if (tracex) Log(STDUSERLOG,"   <= matched\r\n");
					uint64 old = posValues[j] & ((keep) ? resultBits : ( -1LL ^ resultBits));
					if (trace & POS_TRACE && (prepareMode == POS_MODE || prepareMode == POSVERIFY_MODE || prepareMode == PENN_MODE))
					{
						char* which = (keep) ? (char*) "KEEP" : (char*)"DISCARD";
						uint64 properties = old; // old is what is kept
						if (!keep) properties = posValues[j] - old;	// what is discarded
						if (!properties) ReportBug("bad result in pos tag %s\r\n",comment) // SHOULDNT HAPPEN

						char* name = FindNameByValue(resultBits);
						if (!name) name = BitLabels(properties);
						Log(STDUSERLOG," %d) %s: %s %s Rule #%d %s\r\n",j,word,which,name,i,comment+1);
						char buff[MAX_WORD_SIZE];
						*buff = 0;
						PosBits(old,buff);
						Log(STDUSERLOG,"   now %s\r\n",buff);
					}

					// make the change
					posValues[j] = old;
					bitCounts[j] = BitCount(old);
					changed = true;
					if (bitCounts[j] == 1) 
					{
						--ambiguous;
						break; // ending loop on rules
					}
				} // end result change k > limit
				else if (tracex) Log(STDUSERLOG,"   <= unmatched\r\n"); // pattern failed to match
			} // end loop on rules 
		} // end loop on words
		
		if (ambiguous && trace & POS_TRACE && changed) Log(STDUSERLOG,"\r\n%s",DumpAnalysis(posValues,"POS",false,true));
		
		if (!changed && ambiguous && guessAcceptable) // no more rules changed anything and we still need help
		{
			if (trace & POS_TRACE) Log(STDUSERLOG," *** enable GUESS%d\r\n",guessAllowed);
			if (++guessAllowed < 3) changed = true; 
			
#ifndef DISCARDPARSER
			bool modified = false;
			English_ParseSentence(resolved,modified);
			if (modified) --guessAllowed; // dont advance more radical notions yet
			parsed = true;
#endif
		}
	}
	if (trace & POS_TRACE) 
	{
		Log(STDUSERLOG,"%s\r\n",DumpAnalysis(posValues,"Roled SemiTagged POS",false,true));
		DumpSentence(0);
	}
#ifndef DISCARDPARSER
	bool modified = false;
	if (!parsed) English_ParseSentence(resolved,modified);
#endif
	return resolved;
}

static void Showit(char* buffer, const char* what,uint64 bits)
{
	if (bits) strcat(buffer,"+");
	strcat(buffer,what);
}

static void ParseFlags(char* buffer, unsigned int i)
{
	if (parseFlags[i] & FACTITIVE_ADJECTIVE_VERB) strcat(buffer,"factitive_adjective_verb ");
	if (parseFlags[i] & FACTITIVE_NOUN_VERB) strcat(buffer,"factitive_noun_verb ");
	if (parseFlags[i] & CAUSAL_TOINFINITIVE_VERB) strcat(buffer,"causal_toinfinitive_verb ");
	if (parseFlags[i] & QUOTEABLE_VERB) strcat(buffer,"quotable_verb ");
	if (parseFlags[i] & ADJECTIVE_TAKING_NOUN_INFINITIVE) strcat(buffer,"adjective_complement_verb ");
}

static void Tags(char* buffer, unsigned int i)
{
	WORDP D = originalLower[i];
	uint64 tie = (D) ? (D->systemFlags & POSTAG_PRIORITY) : 0; // tie break values
#ifndef DISCARDPARSER
	if (bitCounts[i] <= 1) tie = 0;
#endif
	uint64 type = posValues[i];
	if (type & PUNCTUATION) strcat(buffer,"Punctuation ");
	if (type & PAREN) strcat(buffer,"Parenthesis ");
	if (type & COMMA) strcat(buffer,"Comma ");
	if (type & QUOTE) strcat(buffer,"Quote ");
	if (type & POSSESSIVE) strcat(buffer,"Possessive ");
	if (type & (NOUN | NOUN_BITS|NOUN_INFINITIVE)) 
	{
		if (type & NOUN_PLURAL) Showit(buffer,"Noun_plural ",tie&NOUN);
		if (type & NOUN_GERUND) Showit(buffer,"Noun_gerund ",tie&VERB);
		if (type & NOUN_INFINITIVE) Showit(buffer,"Noun_infinitive ",tie&VERB);
		if (type & NOUN_SINGULAR) Showit(buffer,"Noun_singular ",tie&NOUN);
		if (type & NOUN_PROPER_SINGULAR) Showit(buffer,"Noun_proper_singular ",tie&NOUN);
		if (type & NOUN_PROPER_PLURAL) Showit(buffer,"Noun_proper_plural ",tie&NOUN);
		if (type & NOUN_CARDINAL) Showit(buffer,"Noun_cardinal ",tie&NOUN);
		if (type & NOUN_ORDINAL) Showit(buffer,"Noun_ordinal ",tie&NOUN);
		if (!(type & (NOUN_BITS|NOUN_INFINITIVE))) Showit(buffer,"Noun_unknown ",tie&NOUN);
	}
	if (type & (AUX_VERB | AUX_VERB_BITS)) 
	{
		if (type & AUX_VERB_FUTURE) strcat(buffer,"Aux_verb_future ");
		if (type & AUX_VERB_PAST) strcat(buffer,"Aux_verb_past ");
		if (type & AUX_VERB_PRESENT) strcat(buffer,"Aux_verb_present ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_INFINITIVE) strcat(buffer,"Aux_be_infinitive ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_PAST_PARTICIPLE) strcat(buffer,"Aux_be_pastparticiple ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_PRESENT_PARTICIPLE) strcat(buffer,"Aux_be_presentparticiple ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_PRESENT) strcat(buffer,"Aux_be_present ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_PRESENT_3PS) strcat(buffer,"Aux_be_present_3ps ");
		if (type & AUX_BE && originalLower[i]->properties & VERB_PAST) strcat(buffer,"Aux_be_past ");
		if (type & AUX_HAVE && originalLower[i]->properties & VERB_PRESENT_3PS) strcat(buffer,"Aux_have_present_3ps ");
		if (type & AUX_HAVE && originalLower[i]->properties & VERB_PRESENT && !firstAux && i > 1 && stricmp(wordStarts[i-1],"to")) strcat(buffer,"Aux_have_present "); //  have in first position will be conjugated
		else if (type & AUX_HAVE && originalLower[i]->properties & VERB_INFINITIVE) strcat(buffer,"Aux_have_infinitive "); 
		if (type & AUX_HAVE && originalLower[i]->properties & VERB_PAST) strcat(buffer,"Aux_have_past ");
		if (type & AUX_DO && originalLower[i]->properties & VERB_PAST) strcat(buffer,"Aux_do_past ");
		if (type & AUX_DO && originalLower[i]->properties & VERB_PRESENT) strcat(buffer,"Aux_do_present ");
		if (type & AUX_DO && originalLower[i]->properties & VERB_PRESENT_3PS) strcat(buffer,"Aux_do_present_3ps ");
		if (!(type & AUX_VERB_BITS)) strcat(buffer,"Aux_verb_unknown ");
		firstAux = originalLower[i];
	}
	if (type & (VERB|VERB_TENSES)) 
	{
		if (type & VERB_INFINITIVE) Showit(buffer,"Verb_infinitive ",tie&VERB);
		if (type & VERB_PRESENT_PARTICIPLE) Showit(buffer,"Verb_present_participle ",tie&VERB);
		if (type & VERB_PAST) Showit(buffer,"Verb_past ",tie&VERB);
		if (type & VERB_PAST_PARTICIPLE) Showit(buffer,"Verb_past_participle ",tie&VERB);
		if (type & VERB_PRESENT) Showit(buffer,"Verb_present ",tie&VERB);
		if (type & VERB_PRESENT_3PS) Showit(buffer,"Verb_present_3ps ",tie&VERB);
		if (!(type & VERB_TENSES)) Showit(buffer,"Verb_unknown ",tie&VERB);
	}
	if (type & PARTICLE) strcat(buffer,"Particle ");
	if (type & (ADJECTIVE|ADJECTIVE_BITS))
	{
		if (type & ADJECTIVE_NOUN) Showit(buffer,"Adjective_noun ",tie&NOUN); // can be dual kind of adjective
		if (type & ADJECTIVE_PARTICIPLE) Showit(buffer,"Adjective_participle ",tie&VERB); // can be dual kind of adjective
		if (type & ADJECTIVE_CARDINAL) Showit(buffer,"Adjective_cardinal ",tie&ADJECTIVE);
		if (type & ADJECTIVE_ORDINAL) Showit(buffer,"Adjective_ordinal ",tie&ADJECTIVE);
		if (type & ADJECTIVE_NORMAL) 
		{
			if (lcSysFlags[i] & MORE_FORM) Showit(buffer,"Adjective_more ",tie&ADJECTIVE);
			else if (lcSysFlags[i] & MOST_FORM) Showit(buffer,"Adjective_most ",tie&ADJECTIVE);
			else if (lcSysFlags[i] & BASIC_FORM) Showit(buffer,"Adjective_basic ",tie&ADJECTIVE);
			else Showit(buffer,"Adjective_basic ",tie&ADJECTIVE); // synthesized word lacking system property like "boam-headed"
		}
		if (!(type & ADJECTIVE_BITS))  Showit(buffer,"Adjective_unknown ",tie&ADJECTIVE);
	}
	if (type & (ADVERB|ADVERB_BITS))
	{
		if (lcSysFlags[i] & MORE_FORM) Showit(buffer,"Adverb_more ",tie&ADVERB);
		else if (lcSysFlags[i] & MOST_FORM) Showit(buffer,"Adverb_most ",tie&ADVERB);
		else if (lcSysFlags[i] & BASIC_FORM) Showit(buffer,"Adverb_basic ",tie&ADVERB);
		if (!(type & ADVERB_BITS)) Showit(buffer,"Adverb_unknown ",tie&ADVERB);
	}
	if (type & PREPOSITION) Showit(buffer,"Preposition ",tie&PREPOSITION);
	if (type & TO_INFINITIVE) strcat(buffer,"To_infinitive ");
	if (type & (PRONOUN_BITS))
	{
		if (type & PRONOUN_POSSESSIVE) strcat(buffer,"Pronoun_possessive ");
		if (type & PRONOUN_OBJECT) strcat(buffer,"Pronoun_object ");
		if (type & PRONOUN_SUBJECT) strcat(buffer,"Pronoun_subject ");
	}
	if (type & THERE_EXISTENTIAL)  strcat(buffer,"There_existential ");
	if (type & CONJUNCTION_BITS)
	{
		if (type & CONJUNCTION_COORDINATE) Showit(buffer,"Conjunction_coordinate ",tie&(CONJUNCTION_COORDINATE|CONJUNCTION_SUBORDINATE));
		if (type & CONJUNCTION_SUBORDINATE) Showit(buffer,"Conjunction_subordinate ",tie&(CONJUNCTION_COORDINATE|CONJUNCTION_SUBORDINATE));
	}
	if (type & PREDETERMINER) strcat(buffer,"Predeterminer ");
	if (type & DETERMINER) strcat(buffer,"Determiner ");
	if (type & FOREIGN_WORD)strcat(buffer,"Foreign ");
}

static void ZoneDump(unsigned int zone, char* buffer)
{
	if (zoneData[zone] & ZONE_SUBJECT) strcat(buffer,"ZoneSubject ");
	if (zoneData[zone] & ZONE_VERB) strcat(buffer,"ZoneVerb ");
	if (zoneData[zone] & ZONE_OBJECT) strcat(buffer,"ZoneObject ");
	if (zoneData[zone] & ZONE_CONJUNCT) strcat(buffer,"ZoneConjunct ");
	if (zoneData[zone] & ZONE_FULLVERB) strcat(buffer,"ZoneFullverb ");
	if (zoneData[zone] & ZONE_AUX) strcat(buffer,"ZoneAux ");
	if (zoneData[zone] & ZONE_PCV) strcat(buffer,"ZonePVC ");
	if (zoneData[zone] & ZONE_ADDRESS) strcat(buffer,"ZoneAddress ");
	if (zoneData[zone] & ZONE_ABSOLUTE) strcat(buffer,"ZoneAbsolute ");
	if (zoneData[zone] & ZONE_AMBIGUOUS) strcat(buffer,"ZoneAmbiguous ");
}

char* DumpAnalysis(uint64 flags[MAX_SENTENCE_LENGTH],const char* label,bool original,bool roleDisplay)
{
	static char buffer[BIG_WORD_SIZE];
	*buffer = 0;
	char* ambiguous = "";
	char* faultyparse = "";
	if (!original && tokenFlags & FAULTY_PARSE) faultyparse = "badparse "; // only one of ambiguous (worse) and faultyparse will be true
	sprintf(buffer,"%s%s%s %d words: ",ambiguous,faultyparse,label,wordCount);
	unsigned int lenpre;
	firstAux = NULL;

	for (unsigned int i = 1; i <= wordCount; ++i)
    {
		WORDP D = originalLower[i];
		if (flags[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && originalUpper[i]) D = originalUpper[i];

		if (D) strcat(buffer,D->word); // we know it as lower or upper depending
		else  strcat(buffer,wordStarts[i]); // what he gave which we didnt know as lower or upper
		// canonical
		if (!original) 
		{
			if (canonicalLower[i] && originalLower[i] && !strcmp(canonicalLower[i]->word,originalLower[i]->word)); // same
			else if (canonicalUpper[i] && originalUpper[i] && !strcmp(canonicalUpper[i]->word,originalUpper[i]->word)); // same
			else 
			{
				if (D && !(D->properties & PART_OF_SPEECH) && D->properties & FOREIGN_WORD && canonicalLower[i] && !stricmp(canonicalLower[i]->word,"unknown-word"))  strcat(buffer,"/foreign");
				else if (canonicalLower[i]) 
				{
					strcat(buffer,"/");
					strcat(buffer,canonicalLower[i]->word);
				}
				else if (canonicalUpper[i]) 
				{
					strcat(buffer,"/");
					strcat(buffer,canonicalUpper[i]->word);
				}
				else if (D && D->properties & FOREIGN_WORD)  strcat(buffer,"/foreign");
			}
		}
		strcat(buffer," (");
		lenpre = strlen(buffer);
#ifndef DISCARDPARSER
		if (roleDisplay)
		{
			if (!original && clauses[i] && clauses[i-1] != clauses[i]) strcat(buffer,"<Clause ");
			if (!original && verbals[i] && verbals[i-1] != verbals[i]) strcat(buffer,"<Verbal ");
			if (!original && phrases[i] && phrases[i-1] != phrases[i]) 
			{
				if (posValues[i] & (PREPOSITION|NOUN_BITS|PRONOUN_BITS) || roles[i] & OMITTED_TIME_PREP || i != firstRealWord || phrases[firstRealWord] != phrases[lastRealWord])  
				{
					if (posValues[i] & (NOUN_BITS|PRONOUN_BITS) && zoneData[zoneMember[i]] & ZONE_ABSOLUTE) // is it first in zone?
					{
						strcat(buffer,"<AbsolutePhrase ");
					}
					else strcat(buffer,"<Phrase ");  // wont be true on sentence start if wrapped from end  BUT ABSOLUTE will be true
				}
			}
			if (!original && roles[i]) strcat(buffer,GetRole(i));
		}
#endif

		Tags(buffer,i);

#ifndef DISCARDPARSER
		if (roleDisplay)
		{
			if (!original && phrases[i] && (phrases[i+1] != phrases[i] || i == lastRealWord) ) // a phrase where next is different 
			{
				if (i != lastRealWord || phrases[firstRealWord] != phrases[lastRealWord]) strcat(buffer,"Phrase> "); // if wrapped to start from end of sentence, this wont end here, ends at start
			}
			if (!original && verbals[i] && verbals[i+1] != verbals[i]) strcat(buffer,"Verbal> ");
			if (!original && clauses[i] && clauses[i+1] != clauses[i]) strcat(buffer,"Clause> ");
		}
#endif
		size_t len = strlen(buffer);
		if (len == lenpre) strcpy(buffer+len,")  ");
		else strcpy(buffer+len-1,")  ");
	}

	strcat(buffer,"\r\n");
	return buffer;
}

void MarkTags(unsigned int i)
{
	size_t len = strlen(wordStarts[i]);
	uint64 bits = finalPosValues[i];
	if (bits & NOUN) bits |= allOriginalWordBits[i] & (NOUN_ROLE | NOUN_NODETERMINER | NOUN_MASS | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_SHE | NOUN_HE | NOUN_THEY | NOUN_TITLE_OF_ADDRESS | NOUN_TITLE_OF_WORK| NOUN_ABSTRACT );
	if (allOriginalWordBits[i] & LOWERCASE_TITLE ) bits |= LOWERCASE_TITLE;

	// mark pos data and supplemental checking on original proper names, because canonical may mess up, like james -->  jam
	uint64 bit = START_BIT;
	for (int j = 63; j >= 0; --j)
	{
		if (bits & bit) 
		{
			if (bit & PRONOUN_BITS)  MarkFacts(MakeMeaning(Dpronoun),i,i); // general as opposed to specific
			if (bit & NOUN && finalPosValues[i] & NOUN_SINGULAR) MarkFacts(posMeanings[j],i,i); // NOUNS which have singular like "well" but could be infinitive, are listed as nouns but not infintive
			else if (i > 1 && finalPosValues[i] & NOUN_INFINITIVE && stricmp(wordStarts[i-1],"to") && (bit & (NOUN | NOUN_INFINITIVE))) {;} // skip noun-inifitive lacking TO 
			else MarkFacts(posMeanings[j],i,i);
			if (bits & NOUN_HUMAN) // impute additional meanings
			{
				if (bits & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) MarkFacts(MakeMeaning(Dhumanname),i,i);
				if (bits & NOUN_HE)  MarkFacts(MakeMeaning(Dmalename),i,i);
				if (bits & NOUN_SHE) MarkFacts(MakeMeaning(Dfemalename),i,i);
			}
			if (bit & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && len >= 3) 
			{
				MarkFacts(MakeMeaning(Dpropername),i,i); // short things are not names, even if caps
			}
		}

		bit >>= 1;
	}
		
	// system flags we allow
	bit = START_BIT;
	for (int j = 63; j >= 0; --j)
	{
		if (bit & MARK_FLAGS && originalLower[i] && originalLower[i]->systemFlags & bit)  MarkFacts(sysMeanings[j],i,i);
		bit >>= 1;
	}

	if (bits & (AUX_VERB_BITS | AUX_BE | AUX_HAVE | AUX_DO)) finalPosValues[i] |= VERB;	// lest it report "can" as untyped, and thus become a noun -- but not ~verb from system view

}

/////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////// PARSER CODE

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DISCARDPARSER
#ifdef INFORMATION

The algorithm is:

1. figure out what POS tags we can be certain of
2. Assign roles as we go, forcing bindings that seem to make sense at the time. This is subject to garden-path phenomena, which means when we detect inconsistency later,
we have to go back and patch up the roles and even the POS tags to make a coherent whole. Probability is built into the expectation of role coercions we do as we go.

E.g., when we have two nouns and dont need the second, we have to decide is it appositive or is it start of implied clause.

3. As parsing progresses, we enter and exit new nested roleLevels as phrases, verbals and clauses spring into being and become complete. A roleLevel indicates what we are currently seeking or allow
like a subject, verb, object, etc. So until we see a verb, for example, we are not expected to see an object. We may mislabel an object as a subject (because we were seeking that) but have to
patch later when the structure becomes more apparent.

Structures:

1. Object complement:    It is most often used with verbs of creating or nominating such as make, name, elect, paint, call, choosing, judging, appointing, etc. in the active voice,.. implied missing "to be"
	Subject Verb Object nounphrase.		He considered her his grandmother.
	Subject Verb Object adjective.		He drove her crazy.
	Subject Verb Object prep-phrase.	He loved her like a sister.???
	Subject verb object gerund	?		I saw the Prime Minister sleeping
	Subject verb object clause			I like the shawl *people wear

2. Appostive noun (at end will have a comma, lest it be object ocmplement)
	He considered Joe, a self-starter.	(comma helps)
	The commander stood his ground, a heroic figure. (delayed appositive of commander)
	It is a pleasure *to meet up (delayed appositive of it but fine as appossitive of pleasure)
	I, the leader of the rebellion that defines eternity, call this to order.

3. Clause-omitting starter
	The shawl *people wear is green (subject complement)
	Roger likes the shawl *people wear (object complement)

4. Prep phrase omitting prep
	Time phrases					He will walk *next week

5.	Adjective Object
	Subject verb adjective		The shawl is green


The first noun encountered is generally the subject of the sentence except:

0. Inside a prepositional phrase, verbal, or clause.
1. Appositive noun at sentence start with comma:  "A dark wedge, the eagle hurtled  at nearly 200 miles per hour."
	detectable because it starts the sentence and has no verb before the comma, and has a noun and verb after the comma.
2. Questions often flip subject/object:  "what is your name"
3. Absolute phrase at start. separated by comma, 
	a) lacking a full verb (a participle w/o aux)
		Their reputation as winners secured by victory, the New York Liberty charged into the semifinals
	b) lacking any verb (sometimes implied) --
		Your best friends, where are they now, when you need them?  # NOUN PHRASE (NO VERB)  # http://grammar.ccc.commnet.edu/grammar/phrases.htm#absolute
		The season over, they were mobbed by fans in Times Square  # IMPLIED BEING AS VERB
		Stars all their adult lives, they seemed used to the attention # IMPLIED [Having been AS VERB] 
4. Direct address

PARENS include:
1.  pretend they are commas for appositive: Steve Case (AOL s former CEO)
2. junk extra "You will need a flashlight for the camping trip (don't forget the batteries!). "

COMMAS denote:
1. appositives embedded " Bill, our CEO, ate here"
2. a list of nouns	"he ate green bananas, lush oranges, and pineapple"
3. multiple adjectives " he ate ripe, green fruit."
4. after intro prep phrase  "After the show, John and I went out to dinner."
5. before coord conjunction: "Ryan went to the beach yesterday, but he forgot his sunscreen."
6. Direct address by name: "Amber, could you come here for a moment?"
7. leading adjective_participle: "Running toward third base, he suddenly realized how stupid he looked."

ROLES:

Where a noun infinitive is the complement, we note the role on the verb, not on the To.  If the noun infintiive is not a complement,
then its role will be VERB2 (generic).

PHRASES and CLAUSES

The system recognizes:
1. prepositional phrase + ommited prep time phrase
2. verbals (noun-infinitive, adjective participle, noun-gerund)
3. subordinate clause
	a. started by subordinate conjunction
	b. started by relative pronoun serving as subject
	c. ommitted
4. absolute phrase
5. address phrase
6. appostive phrase

Back to back nouns can occur as follows:

1. A clause/phrase/verbal is ending and noun is subject of next chunk - best with commas
		"Smelling food cats often eat"  though better with a comma
2. Appositive - can use commas
	    "My friend Tom lives here"
		"Tom my friend lives here" though better with comma
		one or both must be determined nouns, where one the other must be a proper name. Determiners a and the must be intermixed, not duplicated.
3.  Adjective nouns -- do not use commas
		"I saw the bank clerk"
4. with commas, we can have address  "Bill, dogs eat meat" -- requires commas

Verb structures:

1.  Intransitive - takes no object
2.  Transitive - takes a direct object
3.  DiTransitive - takes direct and indirect object
4.  Factitive - takes two objects (second is like prep phrase "as") "elect him president" or noun adjective
5.  Causative - causes an action - generally followed by a noun or pronoun, then by an infinitive.  The infinitive in "Peter helped Polly correct exams." is to correct.  The to is understood, and not necessary to include it  


Is there a distinction between: OBJECT_COMPLEMENT  in "they found the way *to go"
and POSTNOMINAL_ADJECTIVE in "They described the way *to survive"
and direct appositive -- "The mistake, *to elect him", was critical - this requires comma separation

Postnominal can occur after any normal noun (including subjects).  It is either a predictable adjective (there are some specific ones that usually are post noun, or
a verbal.

Some Verbs "expect" an object complement after object, but that complement can be 
a POSTNOMINAL_ADJECTIVE or a noun. As a adjective, the adjective is an OBJECT_COMPLEMENT that describes how the object ended up AFTER the
verb happened (causal verbs).

How to tell a interrupter infinitive, sandwiched by commas from an appositive infinitive- cant. we will let it be appositive


#endif

#define MAX_CLAUSES 25
unsigned int parseFlags[MAX_SENTENCE_LENGTH];

// dynamic cumulative data across assignroles calls
static unsigned char clauseBit;
static unsigned char prepBit;
static unsigned char verbalBit;
unsigned int phrases[MAX_SENTENCE_LENGTH];
unsigned int clauses[MAX_SENTENCE_LENGTH];
unsigned int verbals[MAX_SENTENCE_LENGTH];
unsigned char quotedWords[MAX_SENTENCE_LENGTH];
unsigned char coordinates[MAX_SENTENCE_LENGTH];
unsigned char crossReference[MAX_SENTENCE_LENGTH];
unsigned int roles[MAX_SENTENCE_LENGTH];
unsigned char tried[MAX_SENTENCE_LENGTH];
// also posValues

// this data is transient per assignroles
static unsigned int needRoles[MAX_CLAUSES]; // what we seek in direct object land or verb land at currnet level of main/phrase/clause
static unsigned int roleIndex;
static unsigned char auxVerbStack[MAX_CLAUSES];	// most recent aux for this level
static unsigned char subjectStack[MAX_CLAUSES];  // the subject found for this level of sentence piece (tied to roleIndex)
static unsigned char verbStack[MAX_CLAUSES];  // the verb found for this level of sentence piece (tied to roleIndex)
static unsigned char indirectObjectStack[MAX_CLAUSES];  // the subject found for this level of sentence piece (tied to roleIndex)
static unsigned char objectStack[MAX_CLAUSES];  // the  object found for this level of sentence piece (tied to roleIndex)
static unsigned char designatedObjects[MAX_SENTENCE_LENGTH]; // for verb at position, index of word to treat as its direct object, if any
static unsigned int firstnoun;	 // first noun we see in a sentence (maybe object of wrapped prep from end)
static unsigned int determineVerbal;
static unsigned int firstNounClause;

static bool requireNoun;

#define INCLUSIVE 1
#define EXCLUSIVE 2

#define AMBIGUOUS_VERBAL (ADJECTIVE_PARTICIPLE | NOUN_GERUND )
#define AMBIGUOUS_PRONOUN (PRONOUN_SUBJECT | PRONOUN_OBJECT )

bool ApplyRules(unsigned int& ambiguous, bool guessAcceptable);

#define GUESS_NOCHANGE 0
#define GUESS_ABORT 1
#define GUESS_RETRY 2
#define GUESS_CONTINUE 3

void MarkRoles(unsigned int i)
{
	// mark its main parser role - not all roles are marked and parsing might not have been done
	switch(roles[i])
	{
		case MAINSUBJECT: MarkFacts(MakeMeaning(StoreWord("~mainsubject")),i,i); break;
		case MAINVERB: MarkFacts(MakeMeaning(StoreWord("~mainverb")),i,i); break;
		case (OBJECT2|SUBJECT2): MarkFacts(MakeMeaning(StoreWord("~object2")),i,i); MarkFacts(MakeMeaning(StoreWord("~subject2")),i,i);break;
	
		case (MAINOBJECT|SUBJECT2): MarkFacts(MakeMeaning(StoreWord("~mainobject")),i,i); MarkFacts(MakeMeaning(StoreWord("~subject2")),i,i);break;
		case (MAINOBJECT|VERB2): MarkFacts(MakeMeaning(StoreWord("~mainobject")),i,i); MarkFacts(MakeMeaning(StoreWord("~verb2")),i,i);break;
		case MAINOBJECT: MarkFacts(MakeMeaning(StoreWord("~mainobject")),i,i); break;
		case MAININDIRECTOBJECT: MarkFacts(MakeMeaning(StoreWord("~mainindirectobject")),i,i); break;
		case SUBJECT_COMPLEMENT: MarkFacts(MakeMeaning(StoreWord("~subjectcomplement")),i,i); break;
		case OBJECT_COMPLEMENT: MarkFacts(MakeMeaning(StoreWord("~objectcomplement")),i,i); break;
		
		case SUBJECT2: MarkFacts(MakeMeaning(StoreWord("~subject2")),i,i); break;
		case VERB2: MarkFacts(MakeMeaning(StoreWord("~verb2")),i,i); break;
		case OBJECT2: MarkFacts(MakeMeaning(StoreWord("~object2")),i,i); break;
		case INDIRECTOBJECT2: MarkFacts(MakeMeaning(StoreWord("~indirectobject2")),i,i); break;
			
		case ADDRESS: MarkFacts(MakeMeaning(StoreWord("~address")),i,i); break;
		case APPOSITIVE: MarkFacts(MakeMeaning(StoreWord("~appositive")),i,i); break;
		case POSTNOMINAL_ADJECTIVE: MarkFacts(MakeMeaning(StoreWord("~postnominaladjective")),i,i); break;
		case REFLEXIVE: MarkFacts(MakeMeaning(StoreWord("~reflexive")),i,i); break;
	}
	
	// meanwhile mark start/end of phrases
	unsigned int phrase = phrases[i];
	if (phrase && phrase != phrases[i-1])
	{
		if (i == firstRealWord && phrase == phrases[lastRealWord]) {;} // start at end instead
		else
		{
			if (posValues[i] & NOUN_BITS) MarkFacts(MabsolutePhrase,i,i);
			else if (posValues[i] & (ADVERB_BITS|ADJECTIVE_BITS)) MarkFacts(MtimePhrase,i,i);
			else MarkFacts(Mphrase,i,i);
		}
	}
	
	// meanwhile mark start/end of clause
	unsigned int clause = clauses[i];
	if (clause && clause != clauses[i-1])
	{
		unsigned int at = i;
		while (clauses[++at] == clause);
		MarkFacts(MakeMeaning(Dclause),i,at-1);
	}

	// meanwhile mark start/end of verbals
	unsigned int verbal = verbals[i];
	if (verbal && verbal != verbals[i-1])
	{
		unsigned int at = i;
		while (verbals[++at] == verbal);
		MarkFacts(MakeMeaning(Dverbal),i,at-1);
	}

}

void InitEnglishParser()
{
	roleIndex = 0; 
	prepBit = clauseBit = verbalBit = 1;
	memset(tried,0,sizeof(char) * (wordCount + 2));
	memset(phrases,0,sizeof(int) *(wordCount+4));
	memset(verbals,0,sizeof(int) *(wordCount+4));
	memset(clauses,0,sizeof(int) *(wordCount+4));
	memset(designatedObjects,0,wordCount+4);
	memset(roles,0,(wordCount+4) * sizeof(int));
	memset(parseFlags,0,(wordCount+2) * sizeof(int));
}

static void FreezePOS(uint64* base)
{
#ifdef INFO
unsigned int phrases[MAX_SENTENCE_LENGTH];
unsigned int clauses[MAX_SENTENCE_LENGTH];
unsigned int verbals[MAX_SENTENCE_LENGTH];
unsigned int roles[MAX_SENTENCE_LENGTH];
unsigned char coordinates[MAX_SENTENCE_LENGTH];
unsigned char tried[MAX_SENTENCE_LENGTH];
static unsigned char clauseBit;
static unsigned char prepBit;
static unsigned char verbalBit;
// also uint64 posValues + bitcounts
#endif

	unsigned char* ptr = (unsigned char*) base;	// get space to hold data copy
	unsigned int i;

	// 64bit data
	uint64* data = base;
	*data= 0;
	for (i = 1; i <= wordCount; ++i) *data++ = posValues[i];

	// 32bit data
	unsigned int* hold  = (unsigned int*) data;
	for (i = 0; i <= wordCount; ++i)  
	{
		*hold++ = phrases[i];
		*hold++ = clauses[i];
		*hold++ = verbals[i];
		*hold++ = roles[i];
	}

	// char data
	ptr = (unsigned char*) hold;
	*ptr++ = clauseBit;
	*ptr++ = prepBit;
	*ptr++ = verbalBit;
	for (i = 1; i <= wordCount; ++i) 
	{
		*ptr++ = coordinates[i];
		*ptr++ = bitCounts[i];
		*ptr++ = tried[i];
	}
}

static void UnfreezePOS(uint64* data)
{
	unsigned int i;

	// 64bit data
	for (i = 1; i <= wordCount; ++i) posValues[i] = *data++;

	// 32bit data
	unsigned int* hold  = (unsigned int*) data;
	roleIndex = *hold++;
	for (i = 0; i <= wordCount; ++i) 
	{
		phrases[i] = *hold++;
		clauses[i] = *hold++;
		verbals[i] = *hold++;
		roles[i] = *hold++;
	}

	// char data
	unsigned char* ptr = (unsigned char*) hold;
	clauseBit = *ptr++;
	prepBit = *ptr++;
	verbalBit = *ptr++;
	for (i = 1; i <= wordCount; ++i) 
	{
		coordinates[i] = *ptr++;
		bitCounts[i] = *ptr++;
		tried[i] = *ptr++;
	}
}

void AddRole(unsigned int i, unsigned int role)
{
	if (!roles[i])
	{
		SetRole(i,role,false);
		return;
	}
	roles[i] |= role;
	if (trace & POS_TRACE) Log(STDUSERLOG,"   +%s->%s\r\n",wordStarts[i],GetRole(i));

	// set new reference
	if (role & needRoles[roleIndex]) // store role xref when we were looking for it - we might see objects added when we have one already
	{
		if (role & MAINSUBJECT && roleIndex == 1) subjectStack[roleIndex] = (unsigned char)i;
		else if (role & SUBJECT2 && roleIndex > 1) subjectStack[roleIndex] = (unsigned char)i;
		else if (role & MAINVERB && roleIndex == 1) verbStack[roleIndex] = (unsigned char)i;
		else if (role & VERB2 && roleIndex > 1) verbStack[roleIndex] = (unsigned char)i;
		else if (role & MAININDIRECTOBJECT && roleIndex == 1) indirectObjectStack[roleIndex] = (unsigned char)i;
		else if (role & INDIRECTOBJECT2 && roleIndex > 1) indirectObjectStack[roleIndex] = (unsigned char)i;
		else if (role & MAINOBJECT && roleIndex == 1) objectStack[roleIndex] = (unsigned char)i;
		else if (role & OBJECT2 && roleIndex > 1) objectStack[roleIndex] = (unsigned char)i;
	}

	needRoles[roleIndex] &= -1 ^ role; // remove what was supplied
	if (role & (OBJECT2|MAINOBJECT)) needRoles[roleIndex] &= -1 ^ (SUBJECT_COMPLEMENT);	// cant be both - can still have inifitive pending- "he wanted his students to *read"
	if (role & SUBJECT_COMPLEMENT) needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|INDIRECTOBJECT2|OBJECT_COMPLEMENT);	// cant be these now
	if (role & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT;

}

void SetRole(unsigned int i, unsigned int role,bool revise)
{
	// remove old reference if it had one
	if (roles[i] & (MAINSUBJECT|SUBJECT2) && subjectStack[roleIndex] == i) subjectStack[roleIndex] = 0;
	if (roles[i] & (MAINVERB|VERB2) && verbStack[roleIndex] == i) verbStack[roleIndex] = 0;
	if (roles[i] & (MAINOBJECT|OBJECT2) && objectStack[roleIndex] == i) objectStack[roleIndex] = 0;
	if (roles[i] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && indirectObjectStack[roleIndex] == i) indirectObjectStack[roleIndex] = 0;
	
	roles[i] = role;
	if (trace & POS_TRACE) Log(STDUSERLOG,"   +%s->%s\r\n",wordStarts[i],GetRole(i));
	unsigned int loopBase = i;
	int limit = 20;
	if (role && role != CONJUNCT_PARTICLE && role != CONJUNCT_NOUN && role != CONJUNCT_VERB && role != CONJUNCT_ADJECTIVE && role != CONJUNCT_ADVERB && role != CONJUNCT_PHRASE)
	{
		int begin = i;
		while (coordinates[i])
		{
			i = coordinates[i];
			if (!(posValues[i] & (CONJUNCTION_COORDINATE | COMMA))) roles[i] = role; // share any discovered role
			if (i == loopBase) break;
			if (--limit == 0) // PROBLEM
			{
				ReportBug("SetRole on coord conjunct too far from %d %s\r\n",begin,wordStarts[begin]);	
				break;
			}
		}
	}
	// set new reference
	if (role & needRoles[roleIndex] || revise) // store role xref when we were looking for it - we might see objects added when we have one already
	{
		if (role & MAINSUBJECT && roleIndex == 1) subjectStack[roleIndex] = (unsigned char)i;
		else if (role & SUBJECT2 && roleIndex > 1) subjectStack[roleIndex] = (unsigned char)i;
		else if (role & MAINVERB && roleIndex == 1) verbStack[roleIndex] = (unsigned char)i;
		else if (role & VERB2 && roleIndex > 1) verbStack[roleIndex] = (unsigned char)i;
		else if (role & MAININDIRECTOBJECT && roleIndex == 1) indirectObjectStack[roleIndex] = (unsigned char)i;
		else if (role & INDIRECTOBJECT2 && roleIndex > 1) indirectObjectStack[roleIndex] = (unsigned char)i;
		else if (role & MAINOBJECT && roleIndex == 1) objectStack[roleIndex] = (unsigned char)i;
		else if (role & OBJECT2 && roleIndex > 1) objectStack[roleIndex] = (unsigned char)i;
	}
		
	needRoles[roleIndex] &= -1 ^ role; // remove what was supplied
	if (role & (OBJECT2|MAINOBJECT)) needRoles[roleIndex] &= -1 ^ (SUBJECT_COMPLEMENT|MAININDIRECTOBJECT|INDIRECTOBJECT2);	// cant be both - can still have inifitive pending- "he wanted his students to *read"
	if (role & SUBJECT_COMPLEMENT) needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|INDIRECTOBJECT2|OBJECT_COMPLEMENT);	// cant be these now
	if (role & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT;
	if (role & (MAINVERB|VERB2)) needRoles[roleIndex] &= -1 ^ (SUBJECT2|MAINSUBJECT);

	if (role & OBJECT2 && roles[i-1] == OBJECT_COMPLEMENT) SetRole(i-1,INDIRECTOBJECT2); // need to revise backward "I had the mechanic *check the car"
	if (role & MAINOBJECT && roles[i-1] == OBJECT_COMPLEMENT) SetRole(i-1,MAININDIRECTOBJECT);
	
	
	// link back to verb 
	if (!phrases[i] && role & (MAININDIRECTOBJECT|INDIRECTOBJECT2|MAINOBJECT|OBJECT2)) crossReference[i] = verbStack[roleIndex];
	if (phrases[i] && role & OBJECT2) crossReference[i] = (unsigned char) lastPhrase;
}

static bool ProcessSplitNoun(unsigned int verb1,bool &changed)
{
	// 1 when we have dual valid verbs, maybe we swallowed a noun into a prep phrase that shouldnt have been.  (sequence: noun noun (ad:v) verb)
	// 2 Or We may have an inner implied clause -- the shawl people often wear isn't yellow ==  I hate the shawl people wear
	// 3 or a clause in succession when object side- to sleep is the thing eli wanted
	// 4 or a clause on subject side -  what they found thrilled them

	// 1 For 1st verb, see if we have  noun noun {adv} verb  formats (which implies noun should have been separated,
	unsigned int before = verb1;
	if (posValues[verb1] & VERB_TENSES) // past tense needs to go to participle
	{
		while (--before > 0) 
		{
			if (posValues[before] & ADVERB_BITS) continue; // ignore this
			// find the subject just before our 2nd verb
			if (posValues[before] & (NOUN_BITS - NOUN_GERUND - NOUN_CARDINAL - NOUN_ORDINAL + PRONOUN_SUBJECT + PRONOUN_OBJECT) &&
				posValues[before-1] & (NOUN_BITS - NOUN_GERUND - NOUN_CARDINAL - NOUN_ORDINAL  + PRONOUN_SUBJECT + PRONOUN_OBJECT))
			{ // misses, the shawl the people wear
				if (trace) Log(STDUSERLOG,"split noun @ %s(%d) to %s(%d)\r\n",wordStarts[before],before,wordStarts[verb1],verb1);

				if (posValues[before] & (PRONOUN_SUBJECT|PRONOUN_OBJECT)) // pronoun will already be split, but might be a clause starter
				{
					if (canonicalLower[before-1] && canonicalLower[before-1]->systemFlags & POTENTIAL_CLAUSE_STARTER)
					{
						SetRole(before,SUBJECT2,true);
						phrases[before] = 0;
						SetRole(verb1, VERB2,true);
						if (designatedObjects[verb1]) SetRole(designatedObjects[verb1],OBJECT2,true);
						--before; // subsume starter for clause
					}
					else
					{
						SetRole(before,SUBJECT2,true);
						phrases[before] = 0;
						SetRole(verb1,VERB2,true);
						if (designatedObjects[verb1]) SetRole(designatedObjects[verb1],OBJECT2,true);
					}
				}
				else if (posValues[before-1] & (PRONOUN_SUBJECT|PRONOUN_OBJECT))// pronoun will already be split
				{
					SetRole(before,SUBJECT2,true);
					phrases[before] = 0;
					SetRole(verb1,VERB2,true);
					if (designatedObjects[verb1]) SetRole(designatedObjects[verb1],OBJECT2,true);
				}
				else 
				{
					SetRole(before-1,roles[before]);
					SetRole(before,SUBJECT2,true);
					phrases[before] = 0;
					SetRole(verb1,VERB2,true);
					if (designatedObjects[verb1]) SetRole(designatedObjects[verb1],OBJECT2,true);
				}

				// if we find an entire given clause with starter, add the starter "what they found thrilled them"  
				while (before <= verb1) clauses[before++] |= clauseBit;
				clauseBit <<= 1;
				changed = true;
				return true;	
			}
			else break;
		}
	}
	return false;
}

static bool ProcessImpliedClause(unsigned int verb1,bool &changed) 
{
	if (clauses[verb1]) return false;	// already in a clause
	if (roles[verb1] == MAINVERB) return false;	// assume 1st is main verb

	unsigned int subject = verb1 - 1;
	if ( posValues[subject] & (AUX_VERB_BITS|ADVERB_BITS)) --subject;

	if (roles[subject] == MAINOBJECT) // implied clause as direct object  "I hope (Bob will go)"
	{
		if (trace) Log(STDUSERLOG,"implied direct object clause @ %s(%d)\r\n",wordStarts[subject],subject);
		SetRole(subject,SUBJECT2|MAINOBJECT,true);
		SetRole(verb1,VERB2,true);
		while (subject <= verb1) clauses[subject++] |= clauseBit;
		clauseBit <<= 1;
		changed = true;
		return true;	
	}
	return false;
}

static bool ProcessCommaClause(unsigned int verb1,bool &changed)
{
	if (!(roles[verb1] & CLAUSE)) return false; 
	if (trace) Log(STDUSERLOG,"Comma clause\r\n");
	needRoles[verb1] &= -1 ^ CLAUSE;
	unsigned int subject = verb1;
	while (posValues[--subject] != COMMA);
	while (++subject <= verb1) clauses[subject] |= clauseBit;
	clauseBit <<= 1;
	changed = true;
	return true;	
}

static void AddRoleLevel(unsigned int roles)
{
	needRoles[++roleIndex] = roles; 
	subjectStack[roleIndex] = verbStack[roleIndex] = auxVerbStack[roleIndex] = indirectObjectStack[roleIndex] = objectStack[roleIndex] = 0;
}

static bool ProcessOmittedClause(unsigned int verb1,bool &changed) // They were certain (they were happy)
{
	if (clauses[verb1]) return false;	// already done
	unsigned int subject = verb1;
	while (subject)
	{
		if (posValues[--subject] & (NOUN_BITS | PRONOUN_SUBJECT|PRONOUN_OBJECT)) break;
		if (posValues[subject] & (AUX_VERB_BITS|ADVERB_BITS)) continue;
		return false;
	}
	if (!subject) return false;
	AddRoleLevel(CLAUSE|SUBJECT2|VERB2);
	SetRole(verb1,VERB2,true);
	SetRole(subject,SUBJECT2,true);
	while (subject <= verb1) clauses[subject++] |= clauseBit;
	clauseBit <<= 1;
	--roleIndex;
	changed = true;
	return true;
}

static unsigned int LimitValues(unsigned int i, uint64 bits,char* msg,bool& changed)
{
	uint64 old = posValues[i];
	posValues[i] &= bits;
	char buff[MAX_WORD_SIZE];
	if (old != posValues[i])
	{
		if (posValues[i] == 0) posValues[i] = bits;	// if we lost everything, go to what was requested (expect it to be single)
		changed = true;
		bitCounts[i] = BitCount(posValues[i]);
		if (trace & POS_TRACE) 
		{
			*buff = 0;
			PosBits(posValues[i],buff);
			Log(STDUSERLOG,"Limit \"%s\"(%d) %s -> %s\r\n",wordStarts[i],i,msg,buff);
		}
	}
	return bitCounts[i];
}

static unsigned int ForceValues(unsigned int i, uint64 bits,char* msg,bool& changed)
{
	uint64 old = posValues[i];
	posValues[i] = bits;
	if (old != posValues[i])
	{
		changed = true;
		bitCounts[i] = BitCount(posValues[i]);
		if (trace & POS_TRACE) Log(STDUSERLOG,"Force \"%s\"(%d) %s\r\n",wordStarts[i],i,msg);
	}
	return bitCounts[i];
}

static bool ProcessReducedPassive(unsigned int verb1,bool allowObject,bool &changed)
{
	if (clauses[verb1]) return false;	// already in a clause
	if (designatedObjects[verb1] && !allowObject)  // be pessimistic, we dont want the main verb taken over
	{
		// the man given the ball is ok.
		// but the man dressed the cow is not - AND normal verbs might have objects
		// but "called" "named" etc create subjects of their objects and are ok.
		return false;
	}

	// or a past participle clause immediately after a noun --  the woman dressed in red
	// Or past particple clause starting sentence (before subject)  Dressed in red the woman screamed.   OR  In the park dressed in red, the woman screamed
	// OR -- the men driven by hunger ate first (directly known participle past)
	char* base = GetInfinitive(wordStarts[verb1],false);
	if (!base) return false;
	char* pastpart = GetPastParticiple(base);
	if (!pastpart) return false;
	if (stricmp(pastpart,wordStarts[verb1])) return false;	// not a possible past particple

	unsigned int before = verb1;
	if (posValues[verb1] & (VERB_PAST | VERB_PAST_PARTICIPLE)) 
	{
		bool causal = false;
		if (parseFlags[verb1] & CAUSAL_TOINFINITIVE_VERB || canSysFlags[verb1] & DIRECTOBJECT_INFINITIVE_COMPLEMENT)
		{
			while (--before > 0) // past tense needs to go to participle
			{
				if (posValues[before] & ADVERB_BITS) continue; // ignore this
				if (posValues[before] & (NOUN_BITS - NOUN_GERUND - NOUN_CARDINAL - NOUN_ORDINAL + PRONOUN_SUBJECT + PRONOUN_OBJECT)) 
				{
					if (phrases[before]) before = 0;	// cannot take object of prep and make it subject of clause directly
					break;
				}
				if (posValues[before] & COMMA) continue;
				return false;	// doesnt come after a noun or pronoun
			}

			// should this be main verb due to made me do it verb "the devil *made me do it"  causal verbs
			unsigned int after = verb1;
			while (++after < lastRealWord)
			{
				if (canSysFlags[after] & ANIMATE_BEING || IsUpperCase(*wordStarts[after]))
				{
					if (posValues[after+1] & (TO_INFINITIVE|VERB_INFINITIVE)) break;
				}
				else if (posValues[after] & (DETERMINER|ADJECTIVE_BITS)) continue;
				else break;
			}
			if (parseFlags[verb1] & CAUSAL_TOINFINITIVE_VERB && posValues[after+1] & (VERB_INFINITIVE|TO_INFINITIVE)) causal = true; // allow without to just in case
			else if (canSysFlags[verb1] & DIRECTOBJECT_INFINITIVE_COMPLEMENT && posValues[after+1] & (VERB_INFINITIVE|TO_INFINITIVE)) causal = true; // allow with to also
		}

		if (!causal) // "the devil *made me do it" blocks this reduction to a clause
		{
			// before might be 0 when clause at start of sentence
			LimitValues(verb1,VERB_PAST_PARTICIPLE,"reduced passive",changed);
			SetRole(verb1,VERB2,true);
			if (designatedObjects[verb1]) SetRole(designatedObjects[verb1],OBJECT2,true);
			clauses[verb1] |= clauseBit;
			clauseBit <<= 1;
			return true;	
		}
	}
	// alternative is we have coordinating sentences we didnt know about
	// defines the object, we come after main verb - We like the boy Eli *hated -   but "after he left home *I walked out
	// clause defines the subject, we come before main verb - The boy Eli hated *ate his dog - boy was marked as subject, and hated as mainverb.
	return false;
}

static void InitRoleSentence(unsigned int start)
{
	determineVerbal = 0;
	predicateZone = -1;
	roleIndex = 0;
	lastPhrase = lastVerbal = lastClause = 0; 
	firstAux = NULL;
	firstnoun = firstNounClause = 0;
	startSentence = start;
	AddRoleLevel(MAINSUBJECT|MAINVERB);
}

static void AddClause(unsigned int i,char* msg)
{
	if (trace) Log(STDUSERLOG,msg,wordStarts[i]);
	if (!clauses[i])
	{
		clauses[i] |= clauseBit;
		clauseBit <<= 1;
	}

	if (clauses[i] && clauses[i] != clauses[i-1]) // a new clause runs thru here
	{
		AddRoleLevel(CLAUSE|SUBJECT2|VERB2); // start of a clause triggers a new level for it, looking for subject and verb (which will be found).
		// possibly the start is the subject (or object) or possibly not. cannot tell in advance usually.
		if (!firstNounClause) // pending noun clause as subject for verb
		{					
			WORDP D = FindWord(wordStarts[i]);
			if (D && D->systemFlags & CONJUNCT_SUBORD_NOUN) firstNounClause = i; 
		}
		lastClause = i;		// where last clause was, if any
	}
}

static void ExtendChunk(unsigned int from, unsigned int to,unsigned int chunk[MAX_SENTENCE_LENGTH])
{
	unsigned int unit = chunk[from];
	if (!unit) unit = chunk[to]; 
	if (unit) while (from <= to) chunk[from++] |= unit;
}
	
// commas occur after a phrase at start, after an inserted clause or phrase, and in conjunction lists, and with closing phrases

static void FinishSentenceAdjust(bool resolved,bool & changed,unsigned int end)
{
	// we still need a main verb--- might this have been command - "talk about stuff"
	if (needRoles[MAINLEVEL] & MAINVERB && posValues[1] & NOUN_INFINITIVE)
	{
		LimitValues(1,VERB_INFINITIVE,"missing main verb, retrofix to command",changed);
		unsigned int v = verbals[1];
		unsigned int i = 0;
		while (verbals[++i] == v) verbals[i] ^= v; // rip out the verbal
		resolved = false;
		return;
	}


	// we end on indirect object. Must have been a direct object
	if (indirectObjectStack[roleIndex]  && needRoles[roleIndex] & (MAINOBJECT|OBJECT2))
	{
		SetRole(indirectObjectStack[roleIndex],needRoles[roleIndex] & (MAINOBJECT|OBJECT2));
	}

	while (needRoles[roleIndex] & (OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT) || needRoles[roleIndex] == PHRASE || needRoles[roleIndex] == CLAUSE || needRoles[roleIndex] == VERBAL) // close of sentence proves we get no final object... 
	{
		if (needRoles[roleIndex] & (VERB2 | MAINVERB | MAINSUBJECT | SUBJECT2 ) || needRoles[roleIndex] == (PHRASE|OBJECT2)) // we HAVE NOTHING finished here
		{
			resolved = false;
			break;
		}
		if (needRoles[roleIndex] & CLAUSE && lastClause) ExtendChunk(lastClause,end,clauses);
		if (needRoles[roleIndex] & VERBAL && lastVerbal) ExtendChunk(lastVerbal,end,verbals);
		if (needRoles[roleIndex] & PHRASE && lastPhrase) ExtendChunk(lastPhrase,end,phrases);

		--roleIndex; // close out level that is complete
		if (roleIndex == 0) // put back main sentence level
		{
			++roleIndex;
			break;
		}
	}

	int commas = 0;
	unsigned int assignedMainVerb = 0;
	unsigned int assignedMainSubject = 0;
	unsigned int mainVerb = 0;
	for (unsigned int i = startSentence; i <= end; ++i) 
	{
		if (quotedWords[i]) continue;
		if (posValues[i] == COMMA) ++commas;
		else if (roles[i] == MAINSUBJECT) assignedMainSubject = i;
		else if (roles[i] == MAINVERB) assignedMainVerb = i;
		else if (posValues[i] & VERB_TENSES && !mainVerb) mainVerb = i;
	}
	if (mainVerb && !assignedMainVerb)
	{
		SetRole(mainVerb,MAINVERB,true); 
		if (trace & POS_TRACE) Log(STDUSERLOG,"Forcing found verb into main verb \"%s\"(%d)\r\n",wordStarts[mainVerb],mainVerb);
		if (designatedObjects[mainVerb] ) SetRole(designatedObjects[mainVerb],MAINOBJECT,true);
	}

	if (assignedMainSubject && assignedMainVerb && (zoneMember[assignedMainVerb] - zoneMember[assignedMainSubject]) == 1 && clauses[assignedMainSubject]) // was this a clause and an imperative: "whoever comes to the door, ask them to wait"
	{
		if (allOriginalWordBits[assignedMainVerb] & VERB_INFINITIVE) // rewrite as imperative
		{
			SetRole(assignedMainSubject,SUBJECT2,true);
			LimitValues(assignedMainVerb,VERB_INFINITIVE,"clause is not subject, making sentence imperative",changed);
			assignedMainSubject = 0;
			subjectStack[MAINLEVEL] = 0;
		}
	}

	// Hector, address the class  is command, not subject, verb
	if (startSentence == (int)firstRealWord && roles[firstRealWord] ==  MAINSUBJECT && posValues[firstRealWord+1] == COMMA && roles[firstRealWord+2] == MAINVERB && allOriginalWordBits[firstRealWord+2] & VERB_INFINITIVE) 
	{
		if (trace & POS_TRACE) Log(STDUSERLOG,"Forcing subject in leading comma zone to be address \"%s\"(%d)\r\n",wordStarts[firstRealWord],1);
		SetRole(firstRealWord,0);
		SetRole(firstRealWord,ADDRESS,true);
		LimitValues(firstRealWord+2,VERB_INFINITIVE,"Force verb to be infinitive on command",changed);
		assignedMainSubject = 0;
	}
	
	for (unsigned int i = startSentence; i <= end; ++i)
	{
		if (quotedWords[i]) continue;
		if (posValues[i] & VERB_TENSES && !roles[i]) // verb w/o a role?
		{
			// Maybe we made a phrase what should have been a clause
			unsigned int phrase = phrases[i-1];
			unsigned int at = i;
			if (phrase)
			{
				while (--at && phrases[at] == phrase){;} 
				if (posValues[++at] == PREPOSITION && allOriginalWordBits[at] & CONJUNCTION_SUBORDINATE) // change our mind
				{
					unsigned int x = at - 1;
					while (phrases[++x] == phrase) // remove phrase and change object to subject
					{
						phrases[x] = 0; 
						if (roles[x] == OBJECT2) 
						{
							SetRole(x,SUBJECT2);
							if (posValues[x] & PRONOUN_OBJECT) LimitValues(x,PRONOUN_SUBJECT,"forcing object to subject pronoun %s",changed);
						}
					}
				}
				LimitValues(at,CONJUNCTION_SUBORDINATE,"reverting phrase to clause to handle extra verb %s",changed);
				AddClause(at,"make clause of phrase"); 
				ExtendChunk(at,i,clauses);
				if (needRoles[roleIndex] & CLAUSE) --roleIndex;	// discard clause level since we are done
				SetRole(i,VERB2);
				continue;
			}

			ProcessCommaClause(i,changed); // clause bounded by commas?
			ProcessSplitNoun(i,changed); // see if is clause instead
			ProcessReducedPassive(i,false,changed); // we carefully avoid verbs with object in this pass
			ProcessImpliedClause(i,changed);
			if (!clauses[i]) 
			{
				ProcessReducedPassive(i,true,changed); // allow objects to be accepted
				ProcessOmittedClause(i,changed);		  // whole seemingly second sentence
			}
		}
	}	

	// found no main verb? resurrect a clause into main verb - prefer removal of clause modifying prep object
	if (!mainVerb)
	{
		for (unsigned int i = startSentence; i <= end; ++i)
		{
			if (quotedWords[i]) continue;
			if (posValues[i] & VERB_TENSES && clauses[i] && phrases[i-1]) // verb w/o a role?
			{
				SetRole(i,MAINVERB); 
				if ( designatedObjects[i] ) SetRole(designatedObjects[i],MAINOBJECT);
				clauses[i] = 0;
				if (trace & POS_TRACE) Log(STDUSERLOG,"Lacking main verb, resurrect clause into main verb \"%s\"(%d)\r\n",wordStarts[i],i);
				break;
			}
		}
	}

	// sentence starting in qword, not in a clause. maybe flip subject and object 
	if (subjectStack[MAINLEVEL] &&  !clauses[firstRealWord] && verbStack[MAINLEVEL] && objectStack[MAINLEVEL] && (allOriginalWordBits[firstRealWord] & QWORD || (allOriginalWordBits[firstRealWord] & PREPOSITION && allOriginalWordBits[2] & QWORD) ))
	{
		//  WHAT (what did john eat == john did eat what) 
		// but not when WHO (who hit the ball != the ball hit who)
		bool flip = true;
		// NEVER flip particple verbs:   who is hitting the ball. What is hitting the ball.  what squashed the ball
		if (posValues[verbStack[MAINLEVEL]] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE)) flip = false;
		else if (objectStack[MAINLEVEL] < subjectStack[MAINLEVEL]) flip = false; // already flipped
		else if (canonicalLower[subjectStack[MAINLEVEL]] && !(canonicalLower[subjectStack[MAINLEVEL]]->properties & QWORD)) flip = false; // "why did *john hit the ball"
		else flip = true;

		if (flip)
		{
			unsigned int newsubject = objectStack[MAINLEVEL];
			unsigned int newobject = subjectStack[MAINLEVEL];
			SetRole(newobject, MAINOBJECT);
			if (posValues[newobject] == PRONOUN_SUBJECT) posValues[newobject] = PRONOUN_OBJECT;
			SetRole(newsubject,MAINSUBJECT);
			if (posValues[newsubject] == PRONOUN_OBJECT) posValues[newsubject] = PRONOUN_SUBJECT;
			if (trace & POS_TRACE) Log(STDUSERLOG,"Flipping subject and object due to qword start \"%s\"(%d)\r\n",wordStarts[objectStack[MAINLEVEL]],objectStack[MAINLEVEL]);
		}
	}

	// sentence starting in qword and ending in prep- remove qword as main object and make it OBJECT2 of prep
	if (end == lastRealWord && allOriginalWordBits[firstRealWord] & QWORD && posValues[end] == PREPOSITION && roles[firstRealWord] == MAINOBJECT)
	{
		SetRole(firstRealWord,OBJECT2);
		if (allOriginalWordBits[firstRealWord] & PRONOUN_OBJECT) posValues[firstRealWord] = PRONOUN_OBJECT;
		if (trace & POS_TRACE) Log(STDUSERLOG,"Pronoun wrap to start, coercing start to object2 and maybe pronoun object \"%s\"(%d)\r\n",wordStarts[firstRealWord],firstRealWord);
	}

	// sentence starting with here or there as subject
	if (subjectStack[MAINLEVEL] && objectStack[MAINLEVEL] && verbStack[MAINLEVEL] && (!stricmp(wordStarts[subjectStack[MAINLEVEL]],"here") || !stricmp(wordStarts[subjectStack[MAINLEVEL]],"there"))) // HERE and THERE can not be the subject of a sentence:  here is the pig.. BUT  here is nice is legal.
	{
		unsigned int subject = subjectStack[MAINLEVEL];
		unsigned int object = objectStack[MAINLEVEL];
		if (trace & POS_TRACE) Log(STDUSERLOG,"Here/There cannot be subject, revise object\"%s\"(%d)\r\n",wordStarts[objectStack[MAINLEVEL]],objectStack[MAINLEVEL]);
		SetRole(subject,0); 
		if (!stricmp(wordStarts[subjectStack[MAINLEVEL]],"here"))  LimitValues(subject,ADVERB_NORMAL,"Here as subject is adverb",changed);
		else if (posValues[subject] == THERE_EXISTENTIAL) {;}
		else LimitValues(subject,ADVERB_NORMAL,"Tere as subject is adverb not existential",changed); 
		SetRole(object,MAINSUBJECT,true);
	}
	
	// migrate direct objects into clauses and verbals....  (should have been done already)
	for (unsigned int i = startSentence; i <= end; ++i)
	{
		if (quotedWords[i]) continue;
		if (!designatedObjects[i]) continue; // nothing interesting
		unsigned int object;
		unsigned int at;
		if (verbals[i])
		{
			unsigned int verbal = verbals[i];
			unsigned int phrase = phrases[i];	// verbal might be object of a phrase
			at = i;

			// see if it has an object also...spread to cover that...
			while ((object = designatedObjects[at]) && object > at)
			{
				for (unsigned int j = at+1; j <= object; ++j) 
				{
					verbals[j] = verbal;	//  "to eat *rocks"
					if (phrase) phrases[j] = phrase;
				}
				at = designatedObjects[at]; // extend to cover HIS object if he is gerund or infintiive
			}
		}
		if (clauses[i])
		{
			at = i;
			// see if it has an object also...spread to cover that...
			while ((object = designatedObjects[at])  && object > at)
			{
				ExtendChunk(i,object,clauses);
				at = designatedObjects[object]; // extend to cover HIS object
			}
		}
		if (phrases[i])
		{
			unsigned int phrase = phrases[i];
			at = i;
			// see if it has an object also...spread to cover that... "after eating *rocks"
			while ((object = designatedObjects[at]) && object > at)
			{
				for (unsigned int j = at+1; j <= object; ++j) phrases[j] = phrase;
				at = designatedObjects[at]; // extend to cover HIS object
			}
		}
	}

	// if we have OBJECT2 not in a clause or phrase.... maybe we misfiled it. "Hugging the ground, Nathan peered."
	if (resolved) for (unsigned int i = startSentence; i <= end; ++i)
	{
		if (quotedWords[i]) continue;
		if (roles[i] != OBJECT2 || phrases[i] || clauses[i] || verbals[i]) continue;
		if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] < i && posValues[subjectStack[MAINLEVEL]] == NOUN_GERUND) // the Verbal will be an adjective phrase on the main subject
		{
			SetRole(i,MAINSUBJECT);
			SetRole(subjectStack[MAINLEVEL],SUBJECT2);
		}
		// if subject is 1st word, after it is clause, and comma, then our phrase, migrate into clause -- those dressed in red, the men ate first
		if (subjectStack[MAINLEVEL] == 1 && clauses[2] && i < verbStack[MAINLEVEL]) // just assume it is right for now if is before main verb
		{
			SetRole(subjectStack[MAINLEVEL],SUBJECT2); // establishs transient subjectstack on wrong level but is fixed by next setrole
			SetRole(i, MAINSUBJECT);
			clauses[firstRealWord] = clauses[firstRealWord+1];
		}
	}
}

static int FindCoordinate(int i) // i is on comma
{
	bool comma2 = false;
	while ((unsigned int) ++i < lastRealWord)
	{
		if (posValues[i] & COMMA) comma2 = true; // there is a comma
		if (posValues[i] & CONJUNCTION_COORDINATE)
		{
			if (comma2) return i; // conjunction after a second comma
		}
	}
	return 0; // didnt find
}

static void DoCoord(unsigned int i, unsigned int before, unsigned int after, unsigned int type)
{
	SetRole(i,type);	
	SetRole(after,roles[before]); // pass along any earlier role
	needRoles[roleIndex] |= roles[before];	// reinstate goal to find it again
	if ((type == CONJUNCT_NOUN || type == CONJUNCT_VERB))
	{
		// if he has previously filled a noun role, we need to request it again
		if (roles[before] & (SUBJECT2 | MAINSUBJECT | MAINVERB | MAINOBJECT | OBJECT2 )) needRoles[roleIndex] |= roles[before];
	}

	// form or continue a ring - x AND y  where y loops back to x
	unsigned int hook = coordinates[before]; // loop onto any prior ring 
	if (!hook || hook > before) hook = before; // if already hooked later, dont use that hook. it should be part of earlier loop
	coordinates[after] = (unsigned char)hook;
	coordinates[before] = (unsigned char)i;
	coordinates[i] = (unsigned char)after;
	if (phrases[before]) ExtendChunk(before,after,phrases); // expand dual object in prep phrase
	if (trace & POS_TRACE) Log(STDUSERLOG,"Conjunct=%s(%d) %s/%s\r\n",wordStarts[i],i,wordStarts[before],wordStarts[after]);
}

static void ForceNounPhrase(unsigned int i, bool &changed) // insure things before are proper words
{
	while (--i && !(posValues[i] & CONJUNCTION_COORDINATE))
	{
		if (bitCounts[i] != 1 && posValues[i] & (ADJECTIVE_BITS|DETERMINER))
		{
			LimitValues(i,ADJECTIVE_BITS|DETERMINER,"ForceNounPhrase",changed);
		}
	}
}

static unsigned int NearestProbableNoun(unsigned int i)
{
	while (++i < lastRealWord)
	{
		if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS) && (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT) || bitCounts[i] == 1 || (originalLower[i] && originalLower[i]->systemFlags & NOUN))) break; // probable noun
	}
	return (i <= lastRealWord) ? i : 0;
}

static unsigned int NearestProbableVerb(unsigned int i)
{
	while (++i < lastRealWord)
	{
		if (posValues[i] & (VERB_TENSES|AUX_VERB_BITS) && (bitCounts[i] == 1 || (originalLower[i] && originalLower[i]->systemFlags & VERB))) break; // probable verb
	}
	return (i <= lastRealWord) ? i : 0;
}

static void HandleCoordConjunct(unsigned int i,bool &changed) // determine kind of conjunction- 
{// Word (adjective/adjectve, adverb/adverb, noun/noun, verb/verb) 
 // Phrase/Clause/Sentence
	//  x coord y will be linked in a ring structure.

	if (!stricmp(wordStarts[i],"so") || !stricmp(wordStarts[i],"for")) // these only do sentences
	{
		SetRole(i,CONJUNCT_SENTENCE); 
		return;
	}

	unsigned int comma = i;
	while (--comma && !(posValues[comma] & COMMA && roles[comma])){;}	// find the first comma 
	if (roles[comma] && coordinates[comma]) // found a comma role with a conjunction role, replicate it
	{
		uint64 match = 0;
		match = posValues[coordinates[comma]];	// what is being bound
		if (match & (NOUN_BITS | PRONOUN_BITS)) match = NOUN_BITS | PRONOUN_SUBJECT | PRONOUN_OBJECT;
		else if (match & VERB_TENSES) match = VERB_TENSES;
		else if (match & ADJECTIVE_BITS) match = ADJECTIVE_BITS;
		else if (match & ADVERB_BITS) match = ADVERB_BITS;
		else match = 0;
		unsigned int deepAfter = i;
		while (++deepAfter <= lastRealWord)
		{
			if (posValues[deepAfter] & match)
			{
				LimitValues(deepAfter,match,"Forcing comma match of pos",changed);
				break;
			}
		}
		if (deepAfter <= lastRealWord)	
		{
			DoCoord(i,coordinates[comma],deepAfter,roles[comma]);
			return;
		}
	}

	unsigned int before = i - 1;
	unsigned int after = i + 1;
	if (posValues[before] == COMMA) --before;					

	// most like are simple word pairs....
	
	// simple ADVERB before  "moved quickly and quietly"  
	if (posValues[before] & ADVERB_BITS && posValues[after] & ADVERB_BITS) 
	{
		bool badmix = false;
		// - but not if a verb is before and after like "they worked hard and then walked"
		if (posValues[before-1] & (VERB_INFINITIVE|VERB_PAST|VERB_PRESENT|VERB_PRESENT_3PS) && posValues[after+1] & (VERB_INFINITIVE|VERB_PAST|VERB_PRESENT|VERB_PRESENT_3PS) && 
			bitCounts[after+1] == 1) badmix = true; // know verb after
		 // dont mix time and OTHER adverbs with and
		else if (canonicalLower[before] && canonicalLower[after] && (canonicalLower[before]->systemFlags & TIMEWORD) != (canonicalLower[after]->systemFlags & TIMEWORD)) badmix = true;
		else if (bitCounts[before] == 1 && bitCounts[after] != 1) 
		{
			LimitValues(after,ADVERB_BITS,"forcing ADVERB conflict to clear conjunction after",changed);
		}
		else if (bitCounts[after] == 1  && bitCounts[before] != 1) 
		{
			LimitValues(before,ADVERB_BITS,"forcing ADVERB conflict to clear conjunction before",changed);
		}
		if (!badmix && bitCounts[after] == 1 &&  bitCounts[before] == 1)
		{
			DoCoord(i,before,after,CONJUNCT_ADVERB);
			return;
		}
	}

	// after any coord one might have an intervening adverb to ignore like "I like dogs but rarely cats" or "I like dogs and often cats"
	while (posValues[after] == ADVERB_NORMAL && after < lastRealWord) ++after;

	// simple ADJECTIVE  "green and red beets"  
	int deepAfter = after;
	while (posValues[deepAfter] & ADVERB_BITS) ++deepAfter;	// "I like green *and very red ham" can skip adverbs intervening
	if (posValues[before] & ADJECTIVE_BITS && posValues[deepAfter] & ADJECTIVE_BITS) 
	{
		if (bitCounts[before] == 1 && bitCounts[deepAfter] != 1) 
		{
			LimitValues(deepAfter,ADJECTIVE_BITS,"forcing ADJECTIVE conflict to clear conjunction after",changed);
		}
		else if (bitCounts[deepAfter] == 1  && bitCounts[before] != 1) 
		{
			LimitValues(before,ADJECTIVE_BITS,"forcing ADJECTIVE conflict to clear conjunction before",changed);
		}
		if (bitCounts[deepAfter] == 1 &&  bitCounts[before] == 1)
		{
			DoCoord(i,before,deepAfter,CONJUNCT_ADJECTIVE);
			return;
		}
	}

	// test for verb match first before noun, since maybe we can confirm on tense...

	// simple verb after matches verb before (adverb already sorted out earlier) "he danced and pranced"
	if (posValues[after] & VERB_TENSES) 
	{
		int deepBefore = after;
		while (--deepBefore && !(posValues[deepBefore] & VERB_TENSES)){;}		// back up to another potential verb 
		if (deepBefore)
		{
			if (deepBefore) // tense must match up as possible "he pulled carrots and *dip" cannot match up verbs
			{
				uint64 tense = posValues[deepBefore] & VERB_TENSES;
				if (posValues[after] & tense) // maybe can match up verb tenses
				{
					LimitValues(after,tense,"Forcing tense match for AND verb",changed);
					if (posValues[deepBefore] == posValues[after]) // tenses now match
					{
						DoCoord(i,deepBefore,after,CONJUNCT_VERB);
						return;
					}
				}
				// cannot match up verb tenses
				else
				{
					if (bitCounts[after] > 1 && bitCounts[deepBefore] == 1) LimitValues(after,-1 ^ VERB_TENSES,"tenses dont line up for AND, so remove verb potential",changed);
				}
			}
		}
	}

	// simple NOUN after "ate rocks and greens" 
	unsigned int deepAfterVerb = after;
	if (needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT)){;} // wont be sentence conjoin
	else if (!(needRoles[roleIndex] & (OBJECT2|MAINOBJECT)))
	{
		while (++deepAfterVerb < lastRealWord && !(posValues[deepAfterVerb] & (COMMA|CONJUNCTION_COORDINATE)))
		{
			if (posValues[deepAfterVerb] & VERB_TENSES) break;	 // possible verb
		}
	}
	if (posValues[before] & (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS) && posValues[after] &  (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS) && !(posValues[deepAfterVerb] & VERB_TENSES)) 
	{

		if (bitCounts[after] == 1  && bitCounts[before] != 1) 
		{
			LimitValues(before, (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS),"forcing NOUN conflict to clear conjunction before",changed);
		}
		if (bitCounts[after] != 1  && bitCounts[before] == 1) 
		{
			LimitValues(after, (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS),"forcing NOUN conflict to clear conjunction after",changed);
		}
		if (bitCounts[after] == 1 &&  bitCounts[before] == 1) // before and after are known
		{
			DoCoord(i,before,after,CONJUNCT_NOUN);
			ForceNounPhrase(after,changed);
			return;
		}
	}

	// known noun before and described noun after (simple noun would already be done by now)
	if (posValues[before] &  (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS) && bitCounts[before] == 1 && posValues[after] & (DETERMINER|ADVERB_BITS|ADJECTIVE_BITS|POSSESSIVE_BITS)  && bitCounts[after] == 1) 
	{
		int deepAfter = after;
		while (posValues[deepAfter] & (DETERMINER|ADVERB_BITS|ADJECTIVE_BITS|POSSESSIVE_BITS)) ++deepAfter;
		if (deepAfter <= (int)lastRealWord && posValues[deepAfter] &  (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS))
		{
			if (bitCounts[deepAfter] != 1) 
			{
				LimitValues(deepAfter, (PRONOUN_SUBJECT|PRONOUN_OBJECT|NOUN_BITS),"forcing NOUN conflict to clear conjunction after",changed);
			}
			if (bitCounts[deepAfter] == 1 &&  bitCounts[before] == 1)
			{
				DoCoord(i,before,deepAfter,CONJUNCT_NOUN);
				ForceNounPhrase(deepAfter,changed);
				return;
			}
		}
	}

	// known noun after, hunt for before
	if (posValues[after] & NOUN_BITS && bitCounts[after] == 1)
	{
		int deepBefore = i;
		while (--deepBefore) 
		{
			if (posValues[deepBefore] & NOUN_BITS && bitCounts[deepBefore] == 1)
			{
				DoCoord(i,deepBefore,after,CONJUNCT_NOUN);
				ForceNounPhrase(after,changed);
				return;
			}
		}
	}
	
	// phrase before and possible preposition after
	if (phrases[before] && posValues[after] & PREPOSITION)  
	{
		unsigned int deepBefore = before;
		while (deepBefore && phrases[deepBefore] == phrases[deepBefore-1]) --deepBefore; // find the prep
		if (bitCounts[after] != 1) // can we force preposition on after?
		{
			LimitValues(after,PREPOSITION,"forcing PREPOSITION conflict to clear conjunction after",changed);
		}
		if (posValues[deepBefore] == posValues[after]) // preps now match
		{
			DoCoord(i,deepBefore,after,CONJUNCT_PHRASE);
			return;
		}
	}
	// phrase after and possible preposition before
	if (phrases[after] && posValues[after] & PREPOSITION && !phrases[before])  
	{
		unsigned int deepBefore = before;
		while (deepBefore && posValues[deepBefore] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER|PREDETERMINER)) --deepBefore; // find the prep if any
		if (posValues[deepBefore] & PREPOSITION && bitCounts[deepBefore] != 1) // can we force preposition on after?
		{
			LimitValues(deepBefore,PREPOSITION,"forcing PREPOSITION conflict to clear conjunction before",changed);
		}
		if (posValues[deepBefore] == posValues[after]) // preps now match
		{
			DoCoord(i,deepBefore,after,CONJUNCT_PHRASE);
			return;
		}
	}

	if (posValues[before] & PARTICLE && posValues[after] & (VERB_TENSES|NOUN_INFINITIVE))  // he stood up and ran down the street
	{
		unsigned int deepBefore = before;
		while (--deepBefore > 2) if ( posValues[deepBefore] & (VERB_TENSES|NOUN_INFINITIVE)) break;
		DoCoord(i,deepBefore,after,CONJUNCT_PARTICLE);
	}
	else if (posValues[after] & PREPOSITION && posValues[before] & (NOUN_BITS|PRONOUN_SUBJECT|PRONOUN_OBJECT))  
	{
		int r = before;
		while (--r) // prove phrase matching going on
		{
			if (! (posValues[r] & (NOUN_BITS|DETERMINER|PREDETERMINER|ADJECTIVE_BITS|ADVERB_BITS))) break;
		}
		if ( posValues[r] & PREPOSITION) 
		{
			SetRole(i, CONJUNCT_PHRASE);
			coordinates[i] = (unsigned char)r;
		}
		if (trace) Log(STDUSERLOG,"DuplicatePhrase");
	}
	else if (posValues[deepAfter] & VERB_TENSES && verbStack[MAINLEVEL]) // we and to a verb... expect this to be WORD
	{
		SetRole(i,CONJUNCT_VERB);
		unsigned int verb = i;
		while (--verb && !(posValues[verb] & VERB_TENSES)); // find verb before
		SetRole(deepAfter,roles[verb]);	// copy verb role from before
		LimitValues(deepAfter,VERB_TENSES,"conjoined after is VERB",changed);
		coordinates[i] = (unsigned char)verb;
		if (roles[deepAfter] == MAINVERB && canSysFlags[i] & (VERB_DIRECTOBJECT|VERB_INDIRECTOBJECT)) // change to wanting object(s)
		{
			needRoles[MAINLEVEL] |= MAINOBJECT;
			if ( canonicalLower[i]->properties & VERB_INDIRECTOBJECT) needRoles[MAINLEVEL] |= MAININDIRECTOBJECT;
			if (trace) Log(STDUSERLOG,"DuplicateVerb");
		}
	}
	else if (roleIndex > 1 && subjectStack[MAINLEVEL] && verbStack[MAINLEVEL] && subjectStack[2])
	{
		DoCoord(i,subjectStack[MAINLEVEL],subjectStack[2],CONJUNCT_SENTENCE);
	}
	else if (!(needRoles[MAINLEVEL] & (MAINSUBJECT|MAINVERB))) // can it be sentence we need
	{
		// if prior was a direct object and we have only a few words left, replicate it
		unsigned int noun = NearestProbableNoun(i);
		unsigned int verb = NearestProbableVerb(i);
		if (roles[before] == MAINOBJECT && (int)noun > i && !verb ) // we have some noun we could find, and no verb to make into a clause maybe
		{
			needRoles[MAINLEVEL] |= MAINOBJECT;
			roles[i] = CONJUNCT_NOUN;
		}
		else if (noun && verb > noun) // we can find a potential noun and verb in succession, so assume it is a sentence
		{
			SetRole(i,CONJUNCT_SENTENCE); 
		}
	}
	// we dont know what it binds to yet--- might be mainsubject, might be to a current verbal
	if (trace) Log(STDUSERLOG,"Conjunct=%s(%d)\r\n",wordStarts[i],i);
}

static void CloseLevel(int  i)
{
	while (roleIndex > 1 && needRoles[roleIndex] && !(needRoles[roleIndex] & (-1 ^ KINDS_OF_PHRASES)) ) 
	{
		if (needRoles[roleIndex] & PHRASE) // closes out a phrase, drag phrase over it
		{
			if (!phrases[i]) ExtendChunk(lastPhrase,i,phrases);
			lastPhrase = 0;
		}
		else if (needRoles[roleIndex] & CLAUSE) // closes out a clause, drag clause over it
		{
			if (!clauses[i]) ExtendChunk(lastClause,i,clauses);
			lastClause = 0;
		}
		else if (needRoles[roleIndex] & VERBAL) // closes out a verbal, drag verbal over it
		{
			if (!verbals[i]) ExtendChunk(lastVerbal,i,verbals);
			lastVerbal = 0;
		}

		--roleIndex;	// level complete (not looking for anything anymore)
	}
}

static void DecodeneedRoles(unsigned int x,char* buffer)
{
	*buffer = 0;
	if (needRoles[x] & PHRASE) strcat(buffer," Phrase:");
	if (needRoles[x] & CLAUSE) strcat(buffer," Clause:");
	if (needRoles[x] & VERBAL) strcat(buffer," Verbal:");
	if (!(needRoles[x] & (PHRASE|CLAUSE|VERBAL))) strcat(buffer," Main:");
	if (needRoles[x] & MAINSUBJECT) strcat(buffer," MainSubject");
	if (needRoles[x] & MAINVERB) strcat(buffer," MainVerb");
	if (needRoles[x] & SUBJECT2) strcat(buffer," Subject2");
	if (needRoles[x] & VERB2) strcat(buffer," Verb2");
	if (needRoles[x] & INDIRECTOBJECT2) strcat(buffer," Io2");
	if (needRoles[x] & OBJECT2) strcat(buffer," Obj2");
	if (needRoles[x] & MAININDIRECTOBJECT) strcat(buffer," MainIO");
	if (needRoles[x] & MAINOBJECT) strcat(buffer," Mainobj");
	if (needRoles[x] & OBJECT_COMPLEMENT) strcat(buffer," ObjCompl");
	if (needRoles[x] & SUBJECT_COMPLEMENT) strcat(buffer," SubCompl");
}

static void SeekObjects(unsigned int i) // this is a verb, what objects does it want
{
	if (!verbStack[roleIndex]) verbStack[roleIndex] = (unsigned char)i;	// this is the verb for this level
	
	if (roleIndex == 1 && objectStack[MAINLEVEL] && !coordinates[i]) // main verb has an objects already (qword like in "what do you *like to do") - but not if we have a coordinating word verb needing own object
		return;
	
	if (canonicalLower[i] == DunknownWord)  // unknown verb -- presume takes object and indirect object and maybe object complement
	{ // presumed NOT a linking verb, so cannot take SUBJECT_COMPLEMENT.
		if (roleIndex == 1) needRoles[MAINLEVEL] |= MAININDIRECTOBJECT | MAINOBJECT | OBJECT_COMPLEMENT;
		else needRoles[roleIndex] |= OBJECT2 | INDIRECTOBJECT2 | OBJECT_COMPLEMENT;
		return;
	}

	if (canSysFlags[i] & VERB_DIRECTOBJECT) // change to wanting object(s)
	{
		// if a question, maybe object already filled in "what dog do you like"
		if (!objectStack[roleIndex] || coordinates[i]) 
		{
			needRoles[roleIndex] |= (roleIndex == 1) ? MAINOBJECT : OBJECT2;
			if (parseFlags[i] & (FACTITIVE_ADJECTIVE_VERB|FACTITIVE_NOUN_VERB))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we made him *happy"  "we elected him *president"
			else if (canSysFlags[i] & (DIRECTOBJECT_INFINITIVE_COMPLEMENT|DIRECTOBJECT_TOINFINITIVE_COMPLEMENT))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we want him to go"
		}
		// are you able to drive a car wants object2 for car
	}

	if (canSysFlags[i] & (VERB_INDIRECTOBJECT|DIRECTOBJECT_INFINITIVE_COMPLEMENT) || parseFlags[i] & CAUSAL_TOINFINITIVE_VERB) // causal verbs also expect indirect objects
	{
		if ( !(posValues[i+1] & (DETERMINER | PREDETERMINER | ADJECTIVE | ADVERB | PRONOUN_OBJECT  | PRONOUN_POSSESSIVE | NOUN_BITS))); // must come immediately
		else if (roleIndex == 1) needRoles[roleIndex] |= MAININDIRECTOBJECT;
		else needRoles[roleIndex] |= INDIRECTOBJECT2;
	}
	
	if (canSysFlags[i] & LINKING_VERB)  
	{
		needRoles[roleIndex] |= SUBJECT_COMPLEMENT; // linking verbs expect adjective 
		needRoles[roleIndex] |= (roleIndex == 1) ? MAINOBJECT : OBJECT2;
	}
	if (parseFlags[i] & CAUSAL_TOINFINITIVE_VERB) needRoles[roleIndex] |= (roleIndex == 1) ? (MAINOBJECT|MAININDIRECTOBJECT) : (OBJECT2|INDIRECTOBJECT2);
	// warning- we will kill off indirect objects not beings: "I made the mountain sing" will not accept mountain as indirect object.. a weakness

	// verb wants noun complement - we will eventually have to decide on object complement vs indirect object, but when we could be both, we wait
	if (parseFlags[i] & FACTITIVE_NOUN_VERB)  needRoles[roleIndex] |= OBJECT_COMPLEMENT;
}

static bool IsLegalAddress(unsigned int first)
{
	// simple address
	if (posValues[first] & NOUN_PROPER_SINGULAR && posValues[first-1] == COMMA && first == lastRealWord) return true; // tail "I hate you, Barbara"
	if (posValues[first] & NOUN_PROPER_SINGULAR && posValues[first+1] == COMMA && first == firstRealWord && posValues[first+2] & PRONOUN_SUBJECT) return true; // tail "Barbara, I hate you"

	return false;
}

static bool IsLegalAppositive(unsigned int first, unsigned int second)
{
	if (!(posValues[first] & (NOUN_SINGULAR|NOUN_PLURAL|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_ORDINAL)) || bitCounts[first] != 1) return false;
	if (!(posValues[second] & (NOUN_SINGULAR|NOUN_PLURAL|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_ORDINAL)) || bitCounts[second] != 1) return false;

	// nouns must be determined or proper - "my cat, dog" is not legal but "my cat, a dog" is -- or a title like "Julie Winter president of the club" or "CEO Tom"
	unsigned int det1 = first;
	while (det1 > 1 && posValues[det1] & (ADJECTIVE_NORMAL|NOUN_BITS) && !(posValues[det1] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) --det1;
	if (!(posValues[det1] & (DETERMINER|POSSESSIVE_BITS|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) return false;

	unsigned int det2 = second;
	while (det2 > 1 && posValues[det2] & (ADJECTIVE_NORMAL|NOUN_BITS) && !(posValues[det2] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) --det2;
	if (!(posValues[det2] & (DETERMINER|POSSESSIVE_BITS|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) return false;

	// cannot have both using the "the cat, the dog" is illegal. But "A cat, a fine species" is legal
	if (!stricmp(wordStarts[det1],"the") && !stricmp(wordStarts[det2],"the")) return false;

	return true;
}

static void HandleComplement(unsigned int i,bool & changed)
{
	// object complements are handled elsewhere
	unsigned int subject = needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT);
	unsigned int indirectObject = needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2);
	unsigned int directObject = needRoles[roleIndex] & (MAINOBJECT|OBJECT2);
	if (posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE|VERB_TENSES))  indirectObject = 0;	// cannot be indirect object with these...

// beware: Dogs, my favorite animals, are swift.   SUBJECT/APPOSITIVE/PREDICATE
// vs Bill, I hate you.	ADDRESS/SENTENCE 
// Bill, dogs, those fast creatures, can outrun you. ADDRESS/SUBJECT/APPOSITIVE/PREDICATE
//  Fleeing home, they left.
// walking home, my favorite activity, happens often. - gerund, appositive, predicate  OR  adjectivepariticle, subject, predicate (cannot be)
//  subject, predicate is not a possible format...  subject, description, predicate is possible
	// predicate object is not a possible format either.

	// indirect object in normal speech must be an animate being.  But "I told the mountain my name" would be legal, just uncommon.
	if (indirectObject )
	{
		//if (posValues[i+1] == VERB_INFINITIVE ) indirectObject = 0;	// cannot be an indirect object
		//else 
		if (originalLower[i] && originalLower[i]->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_HUMAN)) // proper names are good
		{
		}
		else if (canSysFlags[i] & ANIMATE_BEING) // animate beings are good
		{
		}
		else if (i > 1 && IsUpperCase(*wordStarts[i])){;} // anything uppercase not 1st word is good
		else 
		{
			if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"Abandoning indirect object at %s nonanimate\r\n",wordStarts[i]);
			needRoles[roleIndex] &= -1 ^ (MAININDIRECTOBJECT|INDIRECTOBJECT2); 
			indirectObject = 0;
		}
	}
retry:
	if (subject)
	{
		if (predicateZone == -1) {;}									// never located verb yet (ambiguous or missing)
		else if ((int)currentZone == predicateZone) {;}							// zone data says this will be subject since must happen before verb
		else if ((predicateZone - currentZone) == 1 && roleIndex == 1) subject = 0;			// cannot have main subject in zone immediately before predicate unless its a clause zone
		else if (zoneData[predicateZone] & ZONE_SUBJECT && roleIndex == 1) subject = 0;		// main sentence is elsewhere
		
		if (i < 3 && roleIndex == 1 && needRoles[MAINLEVEL] & MAINSUBJECT && IsLegalAddress(i)) SetRole(i,ADDRESS);
		else if (subject)
		{
			// seeking subject but this is noun matching ending prep, with qword heading, so cant be subject "*who did you give your number to" or 
			if (i == firstnoun && posValues[lastRealWord] & PREPOSITION && originalLower[firstRealWord] && originalLower[firstRealWord]->properties & QWORD)
			{
				SetRole(i,OBJECT2);
			}
			else
			{
				if (roles[i] & (OBJECT2|MAINOBJECT) && clauses[i]) AddRole(i,subject); // clause fulfilling mainobject role or object2 role
				else SetRole(i,subject);
			}
		}
		else if ( posValues[i] & (NOUN_CARDINAL|NOUN_ORDINAL)) // cannot be appositive (except-  "John, the first, went home" maybe)
		{
		}
		return;
	}
	// main sentence ending in preposition, with Qword at front, qword becomes OBJECT2--- Eg. who are you afraid of -- but not what dog are you afraid of
	else if (roleIndex == 1 && directObject && posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT) && posValues[lastRealWord] == PREPOSITION  && canonicalLower[firstRealWord] && canonicalLower[firstRealWord]->properties & QWORD )
	{
		SetRole(i,MAINSUBJECT);
		SetRole(firstRealWord,MAINOBJECT);
		directObject = 0;
	}
	// sentence starting with qword determiner (which and what for example) on this noun  "what fruit do you like"
	else if (roleIndex == 1 && posValues[firstRealWord] & DETERMINER && canonicalLower[firstRealWord]->properties & QWORD )
	{
		SetRole(i,MAINSUBJECT);
		SetRole(2,MAINOBJECT);
		needRoles[roleIndex] &= -1 ^ (OBJECT_COMPLEMENT|MAININDIRECTOBJECT); // in addition to ones cleared by setrole
		if (posValues[i] & PRONOUN_OBJECT)  ForceValues(i,PRONOUN_SUBJECT,"Forcing subject pronoun",changed);
		directObject = 0;
	}
	// sentence starting with qword, have bound subject on it, seeing noun (not in clause or phrase or verbal) but havent seen MAINVERB yet
	else if (roleIndex == 1 && canonicalLower[firstRealWord] && canonicalLower[firstRealWord]->properties & QWORD  && needRoles[roleIndex] & MAINVERB)  // what are *you doing
	{
		needRoles[MAINLEVEL] |= MAINSUBJECT|MAINOBJECT;	// enable role revision
		SetRole(firstRealWord,MAINOBJECT);
		SetRole(i,MAINSUBJECT);
		if (posValues[i] & PRONOUN_OBJECT)  ForceValues(i,PRONOUN_SUBJECT,"Forcing subject pronoun",changed);
		directObject = 0;
	}
	else if (indirectObject )
	{
		if (posValues[i] & PRONOUN_SUBJECT) ForceValues(i,PRONOUN_OBJECT,"Forcing object pronoun for indirect object",changed);

		bool allowedIndirect = false;
		// can be indirect ONLY if next thing is known a noun indicator (including gerund)
		// except for causal verbs expecting To Infinitive
		if (parseFlags[verbStack[roleIndex]] & CAUSAL_TOINFINITIVE_VERB || canSysFlags[verbStack[roleIndex]] & DIRECTOBJECT_INFINITIVE_COMPLEMENT)
		{
			allowedIndirect = true; // causal verb can be in any tense--- "he painted his jalopy black"
		}
		else if (bitCounts[i+1] == 1 && posValues[i+1] & (NOUN_BITS|DETERMINER|ADJECTIVE_BITS|PREDETERMINER|PRONOUN_BITS|TO_INFINITIVE))
		{
			if (posValues[i+1] & ADJECTIVE_BITS && (i+1) == lastRealWord) {;} // if sentence ends in adjective, this is not indirect obj - eg "This made *Jen mad"
			else allowedIndirect = true;
		}
		if (allowedIndirect)
		{
			SetRole(i,indirectObject);
			if (verbStack[roleIndex]) designatedObjects[verbStack[roleIndex]] = (unsigned char)i; // dont disable geenric, in case we find object also
			directObject = 0;
		}
		else
		{
			indirectObject = 0;
			goto retry;
		}
	}
	else if (directObject )
	{
		// if this is NOUN and verb was factitive, revise to not have indirect object
		if (parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB && indirectObjectStack[roleIndex] && posValues[i] & NOUN_BITS)  
		{
			SetRole(indirectObjectStack[roleIndex],(roleIndex == 1) ? MAINOBJECT : OBJECT2);
			SetRole(i,OBJECT_COMPLEMENT);
		}
		// singular direct object must be determined or possessed or mass or Proper -- "I walked my dog" not "I walked dog"
		else if (roleIndex == 1 && (currentZone - predicateZone ) == 1 && !roles[i]) // illegal for object to be in adjacent zone unless it was a propogated already role from Coord conjunct - "tom sent email, cards, and letters"
		{
		}
		else
		{
			if (posValues[i] & PRONOUN_SUBJECT) ForceValues(i,PRONOUN_OBJECT,"Forcing object pronoun",changed);
			SetRole(i,directObject);
			needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2|MAININDIRECTOBJECT) ;
			if (verbStack[roleIndex]) designatedObjects[verbStack[roleIndex]] = (unsigned char)i; // dont disable geenric, in case we find object also
			if (needRoles[roleIndex] & VERBAL) 
			{
				ExtendChunk(lastVerbal,i,verbals);
				--roleIndex;
			}
			if (needRoles[roleIndex] & CLAUSE) 
			{
				ExtendChunk(lastClause,i,clauses);
				--roleIndex;
			}
			if (needRoles[roleIndex] & PHRASE)
			{
				ExtendChunk(lastPhrase,i,phrases);
				--roleIndex;
			}
		}
	}		
	else if (roleIndex == 1 && needRoles[roleIndex] == 0 && IsLegalAddress(i) && !coordinates[i]) SetRole(i,ADDRESS);
	// simple superfluous noun immediately follows simple noun used as object, presume he was adjective_noun
	else if (!subject && !directObject && roles[i-1] == OBJECT2 && posValues[i-1] & NOUN_SINGULAR && posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL))
	{
		roles[i] = OBJECT2; // direct object assign, no level associated any more probably
		roles[i-1] = 0;
		verbals[i] = verbals[i-1];
		phrases[i] = phrases[i-1];
		clauses[i] = clauses[i-1];
		LimitValues(i-1,ADJECTIVE_NOUN,"superfluous noun forcing prior noun to adjective status",changed);
	}
	// appositives will be next noun (not being used as adjective)
	else if (objectStack[roleIndex] && objectStack[roleIndex] < i) // object happens earlier 
	{
		int base = i;
		while (--base > objectStack[roleIndex]) if (posValues[base] & CONJUNCTION_COORDINATE) break;
		if (posValues[base] & CONJUNCTION_COORDINATE) // later must match earlier
		{
			unsigned int after = coordinates[base];
			unsigned int before = coordinates[after];
			SetRole(i,roles[before]);
		}
		else if (needRoles[roleIndex] & OBJECT_COMPLEMENT && verbStack[roleIndex] && objectStack[roleIndex]) SetRole(i,OBJECT_COMPLEMENT);
		else if (base == objectStack[roleIndex] && !(posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE))) SetRole(i,APPOSITIVE); // or form of address
		else  SetRole(i,OBJECT2); // unknown
	}
	else if (subjectStack[roleIndex] && subjectStack[roleIndex] < i) // subject happens earlier 
	{
		if (needRoles[roleIndex] == ( CLAUSE | VERB2)) // clause has subject, was it qword?
		{
			// change subject to object and us be subject
			if (originalLower[subjectStack[roleIndex]] && originalLower[subjectStack[roleIndex]]->properties & QWORD && roles[subjectStack[roleIndex]] & SUBJECT2)
			{
				SetRole(subjectStack[roleIndex],OBJECT2);
				SetRole(i,SUBJECT2);
				return;
			}
		}

		int base = i;
		while (--base > subjectStack[roleIndex]) if (posValues[base] & CONJUNCTION_COORDINATE) break; 
		if ( verbStack[roleIndex] && subjectStack[roleIndex] < verbStack[roleIndex] && i > verbStack[roleIndex]) SetRole(i,OBJECT2); // handles "the man was eager to *die" - adjective_object can take a complement sometimes
		else if (base == subjectStack[roleIndex] && IsLegalAppositive(base,i)) 
		{
			// if the word before is ALSO a noun appositive, convert it to an adjective noun
			if (roles[i-1] == APPOSITIVE && posValues[i-1] & NOUN_BITS)
			{
				roles[i-1] = 0;
				posValues[i-1] = ADJECTIVE_NOUN;
				bitCounts[i-1] = 1;
				changed = true;
				if (trace & POS_TRACE) Log(STDUSERLOG,"Forcing prior noun appositive to be adjective noun %s\r\n",wordStarts[i-1]);
			}
			SetRole(i,APPOSITIVE); 
		}
		else if (!roles[i])  SetRole(i,SUBJECT2); // unknown
		// or form of address
	}
	 // presuming it is appositive after noun :  "I hit my friend *George"
}

static void ShowZone(unsigned int zone)
{
	if (zoneData[zone] & ZONE_AMBIGUOUS) Log(STDUSERLOG,"Ambiguous zone, ");
	else if (zoneData[zone] & ZONE_ABSOLUTE) Log(STDUSERLOG,"Absolute zone, ");
	else if ((zoneData[zone] & (ZONE_SUBJECT|ZONE_VERB)) == (ZONE_SUBJECT|ZONE_VERB)) 
	{
		if (zoneData[zone] & ZONE_FULLVERB) Log(STDUSERLOG,"subject/verb zone, ");
		else Log(STDUSERLOG,"Appositive zone, ");
	}
	else if ((zoneData[zone] & ZONE_VERB)) Log(STDUSERLOG,"Predicate zone, ");
	else if (zoneData[zone] & ZONE_SUBJECT && !(zoneData[zone] & ZONE_VERB)) 
	{
		if (zoneData[zone] & ZONE_ADDRESS) Log(STDUSERLOG,"Address zone, ");
		else Log(STDUSERLOG,"NounPhrase zone, "); // might be subject or object with appositive or something else after it
	}
	else if (zoneData[zone] == ZONE_PCV) Log(STDUSERLOG,"Phrases zone, ");
	else Log(STDUSERLOG,"??? zone,");
}

#ifdef INFORMATION
A needRoles value indicates what we are currently working on (nested), what we want to find to complete this role.
Base level is always the main sentence. Initially subject and verb. When we get a verb, we may add on looking for object and indirect object.
	Other levels may be:
1. phrase (from start of phrase which is usually a preposition but might be omitted) til noun ending phrase
but-- noun might be gerund in which case it extends to find objects of that.
2. clause (from start of clause which might be omitted til verb ending clause
but -- might have objects which extend it, might have gerunds which extend it (and it will include prep phrases attached at the end of it - presumed attached to ending noun)
3. verbal (infinitive and gerund and adjective_participle) - adds a level to include potential objects. 

The MarkPrep handles finding and marking basic phrases and clauses. AssignRoles extends those when the system "might" find an object.
It will backpatch later if it finds it swalllowed a noun into the wrong use.
#endif

static bool ValidateSentence(bool &resolved)
{
	wordStarts[0] = NULL;

	// now verify sentence is proper   
	// 0. all words are resolved and there is a main verb
	// 1. all nouns and pronouns and verbs have roles. Any object2 role noun must be a part of a phrase or clause or verbal (not free)
	// 2. predeterminer determiner or adjective must find its closing noun -- but adjective MAY use linking verb to do so
	// 3. preposition must find its closing noun
	// 4. clause has both subject and verb
	// 5. currently no constraints on adverbs or particles (though they go with a verb)
	// 6. currently dont require predeterminer to find a determiner
	// main object cannot preceed main verb unless in 1st position?
	// if it has no subject, the verb is infinitive.
	// main verb is present/past if no aux, is infinitive or participle if has aux
	unsigned int subject = subjectStack[MAINLEVEL];
	unsigned int verb = verbStack[MAINLEVEL];
	unsigned int object = objectStack[MAINLEVEL];
	unsigned int aux = auxVerbStack[MAINLEVEL];

	bool isQuestion =  ((tokenFlags & QUESTIONMARK) || (allOriginalWordBits[firstRealWord] & QWORD) || (allOriginalWordBits[firstRealWord+1] & QWORD && allOriginalWordBits[firstRealWord] & PREPOSITION)) ? true : false;

	if (roleIndex > 1) 
	{
		resolved = false; 
		if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: roleIndex > 1\r\n");
	}
	else if (!isQuestion && object && object < verb && object != 1)
	{
		resolved = false; // some unfinished business
		if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: object preceeds verb and not 1st or question r\n");
	}
	// chekc main verb  vs aux
	else if (!verb) // main verb doesnt exist
	{
		resolved = false; // some unfinished business
		if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: no main verb\r\n");
	}
	if (auxVerbStack[MAINLEVEL] && resolved) // aux exists, prove main verb matches
	{ // legal: "have you had your breakfast"  -- but so is "you are having your breakfast"
		if (posValues[verb] & (VERB_PRESENT|VERB_PRESENT_3PS))
		{
			resolved = false; // some unfinished business
			if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: main verb %s has aux but not aux tense\r\n",wordStarts[verb]);
		}
	}
	else if (resolved)// no aux exists, prove main verb matches
	{
		if (posValues[verb] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS)){;}
		else if (posValues[verb] == VERB_INFINITIVE && !subjectStack[MAINLEVEL]) {;} // command
		else if (posValues[verb] == VERB_INFINITIVE && aux == 1) {;} // helped verb
		else if (roles[verb] == VERB2) {;} // allowable infinitive
		else
		{
			resolved = false; // some unfinished business
			if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: main verb %s has aux tense but no aux\r\n",wordStarts[verb]);
		}
	}
	// prove subject/verb match
	if (!subject)
	{
		if (resolved && posValues[verb] != VERB_INFINITIVE)
		{
			resolved = false; // some unfinished business
			if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: main verb lacks subject and is not infinitive\r\n");
		}
		else if (resolved && posValues[verb] == VERB_PRESENT  && posValues[subject] & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR))
		{
			resolved = false; // some unfinished business
			if (trace & POS_TRACE  || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: main verb is plural and subject is singulare\r\n");
		}
		else if (resolved && posValues[verb] == VERB_PRESENT_3PS && posValues[subject] & (NOUN_PLURAL|NOUN_PROPER_PLURAL) )
		{
			resolved = false; // some unfinished business
			if (trace & POS_TRACE  || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: main verb is singular and subject is plural\r\n");
		}
	}
	// prove subject is not in zone immediately before verb zone
	if (resolved && subject && verb && (zoneMember[verb] - zoneMember[subject]) == 1 && !clauses[subject]) 
	{
		resolved = false; // some unfinished business
		if (trace & POS_TRACE  || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: subject in zone immediately before verb\r\n");
	}
	// prove object is not in zone immediately after verb zone
	if (resolved && object && verb && (zoneMember[object] - zoneMember[verb]) == 1 )
	{
		resolved = false; // some unfinished business
		if (trace & POS_TRACE  || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: object in zone immediately after verb\r\n");
	}

	int adjective = 0;
	int preposition = 0;
	bool clauseSubject = false;
	bool clauseVerb = false;
	unsigned int currentClause = 0;
	unsigned int i = 0;
	if (resolved) for (i = firstRealWord; i <= lastRealWord+1; ++i) // walk off end of sentence
	{
		if (quotedWords[i]) continue;

		char* word = wordStarts[i]; // for debugging
		// object 2 can occur at start of sentence after ending prep "who did you give it to"
		if (roles[i] == OBJECT2 && !clauses[i] && !verbals[i] && !phrases[i]) 
		{
			if (posValues[lastRealWord] == PREPOSITION && i == firstnoun) 
			{
				for (unsigned int x = 1; x <= firstnoun; ++x) phrases[x] = phrases[lastRealWord];
			}
			else
			{
				if (trace & POS_TRACE  || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse:  %s OBJECT2 not in clause/verbal/phrase\r\n",wordStarts[i]);
				break;	// free floating noun?
			}
		}
		// confirm clause has subject and verb
		if (!clauses[i] && currentClause) // clause is ending
		{
			currentClause = 0;
			if (clauseVerb) // we are allowed to omit a clause subject
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: ending a clause not fulfilled\r\n");
				break;	// didnt fulfill these
			}
		}
		else if (clauses[i] != clauses[i-1]) // clause is starting
		{
			if (currentClause && (clauseVerb))// we are allowed to omit a clause subject
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: changing from a clause not fulfilled\r\n");
				break; // failed to complete earlier clause
			}
			clauseSubject = clauseVerb = true;
			currentClause = clauses[i];
		}
		if (clauseSubject && roles[i] & SUBJECT2) clauseSubject = 0;
		if (clauseVerb && roles[i] & VERB2)  clauseVerb = 0;

		if (preposition && roles[i] & OBJECT2) preposition = 0;

		if (posValues[i] & ADJECTIVE_PARTICIPLE) {;} // doesnt require a noun (comes after or before or with be verb)
		else if (posValues[i] & (ADJECTIVE_BITS | DETERMINER | PREDETERMINER | POSSESSIVE | PRONOUN_POSSESSIVE))
		{
			if (!(posValues[i] & ADJECTIVE_BITS) || roles[i] != SUBJECT_COMPLEMENT) adjective = i; // unfinished adjectives are not these
		}
		else if (posValues[i] & PREPOSITION)
		{
			if (adjective || preposition) // adjective or preposition requires NOUN closure
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: adj or prep not fulfilled on seeing prep %s\r\n",wordStarts[i]);
				break;
			}
			preposition = i;
		}
		else if (posValues[i] & VERB_TENSES)
		{
			if (adjective || preposition) // adjective or preposition requires NOUN closure
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) 
				{
					if (adjective) Log(STDUSERLOG,"badparse: adj %s not fulfilled on seeing verb %s\r\n",wordStarts[adjective],wordStarts[i]);
					else  Log(STDUSERLOG,"badparse: prep %s not fulfilled on seeing verb %s\r\n",wordStarts[preposition],wordStarts[i]);
				}
				break;
			}
			if (!roles[i] && !verbals[i]) // if verbal, might be adverb verbal
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: verb with no known role\r\n");
				break;	// we dont know what it is doing
			}
			if (roles[i] == MAINVERB && !subjectStack[MAINLEVEL] && posValues[i] != VERB_INFINITIVE)// imperative failure
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: non-imperative main verb %s has no subject \r\n,wordStarts[i]");
				break;
			}
		}
		else if (posValues[i] & NOUN_INFINITIVE)
		{
			if (adjective) // cannot describe a noun infinitive
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: unfinished adjective at noun infinitive %s\r\n",wordStarts[i]);
				break;
			}
			// its marked on to infinitve
		}
		else if (posValues[i] & TO_INFINITIVE)
		{
			if (!roles[i]) // we dont know what it is doing, must be an adverb phrase
			{
			}
		}
		else if (posValues[i] & NOUN_BITS)
		{
			adjective = false;
			if (!roles[i]) 	// we dont know what it is doing
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: unknown role for noun %s\r\n",wordStarts[i]);
				break;
			}
		}
		else if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT))
		{
			if (adjective) // adjective requires NOUN closure
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: unfinished adjective at pronoun %s\r\n",wordStarts[i]);
				break;
			}
			if (!roles[i]) 	// we dont know what it is doing
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse: unknown role for pronoun %s\r\n",wordStarts[i]);
				break;
			}
		}
		else if (posValues[i] & CONJUNCTION_SUBORDINATE)
		{
			if (adjective || preposition) // adjective or prep requires NOUN closure
			{
				if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) 
				{
					if (adjective) Log(STDUSERLOG,"badparse: adj %s not fulfilled on seeing verb %s\r\n",wordStarts[adjective],wordStarts[i]);
					else  Log(STDUSERLOG,"badparse: prep %s not fulfilled on seeing verb %s\r\n",wordStarts[preposition],wordStarts[i]);
				}
				break;
			}
		}
		else if (i > lastRealWord) //  end of sentence
		{
			if (preposition && phrases[lastRealWord] == phrases[firstRealWord]) {;} // wraps back to front
			else if (preposition || clauseVerb) // adjective or preposition or clause requires  closure - we can omit clause subjects
			{
				if (posValues[lastRealWord] == PREPOSITION && roles[firstRealWord] == OBJECT2){;} // wrapped prep to start as question
				else
				{
					if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse:  unfinished prep or clause at end of sentence\r\n");
					break; 
				}
			}
			if (adjective && verbStack[MAINLEVEL]) // allowed to hang only if in main be sentence with linking verb or as object complement "This made mom mad"
			{
				if (canonicalLower[verbStack[MAINLEVEL]] && !(canonicalLower[verbStack[MAINLEVEL]]->systemFlags & LINKING_VERB) && roles[adjective-1] != MAINOBJECT) 
				{
					if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) Log(STDUSERLOG,"badparse:  unfinished adjective at end of sentence\r\n");
					break; // not a be sentence -- bug need to do seem as well
				}
			}		
		}
		if (roles[i] == SUBJECT2 && !clauses[i])
		{
			if (trace & POS_TRACE || (prepareMode && prepareMode != POSVERIFY_MODE)) 
			{
				Log(STDUSERLOG,"badparse:  Subject2 %s not in clause\r\n",wordStarts[i]);
				break; 
			}
		}
		// CONSIDER ALSO COORDINATE CONJUNCTION IF SENTENCE COORDINATOR
		//else if (posValues[i] && (ADVERB_BITS | PARTICLES | COMMA | PAREN | NOUN_INFINITIVE | THERE_EXISTENTIAL |  TO_INFINITIVE));
	}
	if (i <= (lastRealWord + 1)) resolved = false; // failed something along the way

	// assign sentence type
	if (!verbStack[MAINLEVEL] || roles[verbStack[MAINLEVEL]] !=  MAINVERB) // FOUND no verb, not a sentence
	{
		if (trace & POS_TRACE || prepareMode == POS_MODE) Log(STDUSERLOG,"Not a sentence\r\n");
		if (tokenFlags & (QUESTIONMARK|EXCLAMATIONMARK)) {;}
		else if (posValues[firstRealWord] & AUX_VERB_BITS) tokenFlags |= QUESTIONMARK;// its a question because AUX starts
		else if (allOriginalWordBits[firstRealWord]  & QWORD) tokenFlags |= QUESTIONMARK;
		else if (allOriginalWordBits[firstRealWord] & PREPOSITION && allOriginalWordBits[firstRealWord+1] & QWORD) tokenFlags |= QUESTIONMARK;
	}
	else if (!(tokenFlags & (QUESTIONMARK|EXCLAMATIONMARK))) // see if question or statement if we dont know
	{
		unsigned int i;
		bool subjectFound = false;
		for (i = firstRealWord; i <= lastRealWord; ++i) 
		{
			if (roles[i] & MAINSUBJECT) subjectFound = true;
		}
		if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] > verbStack[MAINLEVEL] && (allOriginalWordBits[firstRealWord] & QWORD || (allOriginalWordBits[firstRealWord] & PREPOSITION && allOriginalWordBits[firstRealWord+1] & QWORD)))  tokenFlags |= QUESTIONMARK; // qword + flipped order must be question (vs emphasis?)
	
		for (i = firstRealWord; i <= lastRealWord; ++i) 
		{
			if (quotedWords[i]) continue;
			if (roles[i] & MAINSUBJECT) 
			{
				if (i == 1 && originalLower[firstRealWord] && originalLower[firstRealWord]->properties & QWORD) tokenFlags |= QUESTIONMARK;
				break;
			}
			if (phrases[i] || clauses[i] || verbals[i]) continue;
			if ( bitCounts[i] != 1) break;	// all bets about structure are now off
			if (posValues[i] & AUX_VERB_BITS && (i == 1 || subjectFound)) // its a question because AUX or VERB comes before MAINSUBJECT
			{
				tokenFlags |= QUESTIONMARK;
				break;
			}
		}
	}

	return resolved;
}

static void AssignZones()
{
	if (trace & POS_TRACE) Log(STDUSERLOG,"     Zones: ");
	memset(zoneData,0,sizeof(unsigned int) * ZONE_LIMIT); 
	currentZone = 0;
	zoneIndex = 0;
	zoneSubject[zoneIndex] = 0;
	zoneVerb[zoneIndex] = 0;
	for (unsigned int i = firstRealWord; i <= (int)lastRealWord; ++i)
	{
		zoneMember[i] = (unsigned char)zoneIndex; // word is in current zone
		if (phrases[i] || clauses[i] || verbals[i]) zoneData[zoneIndex] |= ZONE_PCV; // a phrase/clause/verbal runs thru here -- and has been marked all the way thru the noun as well
		if (bitCounts[i] != 1) zoneData[zoneIndex] |= ZONE_AMBIGUOUS;
		else if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT|FULL_NOUN_BITS)) // this is definitely a noun
		{
			zoneData[zoneIndex] |=  (zoneData[zoneIndex] & ZONE_VERB) ? ZONE_OBJECT : ZONE_SUBJECT;
			// the only way to know an address zone is NOT the subject, is to find the subject in a different zone in advance (presumably next)
			// and zone 1 wouldnt be appositive in that case:  "Bill, my parrot is good" because a proper name wouldnt be appositive coming first
			if ( i == 1 && posValues[i] & NOUN_PROPER_SINGULAR && posValues[i+1] == COMMA && zoneData[zoneIndex+1] & ZONE_SUBJECT) 
				zoneData[zoneIndex] |= ZONE_ADDRESS;
			if ( i == 1 && posValues[i] & NOUN_PROPER_SINGULAR && posValues[i+1] == COMMA && posValues[i+2] & (PRONOUN_SUBJECT | PRONOUN_OBJECT |NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) 
				zoneData[zoneIndex] |= ZONE_ADDRESS;
			if (!zoneSubject[zoneIndex]) zoneSubject[zoneIndex] = (unsigned char)i; // 1st noun in zone
		}
		else if (posValues[i] & VERB_TENSES) // this is definitely a verb but might be from a clause
		{
			zoneData[zoneIndex] |= ZONE_VERB;
			if (posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PRESENT_PARTICIPLE))
			{
				if (zoneData[zoneIndex] & ZONE_SUBJECT && !(zoneData[zoneIndex] & ZONE_AUX) ){;}
				else if (zoneData[zoneIndex] & ZONE_AUX) predicateZone = zoneIndex;
			}
			else 
			{
				zoneData[zoneIndex] |= ZONE_FULLVERB;
			}
			zoneVerb[zoneIndex] = (unsigned char)i;
		}
		else if (posValues[i] & AUX_VERB_BITS) zoneData[zoneIndex] |= ZONE_FULLVERB | ZONE_AUX; 
		else if (posValues[i] & COMMA) 
		{
			// too many commas
			if (zoneIndex == (ZONE_LIMIT-1)) return;
			zoneData[++zoneIndex] = 0;
			zoneBoundary[zoneIndex] = (unsigned char)i;
			zoneSubject[zoneIndex] = 0;
			zoneVerb[zoneIndex] = 0;
		}
	}
	if (trace & POS_TRACE) 
	{
		for (unsigned int i = 0; i < zoneIndex; ++i) ShowZone(i);
		Log(STDUSERLOG,"\r\n");
	}
	zoneBoundary[++zoneIndex] = (unsigned char)(lastRealWord + 1);	// end of last current zone
}

static unsigned int GuessAmbiguousNoun(unsigned int i, bool &changed)
{
	// we arent expecting anything, so kill off verb meanings
	if (posValues[i] & FULL_NOUN_BITS && posValues[i] & (ADVERB_BITS|ADJECTIVE_BITS|PARTICLE) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0)
	{
		LimitValues(i,-1 ^ FULL_NOUN_BITS,"needing nothing, kill off noun meanings in favor of adverb/adj/particle",changed);
		return GUESS_RETRY;
	}

	if (posValues[i] & NOUN_INFINITIVE && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && i == lastRealWord)
	{
		LimitValues(i,-1 ^ NOUN_INFINITIVE,"no use for noun infinitive at end of sentence",changed); // "i want to take the day off"
		return GUESS_RETRY;
	}


	// *talk about stuff
	if (needRoles[roleIndex] & (MAINSUBJECT|OBJECT2|MAINOBJECT|SUBJECT2) &&
		posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND))
	{
		if (i == 1 && posValues[i] & VERB_INFINITIVE)
		{
			// prove there is no other verb in sight
			unsigned int at;
			for (at = i+1; at < lastRealWord; ++at)
			{
				if (posValues[at] == CONJUNCTION_COORDINATE || posValues[at] == CONJUNCTION_SUBORDINATE) at = lastRealWord; // abort
				else if (posValues[at] & (AUX_VERB_BITS|VERB_TENSES)) break; // potential verb, maybe we are noun
			}
			if (at >= lastRealWord) LimitValues(i,VERB_INFINITIVE,"Potential command at start has object after it",changed); // "let him go"
		}
	}

	if (needRoles[roleIndex] & (MAINSUBJECT|OBJECT2|MAINOBJECT|SUBJECT2) &&
		posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND))
	{
		if (i == 1 && posValues[i] & VERB_INFINITIVE && posValues[i+1] & (PRONOUN_BITS|DETERMINER|ADJECTIVE_BITS|NOUN))
		{
			 LimitValues(i,VERB_INFINITIVE,"Potential command at start has object after it",changed); // "let him go"
		}
		else if (posValues[i] & NOUN_INFINITIVE && verbStack[roleIndex] &&  
			(parseFlags[verbStack[roleIndex]] & CAUSAL_TOINFINITIVE_VERB || canSysFlags[verbStack[roleIndex]] & DIRECTOBJECT_INFINITIVE_COMPLEMENT))  LimitValues(i,NOUN_INFINITIVE,"Need a noun and verb expects infinitives, takethat",changed);
		else if (posValues[i] & NOUN_SINGULAR && posValues[i-1] & (ADJECTIVE_BITS|DETERMINER|POSSESSIVE_BITS)) LimitValues(i,NOUN_SINGULAR,"Need a noun, take simple noun with extras",changed);
		else LimitValues(i,(NOUN_INFINITIVE|NOUN_GERUND),"Need a noun, take potential gerund or infintiive",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if we are not looking for anything at level 1 (had object) but we could be a noun or verb and prior was a noun direct object, presume he should be adjectival noun "he ate the carrot *cake"
	if (roleIndex == 1 && needRoles[MAINLEVEL] == 0 && posValues[i-1] & NOUN_SINGULAR && roles[i-1] == MAINOBJECT && posValues[i] & NOUN_SINGULAR && posValues[i] & VERB_TENSES)
	{
		if (allOriginalWordBits[i-1] & ADJECTIVE_NORMAL)
		{
			LimitValues(i-1,ADJECTIVE_NORMAL,"convert prior presumed noun to adjective that it could have been",changed);
			if (roles[i-1]) SetRole(i-1,0);
		}
		else SetRole(i-1,ADJECTIVE_NOUN);
		needRoles[MAINLEVEL] |= MAINOBJECT;
		LimitValues(i,NOUN_SINGULAR,"unneeded verb that could be noun direct object extended, take that",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if something could be noun or verb, and  we are not looking for a verb yet, be a noun.
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[i] & VERB_TENSES && roleIndex == 1 && !(needRoles[MAINLEVEL] & MAINVERB))
	{
		LimitValues(i,-1 ^ VERB_TENSES,"in noun/verb conflict discarding verb when none is sought",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// )B) if something could be noun followed by potential prep and we need a noun, take it - but "our old dog Gizmo dreamed" should not
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[i+1] & PREPOSITION && needRoles[roleIndex] & (SUBJECT2|OBJECT2|MAINSUBJECT|MAINOBJECT))
	{
		LimitValues(i,NOUN_BITS,"something looking like noun followed by potential prep, make it a noun when we need one",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	// if something could be noun and next could be verb and we are hunting for both, be noun. Unless we have obvious verb coming up
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[i+1] & (VERB_TENSES|AUX_VERB_BITS) && needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT) && needRoles[roleIndex] & (MAINVERB|VERB2))
	{
		unsigned int at = i;
		while (++at <= lastRealWord && posValues[at] & !(VERB_TENSES|AUX_VERB_BITS))
		{
			if (posValues[at] & (COMMA | PREPOSITION | CONJUNCTION_SUBORDINATE | CONJUNCTION_COORDINATE)) break;
		}
		if (!(posValues[at] & (VERB_TENSES|AUX_VERB_BITS)) || bitCounts[at] != 1)
		{
			LimitValues(i,NOUN_BITS,"potential noun followed by potential verb/aux and we need both, take it as noun",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY;
		}
	}

	// if we want a main object and instead have ambigous non-headed noun - "I walked *home yesterday"  BUG- consider the impossiblity "I walked home the dog"
	if (posValues[i] & NOUN_SINGULAR && needRoles[roleIndex] & MAINOBJECT && roleIndex == 1 && !phrases[i])
	{
		LimitValues(i,NOUN_BITS,"forcing noun  since want direct object",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// a potential noun that needs a determiner will abandoned "Cat is where the heart is"  but time words are ok "today is yesterday"
	// but hard to tell so dont do restriction test

	// if noun can be singular or plural, PRESUME plural, unless verb following is 3ps 
	if (posValues[i] == (NOUN_SINGULAR|NOUN_PLURAL))
	{
		if (posValues[i] & VERB_PRESENT_3PS) LimitValues(i, NOUN_SINGULAR,"is singular over plural since verb next is 3ps",changed);
		else LimitValues(i, NOUN_PLURAL,"is plural over singular",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if we want a noun, be one
	if (posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL) && needRoles[roleIndex] & (OBJECT2 | MAINSUBJECT | MAINOBJECT))
	{
		// if our priority is not as noun but as adjective or adverb, and next could be noun or adjective, be adjective and defer - "the *little old dog hit the man"
		if (canSysFlags[i] & (ADJECTIVE|ADVERB) && !(canSysFlags[i] & NOUN))
		{
			if (posValues[i+1] & (NORMAL_NOUN_BITS | ADJECTIVE_BITS))
			{
				LimitValues(i, ADJECTIVE_BITS|ADVERB_BITS,"we are likely to describe a noun following still",changed);
				return GUESS_RETRY;
			}
		}

		// if looking for object and this could be particle, take it unless prep phrase...  "I like getting *up and working out" but not "I rolled on my *back"
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i] & PARTICLE && !(needRoles[roleIndex] & PHRASE))
		{
			LimitValues(i, PARTICLE,"noun vs particle as object, go for particle",changed);
			return GUESS_RETRY;
		}

		LimitValues(i, NOUN_PLURAL|NOUN_SINGULAR,"be a noun because we need one",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousVerb(unsigned int i, bool &changed)
{
	// potential verb infinitive but we dont need anything and are followed by conjunction
	if (posValues[i] & VERB_INFINITIVE && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && posValues[i+1] & CONJUNCTION_BITS)
	{
		LimitValues(i,-1 ^ VERB_INFINITIVE,"infinitive unneeded and followed by conjunction, discard",changed); // "let him go"
		return GUESS_RETRY;
	}

	// command
	if (i == 1 && posValues[i] & VERB_INFINITIVE && posValues[i+1] & (PRONOUN_BITS|DETERMINER|ADJECTIVE_BITS|NOUN))
	{
		LimitValues(i,VERB_INFINITIVE,"Potential command at start has object after it so use it",changed); // "let him go"
		return GUESS_RETRY;
	}

	// we arent expecting anything, so kill off verb meanings
	if (posValues[i] & VERB_TENSES && posValues[i] & (ADVERB_BITS|ADJECTIVE_BITS|PARTICLE) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0)
	{
		LimitValues(i,-1 ^ (VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST | VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE) ,"needing nothing, kill off verb meanings in favor of adverb/adj/particle except infinitive",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	// we arent expecting anything, keep verb infinitive (verbal) after noun or pronoun but must have something after it
	if (posValues[i] & VERB_INFINITIVE && posValues[i-1] & (PRONOUN_BITS|NOUN_BITS) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && i != lastRealWord && !(posValues[i+1] & CONJUNCTION_BITS))
	{
		LimitValues(i,VERB_INFINITIVE ,"needing nothing, keep potential verbal infinitive after noun/pronoun acting as postnominal adjective phrase",changed);
		return GUESS_RETRY;
	}

	// if something could be noun or verb infinitive, and we need a noun, use noun
	if (posValues[i] & NOUN_INFINITIVE && needRoles[roleIndex] & (MAINOBJECT|OBJECT2))
	{
		if (posValues[i] & NOUN_SINGULAR ) LimitValues(i,NOUN_SINGULAR,"conflict noun infinitive and singular, needing object- go noun singular",changed);
		else LimitValues(i,NOUN_INFINITIVE,"conflict noun infinitive, needing object- go noun",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if something at the end of the sentence could be a verb and we dont want one.... dont be  "He can speak Spanish but he can't write it very *well."
	if (posValues[i] & VERB_TENSES && i == (int)lastRealWord && !(needRoles[MAINLEVEL] & MAINVERB) && roleIndex == 1)
	{
		LimitValues(i,-1 ^ VERB_TENSES,"potential verb at end of sentence has no use",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if something might be verb, but AUX is later and no conjunction_subordinate exists, assume it cannot be a verb "*like what does she look"
	// BUT WE CANT TELL ABOUT CLAUSES EASILY

	// when cant tell the tense, pick past  "I *hit my friend"
	if (posValues[i] == (VERB_PRESENT|VERB_PAST))
	{
		LimitValues(i,VERB_PAST,"forcing PRESENT/PAST conflict on verb to past",changed);
		return GUESS_RETRY;
	}

	// do we need a verb next which is in conflict with a noun
	uint64 remainder = posValues[i] & (-1 ^ (NOUN_BITS|VERB_TENSES));
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_TENSES && posValues[i] & NOUN_BITS && !remainder && 
	!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)))
	{
		LimitValues(i,VERB_TENSES,"says verb required, binding tenses over noun",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
			
	// we need a verb- take whatever we can get - "the road following the edge of the frozen lake *died"
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_TENSES &&  !(posValues[i] & AUX_VERB_BITS) && !(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)))
	{
		// if we have no aux and this cannot be absolute because 1st is already that, drop past participle
		if (currentZone != 0 && posValues[i] & VERB_PAST_PARTICIPLE && zoneData[0] & ZONE_ABSOLUTE && !auxVerbStack[roleIndex]) 
			LimitValues(i,-1 ^ VERB_PAST_PARTICIPLE,"limiting to anything but participle",changed);
		if (auxVerbStack[roleIndex] && posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_INFINITIVE|VERB_PAST_PARTICIPLE)) 
			LimitValues(i,VERB_PRESENT_PARTICIPLE|VERB_INFINITIVE|VERB_PAST_PARTICIPLE,"limiting to participles and infinitives",changed);
		else if (posValues[i] & ( VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS )) 
			LimitValues(i, VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS,"limiting to verb tense",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// if this is a clausal verb w/o aux, then we must be normal...
	unsigned int startOf = zoneBoundary[currentZone] + 1; 
	if (clauses[startOf] && needRoles[roleIndex] & VERB2 && !auxVerbStack[roleIndex])
	{
		LimitValues(i,VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS,"says normal verb required for clause w/o aux",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// "arms *folded, we lept"
	// if we are 1st zone and follow a noun with no aux and predicate is in next zone,  we are participle absolute phrase. unless we are a clause
	if (currentZone == 0 && posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && predicateZone >= 0 && zoneData[predicateZone] & ZONE_SUBJECT &&
		zoneData[predicateZone] != ZONE_AMBIGUOUS && (zoneData[currentZone] & ZONE_SUBJECT) && 
		!(zoneData[currentZone] & ZONE_AUX))
	{
		LimitValues(i, VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE,"says VERB_PARTICIPLE required for absolute phrase",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	
	 // can we infer present tense from infinitive when we need a verb -- "do you *attend this school"
	 if (posValues[i] == (VERB_PRESENT|VERB_INFINITIVE) && needRoles[roleIndex] & (VERB2|MAINVERB) )
	 {
		 if (auxVerbStack[roleIndex]) LimitValues(i, VERB_INFINITIVE,"is present tense verb since aux",changed);
		 else LimitValues(i, VERB_PRESENT,"is present tense verb since no aux",changed);
		 if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	 }

	 // she *does her best work - in verb/aux conflict, assume verb, and when we find extra verb later, revert it to aux
	 if (posValues[i] & VERB_TENSES && posValues[i] & (AUX_VERB_TENSES|AUX_BE|AUX_DO|AUX_HAVE))
	 {
		LimitValues(i, -1 ^ (AUX_VERB_TENSES|AUX_BE|AUX_DO|AUX_HAVE),"presume aux is real, override later if extra verb",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	 }
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousAdjective(unsigned int i, bool &changed)
{
	if (posValues[i] == AMBIGUOUS_VERBAL)
	{
		// we need a noun, go for that, unless it is followed by a noun, in which case we be adjective particile
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2|MAINSUBJECT|SUBJECT2))
		{
			if (posValues[i+1] & (ADJECTIVE_BITS|NOUN_BITS) && !(posValues[i+1] & (-1 ^ (ADJECTIVE_BITS|NOUN_BITS))) ) LimitValues(i,ADJECTIVE_PARTICIPLE,"expect we describe following noun",changed); // can only be a noun
			else LimitValues(i,NOUN_GERUND,"need noun so accept noun-gerund",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; 
		}
		//  NOUN_GERUND ADJECTIVE_PARTICIPLE conflict after and which has verbal before it
		else if (i > 1 && posValues[i-1] & CONJUNCTION_COORDINATE && verbals[i-2])
		{
			LimitValues(i,NOUN_GERUND,"gerund/adjparticle conflict after coordinate with verbal before it, go verbal",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; 
		}
	}

	// if this is adjective or noun and following is a possible noun, be adjective
	if (posValues[i] & (ADJECTIVE_NORMAL|NOUN_SINGULAR) && posValues[i+1] & (NOUN_SINGULAR|NOUN_PLURAL))
	{
		// not a conflict if we need indirect and this is human
		if (canSysFlags[i] & ANIMATE_BEING && needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) // "he had the *mechanic check the brakes"
		{
			LimitValues(i,NOUN_SINGULAR ,"adj noun conflict needing indirect, use noun",changed);
			return GUESS_RETRY;
		}
		else
		{
			LimitValues(i,ADJECTIVE_NORMAL ,"adj noun conflict before potential normal noun, be adj",changed);
			return GUESS_RETRY;
		}
	}

	// ending adjective makes no sense
	if (posValues[i] & ADJECTIVE_NORMAL && i == lastRealWord && roleIndex == MAINLEVEL && !(needRoles[MAINLEVEL] & SUBJECT_COMPLEMENT))
	{
		LimitValues(i, -1 ^ ADJECTIVE_NORMAL ,"not ending sentence in adjective since not subject complement",changed);
		return GUESS_RETRY;
	}

	// if ambiguity is adjectvie/adverb, immediately after object/mainobject, go with adjective (assume its postnominal adjective even if verb didnt require it)
	if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB_NORMAL) && roles[i-1] & MAINOBJECT)
	{
		LimitValues(i, -1 ^ ADJECTIVE_NORMAL ,"probable postnominal adjective after main direct object",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if ambiguity is between adjective-noun and adjective_normal, select normal
	if (posValues[i] & ADJECTIVE_NOUN && posValues[i] & ADJECTIVE_NORMAL)
	{
		LimitValues(i, -1 ^ ADJECTIVE_NOUN ,"adjective-noun vs adjective-normal, be normal",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if likely adjective and seeking factitive adjective for object, accept it
	if (posValues[i] & ADJECTIVE_NORMAL && needRoles[roleIndex] & OBJECT_COMPLEMENT)
	{
		LimitValues(i, ADJECTIVE_NORMAL ,"factitive verb expects adjective, be that",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if ambiguity is Adjective_basic Adverb_basic and wanting adjective as object, be adjective if next is junk "Samson was powerful but foolish"
	if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB_BITS) && needRoles[roleIndex] & SUBJECT_COMPLEMENT && (posValues[i+1] & (CONJUNCTION_BITS|COMMA|PAREN) || i == lastRealWord))
	{
		LimitValues(i, ADJECTIVE_NORMAL ,"adjective-adjverb when expecting adjective and next is junk",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// (A) if we have a noun coming up and we can be adjectives to get there, do so if noun is probable - unless gerund
	if (posValues[i] & ADJECTIVE_BITS)
	{
		// "Doc made his students *read 4 books." - verb infinitve expected over participle being adjective
		if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(needRoles[roleIndex] & (MAINOBJECT|OBJECT2))
			&& canSysFlags[verbStack[roleIndex]] & DIRECTOBJECT_INFINITIVE_COMPLEMENT
			&& !(posValues[i-1] & TO_INFINITIVE))
		{
			LimitValues(i,VERB_INFINITIVE,"potential infinitive expected",changed);
			return GUESS_RETRY; 
		}

		int beAdj = 0;
		if (posValues[i+1] & NOUN_BITS) beAdj = i+1;
		else if (i < (lastRealWord - 1) && posValues[i+1] & ADJECTIVE_BITS && posValues[i+2] & NOUN_BITS) beAdj = i+2;
		else if (i < (lastRealWord - 2) && posValues[i+1] & ADJECTIVE_BITS && posValues[i+2] & ADJECTIVE_BITS && posValues[i+3]) beAdj = i+3;
		if (!beAdj){;}
		else if (bitCounts[beAdj] == 1) {;} // yes
		else if (canonicalLower[beAdj] && canonicalLower[beAdj]->systemFlags & NOUN) {;}
		else beAdj = 0;	// not a probable noun
		if (beAdj ) // can be an adjective, have upcoming noun
		{
			if (!(posValues[i] & NOUN_GERUND)) // cant be gerund
			{
				LimitValues(i,ADJECTIVE_BITS,"potential adj preceeding nearby noun, be adjective",changed);
			}
			// if verb cannot take an object, must be adjective   // "he hates *decaying broccoli"
			else if (!(canSysFlags[i]  & VERB_DIRECTOBJECT))
			{
				LimitValues(i,ADJECTIVE_BITS,"potential adj preceeding nearby noun, be adjective",changed);
			}
			// if verb needs an object, be verbal --  // "he hates *eating broccoli"
			else 
			{
				LimitValues(i,NOUN_GERUND,"potential gerund preceeding nearby noun as object, be that",changed);
			}

			if (bitCounts[i] == 1) return GUESS_RETRY;
		}
	}

	// if this is LIKELY an adjective not a noun and we are seeking a noun but could find it later, be an adjective
	if (originalLower[i] && originalLower[i]->systemFlags & ADJECTIVE && !(originalLower[i]->systemFlags & NOUN) && needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|OBJECT2|MAINOBJECT))
	{
		unsigned at = i;
		while (++at <= lastRealWord)
		{ 
			if (posValues[at] & (NOUN_BITS|PRONOUN_SUBJECT|PRONOUN_OBJECT))
			{ // "Juanita and Celso worked *hard and then rested."
				LimitValues(i,ADJECTIVE_BITS|ADVERB_BITS,"probable adj/adverb while needing a noun and one found later, be adj/adv",changed);
				if (bitCounts[i] == 1) return GUESS_RETRY;
				break;
			}
		}
	}

	// if this could be adjective or adverb and is in front of adjective, prefer adjective - "the *little old lady"
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i] & ADVERB_BITS && posValues[i+1] == ADJECTIVE_NORMAL)
	{
		LimitValues(i,-1 ^ ADVERB_BITS,"adverb or adj conflict in front of adjective, be adjective",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if we could be adjective participle having no noun in front of us, we dont want to be gerund if we havent come to subject zone yet
	// we want to describe subject in later zone
	if (posValues[i] & ADJECTIVE_PARTICIPLE && predicateZone >= 0 && zoneData[predicateZone] & ZONE_SUBJECT &&
		currentZone == (predicateZone - 1) && !(zoneData[predicateZone] & ZONE_AMBIGUOUS) && !(zoneData[currentZone] & ZONE_SUBJECT))
	{
		LimitValues(i, ADJECTIVE_PARTICIPLE,"says ADJECTIVE_PARTICIPLE required because subj/verb is in next zone",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		// could we be a noun appositive?  seems unlikely
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousAdverb(unsigned int i, bool &changed)
{
	// if word is sandwiched after a verb and before a conjunction, be adverb
	if (posValues[i-1] & (VERB_TENSES|NOUN_GERUND|NOUN_INFINITIVE) && posValues[i+1] & CONJUNCTION_BITS)
	{
		LimitValues(i, ADVERB_BITS," be adverb sandwiched between verb and conjunction",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if word is sandwiched after a verb and at sentence end
	if (posValues[i-1] & (VERB_TENSES|NOUN_GERUND|NOUN_INFINITIVE) && i == lastRealWord)
	{
		LimitValues(i, ADVERB_BITS," be adverb after verb at end",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if it might be particle or adverb, be particle - "the thought of getting up is bad"
	if (posValues[i] == (ADVERB_NORMAL|PARTICLE))
	{
		LimitValues(i, PARTICLE," be particle in particle-adverb conflict",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; 
	}

	// if a timeword could be an adverb, make it so (if we wanted a noun by now it would be one)  "right *now we will go"
	if (posValues[i] & ADVERB_BITS && originalLower[i] && originalLower[i]->systemFlags & TIMEWORD )
	{
		LimitValues(i, ADVERB_BITS," time word could be adverb, make it so",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	// if word after could be a timeword and we could be an adverb, make it so  "*right now we will go"
	if (posValues[i] & ADVERB_BITS && posValues[i+1] & ADVERB_BITS && originalLower[i+1] && originalLower[i+1]->systemFlags & TIMEWORD )
	{
		LimitValues(i, ADVERB_BITS," next could be time word and we could be adverb, make it so",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousPreposition(unsigned int i, bool &changed)
{
	// we could be particle, no longer considering noun or verb and next is not noun evidence, be particle
	if (posValues[i] & PARTICLE && !(posValues[i] & (VERB_TENSES|FULL_NOUN_BITS)) && !(posValues[i+1] & (ADJECTIVE_BITS|DETERMINER|NOUN_BITS)))
	{
		unsigned int priorVerb = i;
		while (--priorVerb && !(roles[priorVerb] & (VERB2|MAINVERB))){;}
		if (canSysFlags[priorVerb] & SEPARABLE_PHRASAL_VERB && priorVerb != (i-1) && !(posValues[i+1] & (ADJECTIVE_BITS|DETERMINER|PRONOUN_BITS|NOUN_BITS))) // not immediately after "you make a fuss *over me" and no noun after
		{
			LimitValues(i, PARTICLE," particle after particle verb",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// if we have PREPOSITION vs subord conjunct check for following subject/verb "I ate dinner *before I moved"
	if (posValues[i] & PREPOSITION && posValues[i] & CONJUNCTION_SUBORDINATE)
	{
		LimitValues(i,PREPOSITION,"becoming prep as simplest choice over conjunct",changed); // might not be real - may have to override later
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	
	// if we have PREPOSITION vs coord conjunct 
	if (posValues[i] & PREPOSITION && posValues[i] & CONJUNCTION_COORDINATE)
	{
		LimitValues(i,PREPOSITION,"becoming prep as simplest choice over conjunct",changed); // might not be real - may have to override later
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}


	// end is possible prep and start is object
	if (posValues[i] & PREPOSITION && firstnoun && roles[firstnoun] == OBJECT2)
	{
		LimitValues(i,PREPOSITION,"ending possible prep and have object at start, use prep",changed); // might not be real
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousPronoun(unsigned int i, bool &changed)
{
	// possissive vs object followed by clear verbal will be object "we heard *her sing" - but not "Bob pressed *his nose"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[i+1] & (NOUN_INFINITIVE|NOUN_GERUND) && !(posValues[i+1] & NOUN_SINGULAR))
	{
		LimitValues(i,PRONOUN_OBJECT,"resolving pronoun possessive vs object based on following verbal",changed); // might not be real
		return GUESS_RETRY;
	}
	// if possessive vs pronoun object, and followed by an adj or noun clearly not verbal, be possessive  -- "his new car"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[i+1] & (ADJECTIVE_BITS|NOUN_BITS) && !(posValues[i+1] & NOUN_SINGULAR))
	{
		LimitValues(i,PRONOUN_POSSESSIVE,"resolving pronoun possessive vs object based on following noun",changed); // might not be real
		return GUESS_RETRY;
	}
	// if possessive vs pronoun object, and followed by an adj or noun clearly not verbal, be possessive  -- "his new car"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[i+1] & (ADJECTIVE_BITS|NOUN_BITS) )
	{
		LimitValues(i,PRONOUN_POSSESSIVE,"resolving pronoun possessive vs object based on following noun",changed); // might not be real
		return GUESS_RETRY;
	}
	// pronoun possessive his and hers will never be indirect objects, and if in front of adj or noun will be possesive "*his new car"

	// possible determiner  "*what will is this"
	if (i == 1 && posValues[i] & PRONOUN_BITS && posValues[i] & DETERMINER && posValues[i+1] & NOUN_BITS  &&  posValues[i+1] & (VERB_TENSES & AUX_VERB_BITS) )
	{
		LimitValues(i,DETERMINER,"resolving pronoun/determiner to determiner since followed by potential noun and verb",changed); // might not be real
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if something could be a pronoun subject and we are looking for a subject, be it
	if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT) && needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2))
	{
		LimitValues(i,(PRONOUN_SUBJECT|PRONOUN_OBJECT),"needing a subject, force pronoun subject to be it",changed); // even though *some are left behind, so what.
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	
	// possible pronoun followed by possible aux and end is prep "*where do you come from" -- but "*what will is this"
	if (i == 1 && posValues[i] & PRONOUN_BITS && posValues[i+1] & AUX_VERB_BITS && bitCounts[i+1] == 1 && posValues[lastRealWord] == PREPOSITION)
	{
		LimitValues(i,PRONOUN_BITS,"requiring pronoun object since aux follows and prep ends sentence",changed); // might not be real
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// prior prep phrase has not ended with noun and we are becoming aux.... "from *where do you come"
	if (phrases[i-1] && !(posValues[i-1] & (NOUN_BITS|PRONOUN_SUBJECT|PRONOUN_OBJECT)) && posValues[i] & PRONOUN_BITS && posValues[i+1] & AUX_VERB_BITS && bitCounts[i+1] == 1)
	{
		LimitValues(i,PRONOUN_BITS,"requiring pronoun object since aux follows and prep ends sentence",changed); // might not be real
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// if we have pronoun subject and object, decide what we needed
	if (posValues[i] == (PRONOUN_SUBJECT|PRONOUN_OBJECT))
	{
		if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2) || roles[i] == SUBJECT2)
		{
			LimitValues(i,PRONOUN_SUBJECT," says subject pronoun needed",changed);
		}
		else if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2) || roles[i] == OBJECT2)
		{
			LimitValues(i,PRONOUN_OBJECT," says object pronoun needed",changed);
		}
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousAux(unsigned int i, bool &changed)
{
	// if this can be aux and later is verb only, be aux
	if (posValues[i] & AUX_VERB_BITS)
	{
		unsigned int at = i;
		while (++at <= lastRealWord)
		{
			if (posValues[at] & VERB_TENSES && !(posValues[at] & (-1 ^ VERB_TENSES))) break; // some kind of insured verb
		}
		if (at <= lastRealWord) 
		{
			LimitValues(i,AUX_VERB_BITS,"requiring aux since see verb later",changed); // might not be real
			if (bitCounts[i] == 1) return GUESS_RETRY;
		}
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousConjunction(unsigned int i, bool &changed)
{
	if (posValues[i] & ADVERB_BITS && !stricmp(wordStarts[i], "but") && posValues[i+1] & (NOUN_BITS|ADJECTIVE_BITS|PRONOUN_BITS|DETERMINER) && posValues[i-1] & (VERB_TENSES))  // "there is but one god." dont make conjunction
	{
		if (!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2)) && posValues[i] & PREPOSITION) // we cant accept a noun next
		{
			LimitValues(i,PREPOSITION,"defaulting adverb/prep/conjunction as prep like 'but me'",changed); // might not be real -
		}
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	if (posValues[i] & ADVERB_BITS && !stricmp(wordStarts[i], "but") && posValues[i+1] & ADJECTIVE_BITS)  // "there is but one god." dont make conjunction or prep
	{
		if (posValues[i+2] & NORMAL_NOUN_BITS) // need a noun after to make this modify an adjective modifying a noun- otherwise might be conjoined adjective_object
		{
			LimitValues(i,ADVERB_BITS,"defaulting adverb/prep/conjunction as adverb like 'but one god'",changed); 
			if (bitCounts[i] == 1) return GUESS_RETRY;
		}
	}
	if (posValues[i] & PREPOSITION && !stricmp(wordStarts[i], "but") && posValues[i+1] & (ADJECTIVE_BITS|PRONOUN_BITS|DETERMINER|POSSESSIVE))  // "all went but me." 
	{
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|INDIRECTOBJECT2);
		LimitValues(i,PREPOSITION,"defaulting adverb/prep/conjunction like 'but me'",changed); 
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}
	// if this can be conjunction, be one
	LimitValues(i,CONJUNCTION_BITS,"defaulting conjunction",changed); // might not be real - if cant find verb, may need to downgrade to preoposition if subject found
	if (bitCounts[i] == 1) return GUESS_RETRY;
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguous(unsigned int i, bool &changed)
{
	// can we INFER what things are
	unsigned int result;
	uint64 priority = (originalLower[i]) ? (originalLower[i]->systemFlags & POSTAG_PRIORITY) : 0;
	if (!priority) priority = canSysFlags[i]  & POSTAG_PRIORITY;
	uint64 bits = posValues[i];

	// prefer static priority to solve
	if (priority & ADJECTIVE && bits & ADJECTIVE_BITS) 
	{
		bits &= -1 ^ (ADJECTIVE_BITS|DETERMINER);
		result = GuessAmbiguousAdjective(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & ADVERB && bits & ADVERB_BITS) 
	{
		bits &= -1 ^ ADVERB_BITS;
		result = GuessAmbiguousAdverb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & PREPOSITION && bits & PREPOSITION) 
	{
		bits &= -1 ^ PREPOSITION;
		result = GuessAmbiguousPreposition(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & CONJUNCTION && bits & CONJUNCTION_BITS) 
	{
		bits &= -1 ^ CONJUNCTION;
		result = GuessAmbiguousConjunction(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & NOUN && bits & NOUN_BITS) 
	{
		bits &= -1 ^ FULL_NOUN_BITS;
		result = GuessAmbiguousNoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (priority & VERB && bits & VERB_TENSES) 
	{
		bits &= -1 ^ VERB_TENSES;
		result = GuessAmbiguousVerb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}

	// try things we havent tried if they are relevant
	if (bits & CONJUNCTION_BITS)
	{
		result = GuessAmbiguousConjunction(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & (ADJECTIVE_BITS|DETERMINER))
	{
		result = GuessAmbiguousAdjective(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & ADVERB_BITS)
	{
		result = GuessAmbiguousAdverb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & VERB_TENSES)
	{
		result = GuessAmbiguousVerb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & FULL_NOUN_BITS)
	{
		result = GuessAmbiguousNoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & PRONOUN_BITS)
	{
		result = GuessAmbiguousPronoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & AUX_VERB_BITS)
	{
		result = GuessAmbiguousAux(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & PREPOSITION)
	{
		result = GuessAmbiguousPreposition(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	
	// if we are expecting a noun (we saw a determiner or normal adjective), we can solidy adjectives and potential nouns along the way
	if (requireNoun && posValues[i] & (ADJECTIVE_BITS | NOUN_BITS | ADVERB_BITS))
	{
		LimitValues(i,-1 ^ (AUX_VERB_BITS|VERB_TENSES|PREPOSITION|CONJUNCTION_SUBORDINATE),"requiring noun so solidfy adjectives and nouns",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY;
	}

	// can we infer preposition since noun follows.. if not needing noun then yes
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & PREPOSITION) // between subject and verb
	{
		if (posValues[i+1] & ( NOUN_BITS | DETERMINER | ADJECTIVE | PRONOUN_POSSESSIVE) && bitCounts[i+1] == 1) // we know noun is coming
		{
			LimitValues(i,PREPOSITION,"says preposition required to contain upcoming noun",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	
	// this is an ambiguous verbal, we can know we have met its verb level and hunt for its objects
	if (posValues[i] == (NOUN_INFINITIVE | VERB_INFINITIVE) && needRoles[roleIndex] & VERB2)
	{
		needRoles[roleIndex] &= -1 ^ VERB2;
		SetRole(i,VERB2);
		SeekObjects(i);

		// if level below needs a noun, this can be that.. BUT maybe it would be a verb if describing instead???
		if (needRoles[roleIndex-1] & (MAINSUBJECT | SUBJECT2 | MAINOBJECT | OBJECT_COMPLEMENT | OBJECT2))
		{
			LimitValues(i, NOUN_INFINITIVE," is a noun infinitive because we want one below",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// can we infer noun? do we need a subject or an object -- cant use potential adjective/determiner/adverb cause it is legal
	if (needRoles[roleIndex] & (MAINSUBJECT | SUBJECT2 | MAINOBJECT  | OBJECT_COMPLEMENT | OBJECT2) && posValues[i] & NOUN_BITS) // can we conclude this must be a noun?
	{
		// not if it might be adverb and we are looking for an object (meaning coming after a verb)
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2) && posValues[i] & ADVERB_BITS) // "the men ate first."
		{
		}
		else if ( !(posValues[i] & (ADVERB_BITS|ADJECTIVE_BITS|DETERMINER)))
		{
			LimitValues(i, NOUN_BITS,"is a noun because we want one",changed);
			if (posValues[i] & NOUN_SINGULAR) LimitValues(i, NOUN_SINGULAR,"is a singularnoun because",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
		else if (posValues[i] & ADJECTIVE_BITS && !(posValues[i] & NOUN_BITS) && posValues[i+1] & NOUN_BITS) // john has *2 cats.
		{
			LimitValues(i, (ADJECTIVE_BITS - ADJECTIVE_NOUN)," is an adj because we want a noun and thing after it could be and we are not",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// can we infer verb vs aux verb in clause?
	uint64 remainder = posValues[i] & (-1 ^ (AUX_VERB_BITS|VERB_TENSES));
	if (needRoles[roleIndex] & VERB2 && posValues[i] & AUX_VERB_BITS && posValues[i] & VERB_TENSES && !remainder) // can we decide if this is aux or verb?
	{
		// to be aux, we need to find verb shortly. Skip over adverbs, block at pronouns,nouns (except in prep phrases)
		unsigned int x = i;
		while (++x <= lastRealWord)
		{
			if (posValues[x] & (NOUN_BITS|PRONOUN_SUBJECT|PRONOUN_OBJECT)) // failed to find a verb
			{
				LimitValues(i, VERB_TENSES,"is a verb because we found following noun/pronoun",changed);
				return GUESS_ABORT;
			}
			if (posValues[x] & VERB_TENSES) // found a verb. assume aux.
			{
				LimitValues(i, AUX_VERB_BITS,"is a aux not verb because we found verb",changed);
				return GUESS_ABORT;
			}
		}
	}

	// can we infer verb past vs adj participle
	remainder = posValues[i] & (-1 ^ (ADJECTIVE_PARTICIPLE|VERB_PAST));
	if (needRoles[roleIndex] & (MAINVERB|VERB2) && posValues[i] & ADJECTIVE_PARTICIPLE && posValues[i] & VERB_PAST && !remainder) // can we decide if this is adj part or verb past?
	{
		LimitValues(i, VERB_PAST," is a verb past not adj participle because we need it",changed);
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// can we infer adjective_participle vs verb_participle?   "it was a tomb *built by man"
	 remainder = posValues[i] & (-1 ^ (ADJECTIVE_PARTICIPLE|VERB_TENSES));
	 if (posValues[i] & ADJECTIVE_PARTICIPLE && posValues[i] & VERB_TENSES && !remainder) // can we decide if this is aux or verb?
	 {
		 // if BEFORE us is a noun and no one is looking for a verb, we should describe him...
		 if (posValues[i-1] & NOUN_BITS && bitCounts[i-1] == 1 && !(needRoles[roleIndex] & (MAINVERB|VERB2))) 
		 {
			LimitValues(i, ADJECTIVE_PARTICIPLE,"is an adjective participle because we found noun before and no desire for a verb",changed);
			if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	 }

	 // if ambiguity is between adverb and adjective, ignore it and keep going to solve a different ambiguity
	 remainder = posValues[i] & (-1 ^ (ADJECTIVE_BITS|ADVERB_BITS));
	 if (posValues[i] & ADJECTIVE_BITS && posValues[i] & ADVERB_BITS && !remainder) // can we decide if this is aux or verb?
	 {
		 return GUESS_CONTINUE;
	 }

	 // if ambiguity is PARTICLE vs PREPOSITION maybe BASIC_FORM, see if we have noun after -- particle is just a FORM of a preposition sort of
	remainder = posValues[i] & (-1 ^ (PARTICLE|PREPOSITION));
	if (posValues[i] & PARTICLE && posValues[i] & PREPOSITION && (!remainder || remainder == BASIC_FORM))  // "do wildflowers grow *in your back yard"
	{
		unsigned int x = i;
		while (++x <= lastRealWord)
		{
			if (posValues[x] == AMBIGUOUS_PRONOUN){;} // ambiguity unimportant
			else if (bitCounts[x] != 1) break;	// we dont understand what's after here
			if (posValues[x] & (DETERMINER|ADJECTIVE_BITS|PRONOUN_BITS|NOUN_BITS)) // is a referent
			{
				LimitValues(i, PREPOSITION,"is an preposition because we found signs of noun after",changed);
				break;	
			}
			else if (posValues[x] & ADVERB_BITS) continue;	// ignore
			else if (remainder == 0) // not an adverb
			{
				LimitValues(i, PARTICLE," is a particle because we found no noun after",changed);
				break;	// anything else is not allowed and breaks the flow of a preposition, make it a particle
			}
		}
		if (bitCounts[i] == 1) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	return GUESS_CONTINUE;
}

static void AddPhrase(unsigned int i)
{
	if (!phrases[i]) 
	{
		phrases[i] |= prepBit;
		prepBit <<= 1;
	}
	AddRoleLevel(PHRASE|OBJECT2);
	lastPhrase = i;		// where last verbal was, if any
}

static void AddVerbal(unsigned int i)
{
	if (needRoles[roleIndex] & PHRASE && needRoles[roleIndex] & OBJECT2) needRoles[roleIndex] ^= OBJECT2; // verbal acts as object
	if (!verbals[i]) 
	{
		verbals[i] |= verbalBit;
		verbalBit <<= 1;
	}
	AddRoleLevel(VERBAL|VERB2);
	if (posValues[i] == AMBIGUOUS_VERBAL) determineVerbal = i; // If we don't know if this is a GERUND or PARTICIPLE, we need to find out.
	lastVerbal = i;		// where last verbal was, if any
}

static void StartPhrase(unsigned int i,bool &changed)
{
	if (roles[i] == OMITTED_TIME_PREP) // reestablish prior phrase we found
	{
		if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) --roleIndex;	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this
		AddPhrase(i);
		return;
	}

	bool valid = false;
	WORDP D = originalLower[i];
	WORDP E = canonicalLower[i+1];
	if (E && E->systemFlags & TIMEWORD)  // some time expressions can omit a preposition and just have an object
	{
		if (!phrases[i] && !roles[i] && D && D->systemFlags & OMITTABLE_TIME_PREPOSITION) // after it is timeword?  // omitted prepoistion time phrase (adjective)  "next year"  - also "one day"  or   "five days ago"
		{
			if ( i == firstRealWord && lastRealWord > (firstRealWord+3) && wordStarts[firstRealWord+2][0] == ',') valid = true;
			else if ( i == (lastRealWord - 1)) valid = true;
			else if (posValues[3] & (DETERMINER|ADJECTIVE_BITS|NOUN_BITS|(PRONOUN_SUBJECT|PRONOUN_OBJECT))) valid = true; // we  see a subject potential...
			else if (phrases[i+2] || clauses[i+2]  || verbals[i+2] || posValues[i+2] == COMMA) valid = true; // we are isolated from stuff
		}
		else if (posValues[i] & ADJECTIVE_CARDINAL) valid = true; // "one day"
	}
	if (valid) // time phrase at start or end
	{
		LimitValues(i,ADJECTIVE_BITS|DETERMINER,"forced determiner/adj for timephrase",changed);
		LimitValues(i+1,NOUN_BITS,"Assigning noun to time word at start or end",changed);
		if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) --roleIndex;	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this
		SetRole(i, OMITTED_TIME_PREP);
		AddPhrase(i);
	}
}

static void StartClause(unsigned int i,bool & changed)
{
	if (posValues[i] & PREPOSITION) return;	// at best a conflict, we prefer prep and overrule Later.
	if ((i + 2) > lastRealWord) return;	// cannot fit subject and verb

	// find clauses  (markers start, ending with a verb)
	if (!stricmp(wordStarts[i],"however") ||  !stricmp(wordStarts[i],"whoever") || !stricmp(wordStarts[i],"whomever") ||  !stricmp(wordStarts[i],"whatever")|| !stricmp(wordStarts[i],"whenever") || !stricmp(wordStarts[i],"whichever")) {;} // will be a clause most likely
	else if (i == firstRealWord && originalLower[i] && originalLower[i]->properties & QWORD && posValues[firstRealWord+1] & (VERB_TENSES|AUX_VERB_BITS))  // qwords are not clauses but main sentence-- what is my name, EXCEPT "when the"
	{
		return;
	}
		
	// a certain  clause

// SUBJECT clause starters immediately follow a noun they describe
	// OBJECT clause starters arise in any object position and may even be ommited  -This is the man (whom/that) I wanted to speak to . The library didn't have the book I wanted
	// The book whose author won a Pulitzer has become a bestseller.
	// clause starter CANNOT be used as adjective... can be pronoun or can be possessive determiner (like whose)
	// USE originalLower because "that" becomes "a" when canonical
	if (i == firstRealWord) {;} // these things may all be a normal question, not a clause
	else if (posValues[i] == DETERMINER) {;} // that, used as a determiner so not a clause starter
	else if (originalLower[i] && (originalLower[i]->systemFlags & POTENTIAL_CLAUSE_STARTER || posValues[i] & CONJUNCTION_SUBORDINATE)) // who whom whoever which whichever  what whatever whatsoever where wherever when whenever how however why that whether  whose?
	{
		if (posValues[i+1] & AUX_VERB_BITS && !(needRoles[roleIndex] & (MAINOBJECT|OBJECT2))) return;	// more a real sentence, UNLESS we were looking for an object "I ate what was given"
		//whoever, whatever, whichever, however, whenever and wherever are called compound relative pronouns -- acts as a subject, object or adverb in its own clause;
		if (!clauses[i])
		{
			if (tried[i]) return; // been here already, it didn't work out
			if (!stricmp(wordStarts[i],"wherever") || !stricmp(wordStarts[i],"however") || !stricmp(wordStarts[i],"whenever") || !stricmp(wordStarts[i],"how") || !stricmp(wordStarts[i],"where") || !stricmp(wordStarts[i],"why") || !stricmp(wordStarts[i],"when")|| !stricmp(wordStarts[i],"whether")) // whether?
			{
				SetRole(i,ADVERB_CLAUSE);
			}
			else if (!stricmp(wordStarts[i],"what") || !stricmp(wordStarts[i],"who") || !stricmp(wordStarts[i],"whom") || !stricmp(wordStarts[i],"whoever") || !stricmp(wordStarts[i],"whatever") 
				|| !stricmp(wordStarts[i],"whichever") || !stricmp(wordStarts[i],"whatsoever")) 
			{
				if (posValues[i] == CONJUNCTION_SUBORDINATE){;}
				else LimitValues(i,PRONOUN_SUBJECT|PRONOUN_OBJECT,"clause starts with WH-pronoun, forcing pronoun",changed); // WP Penntag
				if (needRoles[roleIndex] & (MAINOBJECT | OBJECT2)) // we will satisfy this as a clause
				{
					SetRole(i,needRoles[roleIndex] & (MAINOBJECT | OBJECT2));
				}
			}
			else if (!stricmp(wordStarts[i],"which") || !stricmp(wordStarts[i],"that") ) // wh-determiners (or simple clause starters)
			{
				if (posValues[i] == CONJUNCTION_SUBORDINATE){;} // leave it alone
				// need an object but not two and one follows us
				else if (posValues[i+1] & (PRONOUN_BITS|DETERMINER))
				{
					if (posValues[i] & CONJUNCTION_SUBORDINATE) LimitValues(i,CONJUNCTION_SUBORDINATE,"clause starts with no need of extra noun so be simple conjunction for now",changed); 
					else LimitValues(i,PRONOUN_OBJECT,"clause starts with no need of extra noun so be object",changed); 
				}
				else if (posValues[i+1] & (AUX_VERB_BITS|VERB_TENSES))
				{
					LimitValues(i,PRONOUN_SUBJECT,"clause starts with WH-pronoun followed by verb or aux, forcing Pronoun subject",changed); // WDT Penntag
				}
			}
			else if (!stricmp(wordStarts[i],"whose") ) // wh-possessive pronoun
			{
				LimitValues(i,PRONOUN_POSSESSIVE,"clause starts with WH-pronoun, forcing POSSESSIVE",changed); // WP$ Penntag
				return;		// just because possessive doesnt mean clause
			}
		}
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT)) // clause should not intervene before direct object unless describing indirect object
		{ 
			SetRole(i,needRoles[roleIndex] & (OBJECT2|MAINOBJECT));
			needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT; // in addition remove this
		}
		AddClause(i,"%s clause - qword starter\r\n");
		if (posValues[i] & CONJUNCTION_SUBORDINATE) LimitValues(i,CONJUNCTION_SUBORDINATE,"adverb clause forcing subord conjunct, forcing",changed); // WRB Penntag
		return;
	}
	else // implicit clause
	{
//normally not implicit: , what, which, who, whoever, whom, whomever, whose -- after, although, as, because, before, if, once, since, though, till, unless, until, , whenever, where, wherever, while 
//I must admit * Ralph was my first and only blind date. (Noun clause--direct object)
//The first blind date * I ever had was Ralph. (Adjective clause BEFORE MAIN VERB) 
// Ralph was my first and only blind date because I married him. (Adverb clause NOTHING OMITTED)
	//I'll never forget the day * I met her.   -- after a noun to describe it -- The day I met her was warm
	//He discovered * he was hungry and * the fridge was empty
	// He believes * Mary is beatiful. -- common AFTER main verb is done
	// President Jefferson believed * the headwaters of the Missouri might reach all the way to the Canadian border and * he could claim all that land for the United States.
//An elliptical clause may seem incorrect as it may be missing essential sentence elements, but it is actually accepted grammatically. As these clauses must appear together with complete clauses which contain the missing words, repetition is avoided by leaving the same words (or relative pronoun) out in the elliptical clause. This conciseness actually adds to the flow of the text and promotes writing that is more elegant.
//The Louvre museum was one of the sites (that) we did not want to miss.
//[The relative pronoun that is omitted from the adjective clause]
//After (we visited) the Louvre, we went out to dinner at a French bistro.
//[subject and verb omitted from adverb clause]
//The French make better croissants than the American (make or do).
//[second half of comparison omitted]
//Though (they) sometimes (appear) impatient and somewhat assertive, most French people are actually kind and warm-hearted.
//[subject and verb omitted from adverb clause]

		// usual PUNCTUATION is  < subclause, main sentence    OR    <  main sentence subclause (no comma)
		int prior = i;
		while (--prior && posValues[prior] & (ADJECTIVE_BITS | DETERMINER | POSSESSIVE_BITS)); // find the major unit before our noun
		// we will need 2 verbs for this to work
		if (needRoles[MAINLEVEL] & MAINVERB)
		{
			unsigned int count = 0;
			unsigned int at = i;
			while (++at <= lastRealWord)
			{
				if (posValues[at] & VERB_TENSES) ++count;
			}
			if (count < 2) return;	// cannot be clause along with main sentence
		}

		// a noun arising unexpected following a noun is likely a clause with "that" or "when" omitted
		if (prior && posValues[prior] & NOUN_BITS && posValues[i] & (NOUN_BITS | ADJECTIVE_NOUN | PRONOUN_SUBJECT|PRONOUN_OBJECT) && !(posValues[i] & (-1 ^ (NOUN_BITS | ADJECTIVE_NOUN| PRONOUN_SUBJECT|PRONOUN_OBJECT))) && !(needRoles[roleIndex] & (MAINSUBJECT|MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2)) )
		{
			if (posValues[i] & NORMAL_NOUN_BITS && verbStack[roleIndex] && parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB) // can take a complement - He painted his old jalopy *purple 
			{
				LimitValues(i,NORMAL_NOUN_BITS,"Forcing 2nd noun as factitive complement",changed);
				return;
			}

			// but if the prior noun could have been adjectives "the little old lady", retrofix them
			else if (posValues[i] & NORMAL_NOUN_BITS && allOriginalWordBits[i-1] & ADJECTIVE_BITS)
			{
				if (prior == (i-1)) 
				{
					LimitValues(i-1,allOriginalWordBits[i-1] & ADJECTIVE_BITS,"Forcing prior to adjective hitting unexpected noun",changed);
					needRoles[roleIndex] |= roles[prior];
					roles[prior] = 0;
					return;
				}
				if (prior == (i-2) && allOriginalWordBits[i-2] & ADJECTIVE_BITS)
				{
					LimitValues(i-2,allOriginalWordBits[i-2] & ADJECTIVE_BITS,"Forcing 2x prior to adjective hitting unexpected noun",changed);
					needRoles[roleIndex] |= roles[prior];
					roles[prior] = 0;
					return;
				}
			}


			if (posValues[i+1] & (AUX_VERB_BITS|VERB_TENSES) && !roles[i]) AddClause(i,"omitted that or when?");
		}
	}
}

static bool AssignRoles(bool &changed)
{
	memset(coordinates,0,wordCount+4);
	memset(crossReference,0,wordCount+4);
	memset(quotedWords,0,wordCount+4);
	
	// Roles and Phrases are cumulative from call to call

	if (trace & POS_TRACE) Log(STDUSERLOG,"\r\n---- Assign roles\r\n");
	char goals[MAX_WORD_SIZE];
	wordStarts[0] = "";
	bool quoting = false;
	unsigned int startComma = 0;
	unsigned int endComma = 0;
	unsigned int commalist = 0;
	requireNoun = false;		// expecting a noun
	
	// "After hitting Sue, who is  the one person that I love among all the people I know who can walk, the ball struck a tree."
	needRoles[0] = COMMA_PHRASE;		// a non-zero level
	InitRoleSentence(1);
	
	// preanalyze comma zones, to see what MIGHT be done within a zone..
	AssignZones();

	// Once a role is assigned, it keeps it forever unless someone overrides it explicitly.
	for (unsigned int i = firstRealWord; i <= lastRealWord; ++i)
	{
		char* word = wordStarts[i];
		if ((*word == '"' || *word == '\'') && !word[1]) // isolated quote 
		{
			if (i > 1 && posValues[i-1] & (NOUN_PROPER_PLURAL|NOUN_PLURAL)) {;}
			else
			{
				quoting = !quoting; // a sentence entirely a quote will get parsed, otherwise we ignore quoted bits
				quotedWords[i] = 1;
				continue;
			}
		}
		if (quoting) 
		{
			quotedWords[i] = 1;
			continue;	// ignore quoted strings
		}

		if (trace & POS_TRACE)
		{
			char flags[MAX_WORD_SIZE];
			strcpy(flags,"Flags: ");
			ParseFlags(flags,i);
			if (!flags[7]) *flags = 0;	// remove header if none there
			char tags[MAX_WORD_SIZE];
			*tags = 0;
			Tags(tags,i);
			strcat(tags,flags);
			Log(STDUSERLOG,"\"%s\" %d (%s)\r\n",word,i,tags);
			for (unsigned int x = roleIndex; x >= 1; --x)
			{
				DecodeneedRoles(x,goals);
				Log(STDUSERLOG, "    need %d %s - s:%d aux:%d v:%d io:%d o:%d\r\n",x,goals,subjectStack[x],auxVerbStack[x],verbStack[x],indirectObjectStack[x],objectStack[x]);
			}
		}
		StartPhrase(i,changed);
		StartClause(i,changed);

		if (bitCounts[i] > 1)
		{
			unsigned int result = GuessAmbiguous(i,changed);
			if (result == GUESS_RETRY) // somebody fixed it. try again
			{
				--i;
				continue;
			}
			if (result == GUESS_ABORT) return false; // abort if we cannot resolve noun or verb conflict
			// otherwise GUESS_CONTINUE
		}

		switch(posValues[i]) // mostly only handles exact match values
		{
		case AUX_VERB_PRESENT: case AUX_VERB_FUTURE: case AUX_VERB_PAST: case AUX_BE: case AUX_HAVE: case AUX_DO: // AUX_VERB_BITS
			// if we are a auxverb immediately after indirectobject, presume this was an object clause instead- "do you think *she can be ready"
			if (posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST) && roles[i-1] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && parseFlags[verbStack[roleIndex]] & OMITTABLE_THAT_VERB)
			{
				SetRole(i-1,OBJECT2); // no longer indirect
				if (posValues[i-1] & PRONOUN_OBJECT) LimitValues(i-1,PRONOUN_OBJECT,"pronoun subject of clause forced to subject",changed);
				AddClause(i-1,"%s clause - implied by verb following indirectobject\r\n");
				AddRole(i-1,SUBJECT2); // head of clause
			}

			if (roleIndex == 2 && needRoles[roleIndex] & CLAUSE && needRoles[roleIndex] & VERB2 && !(needRoles[roleIndex] & SUBJECT2) && !subjectStack[MAINLEVEL]) // misreading "whose bike were you riding" as a clause when it isnt, its a simple question, but if we already HAVE a subject for main level not bad- "I do not know if he will go"
			{
				--roleIndex;	// drop back to main sentence treating this as direct object instead of clause
				for (unsigned int x = lastClause; x < i; ++x) 
				{
					if (roles[x] == SUBJECT2) SetRole(x,MAINOBJECT);
					clauses[x] = 0;
				}
				lastClause = 0;
			}
			if (needRoles[roleIndex] & OBJECT2 && roleIndex > 1)
			{	
				--roleIndex;	// not going to happen, we are done with this level
			}

			if (!verbStack[roleIndex]) auxVerbStack[roleIndex] = (unsigned char)i;	// most recent aux for this level, but only if no verb found yet (must preceed verb)
			break;
		case VERB_INFINITIVE: 
			// if not a TO infinitive
			if (needRoles[roleIndex] & OBJECT_COMPLEMENT && !(posValues[i-1] & TO_INFINITIVE) && (i > 2 && !(posValues[i-2] & TO_INFINITIVE) ))
			{
				uint64 toBits = canSysFlags[verbStack[roleIndex]] & (DIRECTOBJECT_INFINITIVE_COMPLEMENT | DIRECTINFINITIVE_DIRECTOBJECT);
				if (toBits & DIRECTINFINITIVE_DIRECTOBJECT && needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) // I like *swim
				{
					LimitValues(i,NOUN_INFINITIVE,"following TO when verb uses direct infinitive as object",changed);
					SetRole(i,needRoles[roleIndex] & (OBJECT2|MAINOBJECT)|VERB2);
				}
				else if (toBits & DIRECTOBJECT_INFINITIVE_COMPLEMENT && !(needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) && roles[i-1] & (MAINOBJECT|OBJECT2)) 
				{
					LimitValues(i,NOUN_INFINITIVE,"following TO when verb uses direct infinitive as object complement",changed);
					SetRole(i,OBJECT_COMPLEMENT|VERB2);
				}
				AddVerbal(i);
			}
	
			// drop thru to other verbs
		case VERB_PRESENT: case VERB_PRESENT_3PS: case VERB_PAST: case VERB_PAST_PARTICIPLE: case VERB_PRESENT_PARTICIPLE: //  VERB_TENSES
			 // does not include NOUN_INFINITIVE or NOUN_GERUND or ADJECTIVE_PARTICIPLE - its a true verb (not aux)
			{
				// absolute phrase is like implied prep phrase "with" in "legs quivering, we ran".  requires comma seaparation and no aux verbs
				if (!phrases[i] && !auxVerbStack[roleIndex] && (posValues[i] & (VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE)) && (currentZone == 0|| currentZone == (int)(zoneIndex-1)) && zoneIndex > 1)
				{
					unsigned int at = i;
					while (--at && !phrases[at] && !verbals[at] && !clauses[at] && !(posValues[at] & (COMMA|CONJUNCTION_SUBORDINATE)) && !(roles[at] & ( SUBJECT2 | MAINSUBJECT | OBJECT2) )) {;}
					bool abs = false;
					if (roles[at] == SUBJECT2 || roles[at] == OBJECT2 || !roles[at]) 
					{
						roles[at] = OBJECT2;
						abs = true;
					}
					else if (roles[at] == MAINSUBJECT && needRoles[MAINLEVEL] & MAINVERB && !(posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS))) // we need a verb and this cant be it
					{
						// if aux, we can also tolerate participles
						if (posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && auxVerbStack[MAINLEVEL]){;}
						else
						{
							needRoles[MAINLEVEL] |= MAINSUBJECT;
							roles[at] = OBJECT2;
							abs = true;
						}
					}
					if (abs) 
					{
						LimitValues(i,ADJECTIVE_PARTICIPLE,"absolute phrase will take adjective participle",changed);
						zoneData[currentZone] = ZONE_ABSOLUTE;
						AddPhrase(at);
						--roleIndex; // drop phrase level, we're done
						ExtendChunk(at,i,phrases); // cover verbal with prep phrase
					}
				}

				// if we are a verb immediately after indirectobject, presume this was an object clause instead- "do you think *she is ready"
				if (posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST) && roles[i-1] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && parseFlags[verbStack[roleIndex]] & OMITTABLE_THAT_VERB)
				{
					SetRole(i-1,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2); // no longer indirect
					if (posValues[i-1] & PRONOUN_OBJECT) LimitValues(i-1,PRONOUN_OBJECT,"pronoun subject of clause forced to subject",changed);
					AddClause(i-1,"%s clause - implied by verb following indirectobject\r\n");
					AddRole(i-1,SUBJECT2); // head of clause
				}

				// if we dont need a verb and prior was a verb and could have been a noun, revert it and cancel...
				if (posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS) &&
					(allOriginalWordBits[i-1] & NORMAL_NOUN_BITS) && roles[i-1] & (VERB2|MAINVERB) && !(needRoles[roleIndex] & (VERB2|MAINVERB)))
				{
					needRoles[roleIndex] |= roles[i-1];
					unsigned int role = roles[i-1];
					SetRole(i-1,0);
					LimitValues(i-1,allOriginalWordBits[i-1] & NORMAL_NOUN_BITS,"hitting 2 verbs, force prior to noun",changed);
				}

				if (needRoles[roleIndex] & OBJECT2 && roleIndex > 1)
				{	
					if (indirectObjectStack[roleIndex]) SetRole(indirectObjectStack[roleIndex],OBJECT2); // change role over, cant have indirect w/o direct "to know *him is to love him"
					--roleIndex;	// not going to happen, we are done with this level (a verbal presumably)
				}

				// is this a To Infinitive verb... becomes close of objects at level below
				if ( posValues[i-1] == TO_INFINITIVE || (i >= 2 && posValues[i-2] == TO_INFINITIVE) || (i >= 3 && posValues[i-3] == TO_INFINITIVE)) 
				{
					--roleIndex;
					HandleComplement(i,changed);
					++roleIndex;
				}

			//	if (!verbStack[roleIndex]) verbStack[roleIndex] = (unsigned char)i;	// current verb if we dont have one yet  - handled by seekobjects
				
				// when an infinitive is waiting to close because it wants an object, this MAIN verb
				// this might be wrong if you can insert a clause in there somewhere 
				if (needRoles[roleIndex] & OBJECT2)
				{
					needRoles[roleIndex] &= -1 ^ ( OBJECT2 | INDIRECTOBJECT2);	// we wont find these any more
					CloseLevel(i-1);
				}


				// a real verb will terminate a bunch of things
				//To sleep *is (no objects will be found)
				if (needRoles[roleIndex] & VERB2) // verb is inside a clause, probably clausal verb we expected
				{
					// if we are MISSING a needed subject, can we fill that in NOW?
					if (needRoles[roleIndex] & SUBJECT2)
					{
						// if a clause, is the starter a potential subject (like which, who, etc)
						if (lastClause)
						{
							if (canonicalLower[lastClause] && canonicalLower[lastClause]->properties & PRONOUN_BITS)
							{
								SetRole(lastClause,SUBJECT2);
							}
						}
						needRoles[roleIndex] &=  -1 ^ SUBJECT2;
					}

					AddRole(i,VERB2); // in case "I have to *find a ball" assigned MAINOBJECT before VERB2
					if (lastClause && needRoles[roleIndex] & CLAUSE) // drag clause thru here
					{
						// if we still need a subject, can starter of clause be it?
						if (needRoles[roleIndex] & SUBJECT2)
						{
							if (originalLower[lastClause] && originalLower[lastClause]->properties & PRONOUN_SUBJECT)
							{
								SetRole(lastClause,SUBJECT2);
							}
						}
						ExtendChunk(lastClause,i,clauses);
					}
					if (lastVerbal && needRoles[roleIndex] & VERBAL) ExtendChunk(lastVerbal,i,verbals); // drag verbal thru here
				}
				else if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] < startComma && i < endComma) // saw a subject, now in a comma phrase, wont be main verb (except it might be- hugging the wall, nathan *peered  Or comma phrase separated subject from verb as appositive or such
				{
					SetRole(i,VERB2);
					AddRoleLevel(COMMA_PHRASE);
				}
				else if (needRoles[roleIndex] & OBJECT_COMPLEMENT && posValues[i] & VERB_INFINITIVE) // we seek an infinitive complement.
				{
					AddVerbal(i);
					SetRole(i,VERB2); // BUG what should the role be
				}
				else if (needRoles[roleIndex] & MAINVERB) // we seek a main verb....
				{
					// ASSIGN SUBJECT IF WE CAN

					// if we get here and HAVE no subject, but we have a pending verbal, assign it as the subject
					if (needRoles[roleIndex] & MAINSUBJECT && lastVerbal) 
					{
						SetRole(lastVerbal,MAINSUBJECT);
						if ( bitCounts[lastVerbal] != 1 && posValues[lastVerbal] & (NOUN_GERUND|NOUN_INFINITIVE))
						{
							LimitValues(lastVerbal,NOUN_GERUND|NOUN_INFINITIVE,"pending verbal assigned as subject, be gerund or infinitive",changed);
							if (trace & POS_TRACE) Log(STDUSERLOG,"Resolve AMBIGUOUS_VERBAL as NOUN_GERUND or NOUN_INFINITIVE\r\n");
						}
					}

					// we have no subject, but we have a probable noun clause, use that
					if (needRoles[roleIndex] & MAINSUBJECT && firstNounClause) // clause should not intervene before direct object unless describing indirect object
					{
						SetRole(firstNounClause,MAINSUBJECT);
						firstNounClause =  0;
					}

					// NOW HANDLE MAIN VERB and its requirements
	
					SetRole(i,MAINVERB);
					predicateZone = currentZone;

					// "are you" questions do not abandon subject yet, same for non-aux have -- but not are as AUX
					// but "be" will be different as infinitive.
					if ((needRoles[roleIndex] & MAINSUBJECT) != 0 && stricmp(wordStarts[i],"be") && canonicalLower[i] &&  (!stricmp(canonicalLower[i]->word,"be")||!stricmp(canonicalLower[i]->word,"have"))) // no subject yet
					{
					}
					else if ((needRoles[roleIndex] & MAINSUBJECT) != 0 && (posValues[i] & VERB_INFINITIVE) != 0 ) // go with imperative
					{
						LimitValues(i,VERB_INFINITIVE,"Resolve verb as Infinitive",changed);
						needRoles[roleIndex] &= -1 ^ MAINSUBJECT;
					}
					else 
					{
						needRoles[roleIndex] &= -1 ^ MAINSUBJECT;	// can no longer be seeking a subject, maybe it was implied. -- but "be cool" wont work - bug
					}
				}
				else  if (needRoles[roleIndex] & (MAINOBJECT)) // another verb?
				{
					if (posValues[i] & VERB_INFINITIVE && (parseFlags[verbStack[roleIndex]] & CAUSAL_TOINFINITIVE_VERB || canSysFlags[verbStack[roleIndex]] & DIRECTOBJECT_INFINITIVE_COMPLEMENT)) // direct object is a verb phrase
					{
						SetRole(i,MAINOBJECT);
						AddVerbal(i);
						AddRole(i,VERB2);
					}
					else
					{
						AddRoleLevel(CLAUSE);
						verbStack[roleIndex] = (unsigned char)i;
						needRoles[roleIndex] &= -1 & VERB2;
					}
				}
				else if (posValues[i] == VERB_INFINITIVE && !verbals[i]) // discovered verbal -- postnominal adjective verbal?
				{
					AddVerbal(i);
					if (posValues[i-1] & (PRONOUN_BITS|NORMAL_NOUN_BITS)) SetRole(i,POSTNOMINAL_ADJECTIVE);
					else SetRole(i,VERB2);
				}

				// see if verb might take an object of some kind

				// A passive voice does not take an object - need aux to be "be" verb
				bool noobject = false;
				if (posValues[i] & VERB_PAST_PARTICIPLE) // after the dog had been walked (passive takes no objects)
				{
					unsigned int x = i;
					while (--x)
					{
						if (posValues[x] & AUX_VERB_PAST) // window was broken. but not window is broken (participle description)
						{
							if (canonicalLower[x] && canonicalLower[x]->word[0] == 'b' ) noobject = true;
							break;
						}
					}
				}
				SeekObjects(i);
				// if no object is expected. If we see a noun following, PRESUME it is part of an implied "that" clause - "I hope she goes", but might be a time phrase like "this morning."
				if (needRoles[roleIndex] == CLAUSE) lastClause = 0;	// all fulfilled
				if (needRoles[roleIndex] == VERBAL) lastVerbal = 0; // all fulfilled
			}
			break;
			case AMBIGUOUS_VERBAL: // (ADJECTIVE_PARTICIPLE | NOUN_GERUND ) 
				AddVerbal(i);

				// drop thru to adjective participle
			case ADJECTIVE_NOUN: 
			case ADJECTIVE_NORMAL: case ADJECTIVE_PARTICIPLE: case ADJECTIVE_CARDINAL: case ADJECTIVE_ORDINAL: // ADJECTIVE_BITS
			{
				// absolute phrase is like implied prep phrase "with" in "legs quivering, we ran".  requires comma seaparation and no aux verbs
				if (!phrases[i] && !auxVerbStack[roleIndex] && (posValues[i] & ADJECTIVE_PARTICIPLE) && (currentZone == 0|| currentZone == (int)(zoneIndex-1)) && zoneIndex > 1)
				{
					unsigned int at = i;
					while (--at && !phrases[at] && !verbals[at] && !clauses[at] && !(posValues[at] & (COMMA|CONJUNCTION_SUBORDINATE)) && !(roles[at] & ( SUBJECT2 | MAINSUBJECT | OBJECT2) )) {;}
					bool abs = false;
					if (roles[at] == SUBJECT2 || roles[at] == OBJECT2 || !roles[at]) 
					{
						roles[at] = OBJECT2;
						abs = true;
					}
					else if (roles[at] == MAINSUBJECT && needRoles[MAINLEVEL] & MAINVERB && !(posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS))) // we need a verb and this cant be it
					{
						// if aux, we can also tolerate participles
						if (posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && auxVerbStack[MAINLEVEL]){;}
						else
						{
							needRoles[MAINLEVEL] |= MAINSUBJECT;
							roles[at] = OBJECT2;
							abs = true;
						}
					}
					if (abs) 
					{
						LimitValues(i,ADJECTIVE_PARTICIPLE,"absolute phrase will take adjective participle",changed);
						zoneData[currentZone] = ZONE_ABSOLUTE;
						AddPhrase(at);
						--roleIndex; // drop phrase level, we're done
						ExtendChunk(at,i,phrases); // cover verbal with prep phrase
					}
				}
				if (posValues[i] & (ADJECTIVE_NORMAL|ADJECTIVE_CARDINAL|ADJECTIVE_ORDINAL)) 
				{
					if (!(posValues[i-1] & NOUN_BITS))	requireNoun = true; // in case its a postnominal adjective we cant do this
				}

				// adjective roles if not describing a following noun
				if (canSysFlags[verbStack[roleIndex]] & LINKING_VERB && !(posValues[i+1] & ADJECTIVE_BITS|NOUN_BITS|CONJUNCTION_COORDINATE))
					SetRole(i,SUBJECT_COMPLEMENT);
				// verb expects adjective after direct object
				else if (needRoles[roleIndex] & OBJECT_COMPLEMENT && posValues[i-1] & (NOUN_BITS|PRONOUN_BITS) && parseFlags[verbStack[roleIndex]] & FACTITIVE_ADJECTIVE_VERB && !(needRoles[roleIndex] & (OBJECT2|MAINOBJECT)))
					SetRole(i,OBJECT_COMPLEMENT);
				// hopefully a legal trailing adjective
				else if (posValues[i-1] & NOUN_BITS && !(posValues[i+1] & ADJECTIVE_BITS|NOUN_BITS|CONJUNCTION_COORDINATE)) 
					SetRole(i,POSTNOMINAL_ADJECTIVE); 
					
				unsigned int level = roleIndex;
				if (posValues[i] == ADJECTIVE_PARTICIPLE)  // "the men using a sword were"  and "the walking dead"
				{
					if (roles[i]){;}
					// accidents have been reported involving passengers
					else if ( roles[i-1] == MAINVERB && allOriginalWordBits[i] & VERB_PRESENT_PARTICIPLE)
					{
						SetRole(i,VERB2);	// default role -- will be verbal describing verb...EXCEPT it may be an adjective object of a linking verb
					}
					else if (posValues[i+1] & (NOUN_BITS|DETERMINER|PRONOUN_POSSESSIVE)) {;} // describing a noun as boring adjective UNLESS its our object!
					else SetRole(i,0);	// default role
				
					// establish a permanent verbal zone if this is freestanding or following a noun and isnt in a zone
					if (posValues[i-1] & (ADJECTIVE_BITS|DETERMINER|POSSESSIVE|PRONOUN_POSSESSIVE|CONJUNCTION_BITS)){;} // wont take an object, can only describe one "the tall and distinguished gentleman"
					
					// we dont care about adj part occurring BEFORE a noun.. that's ordinary adjective
					else 
					{
						AddVerbal(i); // BUT NOT if it is following an adjective,determiner, possessive, etc (before a noun)
						needRoles[roleIndex] &= -1 ^ VERB2;
					}
					//  "who is a *dating service for" has the object as not part of this level
					
					// present participle can start a sentence and have objects
					bool mightWantObject = (i == 1); // as a sentence start, it can want an object.
					if (posValues[i] & VERB_PAST_PARTICIPLE) mightWantObject = false; // past participle cannot have an object
					else if (verbals[i] && posValues[i-1] & (ADJECTIVE_BITS|DETERMINER)) mightWantObject = false; // not following a noun
					else if (posValues[i] & ADJECTIVE_PARTICIPLE) mightWantObject = false;
					else if (verbals[i] ) // occuring after the noun // note- adjective-participles may occur AFTER the noun and have objects but not when before - we have the people *taken care of
					{
						mightWantObject = true;
						if (canSysFlags[i] & (VERB_DIRECTOBJECT|VERB_INDIRECTOBJECT)) // change to wanting object(s)
						{
							// wont have indirect object we presume
							verbStack[roleIndex] = (unsigned char)i;	// this is the verb for this
							needRoles[roleIndex] |= OBJECT2; // mark this kind of level (infinitive or gerund) UNLESS this is actually the main verb
						}
					}
					//else- occurring before the noun it is just a descriptor and cant take objects

					if (mightWantObject) SeekObjects(i);
				}

				// assign adjective object for this level if appropriate
				// not "he is a green man"
				// he is green. but not they are green and sticky men
				if (canonicalLower[verbStack[level]] && canonicalLower[verbStack[level]]->systemFlags & LINKING_VERB) 
				{
					// see if we are postnominal
					if (posValues[i-1] & (NOUN_BITS | PRONOUN_SUBJECT|PRONOUN_OBJECT) && roles[i-1] == MAINOBJECT ) 
					{
						SetRole(i, OBJECT_COMPLEMENT);
						continue; // not for object complements:  they considered him crazy  
					}
					// check is we are prenominal
					unsigned int j = i;
					while (++j <= lastRealWord) // prove no nouns in our way after this -- are you the scared cat  vs  are you afraid of him
					{
						if ((clauses[j] && clauses[i] != clauses[j]) || (verbals[j] && verbals[i] != verbals[j]) || 
							(phrases[j] && phrases[i] != phrases[j])) break;	// not relevant (though detecting these in a clause would be nice)
						if (posValues[j] & (PAREN | COMMA | CONJUNCTION_BITS|PREPOSITION|TO_INFINITIVE|NOUN_GERUND)) break; // fall off "we are *responsible for it all"
						if (posValues[j] & NOUN_BITS)  //  but not they are green men  -- we probably describe a noun
						{
							j =  2000; // fall off
							break;
						}
					}
					if ( j != 2000) 
					{
						SetRole(i,SUBJECT_COMPLEMENT);
						needRoles[level] &= -1 ^ (MAINOBJECT|MAININDIRECTOBJECT|OBJECT2|INDIRECTOBJECT2); // prior level is done
					}
				}
				// we can be an adj complement IF expected and not in front of some other continuing thing like "I like *gooey ice cream"
				if (!roles[i] && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(posValues[i+1] & (ADJECTIVE_BITS|NOUN_BITS)))
				{
					SetRole(i, OBJECT_COMPLEMENT);
					continue;  
				}

			}
			break;
			case AMBIGUOUS_PRONOUN: // (PRONOUN_SUBJECT | PRONOUN_OBJECT )
				// drop thru to pronouns
			case  NOUN_SINGULAR: case NOUN_PLURAL: case NOUN_PROPER_SINGULAR: case NOUN_PROPER_PLURAL: case NOUN_GERUND: case NOUN_CARDINAL: case NOUN_ORDINAL: case NOUN_INFINITIVE: //Full_NOUN_BITS
			case PRONOUN_SUBJECT: case PRONOUN_OBJECT:
			{
				bool finalNoun = false;
				// forced role on reflexives so wont be  or some kind of compelement "I prefer basketball *myself" but not "I cut *myself"
				if (originalLower[i] && originalLower[i]->systemFlags & PRONOUN_REFLEXIVE && !(needRoles[roleIndex] & (MAINOBJECT|OBJECT2)))
				{
					SetRole(i,REFLEXIVE);
					break;
				}
				if (roles[i] == ADDRESS) break;	// we already know we are an address role, dont rethink it

				if (posValues[i] & (NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL)) requireNoun = false;
				// see if this is final noun in a series
				if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT)) finalNoun = true;
				else if (posValues[i] & NOUN_PLURAL) finalNoun = true; // "She gave the *children homework" 
				else if (posValues[i] & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL) && !(posValues[i+1] & (POSSESSIVE|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) finalNoun = true; 
				else if (posValues[i] & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL) && (posValues[i+1] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) {;} 
				else if (posValues[i] & ( NOUN_INFINITIVE|NOUN_GERUND)) finalNoun = true; // can also accept objects
				else if (!(posValues[i+1] & (ADJECTIVE_BITS|POSSESSIVE|(NOUN_BITS-NOUN_GERUND)))) finalNoun = true;
				else if (IsLegalAppositive(i,i+1)) finalNoun = true;
				else if (canonicalUpper[i+1] && !canonicalLower[i+1] && posValues[i+1] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) finalNoun = true; // this will be noun appositive or omitted preposition - "the thing Eli wanted" or "my brother Billy"
				else if (!clauses[i] && clauses[i+1]) finalNoun = true;	// we break on clause boundary
				else if (bitCounts[i+1] != 1) finalNoun = true;	// we must assume it for now...  "eating children tastes good

				if ((!finalNoun || posValues[i+1] & POSSESSIVE) && posValues[i] & NORMAL_NOUN_BITS)  // not really at end of noun sequence, not the REAL noun
				{
					LimitValues(i,ADJECTIVE_NOUN,"forcing noun adjective since not final",changed);
					SetRole(i,0);
					continue;
				}

				// if we thought a leading verbal was the subject and we have a superfluous noun here in next zone, revise...
				if (roleIndex == 1 && needRoles[MAINLEVEL] & MAINVERB && !(needRoles[MAINLEVEL] & MAINSUBJECT) &&
					currentZone == 1 && subjectStack[MAINLEVEL] && posValues[subjectStack[MAINLEVEL]] & NOUN_GERUND)
				{
					if (firstnoun == subjectStack[MAINLEVEL]) firstnoun = 0;
					LimitValues(subjectStack[MAINLEVEL],ADJECTIVE_PARTICIPLE,"revising old mainsubject on finding new one",changed);
					SetRole(subjectStack[MAINLEVEL], 0);	// remove role
					needRoles[MAINLEVEL] |= MAINSUBJECT;	// add it back
				}

				if (!firstnoun) firstnoun = i; // note noun for potential prep wrapped from end

				// something wanting subject or object can solve unknown pronoun now. 
				if (posValues[i] != AMBIGUOUS_PRONOUN || !(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2)) ){;} // we are not ambigous pronoun and not looking to have a noun at present
				else 
				{
					if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)) 
					{
						LimitValues(i,PRONOUN_SUBJECT,"Resolve AMBIGUOUS_PRONOUN as subject",changed);
					}
					else 
					{
						if (needRoles[roleIndex] & VERBAL) ExtendChunk(lastVerbal,i,verbals); // drag  verbal to here
						else if (needRoles[roleIndex] & CLAUSE) ExtendChunk(lastClause,i,clauses) ; // drag clause  to here
						LimitValues(i,PRONOUN_OBJECT,"Resolve AMBIGUOUS_PRONOUN as object",changed);
					}
				}

				// unexpected noun/pronoun - if we are still awaiting mainverb and think we have main subject as "what"
				// make this the subject
				if (needRoles[MAINLEVEL] & VERB && roleIndex == 1 && roles[firstRealWord] & MAINSUBJECT &&
					canonicalLower[firstRealWord] && canonicalLower[firstRealWord]->properties & QWORD)
				{
					SetRole(i,MAINSUBJECT);
					SetRole(firstRealWord,MAINOBJECT);
				}
				// nexpected noun/pronoun - if we are still awaiting verb for clause and think we have  subject as "what"
				// make this the subject
				else if (needRoles[roleIndex] & VERB && roles[i-1]& SUBJECT2 && canonicalLower[i-1] && canonicalLower[i-1]->properties & QWORD) // "I ate what they said"
				{
					SetRole(i-1,MAINOBJECT);
					SetRole(i,MAINSUBJECT);
				}

				if (parseFlags[i] & FACTITIVE_NOUN_VERB) 
				{
					needRoles[roleIndex] |= OBJECT_COMPLEMENT;
					needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2|MAININDIRECTOBJECT); // we elected Jim president will be caught here  but we elected Jim to run will be parsed wrongly....
				}

				if (roles[i-1] & (MAINOBJECT|OBJECT2) && parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB) SetRole(i,OBJECT_COMPLEMENT);
				else HandleComplement(i,changed);
				
				// if this is a verbal (NOUN_GERUND/NOUN_INFINITIVE) it will be at new level ..
				//  Raise it back to its level and fill it in
				// extend reach for supplemental objects
				if (posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE))  
				{
					// infinitive after aux verb is likely some funny tense
					if (posValues[i-1] & AUX_VERB_BITS) {;}
					else if (i > 1 && posValues[i-1] & ADVERB_BITS && posValues[i-2] & AUX_VERB_BITS) {;} 
					else
					{
						if (!(needRoles[roleIndex] & VERBAL)) 
						{
							AddVerbal(i); // TO xxx might have started it already
							if (posValues[i-1] & TO_INFINITIVE && bitCounts[i-1] > 1)
							{
								LimitValues(i-1,TO_INFINITIVE,"Infinitive clarifieds To",changed);
								ExtendChunk(i-1,i,verbals);
							}
							else if (i > 2 && posValues[i-2] & TO_INFINITIVE && bitCounts[i-2] > 1) // separated with adverb
							{
								LimitValues(i-2,TO_INFINITIVE,"Infinitive clarifieds To before adverb",changed);
								ExtendChunk(i-2,i,verbals);
							}
							AddRole(i,VERB2);
						}
						if (posValues[i] & NOUN_INFINITIVE && posValues[lastVerbal] == TO_INFINITIVE) 
						{
							ExtendChunk(lastVerbal,i,verbals);
							SetRole(i,VERB2);
						}
					}

					SeekObjects(i);
					// are you able to drive a car wants object2 for car
					// "Be a man"
					// need to know if this is supplying an object or is describing a noun...
					// EG  I want to eat   vs  I want chitlins to eat ...
				}

				// if this is not an object or a subject or an indirect object... maybe its comma delimited appositive. its certainly surfeit.
				else if (roles[i]) {;} // we have assigned it a role
				else if (i == lastRealWord || wordStarts[i+1][0] == ',') // set off or at the end... check its content backwards.
				{
					unsigned int x = i;
					while (--x)
					{
						if ( posValues[x] == COMMA)	// we were pure to here
						{
							if (posValues[x-1] & ( NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR  | NOUN_PROPER_PLURAL )) // we are good
							{
								SetRole(i,APPOSITIVE);
								break;
							}
							else break; // not preceeded by a noun
						}
						else if (posValues[x] & (ADJECTIVE_BITS|ADVERB_BITS |PRONOUN_POSSESSIVE));	// fine stuff
						else if (posValues[x] & POSSESSIVE && posValues[x-1] & NOUN_BITS) --x;	// fine duo
						else break;	// useless to us
					}
				}
	
				// "a flying wedge, the *eagle soared" is other order  of appoistive
				// MIGHT be appositive, or might be run-on 
			}
			break;
			case PREPOSITION: // PREPOSITION
			{
				needRoles[roleIndex] &= -1 ^ SUBJECT_COMPLEMENT;	// cannot have linking verb result after this
				if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) --roleIndex;	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this

				AddPhrase(i);// uncovered preposition via ambiguity

				if (needRoles[roleIndex-1] & MAINOBJECT && subjectStack[MAINLEVEL] > verbStack[MAINLEVEL]){;} // if main subject comes after main verb (question), then prep phrase cancels nothing
				else if (needRoles[roleIndex-1] & (OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT) && !designatedObjects[verbStack[roleIndex-1]]) // phrase should not intervene before direct object unless describing indirect object
				{
					needRoles[roleIndex-1] &= -1 ^ (INDIRECTOBJECT2|MAININDIRECTOBJECT|OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT);
				}

				// ENDING of sentence preposition wrapped to front
				if (i == lastRealWord && roles[firstRealWord] == MAINOBJECT  && posValues[firstRealWord] & (PRONOUN_SUBJECT|PRONOUN_OBJECT)  && canonicalLower[firstRealWord] && canonicalLower[firstRealWord]->properties & QWORD )
				{
					SetRole(firstRealWord,OBJECT2);
					objectStack[MAINLEVEL] = (unsigned char)0; // implied higher level for prep phrase, so disable on level 1
					phrases[firstRealWord] = phrases[lastRealWord]; // same marker
				}
			}
			break;
			case CONJUNCTION_COORDINATE: // CONJUNCTION_COORDINATE
			{
				if (i == 1) break;	// ignore coordination since it occurs to PRIOR sentence.
				HandleCoordConjunct(i,changed);
				if (!(roles[i] & (CONJUNCT_ADJECTIVE|CONJUNCT_ADVERB|CONJUNCT_PARTICLE)) && needRoles[roleIndex] & VERBAL && needRoles[roleIndex] & OBJECT2) // also for if we didnt decide its role
				{
					lastVerbal = 0; // cancel existing verbal which is waiting for an object
					--roleIndex;
				}
				if (roles[i] == CONJUNCT_SENTENCE) // we want a whole new sentence
				{
					FinishSentenceAdjust(false,changed,i);
					InitRoleSentence(i+1);
				}
			}
			break;
			case CONJUNCTION_SUBORDINATE:  // any clause will have been started by StartClause because many start w/o conjunction
			{
			}
			break;
			case PARTICLE: //  particle binds to PRIOR verb
			{
				unsigned int x = i;
				while (--x)
				{
					if (bitCounts[x] != 1) x = 1; // we dont know it yet
					else if (posValues[x] & (VERB_TENSES|NOUN_INFINITIVE|NOUN_GERUND)) break;
				}
				if (x) // the clausal verb we assume
				{
					ExtendChunk(x,i,clauses);
					ExtendChunk(x,i,verbals);
					if (!phrases[i] && posValues[x] & NOUN_GERUND) ExtendChunk(x,i,phrases);
				}
			}
			break;
			case ADVERB_NORMAL: // adverb  binds to PRIOR verb, not next verb unless next to next verb ("if this is tea *please bring me sugar")
			{
				if (posValues[i+1] & ADJECTIVE_BITS) {;} // if next is ADJECTIVE, then it binds to adjective?  "I like *very big olives"
				else if (posValues[i+1] & VERB_TENSES) //  next to next verb ("if this is tea please bring me sugar")
				{
					if (clauses[i+1]) clauses[i] = clauses[i+1];
					if (verbals[i+1]) verbals[i] = verbals[i+1];
				}
				else // bind to prior verb
				{
					if (clauses[i-1]) clauses[i] = clauses[i-1];
					if (verbals[i-1]) verbals[i] = verbals[i-1];
					if ( phrases[i-1]  && !phrases[i] && posValues[i-1] & NOUN_GERUND) phrases[i] = phrases[i-1];
				}
				
				if (!strcmp(wordStarts[i],"not")) SetRole(i,NOT);
			}
			break;
			case COMMA: // close needs for indirect objects, ends clauses e.g.   after being reprimanded, she went home
			{
				needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT;
				++currentZone; 
				// MAY NOT close needs for indirect objects, ends clauses because adjectives separated by commas) - hugging the cliff, Nathan follows the narrow, undulating road.
				if (posValues[i-1] & ADJECTIVE_BITS && posValues[i+1] & (NOUN_BITS|ADVERB_BITS|ADJECTIVE_BITS|CONJUNCTION_COORDINATE)) break; 
				
				// objects cannot pass thru comma boundaries unless they get reinstated by handleconjunct from the earlier side role
				if (needRoles[roleIndex] & (INDIRECTOBJECT2|OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|OBJECT_COMPLEMENT)) needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2 | OBJECT2 | MAINOBJECT | MAINOBJECT|OBJECT_COMPLEMENT);
				// one might have a start of sentence commas off from rest, like: After I arrived, he left and went home.
				// one might have a middle part like: I find that often, to my surprise, I like the new food.
				// one might have a tail part like:  I like you, but so what?
				// or comma might be in a list:  I like apples, pears, and rubbish.

				// one might have a start of sentence commas off from rest, like: After I arrived, he left and went home.
				// one might have a middle part like: I find that often, to my surprise, I like the new food.
				// one might have a tail part like:  I like you, but so what?
				// or comma might be in a list:  I like apples, pears, and rubbish.
				if (!startComma) // we have a comma, find a close.  if this is a comma list, potential 3d comma might exist
				{
					startComma = i;
					for (unsigned int j = i+1; j < lastRealWord; ++j) 
					{
						if (posValues[j] & COMMA) // this comma ends first here
						{
							endComma = j;
							break;
						}
					}
				}
				// if we already had a comma and this is the matching end we found for it and we are in a comma-phrase, end it.
				else if (i == endComma && needRoles[roleIndex] & COMMA_PHRASE) 
				{
					--roleIndex; // end phrase
					if (roleIndex == 0) ++roleIndex;
				}

				// commas end some units like phrases, clauses and infinitives

				if (!commalist) // was this a comma list (1st time init)
				{
					if (FindCoordinate(i)) commalist = 1; // yes
					else commalist = (unsigned int)-1;		// conjunction coordinate never found
				}
				if (commalist == 1) // treat this as coordinate conjunction (but skip over possible adj conjunct)
				{
					if (!(posValues[i+1] & CONJUNCTION_COORDINATE)) // when you get , and  --- want it on the and (for type) and not the comma
					{
						HandleCoordConjunct(i,changed);
						if (roles[i] == CONJUNCT_SENTENCE) // start a new sentence now, after cleanup of old sentence
						{
							FinishSentenceAdjust(false,changed,i);
							InitRoleSentence(i+1);
						}
					}
				}
				else 
				{
					needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2 | OBJECT2);
				}
			} 
				break;
			case TO_INFINITIVE:
				// adverb verbal to express purpose of verb (why) but looks like adjective verbal as well:  "he bought flowers *to give his wife"
				// after verbs of thinking and feeling and saying : "he decided *to eat"
				// some (causal_to_infinitive_verbs) verbs take direct object + to infinitive: "he encouraged his friends *to vote"  as object complement
				// some subject complement adjectives take it to give a reason (~adjectivecomplement_taking_noun_infinitive) : " he was unhappy *to live" also treated as object complement
				{
					uint64 toBits = canSysFlags[verbStack[roleIndex]] & (DIRECTOBJECT_TOINFINITIVE_COMPLEMENT | TOINFINITIVE_DIRECTOBJECT);

					// adjective expects a modifier infinitive phrase
					if (posValues[i-1] & ADJECTIVE_NORMAL && parseFlags[i-1] & ADJECTIVE_TAKING_NOUN_INFINITIVE) SetRole(i,ADJECTIVE_COMPLEMENT);
					// needs object and causal verb is particularly happy with to infinitive "we agreed to plan"
					else if (toBits & TOINFINITIVE_DIRECTOBJECT && needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) // I like *to swim
					{
						LimitValues(i+1,NOUN_INFINITIVE,"following TO when verb uses to xxx as object",changed);
						SetRole(i,needRoles[roleIndex] & (OBJECT2|MAINOBJECT));
					}
					else if (toBits & DIRECTOBJECT_TOINFINITIVE_COMPLEMENT && !(needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) && roles[i-1] & (MAINOBJECT|OBJECT2)) 
					{
						LimitValues(i+1,NOUN_INFINITIVE,"following TO when verb uses to xxx as object complement",changed);
						SetRole(i,OBJECT_COMPLEMENT);
					}
					else if (!roles[i] && posValues[i+1] & NOUN_INFINITIVE && needRoles[roleIndex] & (OBJECT2|MAINOBJECT))
					{
						LimitValues(i+1,NOUN_INFINITIVE,"following TO when need object, will be noun_infinitive",changed);
						SetRole(i,needRoles[roleIndex] & (OBJECT2|MAINOBJECT));
					}
					else if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2))
					{
						LimitValues(i+1,NOUN_INFINITIVE,"following TO when need object, will be noun_infinitive",changed);
						SetRole(i,needRoles[roleIndex] & (MAINOBJECT|OBJECT2));
					}
					else if (posValues[i-1] & (NOUN_SINGULAR)) // postnominal adjective
					{
						SetRole(i,POSTNOMINAL_ADJECTIVE);
					}
					// trailing or embedded appositive -- "his wish, to eat, was fulfilled"
					else if (i > 2 && posValues[i+1] & NOUN_INFINITIVE && roles[i-2] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT) && posValues[i-1] == COMMA)
					{
						LimitValues(i+1,NOUN_INFINITIVE,"following TO when following object and comma, will be appositive noun_infinitive",changed);
						SetRole(i,APPOSITIVE);
					}
					// direct appositive -- "it was a mistake *to elect him" -- no postnominal adjective
					//else if (i > 2 && posValues[i+1] & NOUN_INFINITIVE && roles[i-1] & (OBJECT2|MAINOBJECT))
					//{
					//	LimitValues(i+1,NOUN_INFINITIVE,"following TO when following object, will be appositive noun_infinitive",changed);
					//	SetRole(i,APPOSITIVE);
					//}
					AddVerbal(i);
				}
				break;
			case FOREIGN_WORD:
				break;
			case DETERMINER: case PREDETERMINER:
				requireNoun = true;
				break;
			case PRONOUN_POSSESSIVE: case POSSESSIVE:
				requireNoun = true;
				break;
			case THERE_EXISTENTIAL:
				SetRole(i,MAINSUBJECT); // have to overrule it later
				break;
			case PAREN:
				break;
			default:  // UNKNOWN COMBINATIONS (undecided values) 
			{
				if (posValues[i] & (NOUN_BITS|VERB_TENSES)) return false;	// unresolved noun or verb conflict
			}
		}// end of switch
		if (bitCounts[i] != 1 && !requireNoun)	// now still confused about futures....dont risk altering things later on except if we are hunting for a noun
		{
			// if conflict is merely adjective vs adverb, particle, etc, ignore it for now since its outcome doesn't determine much
			if (posValues[i] & (FULL_NOUN_BITS|VERB_TENSES)) return false;
		}
		CloseLevel(i);

		// see if we can assign a pending verbal
		if (determineVerbal && !phrases[i] && !clauses[i] && !verbals[i] && wordStarts[i][0] != ',') // we are back to main sentence
		{
			if (bitCounts[i] != 1) {;} // we dont know what this is
			else if (posValues[i] & (NOUN_BITS | PRONOUN_SUBJECT | PRONOUN_OBJECT)) // we find a noun, it must have described it:  "Walking home, he left me"
			{
				LimitValues(determineVerbal,ADJECTIVE_PARTICIPLE,"says verbal is adjective",changed);
			}
			else if (posValues[i] & (AUX_VERB_BITS | VERB_TENSES)) // we find a verb, it must have been the subject: "Walking home is fun"
			{
				LimitValues(determineVerbal,NOUN_GERUND,"says verbal is gerund",changed);
				SetRole(determineVerbal,SUBJECT2);
			}
			determineVerbal = 0;
		}
	}

	if (trace & POS_TRACE)
	{
		for (unsigned int x = roleIndex; x >= 1; --x)
		{
			DecodeneedRoles(x,goals);
			Log(STDUSERLOG,"      - leftover want %d: %s\r\n",x,goals);
		}

		Log(STDUSERLOG,"        ->  subject:%s  verb:%s  indirectobject:%s  object:%s  lastclause@:%s  lastverbal@:%s\r\n",wordStarts[subjectStack[MAINLEVEL]],wordStarts[verbStack[MAINLEVEL]],wordStarts[indirectObjectStack[MAINLEVEL]],wordStarts[objectStack[MAINLEVEL]],wordStarts[lastClause],wordStarts[lastVerbal]);
		Log(STDUSERLOG,"PreFinishSentence: ");
		for (unsigned int i = 1; i <= wordCount; ++i)
		{
			char word[MAX_WORD_SIZE];
			char* role = GetRole(i);
			if (!*role) 
			{
				role = "";
				if (bitCounts[i] != 1) 
				{
					*word = 0; 
					Tags(word,i);
					role = word;
				}
			}
			Log(STDUSERLOG,"%s (%s) ",wordStarts[i],role);
		}
		Log(STDUSERLOG,"\r\nZones: ");
		for (unsigned int i = 0; i < zoneIndex; ++i) ShowZone(i);
		Log(STDUSERLOG,"\r\n");
	}

	bool resolved = true;
	startSentence = 1;
 	FinishSentenceAdjust(resolved,changed,lastRealWord);
	ValidateSentence(resolved);
	if (!resolved && !changed && noReact) printf("input: %s\r\n",currentInput); //for debugging shows what sentences failed to be resolved by parser acceptibly
	return resolved;
}

void English_ParseSentence(bool & resolved,bool &changed)
{
	resolved =  AssignRoles(changed); 
}
#endif
