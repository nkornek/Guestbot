#ifndef _DICTIONARYSYSTEMH_
#define _DICTIONARYSYSTEMH_

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


typedef unsigned int DICTINDEX;	//   indexed ref to a dictionary entry

//   This file has formatting restrictions because it is machine read by AcquireDefines. All macro and function calls
//   must keep their ( attached to the name and all number defines must not.
//   All tokens of a define must be separated by spaces ( TEST|TEST1  is not legal).

//   These are the properties of a dictionary entry.
//   IF YOU CHANGE THE VALUES or FIELDS of a dictionary node,
//   You need to rework ReadDictionary/WriteDictionary/ReadBinaryEntry/WriteBinaryEntry

// D->properties bits

//   BASIC_POS POS bits like these must be on the bottom of any 64-bit word, so they match MEANING values (at top of 32 bit) as well.
#define NOUN					0x0000000080000000ULL
#define VERB					0x0000000040000000ULL
#define ADJECTIVE				0x0000000020000000ULL	// PENNBANK: JJ when not JJR or JJS
#define ADVERB					0x0000000010000000ULL   // PENNBANK: RB when not RBR or RBS
#define BASIC_POS				( NOUN | VERB | ADJECTIVE | ADVERB )
//   the above four should be first, reflecting the Wordnet files (leaving room for wordnet offset)

#define PREPOSITION				0x0000000008000000ULL  // Pennbank: IN
#define ESSENTIAL_FLAGS			( BASIC_POS | PREPOSITION ) //  these can be type restrictions on MEANINGs

// ESSENTIAL_FLAGS above are shared onto systemflags for most likely pos-tag candidate type
#define CONJUNCTION_COORDINATE  0x0000000004000000ULL //   multiword separable verb (the preposition component) (full verb marked w systemflag SEPARABLE_PHRASAL_VERB) 
#define CONJUNCTION CONJUNCTION_COORDINATE			 // alternate label--- must be right after preposition for tag priority

#define POSTAG_PRIORITY		( NOUN | VERB  | ADJECTIVE  | ADVERB |  PREPOSITION |  CONJUNCTION )

// kinds of conjunctions
#define CONJUNCTION_SUBORDINATE 0x0000000002000000ULL  //   Articles: a and the --  demonstratives: this that these those --  Quantifiers: much many some a_little  -- Possessive pronouns: my your
#define CONJUNCTION_BITS		( CONJUNCTION_COORDINATE | CONJUNCTION_SUBORDINATE )

// kinds of determiners (there is no master bit for determiners)
#define PREDETERMINER 			0x0000000001000000ULL  
#define DETERMINER  			0x0000000000800000ULL   
#define DETERMINER_BITS		   ( DETERMINER | PREDETERMINER )

#define POSSESSIVE				0x0000000000400000ULL	// is a possessive like 's or Taiwanese (but not possessive pronoun)
#define AUX_VERB				0x0000000000200000ULL

#define FOREIGN_WORD			0x0000000000100000ULL					
#define COMMA					0x0000000000080000ULL	

// kinds of adjectives
#define ADJECTIVE_NOUN			0X0000000000040000ULL	// german PREPOSITION_DETERMINER  - noun used as an adjective in front of another ("bank clerk")
#define ADJECTIVE_NORMAL		0x0000000000020000ULL	// "friendly" // "friendlier" // "friendliest"
#define ADJECTIVE_PARTICIPLE	0x0000000000010000ULL	// the "walking" dead  or  the "walked" dog
#define ADJECTIVE_CARDINAL		0x0000000000008000ULL	// the five dogs
#define ADJECTIVE_ORDINAL		0x0000000000004000ULL	// the fifth dog
#define ADJECTIVE_BITS ( ADJECTIVE_NOUN | ADJECTIVE_NORMAL | ADJECTIVE_PARTICIPLE | ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL )

// kinds of adverbs
#define ADVERB_NORMAL			0x0000000000002000ULL	
#define ADVERB_BITS				( ADVERB_NORMAL )

#define PAREN 					 0x0000000000001000ULL					
#define PARTICLE				0x0000000000000800ULL	
#define THERE_EXISTENTIAL		0x0000000000000400ULL	 // "There" is no future in it.

