// markSystem.cpp - annotates the dictionary with what words/concepts are active in the current sentence

#include "common.h"

#ifdef INFORMATION

For every word in a sentence, the word knows it can be found somewhere in the sentence, and there is a 64-bit field of where it can be found in that sentence.
The field is in a hashmap and NOT in the dictionary word, where it would take up excessive memory.

Adjectives occur before nouns EXCEPT:
	1. object complement (with some special verbs)
	2. adjective participle (sometimes before and sometimes after)

In a pattern, an author can request:
	1. a simple word like bottle
	2. a form of a simple word non-canonicalized like bottled or apostrophe bottle
	3. a WordNet concept like bottle~1 
	4. a set like ~dead or :dead

For #1 "bottle", the system should chase all upward all sets of the word itself, and all
WordNet parents of the synset it belongs to and all sets those are in. 

Marking should be done for the original and canonical forms of the word.

For #2 "bottled", the system should only chase the original form.

For #3 "bottle~1", this means all words BELOW this in the wordnet hierarchy not including the word
"bottle" itself. This, in turn, means all words below the particular synset head it corresponds to
and so instead becomes a reference to the synset head: "0173335n" or some such.

For #4 "~dead", this means all words encompassed by the set ~dead, not including the word ~dead.

So each word in an input sentence is scanned for marking. 
the actual word gets to see what sets it is in directly. 
Thereafter the system chases up the synset hierarchy fanning out to sets marked from synset nodes.

#endif

Bit64Map whereInSentence; // hashmap of each word in a sentence and where you can find it
Bit64Map triedMeaning; // hashmap of each word in a sentence and what meanings of it have been marked

// mark debug tracing
bool showMark = false;
static unsigned int markLength = 0; // prevent long lines in mark listing trace
#define MARK_LINE_LIMIT 80

unsigned int matchStamp = 1; // first real is 2. not collide with 1 used for uservar list
char unmarked[MAX_SENTENCE_LENGTH]; // can completely disable a word from mark recognition

void SetPatternStamp(WORDP D)
{
	D->v.patternStamp = matchStamp;
	if (showMark) Log(STDUSERLOG,"Mark: %s %d\r\n",D->word,matchStamp);
}

void SetWhereInSentence(WORDP D,uint64 bits)
{
	whereInSentence[Word2Index(D)] = bits;
}

uint64 GetWhereInSentence(WORDP D)
{
	return whereInSentence[Word2Index(D)];  
}

void ClearWhereInSentence()
{
	whereInSentence.clear();
	NextMatchStamp();
}

void SetTried(WORDP D,uint64 bits)
{
	triedMeaning[Word2Index(D)] = bits;
}

uint64 GetTried(WORDP D)
{
	return triedMeaning[Word2Index(D)];
}

void ClearTried()
{
	triedMeaning.clear();
}

void RemoveMatchValue(WORDP D, uint64 val, unsigned int position)
{
	if (position > PHRASE_MAX_START_OFFSET) return;

	//   see if data on this position is already entered
	int shift = 0;
	uint64 has;
	uint64 newval = val;

	while (shift < PHRASE_STORAGE_BITS && (has = (newval & PHRASE_FIELD )) != 0) 
	{
		unsigned int began = has & PHRASE_START; // his 6bit field starts here
		if ( began == position) // we need to erase him
		{
			uint64 prior;
			prior = newval >> PHRASE_BITS;	// the top part with us removed
			unsigned int inverseShift = 64 - shift;
			if (shift > 31)
			{
				prior <<= 31;
				shift -= 31;
			}
			prior <<= shift; // put it back now

			uint64 after = val;
			if (inverseShift > 31) // shifts beyond 31 bits are unreliable in linux compiler
			{
				inverseShift -= 31;
				after <<= 31;
				after <<= inverseShift; 
				after >>= 31;
				after >>= inverseShift; 
			}
			else
			{
				after <<= inverseShift; 
				after >>= inverseShift; 
			}
			val = prior | after;
			SetWhereInSentence(D,val);
			break;
		}

		shift += PHRASE_BITS;  // 9 bits per phrase: 
		newval >>= PHRASE_BITS;  // incremental shift because some compilers dont do > 32 bit shifts properly
	}
}

