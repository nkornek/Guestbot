 // tagger.cpp - used for pos tagging

#include "common.h"

unsigned int lowercaseWords;
unsigned int knownWords;
unsigned int tagRuleCount = 0;
uint64* tags = NULL;
char** comments = NULL;

char* wordCanonical[MAX_SENTENCE_LENGTH]; //   current sentence tokenization
WORDP originalLower[MAX_SENTENCE_LENGTH];
WORDP originalUpper[MAX_SENTENCE_LENGTH];
WORDP canonicalLower[MAX_SENTENCE_LENGTH];
WORDP canonicalUpper[MAX_SENTENCE_LENGTH];
uint64 finalPosValues[MAX_SENTENCE_LENGTH];
uint64 allOriginalWordBits[MAX_SENTENCE_LENGTH];	// starting pos tags in this word position
uint64 lcSysFlags[MAX_SENTENCE_LENGTH];      // current system tags lowercase in this word position (there are no interesting uppercase flags)
uint64 posValues[MAX_SENTENCE_LENGTH];			// current pos tags in this word position
uint64 canSysFlags[MAX_SENTENCE_LENGTH];		// canonical sys flags lowercase in this word position 

int itAssigned = 0;
int theyAssigned = 0;

void SetParserMarks(WORDP D,bool enable)// used for setting up parseFlags
{
	// set up special marks on the sets named in speical english attributes. Their direct members inherit their flag bit
	unsigned int bit = 1;
	FACT* F = GetObjectHead(D);
	while (F)
	{
		WORDP A = Meaning2Word(F->subject); // most recent first
		if (enable)
		{
			A->inferMark = bit;
			A->properties |= AS_IS;
		}
		else
		{
			A->inferMark = 0;
			A->properties &= -1 ^ AS_IS;
		}
		F = GetObjectNext(F);
		bit <<= 1;
	}
}

char* GetNounPhrase(int i,const char* avoid)
{
	static char buffer[MAX_WORD_SIZE];
	*buffer = 0;

	if (clauses[i-1] != clauses[i]) // noun is a clause
	{
		unsigned int clause = clauses[i];
		unsigned int at = i-1;
		while (clauses[++at] & clause)
		{
			char* word = wordStarts[at];							
			if (tokenFlags & USERINPUT) strcat(buffer,word);		
			else if (!stricmp(word,"my")) strcat(buffer,"your");	
			else if (!stricmp(word,"your")) strcat(buffer,"my");	
			else strcat(buffer,word);
			strcat(buffer," ");
		}
		size_t len = strlen(buffer);
		buffer[len-1] = 0;
		return buffer;
	}

	if (posValues[i+1] & (NOUN_INFINITIVE|NOUN_GERUND)) // noun is a verbal 
	{
		unsigned int verbal = verbals[i];
		unsigned int at = i-1;
		while (verbals[++at] & verbal)
		{
			char* word = wordStarts[at];							
			if (tokenFlags & USERINPUT) strcat(buffer,word);		
			else if (!stricmp(word,"my")) strcat(buffer,"your");	
			else if (!stricmp(word,"your")) strcat(buffer,"my");	
			else strcat(buffer,word);
			strcat(buffer," ");
		}
		size_t len = strlen(buffer);
		buffer[len-1] = 0;
		return buffer;
	}

	if (clauses[i-1] != clauses[i]) return wordStarts[i]; // cannot cross clause boundary
	if (verbals[i-1] != verbals[i]) return wordStarts[i]; // cannot cross verbal boundary
	if (phrases[i-1] != phrases[i]) return wordStarts[i]; // cannot cross phrase boundary

	int start = (int) i; // on the noun
	bool detseen = false;
	// NOTE posvalues still has adjectivenoun as adjective.  Finalposvalues has it as a noun.
	while (--start > 0 && posValues[start] & (NOUN_BITS | COMMA | CONJUNCTION_COORDINATE | ADJECTIVE_BITS | DETERMINER | PREDETERMINER | ADVERB_BITS | POSSESSIVE | PRONOUN_POSSESSIVE)) 
	{
		if (roles[start] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) break; // cannot switch to this
		if (posValues[start] & TO_INFINITIVE) break;
		if (posValues[start] & COMMA && !(posValues[start-1] & ADJECTIVE_BITS)) break; // NOT like:  the big, red, tall human
		if (posValues[start] & CONJUNCTION_COORDINATE)
		{
			if ( canonicalLower[start] && strcmp(canonicalLower[start]->word,"and")) break;	// not "and"
			if (!(posValues[start-1] & (ADJECTIVE_BITS|COMMA))) break;	// NOT like:  the big, red, and very tall human
			if (posValues[start-1] & COMMA && !(posValues[start-2] & ADJECTIVE_BITS)) break;	// NOT like:  the big, red, and very tall human
		}
		if (posValues[start] & NOUN_GERUND) break; 
		if (posValues[start] & ADVERB_BITS && !(posValues[start+1] & ADJECTIVE_BITS)) break;

		WORDP canon = canonicalLower[start];
		WORDP orig = originalLower[start];
		if (orig && (!strcmp("here",orig->word) || !strcmp("there",orig->word))) break;
		//if (orig && (!strcmp("this",orig->word) || !strcmp("that",orig->word) || !strcmp("these",orig->word) || !strcmp("those",orig->word))) break;
		if (canon && canon->properties & PRONOUN_BITS && !strcmp(canon->word,avoid)) break; // avoid recursive pronoun expansions... like "their teeth"
		if (posValues[start] & NOUN_PROPER_SINGULAR && posValues[start] != ADJECTIVE_NOUN) break; // proper singular blocks appostive 
	}
	
	// start is NOT a member
	while (++start <= i)
	{
		char* word = wordStarts[start];							
		if (tokenFlags & USERINPUT) strcat(buffer,word);		
		else if (!stricmp(word,"my")) strcat(buffer,"your");	
		else if (!stricmp(word,"your")) strcat(buffer,"my");	
		else strcat(buffer,word);
		if (start != i) strcat(buffer," ");
	}
	return buffer;
}