// kinds of pronouns
#define PRONOUN_SUBJECT			0x0000000000000200ULL	// I you he she it we  they
#define PRONOUN_POSSESSIVE		0x0000000000000100ULL	// my your his her its our their
#define PRONOUN_OBJECT			0x0000000000000080ULL	// me, you, him, her, etc
#define PRONOUN_BITS	( PRONOUN_OBJECT | PRONOUN_SUBJECT | PRONOUN_POSSESSIVE ) // there is no pronoun flag, just the bits

#define FINITE_SET_WORDS ( PREPOSITION | DETERMINER | PREDETERMINER | CONJUNCTION_SUBORDINATE | CONJUNCTION_COORDINATE | AUX_VERB | PRONOUN_BITS )

#define PART_OF_SPEECH		( FINITE_SET_WORDS  | BASIC_POS   ) 

#define NORMAL_WORD			( PART_OF_SPEECH | FOREIGN_WORD )

// kinds of nouns
#define NOUN_SINGULAR			0x0000000000000040ULL  // Pennbank: NN
#define NOUN_PLURAL				0x0000000000000020ULL // Pennbank: NNS
#define NOUN_PROPER_SINGULAR	0x0000000000000010ULL  //   A proper noun that is NOT a noun is a TITLE like Mr.
#define NOUN_PROPER_PLURAL		0x0000000000000008ULL  // ( or )
#define NOUN_GERUND				0x0000000000000004ULL	// "Walking" is fun
#define NOUN_CARDINAL			0x0000000000000002ULL  // I followed the "20".
#define NOUN_ORDINAL 			0x0000000000000001ULL	
#define NOUN_BITS ( NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR  | NOUN_PROPER_PLURAL | NOUN_GERUND | NOUN_CARDINAL | NOUN_ORDINAL )
#define NORMAL_NOUN_BITS ( NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR  | NOUN_PROPER_PLURAL )

#define POSSESSIVE_BITS ( PRONOUN_POSSESSIVE | POSSESSIVE )

// above 32 bits can be see in pos patterns.


//////////////////////////////16 bits below here can not be used in posValues[] of tagger  
#define AS_IS					0x8000000000000000ULL   //  TRANSIENT INTERNAL dont try to reformat the word (transient flag passed to StoreWord)
#define NOUN_HUMAN				0x4000000000000000ULL  //   person or group of people that uses WHO, he, she, anyone
#define NOUN_FIRSTNAME			0x2000000000000000ULL  //   a known first name -- wiULL also be a sexed name probably
#define NOUN_SHE				0x1000000000000000ULL	//   female sexed word (used in sexed person title detection for example)
#define NOUN_HE					0x0800000000000000ULL	//   male sexed word (used in sexed person title detection for example)
#define NOUN_THEY				0x0400000000000000ULL   
#define NOUN_TITLE_OF_ADDRESS	0x0200000000000000LL	//   eg. mr, miss
#define NOUN_TITLE_OF_WORK		0x0100000000000000ULL
#define LOWERCASE_TITLE			0X0080000000000000ULL	//   lower case word may be in a title (but not a noun)
#define NOUN_ABSTRACT			0x0040000000000000ULL	// can be an abstract noun (maybe also concrete)
#define POSTPOSITION_PREPOSITION 0x0020000000000000ULL  // german postposition preposition
#define QWORD					0x0010000000000000ULL	// who what where why when how whose -- things that can start a question
#define NOUN_ROLE				0x0008000000000000ULL	//  human role (like doctor or pilot or ally)
#define NOUN_NODETERMINER	 	0x0004000000000000ULL	// nouns of location that require no determiner (like "Home is where the heart is")
#define NOUN_MASS				0x0002000000000000ULL	// tagger must use POSSIBLEMASSNOUN test since bit not available
#define PUNCTUATION				0x0001000000000000ULL	// COMMA is a specific punctuation, as is paren

//////////////////////////////bits below here are used in posValues[] of tagger  0x0000 are used in tagger internals...


// kinds of verbs (tenses)
#define TO_INFINITIVE	 		0x0000800000000000ULL	// 20th horse
#define STARTTAGS				0x0000800000000000ULL	// the top bit of the 48 bits visible to tagger