static uint64 AddMatchValue(uint64 val, uint64 mask)
{
	//   see if data on this position is already entered
    int shift = 0;
    uint64 has;
	unsigned int start = mask & PHRASE_START;
	bool added = true;
	uint64 newval = val;
    while (shift < PHRASE_STORAGE_BITS && (has = (newval & PHRASE_FIELD )) != 0)  // 9 bits per field, 6 start + 3 end
    {
		unsigned int began = has & PHRASE_START; // his 6bit field starts here
		if ( began > start) // we need to go before him -- sorted matches
		{
			if ( val & PHRASE_LAST_FIELD) return 0;	// we cant insert it
			added = true;

			// get the bits before us, into correct position
			unsigned int inverseShift = 64 - shift;

			//  get the bits AFTER us
			uint64 after = val;
			if (inverseShift > 31) // shifts beyond 31 bits are unreliable in linux compiler
			{
				inverseShift -= 31;
				after <<= 31;
				after <<= inverseShift; 
				after >>= 31;
				after >>= inverseShift; 
			}
			else
			{
				after <<= inverseShift; 
				after >>= inverseShift; 
			}
			uint64 prior = newval << PHRASE_BITS; // create the open slot
			if (shift > 32)
			{
				mask <<= 32;
				shift -= 32;
				prior <<= 32 ;	// the top part shifted out of the way
			}
			mask <<= shift;  // move to position
			prior <<= shift;	// the top part shifted out of the way
			val = prior | after | mask;
			return val; // new value to set
		}

        if (has == mask) return 0; // no need to add
        shift += PHRASE_BITS;  // 9 bits per phrase: 
		newval >>= PHRASE_BITS;
    }

	//   shift is left spot open slot exists (if one does)
    if (shift <= (PHRASE_STORAGE_BITS - PHRASE_BITS) ) // bugs in compilers with 64 bit shift
    {
		added = true;
		if (shift > 32)
		{
			mask <<= 32;
			shift -= 32;
		}
		mask <<= shift;
		return val | mask;
	}
	return 0;
}

void MarkWordHit(WORDP D, unsigned int start,unsigned int end)
{	//   keep closest to start at bottom, when run out, drop later ones 
    //   field is 64 bit. 
	//   field can hold 7     9bit items
    if (!D || !D->word) return;
    if (start > PHRASE_MAX_START_OFFSET) return; //   leave room for bits on top -- using 5 bits for start and 3 bits for width
	if (end > wordCount) end = wordCount;    
	if (start > wordCount) 
	{
		ReportBug("save position is too big")
		return;
	}

    int diff = end - start;
	// diff < 0 means peering INSIDE a multiword token before last word
	// we label END as the word before it (so we can still see next word) and START as the actual multiword token
    if (diff < 0) diff = PHRASE_END_MAX; 
    else if (diff >= PHRASE_END_MAX) return;  //   refuse things too wide
 
    int realEnd = (diff == PHRASE_END_MAX) ? (start-1) : (start + diff); 
    if (realEnd > (int)wordCount) 
    {
        ReportBug("Position out of range as set")
        return;
    }

	uint64 mask = start | (diff << PHRASE_END_OFFSET);  // this unit looks like this
    uint64 val;
    if (D->v.patternStamp != matchStamp) //   accidently match means a bogus reference, dont care
    {
        val = 0; 
		SetPatternStamp(D);
    }
	else val = GetWhereInSentence(D);
	uint64 newval = AddMatchValue(val,mask);
	if (newval) SetWhereInSentence(D,newval);

	if (( trace & (PREPARE_TRACE|HIERARCHY_TRACE)  || prepareMode == PREPARE_MODE ) && newval)  
	{
		markLength += D->y.length;
		if (markLength > MARK_LINE_LIMIT)
		{
			markLength = 0;
			Log(STDUSERLOG,"\r\n");
			Log(STDUSERTABLOG,"");
		}
		Log(STDUSERLOG,(D->systemFlags & TOPIC_NAME) ? " +T%s " : " +%s",D->word);
		Log(STDUSERLOG,(start != end) ? "(%d-%d)": "(%d) ",start,end);
	}
}