void DumpSentence(unsigned int from)
{
	unsigned int to = from;
	while (++to <= wordCount) if (roles[to] == CONJUNCT_SENTENCE) break;	// split here

#ifndef DISCARDPARSER
	unsigned int subjects[200];
	unsigned int verbs[200];
	unsigned int objects[200];
	unsigned int object2s[200];
	unsigned int indirectobjects[200];
	unsigned int subject = 0, verb = 0, indirectobject = 0, object = 0,object2 = 0;
	unsigned int i;
	bool notFound = false;
	for ( i = 1; i <= wordCount; ++i) // main sentence
	{
		if (i < from || i >= to) continue; // ignore
		if (roles[i] == MAINSUBJECT) subjects[subject++] = i;
		if (roles[i] == MAINVERB) verbs[verb++] = i;
		if (roles[i] == MAININDIRECTOBJECT) indirectobjects[indirectobject++] = i;
		if (roles[i] == MAINOBJECT) objects[object++] = i;
		if (roles[i] == OBJECT_COMPLEMENT && posValues[i] & NOUN_BITS) object2s[object2++] = i;
		if (roles[i] == SUBJECT_COMPLEMENT) objects[object++] = i;
		if (roles[i] == NOT) notFound = true;
	}
	if (!verb) return;	// didnt find a parse

	char* buffer = AllocateBuffer();
	*buffer = 0;
	strcat(buffer,"  MainSentence: ");
	if (subject) 
	{
		for (i = 0; i < subject; ++i)
		{
			unsigned int s = subjects[i];
			if (posValues[s] & NOUN_INFINITIVE) strcat(buffer,"to "); // doesnt display DO NOT here
			char* phrase = GetNounPhrase(s,"");
			if (*phrase) 
			{
				strcat(buffer,"(");
				strcat(buffer,phrase);
				strcat(buffer,") ");
			}
			else strcat(buffer,wordStarts[s]);
			if (i < subject-1) strcat(buffer,"+");
		}
		strcat(buffer," ");
	}
	if (verb) 
	{
		if (notFound) strcat(buffer,"(NOT!) ");
		for (i = 0; i < verb; ++i)
		{
			unsigned int j = verbs[i];
			unsigned int clause = clauses[j];
			for (unsigned int k = 0; k < j; ++k)
			{
				if (clauses[k] == clause && posValues[k] & AUX_VERB_BITS) // has  aux
				{
					strcat(buffer,wordStarts[k]);
					strcat(buffer," ");
				}
			}
			if (j != 1 && posValues[j] & NOUN_INFINITIVE) strcat(buffer,"to ");
			strcat(buffer,wordStarts[j]);
			if (i < verb-1) strcat(buffer," +");
		}
		strcat(buffer," ");
	}
	if (indirectobject) 
	{
		for (i = 0; i < indirectobject; ++i)
		{
			unsigned int o = indirectobjects[i];
			char* phrase = GetNounPhrase(o,"");
			if (*phrase) 
			{
				strcat(buffer,"(");
				strcat(buffer,phrase);
				strcat(buffer,") ");
			}
			else strcat(buffer,wordStarts[indirectobjects[i]]);
			if (i < verb-1) strcat(buffer," +");
		}
		strcat(buffer," ");
	}
	if (object) 
	{
		for (i = 0; i < object; ++i)
		{
			unsigned int o = objects[i];
			char* phrase = GetNounPhrase(o,"");
			if (*phrase) 
			{
				strcat(buffer,"(");
				strcat(buffer,phrase);
				strcat(buffer,") ");
			}
			else strcat(buffer,wordStarts[o]);
			if (i < object-1) strcat(buffer," +");
			strcat(buffer," ");
			if (roles[o+1] & (OBJECT_COMPLEMENT | APPOSITIVE))
			{
				strcat(buffer,"(");
				strcat(buffer,wordStarts[o+1]);
				strcat(buffer,") ");
			}
		}
		strcat(buffer," ");
	}
	if (object2) 
	{
		for (i = 0; i < object2; ++i)
		{
			unsigned int o = object2s[i];
			char* phrase = GetNounPhrase(o,"");
			if (*phrase) 
			{
				strcat(buffer,"(");
				strcat(buffer,phrase);
				strcat(buffer,") ");
			}
			else strcat(buffer,wordStarts[o]);
			if (i < object2-1) strcat(buffer," +");
			strcat(buffer," ");
		}
		strcat(buffer," ");
	}
	if (clauses[1] || from != 0){;}
	else if (!stricmp(wordStarts[1],"when")) strcat(buffer,"(q:when) ");
	else if (!stricmp(wordStarts[1],"where")) strcat(buffer,"(q:where) ");
	else if (!stricmp(wordStarts[1],"why")) strcat(buffer,"(q:why) ");
	else if (!stricmp(wordStarts[1],"who") && subject != 1 && object != 1) strcat(buffer,"(q:who) ");
	else if (!stricmp(wordStarts[1],"what") && subject != 1 && object != 1) strcat(buffer,"(q:what) ");
	else if (!stricmp(wordStarts[1],"how")) strcat(buffer,"(q:how) ");

	if (tokenFlags & QUESTIONMARK)  strcat(buffer,"? ");

	if (tokenFlags & PAST) strcat(buffer," PAST ");
	else if (tokenFlags & FUTURE) strcat(buffer," FUTURE ");
	else if (tokenFlags & PRESENT) strcat(buffer," PRESENT ");

	if (tokenFlags & PERFECT) strcat(buffer,"PERFECT ");
	if (tokenFlags & CONTINUOUS) strcat(buffer,"CONTINUOUS ");

	if (tokenFlags & PASSIVE) strcat(buffer," PASSIVE ");

	for (unsigned int i = 1; i <= wordCount; ++i) // show phrases
	{
		if (i < from || i >= to) continue; // ignore
		if (coordinates[i] && posValues[i] & CONJUNCTION_COORDINATE)
		{
			strcat(buffer,"\r\n Coordinate ");
			if (roles[i] == CONJUNCT_PARTICLE) strcat(buffer,"Particle: ");
			else if (roles[i] == CONJUNCT_NOUN) strcat(buffer,"Noun: ");
			else if (roles[i] == CONJUNCT_VERB) strcat(buffer,"Verb: ");
			else if (roles[i] == CONJUNCT_ADJECTIVE) strcat(buffer,"Adjective: ");
			else if (roles[i] == CONJUNCT_ADVERB) strcat(buffer,"Adverb: ");
			else if (roles[i] == CONJUNCT_PHRASE) strcat(buffer,"Phrase: ");
			else strcat(buffer,"Sentence: ");
			strcat(buffer,wordStarts[i]);
			strcat(buffer," (");
			if (coordinates[i] && coordinates[coordinates[i]])
			{
				strcat(buffer,wordStarts[coordinates[coordinates[i]]]); // the before
				strcat(buffer,"/");
				strcat(buffer,wordStarts[coordinates[i]]); // the after
			}
			strcat(buffer,") ");
		}
	}

	for (unsigned int i = 1; i <= wordCount; ++i) // show phrases
	{
		if (i < from || i >= to) continue; // ignore
		if ( phrases[i])
		{
			if (phrases[i] != phrases[i-1] && (i != wordCount || phrases[wordCount] != phrases[1])) 
			{
				if (posValues[i] & (NOUN_BITS|PRONOUN_BITS)) strcat(buffer,"\r\n  Absolute Phrase: ");
				else if (posValues[i] & (ADJECTIVE_BITS|ADVERB_BITS)) strcat(buffer,"\r\n  Time Phrase: ");
				else strcat(buffer,"\r\n  Phrase: ");
				if (i == 1 && phrases[wordCount] == phrases[1]) 
				{
					strcat(buffer,wordStarts[wordCount]);
					strcat(buffer," ");
				}
			}
			strcat(buffer,wordStarts[i]);
			strcat(buffer," ");

			if (roles[i] & OBJECT2 && !phrases[i])
			{
				char* phrase = GetNounPhrase(i,"");
				if (*phrase) 
				{
					strcat(buffer,"(");
					strcat(buffer,phrase);
					strcat(buffer,") ");
				}
				else strcat(buffer,wordStarts[i]);
				strcat(buffer," ");
			}
		}
	}

	for (unsigned int i = 1; i <= wordCount; ++i) // show clauses
	{
		if (i < from || i >= to) continue; // ignore
		if (clauses[i])
		{
			if (clauses[i-1] != clauses[i]) strcat(buffer,"\r\n Clause: ");
			strcat(buffer,wordStarts[i]);
			strcat(buffer," ");
		}
	}
	for (unsigned int i = 1; i <= wordCount; ++i) // show verbals
	{
		if (i < from || i >= to) continue; // ignore
		if (verbals[i])
		{
			if (verbals[i-1] != verbals[i]) strcat(buffer,"\r\n Verbal: ");
			strcat(buffer,wordStarts[i]);
			strcat(buffer," ");
		}
	}

	Log(STDUSERLOG,"%s\r\n\r\n",buffer);
	FreeBuffer();
	if (to <= wordCount) DumpSentence(to); // show next piece
#endif
 }