#define VERB_PRESENT			0x0000400000000000ULL	// present plural (usually infinitive)
#define VERB_PRESENT_3PS		0x0000200000000000ULL	// 3rd person singular singular
#define VERB_PRESENT_PARTICIPLE 0x0000100000000000ULL	// GERUND,  Pennbank: VBG
#define VERB_PAST				0x0000080000000000ULL	// Pennbank: VBD
#define VERB_PAST_PARTICIPLE    0x0000040000000000ULL	// Pennbank VBN
#define VERB_INFINITIVE			0x0000020000000000ULL	//   all tense forms are linked into a circular ring
#define NOUN_INFINITIVE			0x0000010000000000ULL	
#define VERB_TENSES (  VERB_INFINITIVE | VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST | VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE  )

#define QUOTE 					0x0000008000000000ULL	// quoted string
#define CIRCUMPOSITION			QUOTE					// GERMAN 
#define POSTPOSITION			0x0000004000000000ULL	// GERMAN
#define	AUX_DO 					0x0000002000000000ULL	
#define AUX_HAVE				0x0000001000000000ULL 
		

// kinds of aux verbs (tenses)
#define	AUX_BE	0x0000000800000000ULL	
#define AUX_VERB_PRESENT		0x0000000400000000ULL
#define AUX_VERB_FUTURE			0x0000000200000000ULL
#define AUX_VERB_PAST			0x0000000100000000ULL
#define AUX_VERB_TENSES ( AUX_VERB_PRESENT | AUX_VERB_FUTURE | AUX_VERB_PAST ) // modal verbs
#define AUX_VERB_BITS ( AUX_VERB_TENSES | AUX_BE | AUX_HAVE | AUX_DO )

// + particles (which we treat as derived from prepositions) + interjections 

#define NOUN_TITLE		 (  NOUN | NOUN_TITLE_OF_WORK | NOUN_PROPER_SINGULAR )
#define NOUN_HUMANGROUP		( NOUN | NOUN_HUMAN | NOUN_TITLE_OF_WORK | NOUN_THEY )
#define NOUN_MALEHUMAN     ( NOUN | NOUN_HUMAN | NOUN_HE | NOUN_PROPER_SINGULAR )
#define NOUN_FEMALEHUMAN    ( NOUN |  NOUN_HUMAN | NOUN_SHE | NOUN_PROPER_SINGULAR )
#define NOUN_HUMANNAME ( NOUN_HUMAN | NOUN | NOUN_PROPER_SINGULAR )
#define NOUN_PROPERTIES ( NOUN_ROLE | NOUN_MASS | NOUN_HE | NOUN_THEY | NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_ABSTRACT | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_SHE | NOUN_TITLE_OF_ADDRESS | NOUN_TITLE_OF_WORK  )
#define NUMBER_BITS ( NOUN_CARDINAL | NOUN_ORDINAL | ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL )
#define NOUN_DESCRIPTION_BITS ( ADJECTIVE_BITS | DETERMINER  | PREDETERMINER  | ADVERB_BITS  | PRONOUN_POSSESSIVE  | POSSESSIVE | NOUN_BITS )
#define FULL_NOUN_BITS ( NOUN_BITS | NOUN_INFINITIVE )
#define NOUN_PROPER ( NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_PROPERTIES )

#define NUMBER_PROPERTIES ( ADJECTIVE_CARDINAL | ADJECTIVE_ORDINAL | NOUN_CARDINAL | NOUN_ORDINAL )

#define VERB_PROPERTIES (  VERB_TENSES )

#define TAG_TEST ( QUOTE | COMMA | PAREN | PARTICLE | VERB_TENSES | NOUN_BITS | FOREIGN_WORD | NOUN_INFINITIVE | PREDETERMINER | DETERMINER | ADJECTIVE_BITS | AUX_VERB_BITS | ADVERB_BITS  | PRONOUN_BITS | CONJUNCTION_BITS | POSSESSIVE | THERE_EXISTENTIAL | PREPOSITION | TO_INFINITIVE )

#define NOUN_PHRASE_BITS		( DETERMINER | ADJECTIVE_BITS | ADVERB_BITS | NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_CARDINAL )

// system flags -- THIS IS A REQUIRED MARKER FOR BELOW ZONE

// english word attribues 
// PREP_INTRODUCTION			0x0000000000000001ULL
// PREP_RELATION				0x0000000000000002ULL
#define PREP_TIME					0x0000000000000004ULL
#define PREP_LOCATION				0x0000000000000008ULL