unsigned int GetIthSpot(WORDP D,int i)
{
    if (!D || D->v.patternStamp != matchStamp) return 0; //   not in sentence
	uint64 spot = GetWhereInSentence(D);

	unsigned int start = (unsigned int) (spot & PHRASE_START);
	while (start && --i) //   none left or not the ith choice
	{
		spot >>= PHRASE_BITS;	//   look at next slot
		start = (unsigned int) (spot & PHRASE_START);
	}
	if (!start) return 0;	//   failed to find any matching entry
    if (start > wordCount) //   bad cache- timestamp rollover?
    {
        ReportBug("Position out of range ith1")
        return 0;
    }
 
    //   match found
    positionStart =  positionEnd = start;

	//   check to see if its a range value -
	unsigned int offset = (unsigned int)((spot >> PHRASE_END_OFFSET) & PHRASE_END_MAX); // end offset from start
    if (offset == PHRASE_END_MAX) --positionEnd; // we mark BEFORE the composite word
    else positionEnd += offset;
    if (positionEnd > (int)wordCount) ReportBug("Position out of range ith2")
    return positionStart;
}

unsigned int GetNextSpot(WORDP D,int start,unsigned int &positionStart,unsigned int& positionEnd)
{//   spot can be 1-31,  range can be 0-7 -- 7 means its a string, set last marker back before start so can rescan
	//   BUG - we should note if match is literal or canonical, so can handle that easily during match eg
	//   '~shapes matches square but not squares (whereas currently literal fails because it is not ~shapes
    if (!D || D->v.patternStamp != matchStamp) return 0; //   not in sentence
	uint64 spot = GetWhereInSentence(D);
	int value = (spot & PHRASE_START); // start of the match (1st choice)
	if (value > (int)wordCount) ReportBug("start position start out of range1 %d of %d\r\n",value,wordCount) //   bad cache- timestamp rollover?
	while (spot && (value <= start || unmarked[value])) //   no match, shift positions
	{
		spot >>= PHRASE_BITS;	//   look at next slot
		value = spot & PHRASE_START;
		if (value > (int)wordCount) ReportBug("start position out of range1 %d of %d\r\n",value,wordCount) //   bad cache- timestamp rollover?
 	}
	if (!value || unmarked[value]) return 0;	//   failed to find any matching entry or marks all turned off for this
    if (value > (int)wordCount)   return 0; //   bad cache- timestamp rollover?
 
    //   match found
    positionStart =  positionEnd = (int) value;

	//   check to see if its a range value -
	// NORMAL phrases have a length offset past start and return a position BEFORE their start, so next word in line could also match separately.
	// A phrase which is MAX length is special and marks itself as ending BEFORE it starts.
	value = ((spot >> PHRASE_END_OFFSET) & PHRASE_END_MAX);
    if (value == PHRASE_END_MAX) --positionEnd;
    else positionEnd += (int) value;
    if (positionEnd > (int)wordCount) 
    {
        ReportBug("end Position out of range")
        positionEnd = wordCount;
    }
    return positionStart;
}

static int MarkSetPath(MEANING M, unsigned int start, unsigned  int end, unsigned int depth, bool canonical) //   walks set hierarchy
{//   travels up concept/class sets only, though might start out on a synset node or a regular word
	unsigned int flags = M & ESSENTIAL_FLAGS;
	if (!flags) flags = ESSENTIAL_FLAGS;
	WORDP D = Meaning2Word(M);
	unsigned int index = Meaning2Index(M); // always 0 for a synset or set
	
	// check for any repeated accesses of this synset or set or word
	uint64 offset = 1 << index;
	uint64 tried = GetTried(D);
 	if (D->inferMark == inferMark) // been thru this word recently
	{
		if (tried & offset)	return -1;	// done this branch already
	}
	else //   first time accessing, note recency and clear tried bits
	{
		D->inferMark = inferMark;
		SetTried(D,0);
		tried = 0;
	}
 	SetTried(D,tried |offset);

	int result = 0;
	FACT* F = GetSubjectHead(D); 
	while (F)
	{
		if (F->verb == Mmember) // ~concept members and word equivalent
		{
			char word[MAX_WORD_SIZE];
			char* fact;
			if (trace == HIERARCHY_TRACE)  
			{
				fact = WriteFact(F,false,word); // just so we can see it
				unsigned int hold = globalDepth;
				globalDepth = depth;
				Log(STDUSERTABLOG,"%s   ",fact); // \r\n
				globalDepth = hold;
			}
			unsigned int restrict = F->subject & TYPE_RESTRICTION;
			if (restrict && !(restrict & flags)) {;} // type restriction in effect for this concept member
			else if (canonical && F->flags & ORIGINAL_ONLY) {;} // incoming is not original words and must be

			//   index meaning restriction (0 means all)
			else if (index == Meaning2Index(F->subject)) // match generic or exact subject 
			{
				// test for word not included in set
				WORDP E = Meaning2Word(F->object); // this is a topic or concept
				bool mark = true;
				if (E->systemFlags & HAS_EXCLUDE) // set has some members it does not want
				{
					FACT* G = GetObjectHead(E);
					while (G)
					{
						if (G->verb == Mexclude) // see if this is marked for this position, if so, DONT trigger topic
						{
							WORDP S = Meaning2Word(G->subject);
							unsigned int positionStart,positionEnd;
							if (GetNextSpot(S,start-1,positionStart,positionEnd) && positionStart == start && positionEnd == end)
							{
								mark = false;
								break;
							}
						}
						G = GetObjectNext(G);
					}
				}

				if (mark)
				{
					MarkWordHit(E,start,end);
					if (MarkSetPath(F->object,start,end,depth+1,canonical) != -1) result = 1; // someone marked
				}
			}
		}
		F = GetSubjectNext(F);
	}
	return result;
}

