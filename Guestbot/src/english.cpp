
#include "common.h"

unsigned int posTiming;

typedef struct EndingInfo 
{
	char* word;	
	uint64 properties;
	uint64 flags;
} EndingInfo;
	
EndingInfo noun2[] = 
{
	{"th",NOUN|NOUN_SINGULAR}, 
	{"ar",NOUN|NOUN_SINGULAR}, 
	{"ty",NOUN|NOUN_SINGULAR}, 
	{"et",NOUN|NOUN_SINGULAR}, 
	{"or",NOUN|NOUN_SINGULAR}, 
	{"al",NOUN|NOUN_SINGULAR}, 
	{"er",NOUN|NOUN_SINGULAR}, 
	{"ee",NOUN|NOUN_SINGULAR}, 
	{"id",NOUN|NOUN_SINGULAR}, 
	{"cy",NOUN|NOUN_SINGULAR}, 
	{0},
};

EndingInfo noun3[] = 
{
	{"ory",NOUN|NOUN_SINGULAR}, 
	{"ant",NOUN|NOUN_SINGULAR}, 
	{"eer",NOUN|NOUN_SINGULAR}, 
	{"log",NOUN|NOUN_SINGULAR}, 
	{"oma",NOUN|NOUN_SINGULAR}, 
	{"dom",NOUN|NOUN_SINGULAR}, 
	{"ard",NOUN|NOUN_SINGULAR}, 
	{"ide",NOUN|NOUN_SINGULAR}, 
	{"oma",NOUN|NOUN_SINGULAR}, 
	{"ity",NOUN|NOUN_SINGULAR}, 
	{"ist",NOUN|NOUN_SINGULAR}, 
	{"ism",NOUN|NOUN_SINGULAR}, 
	{"ing",NOUN|NOUN_SINGULAR}, 
	{"gon",NOUN|NOUN_SINGULAR}, 
	{"gam",NOUN|NOUN_SINGULAR}, 
	{"ese",NOUN|NOUN_SINGULAR}, 
	{"ure",NOUN|NOUN_SINGULAR}, 
	{"acy",NOUN|NOUN_SINGULAR}, 
	{"age",NOUN|NOUN_SINGULAR}, 
	{"ade",NOUN|NOUN_SINGULAR}, 
	{"ery",NOUN|NOUN_SINGULAR}, 
	{"ary",NOUN|NOUN_SINGULAR}, 
	{"let",NOUN|NOUN_SINGULAR}, 
	{"ess",NOUN|NOUN_SINGULAR}, 
	{"ice",NOUN|NOUN_SINGULAR}, 
	{"ice",NOUN|NOUN_SINGULAR}, 
	{"ine",NOUN|NOUN_SINGULAR}, 
	{"ent",NOUN|NOUN_SINGULAR}, 
	{"ion",NOUN|NOUN_SINGULAR}, 
	{"oid",NOUN|NOUN_SINGULAR}, 
	{"ite",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun4[] = 
{
	{"tion",NOUN|NOUN_SINGULAR}, 
	{"ment",NOUN|NOUN_SINGULAR}, 
	{"emia",NOUN|NOUN_SINGULAR}, 
	{"opsy",NOUN|NOUN_SINGULAR}, 
	{"itis",NOUN|NOUN_SINGULAR}, 
	{"opia",NOUN|NOUN_SINGULAR}, 
	{"hood",NOUN|NOUN_SINGULAR}, 
	{"ness",NOUN|NOUN_SINGULAR}, 
	{"logy",NOUN|NOUN_SINGULAR}, 
	{"ette",NOUN|NOUN_SINGULAR}, 
	{"cide",NOUN|NOUN_SINGULAR}, 
	{"sion",NOUN|NOUN_SINGULAR}, 
	{"ling",NOUN|NOUN_SINGULAR}, 
	{"cule",NOUN|NOUN_SINGULAR}, 
	{"osis",NOUN|NOUN_SINGULAR}, 
	{"esis",NOUN|NOUN_SINGULAR}, 
	{"ware",NOUN|NOUN_SINGULAR}, 
	{"tude",NOUN|NOUN_SINGULAR}, 
	{"cian",NOUN|NOUN_SINGULAR}, 
	{"ency",NOUN|NOUN_SINGULAR}, 
	{"ence",NOUN|NOUN_SINGULAR}, 
	{"ancy",NOUN|NOUN_SINGULAR}, 
	{"ance",NOUN|NOUN_SINGULAR}, 
	{"tome",NOUN|NOUN_SINGULAR}, 
	{"tomy",NOUN|NOUN_SINGULAR}, 
	{"crat",NOUN|NOUN_SINGULAR}, 
	{"ship",NOUN|NOUN_SINGULAR}, 
	{"pnea",NOUN|NOUN_SINGULAR}, 
	{"path",NOUN|NOUN_SINGULAR}, 
	{"gamy",NOUN|NOUN_SINGULAR}, 
	{"onym",NOUN|NOUN_SINGULAR}, 
	{"icle",NOUN|NOUN_SINGULAR}, 
	{"wise",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun5[] = 
{
	{"cracy",NOUN|NOUN_SINGULAR}, 
	{"scope",NOUN|NOUN_SINGULAR}, 
	{"scopy",NOUN|NOUN_SINGULAR}, 
	{"ocity",NOUN|NOUN_SINGULAR}, 
	{"acity",NOUN|NOUN_SINGULAR}, 
	{"loger",NOUN|NOUN_SINGULAR}, 
	{"ation",NOUN|NOUN_SINGULAR}, 
	{"arian",NOUN|NOUN_SINGULAR}, 
	{"ology",NOUN|NOUN_SINGULAR}, 
	{"algia",NOUN|NOUN_SINGULAR}, 
	{"sophy",NOUN|NOUN_SINGULAR}, 
	{"cycle",NOUN|NOUN_SINGULAR}, 
	{"orium",NOUN|NOUN_SINGULAR}, 
	{"arium",NOUN|NOUN_SINGULAR}, 
	{"phone",NOUN|NOUN_SINGULAR}, 
	{"iasis",NOUN|NOUN_SINGULAR}, 
	{"pathy",NOUN|NOUN_SINGULAR}, 
	{"phile",NOUN|NOUN_SINGULAR}, 
	{"phyte",NOUN|NOUN_SINGULAR}, 
	{"otomy",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun6[] = 
{
	{"script",NOUN|NOUN_SINGULAR}, 
	{"phobia",NOUN|NOUN_SINGULAR}, 
	{"iatric",NOUN|NOUN_SINGULAR}, 
	{"logist",NOUN|NOUN_SINGULAR}, 
	{"oholic",NOUN|NOUN_SINGULAR}, 
	{"aholic",NOUN|NOUN_SINGULAR}, 
	{"plegia",NOUN|NOUN_SINGULAR}, 
	{"plegic",NOUN|NOUN_SINGULAR}, 
	{"ostomy",NOUN|NOUN_SINGULAR}, 
	{"ectomy",NOUN|NOUN_SINGULAR}, 
	{"trophy",NOUN|NOUN_SINGULAR}, 
	{"escent",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun7[] = 
{
	{"escence",NOUN|NOUN_SINGULAR}, 
	{"ization",NOUN|NOUN_SINGULAR}, 
	{0},
};

	
EndingInfo verb5[] = 
{
	{"scribe",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb4[] = 
{
	{"sect",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb3[] = 
{
	{"ise",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{"ize",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{"ify",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{"ate",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb2[] = 
{
	{"en",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{"er",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{"fy",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};


EndingInfo adverb4[] = 
{
	{"wise",ADVERB|ADVERB_NORMAL,BASIC_FORM}, 
	{"ward",ADVERB|ADVERB_NORMAL,BASIC_FORM}, 
	{0},
};
EndingInfo adverb3[] = 
{
	{"ily",ADVERB|ADVERB_NORMAL,BASIC_FORM}, 
	{0},
};
EndingInfo adverb2[] = 
{
	{"ly",ADVERB|ADVERB_NORMAL,BASIC_FORM}, 
	{0},
};


EndingInfo adjective7[] = 
{
	{"iferous",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // comprised of
	{0},
};
EndingInfo adjective6[] = 
{
	{"escent",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},
	{0},
};
EndingInfo adjective5[] = 
{
	{"ative",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // tending toward
	{"esque",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},
	{"-free",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
	{"gonal",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
	{"gonic",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // angle
	{"proof",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
	{"sophic",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // knowledge
	{"esque",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // in the style of
	{0},
};
EndingInfo adjective4[] = 
{
		{"less",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // without
		{"etic",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"_out",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"ular",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"uous",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // characterized by
		{"ical",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // pertaining to
		{"-off",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // capable of being
		{"ious",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // characterized by
		{"able",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // capable of being
		{"ible",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // capable of being
		{"like",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // resembling
		{"some",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // characterized by
	{0},
};
EndingInfo adjective3[] = 
{
		{"ial",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"oid",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // shape of
		{"ous",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // characterized by
		{"ive",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // having the nature of
		{"ate",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // quality of
		{"ful",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // quality of
		{"ful",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // notable for
		{"ese",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to a place
		{"fic",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
		{"ant",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // inclined to
		{"ent",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // one who causes
		{"ern",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // quality of
		{"ian",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"ile",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"_to",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // to
		{"_of",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // of
		{"ing",ADJECTIVE|ADJECTIVE_PARTICIPLE,0},  // verb present participle as adjective  // BUG adjectiveFormat = ADJECTIVE_PARTICIPLE;
		{"ied",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // verb past participle as adjective
		{"ine",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
		{"ual",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // gradual
	{0},
};
EndingInfo adjective2[] = 
{
		{"ic",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},  // pertaining to
		{"ar",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},  // relating to
		{"ac",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},  // pertaining to
		{"al",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM},  // pertaining to
		{"en",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
		{"an",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, // relating to
	{0},
};
EndingInfo adjective1[] = 
{
		{"y",ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM}, 
	{0},
};

uint64 GetPosData(char* original,WORDP &entry,WORDP &canonical,bool firstTry,bool nogenerate) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	uint64 flags = 0;
	canonical = 0;
	entry = FindWord(original,0,PRIMARY_CASE_ALLOWED);
	size_t len = strlen(original);

	if (IsUrl(original,0)) entry = canonical = StoreWord(original,NOUN|NOUN_SINGULAR,WEB_URL); 

	// mark numeric fractions
	char* fraction = strchr(original,'/');
	if ( fraction && IsDigit(fraction[1]))
	{
		char number[MAX_WORD_SIZE];
		strcpy(number,original);
		number[fraction-original] = 0;
		if (IsNumber(number) && IsNumber(fraction+1))
		{
			int x = atoi(number);
			int y = atoi(fraction+1);
			float val = (float)((float)x / (float)y);
			sprintf(number,"%2.2f",val);
			flags = ADJECTIVE|NOUN|ADJECTIVE_CARDINAL|NOUN_CARDINAL|NOUN_ORDINAL;
			if (!entry) entry = StoreWord(original,flags);
			canonical = FindWord(number,0,PRIMARY_CASE_ALLOWED);
			if (canonical) flags |= canonical->properties;
			else canonical = StoreWord(number,flags);
			return flags;
		}
	}
	else if (*original && IsNumber(original)) // bug unneeeded?
	{
		char number[MAX_WORD_SIZE];
		char* value;
		uint64 baseflags = (entry) ? entry->properties : 0;
		if (IsPlaceNumber(original))
		{
			sprintf(number,"%d",(int)Convert2Integer(original));
			flags = ADJECTIVE|ADJECTIVE_ORDINAL|NOUN|NOUN_ORDINAL| (baseflags & TAG_TEST);
		}
		else if (GetCurrency(original,value)) // money
		{
			int64 n = Convert2Integer(value);
			float fn = (float)atof(value);
			if ((float)n == fn) sprintf(number,"%d",n);
			else if (strchr(value,'.')) sprintf(number,"%2.2f",fn);
			else 
			{
#ifdef WIN32
				sprintf(number,"%I64d",n); 
#else
				sprintf(number,"%lld",n); 
#endif
			}
			flags = NOUN|NOUN_CARDINAL;
		}
		else
		{
			if (strchr(original,'.')) sprintf(number,"%2.2f",atof(original));
			else 
			{
				int64 val = Convert2Integer(original);
				if (val < 1000000000 && val >  -1000000000)
				{
					int smallval = (int) val;
					sprintf(number,"%d",smallval);
				}
				else
				{
#ifdef WIN32
					sprintf(number,"%I64d",val);	
#else
					sprintf(number,"%lld",val);	
#endif
				}
			
			}
			flags = ADJECTIVE|NOUN|ADJECTIVE_CARDINAL|NOUN_CARDINAL;
		}
		canonical = StoreWord(number,flags);
		if (entry && entry->properties & PART_OF_SPEECH) 
		{
			uint64 val = entry->properties; // numbers we "know" in some form should be as we know them. like "once" is adverb and adjective, not cardinal noun
			if (val & ADJECTIVE_NORMAL) // change over to known number
			{
				flags ^= ADJECTIVE_NORMAL;
				flags |= ADJECTIVE_CARDINAL;
			}
			if (val & NOUN_SINGULAR) // change over to known number
			{
				flags ^= NOUN_SINGULAR;
				flags |= NOUN_CARDINAL;
			}
			if (val & PREPOSITION) flags |= PREPOSITION; // like "once"
		}
		else entry = StoreWord(original);
		return flags;
	}
	// handle time like 4:30
	if (len < 6 && IsDigit(*original) && IsDigit(original[len-1]) && (original[1] == ':' || original[2] == ':')) // 18:32
	{
		entry = canonical = StoreWord(original,NOUN|NOUN_SINGULAR|ADJECTIVE|ADJECTIVE_NORMAL,ACTUAL_TIME|TIMEWORD|BASIC_FORM); // 18:32
		return NOUN|NOUN_SINGULAR|ADJECTIVE|ADJECTIVE_NORMAL;
	}
			
	if (!canonical)
	{
		char* canon = GetCanonical(original);
		if (canon) canonical = StoreWord(canon);
	}

	if (entry && entry->properties & (PART_OF_SPEECH|TAG_TEST|PUNCTUATION)) // we know this usefully
	{
		flags = entry->properties;
		char* canon = GetCanonical(entry);
		canonical = (canon) ? FindWord(canon) : NULL;
		// possessive pronoun-determiner like my is always a determiner, not a pronoun. 
		if (entry->properties & (COMMA | PUNCTUATION | PAREN | QUOTE | POSSESSIVE | PUNCTUATION)) return flags;
	}
	bool known = (entry) ? ((entry->properties & PART_OF_SPEECH)  != 0) : false;
	bool preknown = known;

	if (!(flags & VERB_TENSES)) // could it be a verb we dont know directly (even if we know the word)
	{
		char* verb =  GetInfinitive(original,true); 
		if (verb)  
		{
			if (!canonical) canonical = FindWord(verb);
			known = true;
			flags |= VERB | verbFormat;
			entry = StoreWord(original,VERB | verbFormat);
			if (verbFormat & VERB_PRESENT_PARTICIPLE) 
			{
				flags |= NOUN|NOUN_GERUND;
				StoreWord(original,NOUN|NOUN_GERUND);
			}
			if (flags & (VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE)) // if it can be participle, it can be adjective participle
			{
				flags |= ADJECTIVE|ADJECTIVE_PARTICIPLE;
				StoreWord(original,ADJECTIVE|ADJECTIVE_PARTICIPLE);
			}
			canonical =  FindWord(verb,0,PRIMARY_CASE_ALLOWED); // we prefer verb as canonical form
		}
	}

	if (!(flags & (NOUN_BITS|PRONOUN_BITS))) // could it be plural noun we dont know directly -- eg dogs or the plural of a singular we know differently-- "arms is both singular and plural" - avoid pronouns like "his" or "hers"
	{
		if (original[len-1] == 's')
		{
			char* noun = GetSingularNoun(original,true,true);
			if (noun && strcmp(noun,original)) 
			{
				WORDP D = StoreWord(original,NOUN);
				uint64 which = (D->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_PLURAL : NOUN_PLURAL;
				AddProperty(D,which);
				if (!entry) entry = D;
				flags |= NOUN|which;
				if (!canonical) canonical = FindWord(noun); // 2ndary preference for canonical is noun
			}
		}
	}
	else if (flags & NOUN_PLURAL && original[len-1] == 's')
	{
		char* noun = GetSingularNoun(original,true,true);
		if (noun && strcmp(noun,original)) 
		{
			WORDP D = FindWord(noun);
			if (!entry) entry = D;
			flags |= NOUN|NOUN_PLURAL;
			if (!canonical) canonical = FindWord(noun); // 2ndary preference for canonical is noun
		}
	}

	if (!canonical) // we dont know the word and didn't interpolate it from noun or verb advanced forms
	{
		if (flags & (VERB|NOUN_GERUND)) canonical = FindWord(GetInfinitive(original,true)); // verb or known gerund (ing) or noun plural (s) which might be a verb instead
		if (flags & (NOUN_PROPER_PLURAL |  NOUN_PLURAL))
		{
			char* noun = GetSingularNoun(original,true,false);
			if (noun && (!canonical || (!stricmp(canonical->word,original)))) canonical = FindWord(noun); // if its different, use that
		} 
	}

	// interpolate singular normal nouns to adjective_noun
	if (entry && entry->properties & (NOUN_PROPER_SINGULAR|NOUN_SINGULAR)) 
	{
		flags |= ADJECTIVE_NOUN;
		AddProperty(entry,ADJECTIVE_NOUN);
	}

	if (!preknown) // if we didnt know the original word, then even if we've found noun/verb forms of it, we need to test other options
	{
		// process by know parts of speech or potential parts of speech
		if (!(flags & NOUN)) // could it be a noun but not already known as a known (eg noun_gerund from verb)
		{
			char* noun = GetSingularNoun(original,true,true); 
			if (noun) 
			{
				known = true;
				flags |= NOUN | nounFormat;
				entry = StoreWord(original,NOUN | nounFormat);
				if (!canonical || !stricmp(canonical->word,original)) canonical = FindWord(noun);
			}
		}
		if (!(flags & ADJECTIVE)) 
		{
			char* adjective = GetAdjectiveBase(original,true); 
			if (adjective) 
			{
				known = true;
				flags |= ADJECTIVE|ADJECTIVE_NORMAL;
				entry = StoreWord(original,ADJECTIVE|ADJECTIVE_NORMAL,adjectiveFormat);
				if (!canonical) canonical = FindWord(adjective);
			}
		}
		if (!(flags & ADVERB)) 
		{
			char* adverb = GetAdverbBase(original,true); 
			if (adverb)  
			{
				known = true;
				flags |= ADVERB|ADVERB_NORMAL;
				entry = StoreWord(original,ADVERB|ADVERB_NORMAL,adverbFormat);
				if (!canonical) canonical = FindWord(adverb);
			}
		}
	}
	if (!canonical)
	{
		if (flags & ADJECTIVE)  // want adj before noun, so "worse" which is both, will be adjective converted
		{
			if (!canonical  || !stricmp(original,canonical->word)) canonical = FindWord(GetAdjectiveBase(original,true));
		}
		if (flags & ADVERB)
		{
			if (!canonical || !stricmp(original,canonical->word)) canonical = FindWord(GetAdverbBase(original,true));
		}
		if (flags & NOUN) // EVEN if we do know it... flies is a singular and needs canoncial for fly BUG
		{
			// even if it is a noun, if it ends in s and the root is also a noun, make it plural as well (e.g., rooms)
			if (original[len-1] == 's') 
			{
				char* singular = GetSingularNoun(original,true,true);
				if (singular && stricmp(singular,original))
				{
					canonical = StoreWord(singular);
					known = true;
					flags |= NOUN_PLURAL;
				}
			}
			if (!canonical  || !stricmp(original,canonical->word)) canonical = FindWord(GetSingularNoun(original,true,true));
		}
		if (!canonical) canonical = entry; //  for all others
	}

	if (flags){;}
	else if (tokenControl & (STRICT_CASING|ONLY_LOWERCASE)) {;}
	else 
	{
#ifndef NOPOSPARSER
		if (firstTry ) // auto try for opposite case if we dont recognize the word
		{
			WORDP D = FindWord(original,0,SECONDARY_CASE_ALLOWED);
			if (D) return GetPosData(D->word,entry,canonical,false,nogenerate);
		}
#else
		if ( IsUpperCase(*original)) // dont recognize this, see if we know  lower case if this was upper case
		{
			WORDP D = FindWord(original,0,LOWERCASE_LOOKUP);
			if (D) return GetPosData(D->word,entry,canonical,false,nogenerate);
		}
#endif
	}
	// we still dont know this word, go generic if its lower and upper not known
	if (nogenerate){;}
	else if (!flags && (firstTry || !FindWord(original,0,UPPERCASE_LOOKUP))) // if being lower case and already tried and have an upper case word for this, ignore this
	{
		size_t len = strlen(original);
		if (IsUpperCase(*original) || IsUpperCase(original[1]) || IsUpperCase(original[2]) )
		{
			flags |= (original[len-1] == 's') ? NOUN_PROPER_PLURAL : NOUN_PROPER_SINGULAR;
		}
		else 
		{
			flags |= (original[len-1] == 's') ? NOUN_PLURAL : NOUN_SINGULAR;
		}
		bool mixed = false;
		for (unsigned int i = 0; i < len; ++i) if (!IsAlpha(original[i]) && original[i] != '-' && original[i] != '_') mixed = true; // has non alpha in it

		if (mixed || IsUpperCase(*original)) flags |= NOUN;  // non-real word OR proper name
		else flags |= VERB | VERB_PRESENT | VERB_INFINITIVE | NOUN | NOUN_INFINITIVE |  ADJECTIVE | ADJECTIVE_NORMAL | ADVERB ;
		
		// treat all hypenated words as adjectives "bone-headed"
		if (strchr(original,'-')) flags = ADJECTIVE_NORMAL;

		canonical = DunknownWord;
	}
	if (flags & VERB_INFINITIVE) flags |= NOUN_INFINITIVE;

	if (canonical && entry) entry->systemFlags |= canonical->systemFlags & AGE_LEARNED; // copy age data across
	return flags;
}


void English_SetTense()
{
	uint64 aux[25];
	WORDP  auxName[25];
	WORDP  auxCanon[25];
	unsigned int auxIndex = 0;
	uint64 mainverbTense = 0;
	unsigned int mainVerb = 0;
	memset(aux,0,25 * sizeof(uint64));
	unsigned int defaultTense = 0;

	// determine sentence tense when not past from verb using aux (may pick wrong aux)
	for (unsigned int i = 1; i <= wordCount; ++i)
    {
		bool notclauseverbal = true;
#ifndef DISCARDPARSER
		if (roles[i] == MAINVERB) 
		{
			mainverbTense = posValues[i] & VERB_TENSES;
			mainVerb = i;
			if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PAST)) tokenFlags |= PAST;
			else if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS)) defaultTense = PRESENT;
		}
		// if not parsing but are postagging
		if (!mainverbTense && !roles[i] && posValues[i] & VERB_TENSES) 
		{
			mainverbTense = posValues[i] & VERB_TENSES;
			mainVerb = i;
			if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PAST)) tokenFlags |= PAST;
			else if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS)) defaultTense = PRESENT;
		}
		if (clauses[i] || verbals[i]) notclauseverbal = false;

#endif

		if (posValues[i] & AUX_VERB_BITS && notclauseverbal)
		{
			auxName[auxIndex] = originalLower[i];
			auxCanon[auxIndex] = canonicalLower[i];
			aux[auxIndex] = originalLower[i] ? (originalLower[i]->properties & (AUX_VERB_BITS | VERB_TENSES)) : 0;	// pattern of aux

			// question? 
			if (i == 1 && !(tokenControl & NO_INFER_QUESTION) ) tokenFlags |= QUESTIONMARK;
			if (defaultTense){;}
			else if (aux[auxIndex] & AUX_BE)
			{
				if (aux[auxIndex]  & (VERB_PRESENT|VERB_PRESENT_3PS)) tokenFlags |= PRESENT;
				else if (aux[auxIndex]  & (VERB_PAST | VERB_PAST_PARTICIPLE)) tokenFlags |= PAST;
			}
			else if (aux[auxIndex]  & AUX_VERB_FUTURE ) tokenFlags |= FUTURE;
			else if (aux[auxIndex]  & AUX_VERB_PAST) tokenFlags |= PAST;
			auxIndex++;
			if (auxIndex > 20) break;
		}
	}
	if (!auxIndex && canonicalLower[1] && !stricmp(canonicalLower[1]->word,"be") && !(tokenControl & NO_INFER_QUESTION)) tokenFlags |= QUESTIONMARK;  // are you a bank teller
	if ( canonicalLower[1] && canonicalLower[1]->properties & QWORD && canonicalLower[2] && canonicalLower[2]->properties & AUX_VERB_BITS  && !(tokenControl & NO_INFER_QUESTION))   tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?
	else if ( posValues[1] & PREPOSITION && canonicalLower[2] && canonicalLower[2]->properties & QWORD && canonicalLower[3] && canonicalLower[3]->properties & AUX_VERB_BITS  && !(tokenControl & NO_INFER_QUESTION))   tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?

#ifdef INFORMATION
	Active tenses:  have + past participle makes things perfect  --		 be + present particicple makes things continuous
		1. present
			a. simple
			b. perfect - I have/has + past participle
			c. continuous - I am + present participle
			d. perfect continuous - I have been + present participle
		2.  past
			a. simple
			b. perfect - I had + past participle
			c. continuous - I was + present participle
			d. perfect continuous - I had been + present participle
		3. future
			a: simple - I will + infinitive or  I am going_to + infinitive
			b. perfect - I will have + past participle 
			c. continuous - I will be + present participle
			d. perfect continuous - I will have been + present participle
	Passive tenses:  be + past participle makes things passive
		1. present
			a. simple - I am  + past participle
			b. perfect - I have/has + been + past participle
			c. continuous - I am + being + past participle
			d. perfect continuous - I have been + being + past participle
		2.  past
			a. simple - I was + past participle
			b. perfect - I had + been + past participle
			c. continuous - I was +  being + past participle
			d. perfect continuous - I had been + being + past participle
		3. future
			a: simple - I will + be + past participle  or  I am going_to + be + past participle
			b. perfect - I will have + been + past participle 
			c. continuous - I will be + past participle
			d. perfect continuous - I will have been + being + past participle
#endif

	if (auxIndex)
	{
		// special future "I am going to swim"
		if (*aux & (VERB_PRESENT|VERB_PRESENT_3PS) && *aux & AUX_BE && auxIndex == 1 && mainverbTense == VERB_PRESENT_PARTICIPLE && !stricmp(wordStarts[mainVerb],"going") && posValues[mainVerb+1] == TO_INFINITIVE)  tokenFlags |= FUTURE;
		else if (aux[auxIndex-1] & AUX_HAVE && mainverbTense & VERB_PAST_PARTICIPLE) 
		{
			if (*aux & (VERB_PRESENT|VERB_PRESENT_3PS)) 
			{
				tokenFlags |= PRESENT_PERFECT|PAST; 
				if (tokenFlags & PRESENT)  tokenFlags ^= (PRESENT|PERFECT);
			}
			else tokenFlags |= PERFECT; 
		}
		else if (aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) tokenFlags |= CONTINUOUS; 

		if (aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PAST_PARTICIPLE) 
		{
			tokenFlags |= PASSIVE;
			if (aux[auxIndex-1] & VERB_PRESENT_PARTICIPLE) 
			{
				tokenFlags |= CONTINUOUS;	// being xxx
			}
		}
		if (*aux & AUX_HAVE && aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) 
		{
				tokenFlags |= PERFECT;	// I have/had been xxx
		}
		if (aux[1] & AUX_HAVE && aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) 
		{
				tokenFlags |= PERFECT;	// I will have/had been xxx
		}
		if (*aux & AUX_VERB_FUTURE)  
		{
			tokenFlags |= FUTURE;
			if (mainverbTense & VERB_PRESENT_PARTICIPLE && aux[auxIndex-1] & AUX_BE && aux[auxIndex-1] & VERB_INFINITIVE) tokenFlags |= CONTINUOUS;	// be painting
		}
		else if (*aux & (AUX_VERB_PAST | VERB_PAST))  tokenFlags |= PAST; 
		if (tokenFlags & PERFECT && !(tokenFlags & FUTURE)) 
		{
				tokenFlags |= PAST; // WE CAN NOT LET PRESENT PERFECT STAND, we need it to be in the past! "we have had sex"
		}
		else if (!(tokenFlags & (PAST|FUTURE))) tokenFlags |= PRESENT; 
	}
	else if (defaultTense) tokenFlags |=  defaultTense;
	else tokenFlags |= PRESENT; 
}

static char* MakePastTense(char* original,WORDP D)
{
	static char buffer[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	strcpy(word,original);

    //   check for multiword behavoir. Always change the 1st word only
    char* at =  strchr(word,'_'); 
	if (!at) at = strchr(word,'-');
    if (at && at[1])
    {
		int cnt = BurstWord(word,HYPHENS);
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		unsigned int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			if (D && (D->systemFlags & (VERB_CONJUGATE1|VERB_CONJUGATE2|VERB_CONJUGATE3)))
			{
				if (D->systemFlags & VERB_CONJUGATE1 && i != 1) continue;
				if (D->systemFlags & VERB_CONJUGATE2 && i != 2) continue;
				if (D->systemFlags & VERB_CONJUGATE3 && i != 3) continue;
			}
			WORDP V = FindWord(words[i]);
			if (!V || !(V->properties & VERB)) continue;
			char* inf = GetPastTense(words[i]); //   is this word an infinitive?
			if (!inf) continue;
			*trial = 0;
			char* at1 = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at1,inf);
					at1 += strlen(inf);
					*at1++ = separators[j];
				}
				else
				{
					strcpy(at1,words[j]);
					at1 += lens[j];
					*at1++ = separators[j];
				}
			}
			strcpy(buffer,trial);
			return buffer;
		}
	}

    strcpy(buffer,word);
    size_t len = strlen(buffer);
    if (buffer[len-1] == 'e') strcat(buffer,"d");
    else if (!(IsVowel(buffer[len-1]) || buffer[len-1] == 'y' ) && IsVowel(buffer[len-2]) && len > 2 && !IsVowel(buffer[len-3])) 
    {
        char lets[2];
        lets[0] = buffer[len-1];
        lets[1] = 0;
        strcat(buffer,lets); 
        strcat(buffer,"ed");
    }
    else if (buffer[len-1] == 'y' && !IsVowel(buffer[len-2])) 
    {
        buffer[len-1] = 'i';
        strcat(buffer,"ed");
    }   
    else strcat(buffer,"ed");
    return buffer; 
}

char* English_GetPastTense(char* original)
{
    WORDP D = FindWord(original);
    if (D && D->properties & VERB && D->conjugation ) //   die is both regular and irregular?
    {
        int n = 9;
        while (--n && D)
        {
            if (D->properties & VERB_PAST) return D->word; //   might be an alternate present tense moved to main
            D = GetTense(D); //   known irregular
        }
        return NULL;
    }
	return MakePastTense(original,D);
}

char* English_GetPastParticiple(char* word)
{
    WORDP D = FindWord(word,0);
    if (D && D->properties & VERB && D->conjugation ) 
    {
        int n = 9;
        while (--n && D)
        {
            if (D->properties & VERB_PAST_PARTICIPLE) return D->word; //   might be an alternate present tense moved to main
            D = GetTense(D); //   known irregular
        }
   }
   return MakePastTense(word,D);
}

char* English_GetPresentParticiple(char* word)
{
    static char buffer[MAX_WORD_SIZE];
    WORDP D = FindWord(word,0);
    if (D && D->properties & VERB && D->conjugation ) 
    {//   born (past) -> bore (a past and a present) -> bear 
        int n = 9;
        while (--n && D) //   we have to cycle all the conjugations past and present of to be
        {
            if (D->properties &  VERB_PRESENT_PARTICIPLE) return D->word; 
            D = GetTense(D); //   known irregular
        }
    }
    
    strcpy(buffer,word);
    size_t len = strlen(buffer);
    char* inf = GetInfinitive(word,false);
    if (!inf) return 0;

    if (buffer[len-1] == 'g' && buffer[len-2] == 'n' && buffer[len-3] == 'i' && (!inf || stricmp(inf,word))) return word;   //   ISNT participle though it has ing ending (unless its base is "ing", like "swing"

    //   check for multiword behavoir. Always change the 1st word only
    char* at =  strchr(word,'_'); 
	if (!at) at = strchr(word,'-');
    if (at)
    {
		int cnt = BurstWord(word,HYPHENS);
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			if (D && (D->systemFlags & (VERB_CONJUGATE1|VERB_CONJUGATE2|VERB_CONJUGATE3)))
			{
				if (D->systemFlags & VERB_CONJUGATE1 && i != 1) continue;
				if (D->systemFlags & VERB_CONJUGATE2 && i != 2) continue;
				if (D->systemFlags & VERB_CONJUGATE3 && i != 3) continue;
			}
			WORDP E = FindWord(words[i]);
			if (!E || !(E->properties & VERB)) continue;
			char* inf = GetPresentParticiple(words[i]); //   is this word an infinitive?
			if (!inf) continue;
			*trial = 0;
			char* at = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at,inf);
					at += strlen(inf);
					*at++ = separators[j];
				}
				else
				{
					strcpy(at,words[j]);
					at += lens[j];
					*at++ = separators[j];
				}
			}
			strcpy(buffer,trial);
			return buffer;
		}
	}


    strcpy(buffer,inf); //   get the real infinitive

    if (!stricmp(buffer,"be"));
    else if (buffer[len-1] == 'h' || buffer[len-1] == 'w' ||  buffer[len-1] == 'x' ||  buffer[len-1] == 'y'); //   no doubling w,x,y h, 
    else if (buffer[len-2] == 'i' && buffer[len-1] == 'e') //   ie goes to ying
    {
        buffer[len-2] = 'y';
        buffer[len-1] = 0;
    }
    else if (buffer[len-1] == 'e' && !IsVowel(buffer[len-2]) ) //   drop ending Ce  unless -nge (to keep the j sound) 
    {
        if (buffer[len-2] == 'g' && buffer[len-3] == 'n');
        else buffer[len-1] = 0; 
    }
    else if (buffer[len-1] == 'c' ) //   add k after final c
    {
        buffer[len-1] = 'k'; 
        buffer[len] = 0;
    }
   //   double consonant 
    else if (!IsVowel(buffer[len-1]) && IsVowel(buffer[len-2]) && (!IsVowel(buffer[len-3]) || (buffer[len-3] == 'u' && buffer[len-4] == 'q'))) //   qu sounds like consonant w
    {
        char* base = GetInfinitive(word,false);
        WORDP D = FindWord(base,0);
        if (D && D->properties & VERB && D->conjugation ) 
        {
            int n = 9;
            while (--n && D)
            {
                if (D->properties & VERB_PAST) 
                {
                    unsigned int len = D->y.length;
                    if (D->word[len-1] != 'd' || D->word[len-2] != 'e' || len < 5) break; 
                    if (IsVowel(D->word[len-3])) break; //   we ONLY want to see if a final consonant is doubled. other things would just confuse us
                    strcpy(buffer,D->word);
                    buffer[len-2] = 0;      //   drop ed
                    strcat(buffer,"ing");   //   add ing
                    return buffer; 
                }
                D = GetTense(D); //   known irregular
            }
            if (!n) ReportBug("verb loop") //   complain ONLY if we didnt find a past tense
        }

        char lets[2];
        lets[0] = buffer[len-1];
        lets[1] = 0;
        strcat(buffer,lets);    //   double consonant
    }
    strcat(buffer,"ing");
    return buffer; 
}


static char* InferVerb(char* original, unsigned int len)
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
	
	char* item;
	char* test;
	int i;
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = verb5[++i].word)) if (!stricmp(test,word)) return StoreWord(word,verb5[i].flags)->word;
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = verb4[++i].word)) if (!stricmp(test,word)) return StoreWord(word,verb4[i].flags)->word;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = verb3[++i].word)) if (!stricmp(test,word)) return StoreWord(word,verb3[i].flags)->word;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = verb2[++i].word)) if (!stricmp(test,word)) return StoreWord(word,verb2[i].flags)->word;
	}
	return NULL;
}

char* English_GetThirdPerson(char* word)
{
    if (!word || !*word) return NULL;
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len);
	if (!D || (D->properties & VERB) == 0) return NULL; 

    if (D && D->properties & VERB_PRESENT_3PS)  return D->word; 

    if (D && D->properties & VERB && D->conjugation ) 
    {//   born (past) -> bore (a past and a present) -> bear 
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & VERB_PRESENT_3PS) return D->word; 
            D = GetTense(D); //   known irregular
        }
    }

	static char result[MAX_WORD_SIZE];
	strcpy(result,word);
	strcat(result,"s");
	return result;
}

char* English_GetInfinitive(char* word, bool nonew)
{
	verbFormat = 0;	//   secondary answer- std infinitive or unknown
    if (!word || !*word) return NULL;
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len);
    if (D && D->properties & VERB_INFINITIVE) 
	{
		verbFormat = VERB_INFINITIVE;
		return D->word; //    infinitive value
	}

    if (D && D->properties & VERB && D->conjugation ) 
    {//   born (past) -> bore (a past and a present) -> bear 
		if (D->properties & VERB_PRESENT_PARTICIPLE) verbFormat = VERB_PRESENT_PARTICIPLE;
		else if (D->properties & (VERB_PAST|VERB_PAST_PARTICIPLE)) 
		{
			verbFormat = 0;
			if (D->properties & VERB_PAST) verbFormat |= VERB_PAST;
			if (D->properties & VERB_PAST_PARTICIPLE) verbFormat |= VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE;
			
		}
		else if (D->properties & (VERB_PRESENT|VERB_PRESENT_3PS)) verbFormat = VERB_PRESENT;
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & VERB_INFINITIVE) return D->word; 
            D = GetTense(D); //   known irregular
        }
    }

	char last = word[len-1];  
    char prior = (len > 2) ? word[len-2] : 0;  //   Xs
    char prior1 = (len > 3) ? word[len-3] : 0; //   Xes
    char prior2 = (len > 4) ? word[len-4] : 0; //   Xhes
	char prior3 = (len > 5) ? word[len-5] : 0; //   Xhes

    //   check for multiword behavior. 
	int cnt = BurstWord(word,HYPHENS);
    if (cnt > 1)
    {
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		unsigned int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		unsigned int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			char* inf = GetInfinitive(words[i],false); //   is this word an infinitive?
			if (!inf) continue;
			*trial = 0;
			char* at = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at,inf);
					at += strlen(inf);
					*at++ = separators[j];
				}
				else
				{
					strcpy(at,words[j]);
					at += lens[j];
					*at++ = separators[j];
				}
			}
			WORDP E = FindWord(trial);
			if (E && E->properties & VERB_INFINITIVE) return E->word;
		}

       return NULL;  //   not a verb
    }

    //   not known verb, try to get present tense from it
    if (last == 'd' && prior == 'e' && len > 3)   //   ed ending?
    {
		verbFormat = VERB_PAST|VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE;

		// if vowel-vowel-consonant e d, prefer that
		if (len > 4 && !IsVowel(prior1) && IsVowel(prior2) && IsVowel(prior3))
		{
			D = FindWord(word,len-2);    //   drop ed
 			if (D && D->properties & VERB) return D->word;
		}

		D = FindWord(word,len-1);	//   drop d, on scare
		if (D && D->properties & VERB) return D->word;
        D = FindWord(word,len-2);    //   drop ed
        if (D && D->properties & VERB) return D->word; //   found it
        D = FindWord(word,len-1);    //   drop d
        if (D && D->properties & VERB) return D->word; //   found it
        if (prior1 == prior2  )   //   repeated consonant at end
        {
            D = FindWord(word,len-3);    //   drop Xed
            if (D && D->properties & VERB) return D->word; //   found it
        }
        if (prior1 == 'i') //   ied came from y
        {
            word[len-3] = 'y'; //   change i to y
            D = FindWord(word,len-2);    //   y, drop ed
            word[len-3] = 'i';
            if (D && D->properties & VERB) return D->word; //   found it
        }

		if (!buildDictionary && !nonew && !fullDictionary)
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if (len > 4 && !IsVowel(prior1) && IsVowel(prior2) && IsVowel(prior3))
			{
				wd[len-2] = 0;
			}
			else if (prior1 == prior2) // double last and add ed
			{
				wd[len-3] = 0;
			}
			else if (!IsVowel(prior2) && prior1 == 'i') // ied => y copied->copy
			{
				strcpy(wd+len-3,"y");
			}
			else if (!IsVowel(prior1) && IsVowel(prior2)) // Noted -> note 
			{
				wd[len-1] = 0;	// just chop off the s, leaving the e
			}
			else wd[len-2] = 0; // chop ed off
			return StoreWord(wd,VERB|VERB_PAST|VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE|ADJECTIVE)->word;
		}
     }
   
    //   could this be a participle verb we dont know about?
    if (prior1 == 'i' && prior == 'n' && last == 'g' && len > 4)//   maybe verb participle
    {
        char word1[MAX_WORD_SIZE];
		verbFormat = VERB_PRESENT_PARTICIPLE;
 
        //   try removing doubled consonant
        if (len > 4 &&  word[len-4] == word[len-5])
        {
            D = FindWord(word,len-4);    //   drop Xing spot consonant repeated
            if (D && D->properties & VERB) return D->word; //   found it
        }

        //   y at end, maybe came from ie
        if (word[len-4] == 'y')
        {
            strcpy(word1,word);
            word1[len-4] = 'i';
            word1[len-3] = 'e';
            word1[len-2] = 0;
            D = FindWord(word1,len-2);    //   drop ing but add ie
            if (D && D->properties & VERB) return D->word; //   found it
        }

        //   two consonants at end, see if raw word is good priority over e added form
        if (len > 4 && !IsVowel(word[len-4]) && !IsVowel(word[len-5])) 
        {
            D = FindWord(word,len-3);    //   drop ing
            if (D && D->properties & VERB) return D->word; //   found it
        }

        //   otherwise try stem with e after it
        strcpy(word1,word);
        word1[len-3] = 'e';
        word1[len-2] = 0;
        D = FindWord(word1,len-2);    //   drop ing and put back 'e'
        if (D && D->properties & VERB) return D->word; //   found it

        //   simple ing added to word
        D = FindWord(word,len-3);    //   drop ing
        if (D && D->properties & VERB) return D->word; //   found it

		if (!buildDictionary && !nonew)
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if (prior3 == prior2) // double last and add ing like swimming => swim
			{
				wd[len-4] = 0;
			}
			else wd[len-3] = 0; // chop ing off
			return StoreWord(wd,VERB|VERB_PRESENT_PARTICIPLE|NOUN_GERUND|NOUN)->word;
		}
	}
    //   ies from y
    if (prior1 == 'i' && prior == 'e' && last == 's' && len > 4)//   maybe verb participle
    {
 		verbFormat = VERB_PRESENT_3PS;
        char word1[MAX_WORD_SIZE];
        strcpy(word1,word);
        word1[len-3] = 'y';
        word1[len-2] = 0;
        D = FindWord(word1,len-2);    //   drop ing, add e
        if (D && D->properties & VERB) return D->word; //   found it
	}

     //   unknown singular verb
    if (last == 's' && len > 3 && prior != 'u' && prior != '\'') // but should not be "us" ending (adjectives)
    {
 		verbFormat = VERB_PRESENT_3PS;
        D = FindWord(word,len-1);    //   drop s
        if (D && D->properties & VERB && D->properties & VERB_INFINITIVE) return D->word; //   found it
		if (D && D->properties & NOUN) return NULL; //   dont move bees to be
        else if (prior == 'e')
        {
            D = FindWord(word,len-2);    //   drop es
            if (D && D->properties & VERB) return D->word; //   found it
        }

		if (!buildDictionary && !nonew)
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if ( prior == 'e' && prior1 == 'i') // was toadies  from toady
			{
				strcpy(wd+len-3,"y");
			}
			else wd[len-1] = 0; // chop off tail s
			return StoreWord(wd,VERB_PRESENT_3PS|VERB|NOUN|NOUN_SINGULAR)->word;
		}
   }

    if (IsHelper(word)) 
	{
		verbFormat = 0;
		return word;
	}
	if ( nonew || buildDictionary ) return NULL;
	verbFormat = VERB_INFINITIVE;
	return InferVerb(word,len);
}

char* English_GetPluralNoun(WORDP noun)
{
	if (!noun) return NULL;
    if (noun->properties & NOUN_PLURAL) return noun->word; 
    WORDP plural = GetPlural(noun);
	if (noun->properties & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR)) 
    {
        if (plural) return plural->word;
        static char word[MAX_WORD_SIZE];
		unsigned int len = noun->y.length;
		char end = noun->word[len-1];
		char before = (len > 1) ? (noun->word[len-2]) : 0;
		if (end == 's') sprintf(word,"%ses",noun->word); // glass -> glasses
		else if (end == 'h' && (before == 'c' || before == 's')) sprintf(word,"%ses",noun->word); // witch -> witches
		else if ( end == 'o' && !IsVowel(before)) sprintf(word,"%ses",noun->word); // hero -> heroes>
		else if ( end == 'y' && !IsVowel(before)) // cherry -> cherries
		{
			if (IsUpperCase(*noun->word)) sprintf(word,"%ss",noun->word); // Germany->Germanys
			else
			{
				strncpy(word,noun->word,len-1);
				strcpy(word+len-1,"ies"); 
			}
		}
		else sprintf(word,"%ss",noun->word);
        return word;
    }
    return noun->word;
}

static char* InferNoun(char* original,unsigned int len) // from suffix might it be singular noun? If so, enter into dictionary
{
	if (len == 0) len = strlen(original);
	char word[MAX_WORD_SIZE];
	strncpy(word,original,len);
	word[len] = 0;
	
	char* item;
	char* test;
	int i;
	if (len >= 10) // word of 3 + suffix of 7
	{
		test = word+len-7;
		i = -1;
		while ((item = noun7[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun7[i].flags)->word;
	}	
	if (len >= 9) // word of 3 + suffix of 6
	{
		test = word+len-6;
		i = -1;
		while ((item = noun6[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun6[i].flags)->word;
	}	
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = noun5[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun5[i].flags)->word;
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = noun4[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun4[i].flags)->word;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = noun3[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun3[i].flags)->word;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = noun2[++i].word)) if (!stricmp(test,word)) return StoreWord(word,noun2[i].flags)->word;
	}

	// ings (plural of a gerund like paintings)
	if (len > 6 && !stricmp(word+len-4,"ings"))
	{
		StoreWord(word,NOUN|NOUN_PLURAL);
		word[len-1] = 0;
		return StoreWord(word,NOUN|NOUN_SINGULAR)->word; // return the singular form
	}
		
	// ves (plural form)
	if (len > 4 && !strcmp(word+len-3,"ves") && IsVowel(word[len-4])) // knife
	{
		//Plurals of words that end in -f or -fe usually change the f sound to a v sound and add s or -es.
		word[len-3] = 'f';
		char* singular = GetSingularNoun(word,false,false);
		if (singular && !stricmp(singular,word)) 
		{
			nounFormat = NOUN_PLURAL;
			return StoreWord(singular,NOUN|NOUN_SINGULAR)->word;
		}
		word[len-2] = 'e';
		singular = GetSingularNoun(word,false,true);
		if (singular && !stricmp(singular,word)) 
		{
			nounFormat = NOUN_PLURAL;
			return StoreWord(singular,NOUN|NOUN_SINGULAR)->word;
		}
	}

	if (strchr(word,'_')) return NULL;		// dont apply suffix to multiple word stuff

	if (IsUpperCase(*word)) return StoreWord(word,NOUN|NOUN_PROPER_SINGULAR)->word;

	// if word is an abbreviation it is a noun (proper if uppcase)
	if (strchr(word,'.')) return StoreWord(word,NOUN|NOUN_SINGULAR)->word;

	// hypenated word check word at end
	char* hypen = strchr(word+1,'-');
	if ( hypen && len > 2)
	{
		char* stem = GetSingularNoun(word+1,true,false);
		if (stem && !stricmp(stem,word+1)) return StoreWord(word,NOUN|NOUN_SINGULAR)->word;
		if (stem)
		{
			strcpy(word+1,stem);
			return StoreWord(word,NOUN|NOUN_SINGULAR)->word;
		}
	}
	return NULL;
}

char* English_GetSingularNoun(char* word, bool initial, bool nonew)
{ 
	nounFormat = 0;
    if (!word) return NULL;
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,0,PRIMARY_CASE_ALLOWED);
	nounFormat = NOUN_SINGULAR;
	if (D && D->properties & NOUN_PROPER_SINGULAR) //   is already singular
	{
		nounFormat = NOUN_PROPER_SINGULAR;
		return D->word;
	}

    //   we know the noun and its plural, use singular
    if (D && D->properties & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR)) //   is already singular
    {
		nounFormat = D->properties & (NOUN_MASS|NOUN_SINGULAR|NOUN_PROPER_SINGULAR);
		if (word[len-1] == 's') // even if singular, if simpler exists, use that. Eg.,  military "arms"  vs "arm" the part
		{
			if (nonew)
			{
				WORDP E = FindWord(word,len-1);
				if  (E && E->properties & NOUN) return E->word;
			}
			char* sing = InferNoun(word,len);
			if (sing) return sing;
		}

        //   HOWEVER, some singulars are plurals--- words has its own meaning as well
        unsigned int len = D->y.length;
        if (len > 4 && D->word[len-1] == 's')
        {
            WORDP F = FindWord(D->word,len-1,PRIMARY_CASE_ALLOWED);
            if (F && F->properties & NOUN)  return F->word;  
        }
        return D->word; 
    }
	WORDP plural = (D) ? GetPlural(D) : NULL;
    if (D  && D->properties & NOUN_PLURAL && plural) 
	{
		nounFormat = NOUN_PLURAL;
		return plural->word; //   get singular form from plural noun
	}

	if (D && D->properties & NOUN && !(D->properties & NOUN_PLURAL) && !(D->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) return D->word; //   unmarked as plural, it must be singular unless its a name
    if (!D && IsNumber(word))  return word;
	if (D && D->properties & AUX_VERB) return NULL; // avoid "is" or "was" as plural noun

	// check known from plural s or es
	if (len > 2 && word[len-1] == 's')
	{
		char mod[MAX_WORD_SIZE];
		strcpy(mod,word);
		mod[len-1] = 0;
		char* singular = GetSingularNoun(mod,false,true);
		uint64 format = (IsUpperCase(*word)) ? NOUN_PROPER_PLURAL : NOUN_PLURAL; // would fail on iPhones BUG
		nounFormat = (IsUpperCase(*word)) ? NOUN_PROPER_PLURAL : NOUN_PLURAL; // would fail on iPhones BUG
		if (singular) return singular; // take off s is correct
		if (word[len-2] == 'e') // es
		{
			mod[len-2] = 0;
			singular = GetSingularNoun(mod,false,true);
			nounFormat = format;
			if (singular) return singular; // take off es is correct
			//With words that end in a consonant and a y, you'll need to change the y to an i and add es
			if (len > 4 && mod[len-3] == 'i' && !IsVowel(mod[len-4]))
			{
				mod[len-3] = 'y';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ies is correct, change to y
			}

			//Plurals of words that end in -f or -fe usually change the f sound to a v sound and add s or -es.
			if (len > 4 && mod[len-3] == 'v')
			{
				mod[len-3] = 'f';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ves is correct, change to f
				mod[len-2] = 'e';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ves is correct, change to fe
			}
		}
	}
	if ( nonew || buildDictionary ) return NULL;

	nounFormat = (IsUpperCase(*word)) ? NOUN_PROPER_SINGULAR : NOUN_SINGULAR;
	return InferNoun(word,len);
}


static char* InferAdverb(char* original, unsigned int len) // might it be adverb based on suffix? if so, enter into dictionary
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;

	char* item;
	char* test;
	int i;
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = adverb4[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adverb4[i].flags)->word;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = adverb3[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adverb3[i].flags)->word;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = adverb2[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adverb2[i].flags)->word;
	}

	//est
	if (len >= 3 && !strcmp(word+len-3,"est"))
	{
		adjectiveFormat = MOST_FORM;
		WORDP E = StoreWord(word,ADVERB|ADVERB_NORMAL,MOST_FORM);
		WORDP base = E;
		if (word[len-3] == word[len-4]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADVERB|ADVERB_NORMAL,BASIC_FORM);
			E->comparison = base;
			base->comparison = E;
		}
		return base->word;
	}
	//er
	if (len >= 3 && !strcmp(word+len-2,"er")  )
	{
		adjectiveFormat = MORE_FORM;
		WORDP E = StoreWord(word,ADVERB|ADVERB_NORMAL,MORE_FORM);
		WORDP base = E;
		if (word[len-3] == word[len-4]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADVERB,BASIC_FORM);
			E->comparison = base;
			base->comparison = E;
		}
		return base->word;
	}
	return NULL;
}

char* English_GetAdverbBase(char* word, bool nonew)
{
 	adverbFormat = 0;
    if (!word) return NULL;
	if (IsUpperCase(*word)) return NULL; // not as proper
    size_t len = strlen(word);
    if (len == 0) return NULL;
    char lastc = word[len-1];  
    char priorc = (len > 2) ? word[len-2] : 0; 
    char prior2c = (len > 3) ? word[len-3] : 0; 
    WORDP D = FindWord(word,0);
	adverbFormat = BASIC_FORM;
    if (D && D->properties &  QWORD) return D->word; //   we know it as is
	if (D && D->systemFlags & BASIC_FORM ) return D->word; //   we know it as is

	if (D && D->properties & ADVERB)
    {
        int n = 5;
		WORDP original = D;
        while (--n  && D)
        {
            D = GetComparison(D);
            if (D && !(D->systemFlags & (MORE_FORM|MOST_FORM))) 
			{
				if (original->systemFlags & MORE_FORM) adjectiveFormat = MOST_FORM;
				else if (original->systemFlags & MOST_FORM) adjectiveFormat = MOST_FORM;
				return D->word;
			}
        }
    }
 
    if (len > 4 && priorc == 'l' && lastc == 'y')
    {
		char form[MAX_WORD_SIZE];
        D = FindWord(word,len-2); // rapidly
        if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
		if (prior2c == 'i')
		{
			D = FindWord(word,len-3); // lustily
			if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
			// if y changed to i, change it back
			strcpy(form,word);
			form[len-3] = 'y';
			form[len-2] = 0;
			D = FindWord(word,len-2); // happily  from happy
			if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
		}
		// try terrible -> terribly
		strcpy(form,word);
		form[len-1] = 'e';
 		D = FindWord(word,len-2); // happily  from happy
		if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
    }
	if (len > 5 && priorc == 'e' && lastc == 'r')
    {
        D = FindWord(word,len-2);
        if (D && D->properties & ADVERB) 
		{
			adverbFormat = MORE_FORM;
			return D->word;
		}
    }
	if (len > 5 && prior2c == 'e' && priorc == 's' && lastc == 't')
    {
        D = FindWord(word,len-3);
        if (D && D->properties & ADVERB) 
		{
			adverbFormat = MORE_FORM;
			return D->word;
		}
    }
	if ( nonew || buildDictionary) return NULL;
	
	return InferAdverb(word,len);
}


static char* InferAdjective(char* original, unsigned int len) // might it be adjective based on suffix? If so, enter into dictionary
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;

	int i;
	char* test;
	char* item;
	if (len >= 10) // word of 3 + suffix of 7
	{
		test = word+len-7;
		i = -1;
		while ((item = adjective7[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective7[i].flags)->word;
	}
	if (len >= 9) // word of 3 + suffix of 6
	{
		test = word+len-6;
		i = -1;
		while ((item = adjective6[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective6[i].flags)->word;
	}
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = adjective5[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective5[i].flags)->word;
	}
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = adjective4[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective4[i].flags)->word;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = adjective3[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective3[i].flags)->word;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = adjective2[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective2[i].flags)->word;
	}
	if (len >= 4) // word of 3 + suffix of 1
	{
		test = word+len-1;
		i = -1;
		while ((item = adjective1[++i].word)) if (!stricmp(test,word)) return StoreWord(word,adjective1[i].flags)->word;
	}

	// est -  comparative
	if (len >= 4 &&  !strcmp(word+len-3,"est")  )
	{
		adjectiveFormat = MOST_FORM;
		WORDP E = StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,MOST_FORM);
		WORDP base = E;
		if (word[len-4] == word[len-5]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-3] = 0;
			base =  StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM);
			E->comparison = base;
			base->comparison = E;
		}
		
		return base->word;
	}

	// er -  comparative
	if (len >= 4 &&  !strcmp(word+len-2,"er")  )
	{
		adjectiveFormat = MORE_FORM;
		WORDP E = StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,MORE_FORM);
		WORDP base = E;
		if (word[len-3] == word[len-4]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,BASIC_FORM);
			E->comparison = base;
			base->comparison = E;
		}
		return base->word;
	}

	return 0;
}


char* English_GetAdjectiveBase(char* word, bool nonew)
{
	adjectiveFormat = 0;
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,0);
	char lastc = word[len-1];  
    char priorc = (len > 2) ? word[len-2] : 0;  //   Xs
    char priorc1 = (len > 3) ? word[len-3] : 0; //   Xes
    char priorc2 = (len > 4) ? word[len-4] : 0; //   Xhes
    char priorc3 = (len > 5) ? word[len-5] : 0; //   Xgest
 
    if (D && D->properties & ADJECTIVE && D->systemFlags & BASIC_FORM)
	{
		adjectiveFormat = BASIC_FORM;
		return D->word; //   already base
	}
    if (D && D->properties & ADJECTIVE)
    {
        int n = 5;
		WORDP original = D;
        while (--n  && D)
        {
            D = GetComparison(D);
            if (D && !(D->systemFlags & (MORE_FORM|MOST_FORM))) // this is the base
			{
				if (original->systemFlags & MORE_FORM) adjectiveFormat = MORE_FORM;
				else if (original->systemFlags & MOST_FORM) adjectiveFormat = MOST_FORM;
				return D->word;
			}
        }
    }
 
    //   see if composite
    char composite[MAX_WORD_SIZE];
    strcpy(composite,word);
    char* hyphen = strchr(composite+1,'-');
    if (hyphen)
    {
        hyphen -= 4;
		char* althyphen = (hyphen - composite) + word;
        if (hyphen[2] == 'e' && hyphen[3] == 'r') //   lower-density?
        {
            strcpy(hyphen+2,althyphen+4); //   remove er
            char* answer = GetAdjectiveBase(composite,false);
            if (answer) return answer;
        }
        if (hyphen[1] == 'e' && hyphen[2] == 's' && hyphen[2] == 't' ) //   lowest-density?
        {
            strcpy(hyphen+1,althyphen+4); //   remove est
            char* answer = GetAdjectiveBase(composite,false);
            if (answer) return answer;
        }
    }

    if (len > 4 && priorc == 'e' && lastc == 'r') //   more
    {
		 adjectiveFormat = MORE_FORM;
         D = FindWord(word,len-2);
         if (D && D->properties & ADJECTIVE) return D->word; //   drop er
         D = FindWord(word,len-1);
         if (D && D->properties & ADJECTIVE) return D->word; //   drop e (already ended in e)

         if (priorc1 == priorc2  )  
         {
            D = FindWord(word,len-3);
            if (D && D->properties & ADJECTIVE) return D->word; //   drop Xer
         }
         if (priorc1 == 'i') //   changed y to ier?
         {
            word[len-3] = 'y';
            D = FindWord(word,len-2);
            word[len-3] = 'i';
            if (D && D->properties & ADJECTIVE) return D->word; //   drop Xer
          }
	}  
	else if (len > 5 && priorc1 == 'e' &&  priorc == 's' && lastc == 't') //   most
    {
		adjectiveFormat = MOST_FORM;
        D = FindWord(word,len-3);//   drop est
        if (D && D->properties & ADJECTIVE) return D->word; 
        D = FindWord(word,len-2);//   drop st (already ended in e)
        if (D && D->properties & ADJECTIVE) return D->word; 
        if (priorc2 == priorc3  )   
        {
             D = FindWord(word,len-4);//   drop Xest
             if (D && D->properties & ADJECTIVE) return D->word; 
        }
        if (priorc2 == 'i') //   changed y to iest
        {
             word[len-4] = 'y';
             D = FindWord(word,len-3); //   drop est
             word[len-4] = 'i';
             if (D && D->properties & ADJECTIVE) return D->word; 
        }   
    }
	if ( nonew || buildDictionary) return NULL;
	
	return InferAdjective(word,len);
}