#define VERB_CONJUGATE3				0x0000000000000010ULL	//	3rd word of composite verb conjugates	
#define VERB_CONJUGATE2				0x0000000000000020ULL	//   2nd word of composite verb conjugates  (e.g., re-energize)
#define VERB_CONJUGATE1				0x0000000000000040ULL	//   1st word of composite verb conjugates  (e.g. peter_out)
#define LINKING_VERB				0x0000000000000080ULL 	//  be seem etc (copular verb) links adjectives and adjective participles to subjects
#define SEPARABLE_PHRASAL_VERB		0x0000000000000100ULL	// accepts particles

// these apply to adjectives AND adverbs
#define BASIC_FORM					0x0000000000000200ULL
#define MORE_FORM					0x0000000000000400ULL  
#define MOST_FORM					0x0000000000000800ULL  

#define VERB_INDIRECTOBJECT			0x0000000000001000ULL 		
#define VERB_DIRECTOBJECT			0x0000000000002000ULL  
#define POTENTIAL_CLAUSE_STARTER 	0x0000000000004000ULL				

#define ANIMATE_BEING 				0x0000000000008000ULL 	
#define TIMEWORD 					0x0000000000010000ULL 

#define ACTUAL_TIME					0x0000000000020000ULL	// a time word like 14:00
#define WEB_URL						0x0000000000040000ULL	// an url
#define COMMON_PARTICIPLE_VERB		0x0000000000080000ULL	// will be adjective after "be or seem" rather than treated as a verb participle

#define GRADE5_6					0x0000000000100000ULL
#define GRADE3_4					0x0000000000200000ULL
#define GRADE1_2  					0x0000000000400000ULL
#define KINDERGARTEN				0x0000000000800000ULL
#define AGE_LEARNED ( KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6 )  // adult is all the rest

#define OMITTABLE_TIME_PREPOSITION	0x0000000001000000ULL // can be used w/o preposition
#define ALWAYS_PROPER_NAME_MERGE	0x0000000002000000ULL // if see in any form in input, do proper name merge upon it
#define CONJUNCT_SUBORD_NOUN 		0x0000000004000000ULL	  

// these match the values in properties above
// PREPOSITION TIEBREAK				0x0000000008000000ULL
// ADVERB TIEBREAK					0x0000000010000000ULL
// ADJECTIVE TIEBREAK				0x0000000020000000ULL
// VERB TIEBREAK					0x0000000040000000ULL
// NOUN TIEBREAK					0x0000000080000000ULL

#define OTHER_PLURAL				0x0000000100000000ULL	
#define OTHER_SINGULAR				0x0000000200000000ULL	
#define VERBS_ACCEPTING_OF_AFTER	0x0000000400000000ULL // of can follow this verb
#define PREDETERMINER_TARGET		0x0000000800000000ULL	// predeterminer can come before these (a an the)

#define ADJECTIVE_POSTPOSITIVE		0x0000001000000000ULL	// adjective can occur AFTER the noun
#define EXTENT_ADVERB				0x0000002000000000ULL	
#define EXISTENTIAL_BE				0x0000004000000000ULL	// "there is or there seem"  # copylar verbs, taking particple and other adjective linking to subject
#define NO_PROPER_MERGE				0x0000008000000000ULL	// do not merge this word into any proper name

// wordnet sentence prototypes
#define DIRECTOBJECT_TOINFINITIVE_COMPLEMENT		0x0000010000000000ULL   // proto 24  --  verbs taking to infinitive after object: "Somebody ----s somebody to INFINITIVE"  "I advise you *to go"
#define DIRECTOBJECT_INFINITIVE_COMPLEMENT		0x0000020000000000ULL	// proto 25  -  verbs that take direct infinitive after object  "Somebody ----s somebody INFINITIVE"  "i heard her sing"
#define TOINFINITIVE_DIRECTOBJECT		0x0000040000000000ULL   // proto 28 "Somebody ----s to INFINITIVE"   "we agreed to plan"
#define DIRECTINFINITIVE_DIRECTOBJECT	0x0000080000000000ULL	// proto 32, 35 "Somebody ----s INFINITIVE"   "Something ----s INFINITIVE"
//  ~factitive_adjective_Verbs take object complement adjective after direct object noun
// ~factitive_noun_verbs take object complement noun after direct object noun
// ~adjectivecomplement_taking_noun_infinitive adjectives can take a noun to infinitive after them as adjective "I was able to go"
//  ~causal_to_infinitive_verbs  verb indirectobject "to infinitive maybe with to and maybe not" as direct object 