static void RiseUp(MEANING M,unsigned int start, unsigned int end,unsigned int depth,bool canonical) //   walk wordnet hierarchy above a synset node
{	// M is always a synset head 
	M &= -1 ^ SYNSET_MARKER;
	unsigned int index = Meaning2Index(M);
	WORDP D = Meaning2Word(M);
	WORDP X;
	char word[MAX_WORD_SIZE];
	sprintf(word,"%s~%d",D->word,index);
	X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
	if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern

	sprintf(word,"%s~%d",D->word,index);
	X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
	if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern


	// now spread and rise up
	if (MarkSetPath(M,start,end,depth,canonical) == -1) return; // did the path
	FACT* F = GetSubjectHead(D); 
	while (F)
	{
		if (F->verb == Mis && (index == 0 || F->subject == M)) RiseUp(F->object,start,end,depth+1,canonical); // allowed up
		F = GetSubjectNext(F);
	}
}

static void MarkSequenceTitleFacts(MEANING M, unsigned int start, unsigned int end,bool canonical) // title phrases in sentence
{
    if (!M) return;
	WORDP D = Meaning2Word(M);
	if (D->properties & NOUN_TITLE_OF_WORK && canonical) return; // accidental canonical match. not intended

	if (D->properties & PART_OF_SPEECH) // mark pos data
	{
		uint64 bit = START_BIT;
		for (int j = 63; j >= 0; --j)
		{
			if (D->properties & bit) MarkFacts(posMeanings[j],start,end,canonical,(D->properties & NOUN_TITLE_OF_WORK && !canonical) ? false : true); // treat original title as a full normal word
			bit >>= 1;
		}
	}

	MarkFacts(M,start,end,canonical,true);
}

void MarkFacts(MEANING M,unsigned int start, unsigned int end,bool canonical,bool sequence) 
{ // M is always a word or sequence from a sentence

    if (!M) return;
	WORDP D = Meaning2Word(M);
	if (!sequence || D->properties & (PART_OF_SPEECH|NOUN_TITLE_OF_WORK|NOUN_HUMAN) || D->systemFlags & PATTERN_WORD) MarkWordHit(D,start,end); // if we want the synset marked, RiseUp will do it.
	int result = MarkSetPath(M,start,end,0,canonical); // generic membership of this word all the way to top
	if (sequence && result == 1) MarkWordHit(D,start,end); // if we want the synset marked, RiseUp will do it.
	WORDP X;
	char word[MAX_WORD_SIZE];
	if (M & NOUN) // BUG- this wont work up the ontology, only at the root of what the script requests
	{
		sprintf(word,"%s~n",D->word);
		X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
		if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern
	}
	if (M & VERB)
	{
		sprintf(word,"%s~v",D->word);
		X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
		if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern
	}
	if (M & ADJECTIVE)
	{
		sprintf(word,"%s~a",D->word);
		X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
		if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern
	}
	if (M & ADVERB)
	{
		sprintf(word,"%s~b",D->word);
		X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
		if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern
	}
	if (M & PREPOSITION)
	{
		sprintf(word,"%s~p",D->word);
		X = FindWord(word,0,PRIMARY_CASE_ALLOWED);
		if (X) 	MarkWordHit(X,start,end); // direct reference in a pattern
	}

	//   now follow out the allowed synset hierarchies 
	unsigned int index = Meaning2Index(M);
	unsigned int size = GetMeaningCount(D);
	uint64 flags = M & ESSENTIAL_FLAGS;
	if (!flags) flags = ESSENTIAL_FLAGS; // unmarked ptrs can rise all branches
	for  (unsigned int k = 1; k <= size; ++k) 
	{
		M = D->meanings[k]; // it is a flagged meaning unless it self points

		// walk the synset words and see if any want vague concept matching like dog~~
		MEANING T = M;
		unsigned int n = (index && k != index) ? 30 : 0;	// only on this meaning or all meanings
		while (n < 20)
		{
			WORDP X = Meaning2Word(T);
			unsigned int ind = Meaning2Index(T);
			sprintf(word,"%s~~",X->word);
			WORDP V = FindWord(word,0,PRIMARY_CASE_ALLOWED);
			if (V) 	MarkWordHit(V,start,end); // direct reference in a pattern
			T = GetMeanings(X)[ind];
			if (T == M ) break; // end of loop
		}

		M = (M & SYNSET_MARKER) ? MakeMeaning(D,k) : GetMaster(M); // we are the master itself or we go get the master
		RiseUp(M,start,end,0,canonical); // allowed meaning pos (self ptrs need not rise up)
	}
}