char* GetRole(unsigned int i)
{
#ifndef DISCARDPARSER
	if ( roles[i] == MAINSUBJECT) return "MAINSUBJECT ";
	else if (roles[i] == (MAINSUBJECT|VERB2)) return "MAINSUBJECT VERB2 ";
	else if (roles[i] == (MAINSUBJECT|SUBJECT2)) return "MAINSUBJECT SUBJECT2 ";
	else if (roles[i] == SUBJECT2) return "SUBJECT2 ";
	else if (roles[i] == (SUBJECT2|OBJECT2)) return "OBJECT2 SUBJECT2 ";
	else if (roles[i] == MAINVERB) return "MAINVERB ";
	else if (roles[i] == VERB2) return "VERB2 ";
	else if (roles[i] == MAINOBJECT) return "MAINOBJECT ";
	else if (roles[i] == (MAINOBJECT|VERB2)) return "MAINOBJECT VERB2 ";
	else if (roles[i] == (MAINOBJECT|SUBJECT2)) return "MAINOBJECT SUBJECT2 ";
	else if (roles[i] == OBJECT2) return "OBJECT2 ";
	else if (roles[i] == (OBJECT2|VERB2)) return "OBJECT2 VERB2 ";
	else if (roles[i] == MAININDIRECTOBJECT) return "MAININDIRECTOBJECT ";
	else if (roles[i] == OBJECT_COMPLEMENT) return "OBJECT_COMPLEMENT ";
	else if (roles[i] == INDIRECTOBJECT2) return "INDIRECTOBJECT2 ";
	else if (roles[i] == CONJUNCT_PARTICLE) return "CONJUNCT_PARTICLE ";
	else if (roles[i] == CONJUNCT_NOUN) return "CONJUNCT_NOUN ";
	else if (roles[i] == CONJUNCT_VERB) return "CONJUNCT_VERB ";
	else if (roles[i] == CONJUNCT_ADJECTIVE) return "CONJUNCT_ADJECTIVE ";
	else if (roles[i] == CONJUNCT_ADVERB) return "CONJUNCT_ADVERB ";
	else if (roles[i] == CONJUNCT_PHRASE) return "CONJUNCT_PHRASE ";
	else if (roles[i] == CONJUNCT_CLAUSE) return "CONJUNCT_CLAUSE ";
	else if (roles[i] == CONJUNCT_SENTENCE) return "CONJUNCT_SENTENCE ";
	else if (roles[i] == APPOSITIVE) return "APPOSITIVE ";
	else if (roles[i] == SUBJECT_COMPLEMENT) return "SUBJECT_COMPLEMENT ";
	else if (roles[i] == POSTNOMINAL_ADJECTIVE) return "POSTNOMINAL_ADJECTIVE ";
	else if (roles[i] == ADJECTIVE_COMPLEMENT) return "ADJECTIVE_COMPLEMENT ";
	else if (roles[i] == OMITTED_TIME_PREP) return "OMITTED_TIME_PREP ";
	else if (roles[i] == ADVERB_CLAUSE) return "ADVERB_CLAUSE ";
	else if (roles[i] == ADDRESS) return "ADDRESS ";
	else if (roles[i] == COMMA_PHRASE) return "COMMA_PHRASE ";
	else if (roles[i] == NOT) return "NOT ";
	else if (roles[i] == REFLEXIVE) return "REFLEXIVE ";
#endif
	return "";
}