#define ADJECTIVE_INFLECTED			0x0000080000000000ULL  // german
#define PROTO5						0x0000100000000000ULL
#define ADJECTIVE_UNINFLECTED		0x0000100000000000ULL	// german
#define PROTO6		0x0000200000000000ULL	

#define PRONOUN_REFLEXIVE			0x0000400000000000ULL 

// 16 bits CANNOT REFERENCE BELOW IN POS RULES - chatscript internal markers

// transient over a duration of volley or compile
#define IN_DICTIONARY				0x0000800000000000ULL  // keep in dictionary during dict build
#define PATTERN_ALLOWED				0x0001000000000000ULL	// dont warn about spelling during script compile
#define CONJUNCT_SUBORD_ADVERB 		0x0002000000000000ULL	// a kind of coordinating conjunction that only coordinates clauses- listed as subordinate conjunction with this flag

// chatscript roles of dictionary entry
#define SUBSTITUTE					0x0004000000000000ULL  //   word has substitute attached 
#define SUBSTITUTE_RECIPIENT		0x0008000000000000ULL  
#define QUERY_KIND					0x0010000000000000ULL	// is a query item
#define FUNCTION_NAME				0x0020000000000000ULL 	//   name of a ^function  (has non-zero ->x.codeIndex if system, else is user) only applicable to ^ words
#define CONCEPT						0x0040000000000000ULL	// topic or concept has been read via a definition
#define TOPIC_NAME					0x0080000000000000ULL	//  this is a ~xxxx topic name in the system - only applicable to ~ words
#define IS_TABLE_MACRO				0x0100000000000000ULL	// function is a table macro
#define IS_PATTERN_MACRO			0x0200000000000000ULL	// pattern vs output use

// chatscript properties
#define HAS_EXCLUDE					0x0400000000000000ULL	// concept/topic has keywords to exclude
#define BUILD0						0x0800000000000000ULL	// comes from build0 data (marker on functions, concepts, topics)
#define BUILD1						0x1000000000000000ULL	// comes from build1 data
#define PATTERN_WORD 				0x2000000000000000ULL

#define DELAYED_RECURSIVE_DIRECT_MEMBER		0x4000000000000000ULL  // concept will be built with all indirect members made direct
#define TRACE_MACRO					0x8000000000000000ULL	// turn on tracing for this function

// CANNOT REFERNCE ABOVE IN POS TAGGING RULES
#define NO_EXTENDED_WRITE_FLAGS ( PATTERN_ALLOWED | HAS_EXCLUDE | SUBSTITUTE_RECIPIENT | PATTERN_WORD | FUNCTION_NAME | IS_TABLE_MACRO | IS_PATTERN_MACRO |  BUILD0 | BUILD1 | QUERY_KIND | PATTERN_ALLOWED | TOPIC_NAME | SUBSTITUTE )

// system flags that get marks
#define MARK_FLAGS ( KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6 | BASIC_FORM | MORE_FORM |  MOST_FORM | TIMEWORD | ACTUAL_TIME | WEB_URL | PREP_TIME | PREP_LOCATION )

// end system flags -- THIS IS A REQUIRED MARKER FOR ABOVE ZONE

// flags on facts

// USER FLAGS: 0xffff0000ULL

#define USER_FLAGS			0xFFFFF000
#define SYSTEM_FLAGS		0x00000FFF

// transient flags
#define MARKED_FACT         0x00000800  //   TRANSIENT : used during inferencing sometimes to see if fact is marked
#define MARKED_FACT2        0x00000400  //   TRANSIENT: used during inferencing sometimes to see if 2ndary fact is marked
#define FACTDEAD			0x00000200  //   has been killed off
#define FACTTRANSIENT       0x00000100  //   save only with a set, not with a user or base system

// permanent flags
#define FACTSUBJECT         0x00000080  //   index is - relative number to fact 
#define FACTVERB			0x00000040	//   is 1st in its bucket (transient flag for read/WriteBinary) which MIRRORS DICT BUCKETHEADER flag: 
#define FACTOBJECT		    0x00000020  //   does not apply to canonical forms of words, only the original form - for classes and sets, means dont chase the set
#define FACTDUPLICATE		0x00000010	//   allow repeats of this face

//#define UNUSEDF3	        0x00000008 
//#define UNUSEDF2	        0x00000004  
//#define UNUSEDF1	        0x00000002 
#define ORIGINAL_ONLY       0x00000001  //  dont match on canonicals