static void SetSequenceStamp() //   mark words in sequence, original and canonical (but not mixed) - detects proper name potential up to 5 words
{
	char* rawbuffer = AllocateBuffer();
	char* canonbuffer1 = AllocateBuffer();
	unsigned int oldtrace = trace;
	unsigned int usetrace = trace;
	char* buffer2 = AllocateBuffer();
	if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) 
	{
		Log(STDUSERLOG,"\r\n    sequences=\r\n");
		usetrace = (unsigned int) -1;
	}

	//   consider all sets of up to 3-in-a-row 
	int limit = ((int)wordCount) - 1;
	for (int i = 1; i <= limit; ++i)
	{
		if (!IsAlphaOrDigit(*wordStarts[i])) continue; // we only composite words, not punctuation or quoted stuff

		//   set base phrase
		*rawbuffer = 0;
		canonbuffer1[0] = 0;
		strcat(rawbuffer,wordStarts[i]);
		strcat(canonbuffer1,wordCanonical[i]);
       
		//   fan out for addon pieces
		unsigned int k = 0;
		int index = 0;
		uint64 logbase = logCount; // see if we logged anything
		while ((++k + i) <= wordCount)
		{
	
			strcat(rawbuffer,"_");
			strcat(canonbuffer1,"_");
			strcat(rawbuffer,wordStarts[i+k]);
			strcat(canonbuffer1,wordCanonical[i+k]);

			if (!IsAlphaOrDigit(*wordStarts[i+k])) break; // we only composite words, not punctuation or quoted stuff except we can include them as part of something
			NextinferMark();

			// special hack for infinitive phrases: to swim  etc
			if (k == 1 && *rawbuffer == 't' && rawbuffer[1] == 'o'  && rawbuffer[2] == '_' )
			{
				WORDP D = FindWord(wordCanonical[i+1],0,LOWERCASE_LOOKUP);
				if (D)
				{
					trace = (D->subjectHead || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0; // being a subject head means belongs to some set. being a marked word means used as a keyword
					if (D->properties & VERB) MarkFacts(MakeMeaning(FindWord("~noun_infinitive")),i,i+k);
				}
			}

			// for now, accept upper and lower case forms of the decomposed words for matching
			// storeword instead of findword because we normally dont store keyword phrases in dictionary
			MakeLowerCopy(buffer2,rawbuffer);
			WORDP D = FindWord(buffer2,0,LOWERCASE_LOOKUP); 
			if (D)
			{
				trace = (D->subjectHead || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0; // being a subject head means belongs to some set. being a marked word means used as a keyword
				MarkFacts(MakeMeaning(D),i,i+k,false,true); 
			}
			MakeUpperCopy(buffer2,rawbuffer);
			D = FindWord(buffer2,0,UPPERCASE_LOOKUP);
			if (D)
			{
				trace = (D->subjectHead || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0;
				MarkSequenceTitleFacts(MakeMeaning(D),i,i+k,false);
			}
			MakeLowerCopy(buffer2,canonbuffer1);
			D = FindWord(buffer2,0,LOWERCASE_LOOKUP);
			if (D) 
			{
				trace = (D->subjectHead  || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0;
				MarkFacts(MakeMeaning(D),i,i+k,true,true); 
			}
			MakeUpperCopy(buffer2,canonbuffer1);
			D = FindWord(buffer2,0,UPPERCASE_LOOKUP);
			if (D)
			{
				trace = (D->subjectHead  || D->systemFlags & PATTERN_WORD || D->properties & PART_OF_SPEECH)  ? usetrace : 0;
				MarkSequenceTitleFacts(MakeMeaning(D),i,i+k,true); 
			}
			if (logCount != logbase && usetrace)  Log(STDUSERLOG,"\r\n"); // if we logged something, separate
			
			if (++index >= SEQUENCE_LIMIT) break; //   up thru 5 words in a phrase
		}
	}
	trace = oldtrace;
	FreeBuffer();
	FreeBuffer();
	FreeBuffer();
}

void NextMatchStamp() //   used as timestamp on an entire sentence
{
    ++matchStamp;   
}

static void StdMark(MEANING M, unsigned int start, unsigned int end) 
{
	if (!M) return;
	MarkFacts(M,start,end);		//   the basic word
	WORDP D = Meaning2Word(M);
	if (D->systemFlags & TIMEWORD && !(D->properties & PREPOSITION)) MarkFacts(MakeMeaning(Dtime),start,end);
}

void MarkAllImpliedWords()
{
	ChangeDepth(1,"MarkAllImpliedWords");
	unsigned int i;
 	for (i = 1; i <= wordCount; ++i)  capState[i] = IsUpperCase(*wordStarts[i]); // note cap state
	TagIt(); // pos tag and maybe parse

	if ( prepareMode == POS_MODE || prepareMode == PENN_MODE || prepareMode == POSVERIFY_MODE  || prepareMode == POSTIME_MODE) 
	{
		ChangeDepth(-1,"MarkAllImpliedWords");
		return;
	}

    if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,"\r\nConcepts: \r\n");
 	if (showMark) Log(STDUSERLOG,"----------------\r\n");
	markLength = 0;
	
	//   now mark every word in sentence
    for (i = 1; i <= wordCount; ++i) //   mark that we have found this word, either in original or canonical form
    {
		char* original =  wordStarts[i];
		if (showMark) Log(STDUSERLOG,"\r\n");
		NextinferMark(); // blocks circular fact marking. main mark is on matchstamp, this mark is error protection
 		if (trace  & (HIERARCHY_TRACE | PREPARE_TRACE) || prepareMode == PREPARE_MODE) Log(STDUSERLOG,"\r\n%d: %s raw= ",i,original);
	
		uint64 flags = posValues[i];
		if (flags & ADJECTIVE_NOUN) // transcribe back to noun
		{
			MarkFacts(MadjectiveNoun,i,i); 
			flags &= -1 ^ ADJECTIVE_NOUN;
			if (originalLower[i]) flags |= originalLower[i]->properties & (NOUN_SINGULAR|NOUN_PLURAL|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL);
		}
		finalPosValues[i] = flags; // these are what we finally decided were correct pos flags from tagger
		// put back non-tagger generalized forms of bits
		if (flags & FULL_NOUN_BITS) finalPosValues[i] |= NOUN;
		if (flags & (VERB_TENSES | NOUN_INFINITIVE| NOUN_GERUND)) finalPosValues[i] |= VERB;
		if (flags & AUX_VERB_BITS) finalPosValues[i] |= AUX_VERB;
		if (flags & ADJECTIVE_BITS) finalPosValues[i] |= ADJECTIVE;
		if (flags & ADVERB_BITS) finalPosValues[i] |= ADVERB;

		MarkTags(i);
#ifndef DISCARDPARSER
		MarkRoles(i);
#endif
	
		// mark general number property
		if (finalPosValues[i] & (NOUN_ORDINAL | NOUN_CARDINAL | ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL))  
		{
			MarkFacts(Mnumber,i,i); 
			//   handle finding fractions as 3 token sequence  mark as placenumber 
			if (*wordStarts[i+1] == '/' && wordStarts[i+1][1] == 0 && finalPosValues[i+2] & (NOUN_ORDINAL | NOUN_CARDINAL | ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL))
			{
				MarkFacts(MakeMeaning(Dplacenumber),i,i);  
				if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,"=%s/%s \r\n",wordStarts[i],wordStarts[i+2]);
			}
			else if (finalPosValues[i] & (NOUN_ORDINAL | NOUN_CARDINAL | ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL) && IsPlaceNumber(wordStarts[i]))
			{
				MarkFacts(MakeMeaning(Dplacenumber),i,i);  
			}
			// special temperature property
			char c = GetTemperatureLetter(original);
			if (c)
			{
				if (c == 'F') MarkFacts(MakeMeaning(StoreWord("~fahrenheit")),i,i);
				else if (c == 'C') MarkFacts(MakeMeaning(StoreWord("~celsius")),i,i);
				else if (c == 'K')  MarkFacts(MakeMeaning(StoreWord("~kelvin")),i,i);
				MarkFacts(Mnumber,i,i);
				char number[MAX_WORD_SIZE];
				sprintf(number,"%d",atoi(original));
				WORDP X = StoreWord(number,(NOUN_ORDINAL | NOUN_CARDINAL | ADJECTIVE_CARDINAL|ADJECTIVE_ORDINAL));
				wordCanonical[i] = X->word;
				canonicalLower[i] = X;
			}

			// special currency property
			char* number;
			char* currency = GetCurrency(wordStarts[i],number); 
			if (currency) 
			{
				MarkFacts(Mmoney,i,i); 
				if (*currency == '$') MarkFacts(MakeMeaning(StoreWord("$")),i,i);
				else if (*currency == 0xe2 && currency[1] == 0x82 && currency[2] == 0xac) MarkFacts(MakeMeaning(StoreWord("€")),i,i);
				else if (*currency == 0xc2 && currency[1] == 0xa5 ) MarkFacts(MakeMeaning(StoreWord("¥")),i,i);
				else if (*currency == 0xc2 && currency[1] == 0xa3 ) MarkFacts(MakeMeaning(StoreWord("£")),i,i);
			}
		}
	
        WORDP OL = originalLower[i];
		WORDP CL = canonicalLower[i];
 		WORDP OU = originalUpper[i]; 
        WORDP CU = canonicalUpper[i]; 
		if (!OU && !OL) OU = FindWord(original,0,UPPERCASE_LOOKUP);	 // can this ever work?
		if (!CU && original[1]) // dont convert single letters to upper case "a" if it hasnt already decided its not a determiner
		{
			CU = FindWord(original,0,UPPERCASE_LOOKUP);	// try to find an upper to go with it, in case we can use that, but not as a human name
			if (OU); // it was originally uppercase or there is no lower case meaning
			else if (CU && CU->properties & (NOUN_FIRSTNAME|NOUN_HUMAN)) CU = NULL;	// remove accidental names 
		}
	
		if (!OL && (!OU || !OU->properties) && !CL && (!CU || !CU->properties)) // allow unknown proper names to be marked unknown
		{
			MarkFacts(MakeMeaning(Dunknown),i,i); // unknown word
			MarkFacts(MakeMeaning(StoreWord(original)),i,i);		// allowed word
		}
		
		StdMark(MakeTypedMeaning(OL,0,(unsigned int)(finalPosValues[i] & TYPE_RESTRICTION)), i, i);
        if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG," // "); //   close original meanings lowercase

		markLength = 0;
		StdMark(MakeTypedMeaning(OU,0,(unsigned int)(finalPosValues[i] & TYPE_RESTRICTION)), i, i);
		
		if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) 
		{
			if (CL) Log(STDUSERLOG,"\r\n%d: %s canonical= ", i,CL->word ); //    original meanings lowercase
			else Log(STDUSERLOG,"\r\n%d: %s canonical= ", i,(CU) ? CU->word : "" ); //    original meanings uppercase
		}

		//   canonical word
  		StdMark(MakeTypedMeaning(CL,0, (unsigned int)(finalPosValues[i] & TYPE_RESTRICTION)), i, i);

 		markLength = 0;
	    if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG," // "); //   close canonical form lowercase
 		
		StdMark(MakeTypedMeaning(CU,0, (unsigned int)(finalPosValues[i] & TYPE_RESTRICTION)), i, i);

		// canonical word is a number (maybe we didn't register original right) eg. "how much is 24 and *seven"
		if (canonicalLower[i] && IsDigit(*canonicalLower[i]->word) && IsNumber(canonicalLower[i]->word)) MarkFacts(Mnumber,i,i);  

		if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG," "); //   close canonical form uppercase
		markLength = 0;
	
        //   peer into multiword expressions  (noncanonical), in case user is emphasizing something so we dont lose the basic match on words
        //   accept both upper and lower case forms . 
		// But DONT peer into something proper like "Moby Dick"
		unsigned int  n = BurstWord(wordStarts[i]); // peering INSIDE a single token....
		WORDP D,E;
		if (finalPosValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) n = 1;
		if (tokenControl & NO_WITHIN);  // peek within hypenated words
        else if (n >= 2 && n <= 4) //   longer than 4 is not emphasis, its a sentence - we do not peer into titles
        {
			static char words[5][MAX_WORD_SIZE];
			unsigned int k;
			for (k = 0; k < n; ++k) strcpy(words[k],GetBurstWord(k)); // need local copy since burstwords might be called again..

            for (unsigned int k = 0; k < n; ++k)
            {
  				unsigned int prior = (k == (n-1)) ? i : (i-1); //   -1  marks its word match INSIDE a string before the last word, allow it to see last word still
                E = FindWord(words[k],0,PRIMARY_CASE_ALLOWED); 
                if (E)
				{
					if (!(E->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) StdMark(MakeMeaning(E),i,prior);
					else MarkFacts(MakeMeaning(E),i,prior);
				}
                E = FindWord(words[k],0,SECONDARY_CASE_ALLOWED); 
				if (E)
				{
					if (!(E->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) StdMark(MakeMeaning(E),i,prior);
					else MarkFacts(MakeMeaning(E),i,prior);
				}
           }
        }

		// now look on either side of a hypenated word
		char* hypen = strchr(wordStarts[i],'-');
		if (hypen) 
		{
			MarkFacts(MakeMeaning(StoreWord(hypen)),i,i); // post form -colored
			char word[MAX_WORD_SIZE];
			strcpy(word,wordStarts[i]);
			word[hypen+1-wordStarts[i]] = 0;
			MarkFacts(MakeMeaning(StoreWord(word)),i,i); // pre form  light-
		}
		
		D = (CL) ? CL : CU; //   best recognition
		char* last;
		if (D && D->properties & NOUN && (last = strrchr(D->word,'_')) && finalPosValues[i] & NOUN) StdMark(MakeMeaning(FindWord(last+1,0)), i, i); //   composite noun, store last word as referenced also

        if (trace & PREPARE_TRACE || prepareMode == PREPARE_MODE) Log(STDUSERLOG,"\r\n");
    }
 
	//   check for repeat input by user - but only if more than 2 words or are unknown (we dont mind yes, ok, etc repeated)
	//   track how many repeats, for escalating response
	bool brief = (wordCount > 2);
	if (wordCount == 1 && !FindWord(wordStarts[1])) brief = true;
    unsigned int counter = 0;
    if (brief && humanSaidIndex) for (int j = 0; j < (int)(humanSaidIndex-1); ++j)
    {
        if (strlen(humanSaid[j]) > 5 && !stricmp(humanSaid[humanSaidIndex-1],humanSaid[j])) //   he repeats himself
        {
            ++counter;
            char buf[100];
			strcpy(buf,"~repeatinput");
			buf[12] = (char)('0' + counter);
			buf[13] = 0;
 			MarkFacts(MakeMeaning(FindWord(buf,0,PRIMARY_CASE_ALLOWED)),1,1); //   you can see how many times
        }
    }

	//   now see if he is repeating stuff I said
	counter = 0;
    if (wordCount > 2) for (int j = 0; j < (int)chatbotSaidIndex; ++j)
    {
        if (humanSaidIndex && strlen(chatbotSaid[j]) > 5 && !stricmp(humanSaid[humanSaidIndex-1],chatbotSaid[j])) //   he repeats me
        {
			if (counter < wordCount) ++counter;
			MarkFacts(MakeMeaning(FindWord("~repeatme",0,PRIMARY_CASE_ALLOWED)),counter,counter); //   you can see how many times
        }
    }

    //   handle phrases now
	markLength = 0;
    SetSequenceStamp(); //   squences of words

	ChangeDepth(-1,"MarkAllImpliedWords");
}