// topic control flags

// permanent flags 
#define TOPIC_KEEP 1		// don't erase rules
#define TOPIC_REPEAT 2      // allow repeated output
#define TOPIC_RANDOM 4      // random access responders (not gambits)
#define TOPIC_SYSTEM 8		// combines NOSTAY, KEEP, and special status on not accessing its gambits  
#define TOPIC_NOSTAY 16		// do not stay in this topic
#define TOPIC_PRIORITY 32	// prefer this more than normal
#define TOPIC_LOWPRIORITY 64 // prefer this less than normal
#define TOPIC_NOBLOCKING 128 // :verify should not complain about blocking
#define TOPIC_NOKEYS 256	// :verify should not complain about keywords missing
#define TOPIC_SAFE	512		// update safe

//   TRANSIENT FLAGS
#define TOPIC_GAMBITTED 1024	//   (transient per user) gambit issued from this topic
#define TOPIC_RESPONDED 2048	//   (transient per user) responder issued from this topic
#define TOPIC_REJOINDERED 4096	//   (transient per user) rejoinder issued from this topic
#define TOPIC_BLOCKED 8192		//   (transient per user) disabled by some users 
#define TOPIC_USED 16384		//   (transient per user) indicates need to write out the topic

// pos tagger result operators
#define DISCARD 1
#define KEEP 2
#define TRACE 8

// pos tagger pattern values   5 bits (0-31) + 3 flags
#define HAS 1 // any bit matching 
#define IS 2	// is exactly this
#define INCLUDE 3 // has bit AND has other bits
#define ISORIGINAL 4 // is this word
#define POSITION 5 // sentence boundary
#define START POSITION
#define END POSITION
#define ISCANONICAL 6 // canonical word is this
#define ISMEMBER 7	// is member of this set
#define RESETLOCATION 8	
#define HAS2VERBS 9	
#define CANONLYBE  10	// has all these bits by category 
#define HASPROPERTY 11
#define HASALLPROPERTIES 12	
#define ENDSWITH 13			// suffix it ends in
#define ORIGINALVALUE 14		
#define HASCANONICALPROPERTY 15
#define ISQWORD 16
#define ISQUESTION 17
#define ISABSTRACT 18
#define NONEAFTER 19
#define NONEBEFORE 20
#define POSSIBLEMASSNOUN 21
// 22
#define PRIORCANONICAL 23
// 24
// 25
// 26
#define LASTCONTROL PRIORCANONICAL  // add new ops to optable as well
	
#define SKIP 1 // if it matches, move ptr along, if it doesnt DONT
#define STAY 2
#define NOTCONTROL 4 

// pos tagger roles and states on roles[] && needRoles[] 
// needRoles values can be composed of multiple choices. roles are exactly one choice
#define MAINSUBJECT			1		// 0x000001			// noun roles like Mainsubject etc are ordered lowest first for pronoun IT priority
#define MAINVERB			2		// 0x000002
#define MAINOBJECT			4		// 0x000004
#define MAININDIRECTOBJECT	8		// 0x000008

#define SUBJECT2			16		// 0x000010
#define VERB2				32		// 0x000020
#define OBJECT2				64		// 0x000040
#define INDIRECTOBJECT2		128		// 0x000080

#define APPOSITIVE			256		// 0x000100			2nd noun restates first
#define ADJECTIVE_COMPLEMENT	512		// 0x000200		2ndary modifier of an adjective "he was ready *to *go"	    noun clause or a prepositional phrase 
// 		1024	// 0x000400	 

#define SUBJECT_COMPLEMENT		2048	// 0x000800   main object after a linking verb is an adjective "he seems *strong" (aka subject complement) -- a prep phrase instead is also considered a subject complement
#define REFLEXIVE		 	4096    //  0x0001000   	

#define PHRASE							0x0002000 	
#define VERBAL 							0x0004000	 

// what does coordinate conjunction conjoin (8 choices = 4 bits) -- NOT available on needRoles 
#define CONJUNCT_PHRASE					0x0008000 	
#define CONJUNCT_CLAUSE 				0x0010000 	
#define CONJUNCT_SENTENCE				0x0018000	
#define CONJUNCT_PARTICLE 				0x0020000	
#define CONJUNCT_NOUN					0x0028000	
#define CONJUNCT_VERB 					0x0030000	
#define CONJUNCT_ADJECTIVE				0x0038000	
#define CONJUNCT_ADVERB					0x0040000

#define POSTNOMINAL_ADJECTIVE			0x0080000	// true postnominal adjective or verbal that acts as postnominal adjective  (noun role is named also, so anonymous means adverb role)
//										0x0100000
//										0x0200000
//										0x0400000
#define NOT								0x0800000
#define ADDRESS 						0x1000000  // for factitive verbs
#define OBJECT_COMPLEMENT				0x2000000  // can be noun or adjective or to/infinitive after direct object..."that should keep them *happy" "I knight you *Sir *Peter" (verb expects it)
//			0x4000000  

#define COMMA_PHRASE 					0x10000000 	
#define CLAUSE 							0x20000000 	
#define OMITTED_TIME_PREP 				0x40000000 	
#define ADVERB_CLAUSE 					0x80000000 

#define KINDS_OF_PHRASES ( CLAUSE | PHRASE | VERBAL | OMITTED_TIME_PREP )

// pos tagger ZONE roles for a comma zone
#define ZONE_SUBJECT			0x000001	// noun before any verb
#define ZONE_VERB				0x000002
#define ZONE_OBJECT				0x000004	// noun AFTER a verb
#define ZONE_CONJUNCT			0x000008	// coord or subord conjunction
#define ZONE_FULLVERB			0x000010	// has normal verb tense or has aux
#define ZONE_AUX				0x000020	// there is aux in the zone
#define ZONE_PCV				0x000040	// zone is entirely phrases, clauses, and verbals
#define ZONE_ADDRESS			0x000080	// zone is an addressing name start. "Bob, you are amazing."
#define ZONE_ABSOLUTE			0x000100	// absolute zone has subject and partial participle verb, used to describe noun in another zone
#define ZONE_AMBIGUOUS			0x000200	// type of zone is not yet known

// control over tokenization (tokenControl set from user $token variable)
// these values MIRRORED as resulting used values in %tokenflags (tokenFlags)
#define DO_ESSENTIALS			0x00000001 
#define DO_SUBSTITUTES			0x00000002
#define DO_CONTRACTIONS			0x00000004
#define DO_INTERJECTIONS		0x00000008
#define DO_BRITISH				0x00000010 
#define DO_SPELLING				0x00000020 
#define DO_TEXTING				0x00000040 
#define DO_SUBSTITUTE_SYSTEM	0x000000ff	// 1 OTHER RESERVED FOR FUTURE FILES TIL BELOW

#define DO_NUMBER_MERGE			0x00000100
#define DO_PROPERNAME_MERGE		0x00000200
#define DO_SPELLCHECK			0x00000400
#define DO_INTERJECTION_SPLITTING 0x00000800

// this do not echo into tokenFlags
#define DO_POSTAG				0x00001000
#define DO_PARSE				0x00002000
#define NO_IMPERATIVE			0x00004000
#define NO_VERB					0x00008000
#define NO_WITHIN				0x00010000
#define NO_SENTENCE_END			0x00020000
#define DO_CONDITIONAL_POSTAG	0x00040000
#define NO_INFER_QUESTION		0x00080000

// tenses do not echo into tokenControl
#define PRESENT					0x00001000    // basic tense
#define PAST					0x00002000	  // basic tense- both present perfect and past perfect map to it
#define FUTURE					0x00004000    // basic tense
#define PRESENT_PERFECT			0x00008000    // distinguish PAST PERFECT from PAST PRESENT_PERFECT
#define CONTINUOUS				0x00010000 
#define PERFECT					0x00020000    
#define PASSIVE					0x00040000    

// tokencontrol parallel values  echoed to tokenflags
#define NO_HYPHEN_END			0x00100000 // dont end sentences using hypens  - shares PRESENT BIT
#define NO_COLON_END			0x00200000 // dont end sentences using colons  - shares PAST BIT
#define NO_SEMICOLON_END		0x00400000 // dont end sentences using semicolons  - shares FUTURE BIT
#define STRICT_CASING			0x00800000 // trust that user means casing (on non-start words)
#define ONLY_LOWERCASE			0x01000000 // never recognize uppercase words, treat them all as lowercase

//   values of tokenFlags (seen processing input) 
#define QUESTIONMARK			0x10000000    
#define EXCLAMATIONMARK			0x20000000   
#define PERIODMARK				0x40000000   
#define USERINPUT				0x80000000    

#define FOREIGN_TOKENS			0x0000000100000000ULL
#define FAULTY_PARSE			0x0000000200000000ULL   

// flags to control output processing
#define    OUTPUT_ONCE 1 
#define    OUTPUT_KEEPSET  2			// don't expand class or set
#define    OUTPUT_KEEPVAR  4			// don't expand a var past its first level
#define    OUTPUT_KEEPQUERYSET 8		// don't expand a fact var like @1object
#define    OUTPUT_SILENT 16				// don't show stuff if trace is on
#define    OUTPUT_NOCOMMANUMBER 32		// don't add to numbers
#define    OUTPUT_NOTREALBUFFER  64		// don't back up past start at all
#define	   OUTPUT_ISOLATED_PERIOD 128	// don't join periods onto numbers
#define    OUTPUT_NOQUOTES  256			// strip quotes off strings
#define	   OUTPUT_LOOP 512				// coming from a loop, fail does not cancel output
#define	   OUTPUT_UNTOUCHEDSTRING 1024	// leave string alone
#define	   OUTPUT_FACTREAD 2048			// reading in fact field
#define    OUTPUT_EVALCODE 4096			
#define	   OUTPUT_DQUOTE_FLIP 8192
#define	   OUTPUT_ECHO 16384

struct WORDENTRY;
typedef WORDENTRY* WORDP;

typedef void (*DICTIONARY_FUNCTION)(WORDP D, uint64 data);

struct FACT;
typedef unsigned int MEANING; //   a flagged indexed dict ptr

typedef unsigned int FACTOID; //   a fact index

typedef struct WORDENTRY //   a dictionary entry  - starred items are written to the dictionary
{
	uint64  properties;				//   main language description of this node 
	uint64	hash;					//   we presume 2 hashs never collide, so we dont check the string for matching
	uint64  systemFlags;			//   additional dictionary and non-dictionary properties

	//   if you edit this, you may need to change ReadBinaryEntry and WriteBinaryEntry
	char*     word;					//   entry name
  	MEANING*  meanings;				//   list of meanings (synsets) of this word - Will be wordnet synset id OR self ptr -- 1-based since 0th meaning means all meanings
	MEANING*  glosses;				//   list of glosses for synset head meanings - is offset to allocstring and id index of meaning involved.

	//   related members of this word (circular lists) 
	WORDP			conjugation;	//   conjugation list for irregular verbs 
	WORDP			plurality;		//   plurality list for irregular nouns	
	WORDP			comparison;		//   comparison list for irregular adjective/adverb

	union {
		char* botNames;				//   for topic name (start with $) - bot topic applies to  - only used by script compiler
	    char* fndefinition;			//   for macro name (start with ^) - if systemflag is on, is user script
	    char* userValue;			//   if a $uservar (start with $) OR if a search label uservar 
		WORDP substitutes;			//   words (with systemflag SUBSTITUTE) that should be adjusted to during tokenization
	}w;
		
	FACTOID subjectHead;		//  start threads for facts run thru here 
	FACTOID verbHead;			//  start threads for facts run thru here 
	FACTOID objectHead;			//  start threads for facts run thru here 
	MEANING	factBack;			//	backtrace thread on search

    union {
          unsigned short topicIndex;	//   for a ~topic or %systemVariable, this is its id
		  unsigned short codeIndex;		//   for a function, its the table index for it
		  unsigned short macroFlags;	//	 for a table, these bits signify special handling of its arguments (1 bit per argument)
    }x;
	
	union {
          unsigned short length;		//  length of the word
    }y;

  	unsigned int nextNode;		// bucket-link for dictionary hash + top bye GETMULTIWORDHEADER // can this word lead a phrase to be joined - can vary based on :build state -- really only needs 4 bits
    unsigned int spellNode;		// next word of same length as this
	unsigned int inferMark;		// been here marker during marking facts, inferencing (for propogation) and during scriptcompile 
	unsigned int internalBits;
    //   these two fields used in pattern matching set up once
    union {
        unsigned int  patternStamp;  //   is this found in sentence (current timestamp, in which case whereInSentence is locations)
        unsigned int  argumentCount; //   for function defintions - if systemflag is on
    }v;


} WORDENTRY;



#include "dictionaryMore.h"

#endif
