#include "common.h"
#ifdef INFORMATION

Function calls all run through DoCommand().
	
A function call can either be to a system routine or a user routine. 
	
User routines are like C macros, executed in the context of the caller, so the argument 
are never evaluated prior to the call. If you evaluated an argument during the mustering,
you could get bad answers. Consider:
	One has a function: ^foo(^arg1 ^arg2)  ^arg2 ^arg1
	And one has a call ^foo(($val = 1 ) $val )
This SHOULD look like inline code:  $val  $val = 1 
But evaluation at argument time would alter the value of $val and pass THAT as ^arg2. Wrong.

The calling Arguments to a user function are in an array, whose base starts at callArgumentBase and runs
up to (non-inclusive) callArgumentIndex.

System routines are proper functions, whose callArgumentList may or may not be evaluated. 
The callArgumentList are in an array, whose base starts at index CallingArgumentBase and runs
up to (non-inclusive) CallingArgumentIndex. The description of a system routine tells
how many callArgumentList it expects and in what way. Routines that set variables always pass
that designator as the first (unevaluated) argument and all the rest are evaluated callArgumentList.

The following argument passing is supported:
	1. Evaluated - each argument is evaluated and stored (except for a storage argument). 
		If the routine takes optional callArgumentList these are already also evaluated and stored, 
		and the argument after the last actual argument is a null string.
	2. STREAM_ARG - the entire argument stream is passed unevaled as a single argument,
		allowing the routine to handle processing them itself.

All calls have a context of "executingBase" which is the start of the rule causing this 
evaluation. All calls are passed a "buffer" which is spot in the currentOutputBase it
should write any answers.

Anytime a single argument is expected, one can pass a whole slew of them by making
them into a stream, encasing them with ().  The parens will be stripped and the
entire mess passed unevaluated. This makes it analogous to STREAM_ARG, but the latter
requires no excess parens to delimit it.

In general, the system does not test result codes on argument evaluations. So
issuing a FAILRULE or such has no effect there.

#endif

#define MAX_TOPIC_KEYS 5000

// potential statistical use
CharIntMap mapint0;
CharFloatMap mapfloat0;	
CharFloatMap mapfloat1;

int globalDepth = 0;

//   spot callArgumentList are stored for  function calls
#define MAX_ARG_BYTES MAX_WORD_SIZE * 2
char callArgumentList[MAX_ARGUMENT_COUNT+1][MAX_ARG_BYTES];    // arguments to functions
unsigned int callArgumentIndex;
unsigned int callArgumentBase;
unsigned int fnVarBase;

char lastInputSubstitution[INPUT_BUFFER_SIZE];
int wasCommand; // special result passed back from some commands to control chatscript

static char oldunmarked[MAX_SENTENCE_LENGTH];
static unsigned int spellSet;			// place to store word-facts on words spelled per a pattern

//////////////////////////////////////////////////////////
/// BASIC FUNCTION CODE
//////////////////////////////////////////////////////////

void InitFunctionSystem() // register all functions
{
	unsigned int k = 0;
	SystemFunctionInfo *fn;
	while ((fn = &systemFunctionSet[++k]) && fn->word)
	{
		if (*fn->word == '^' ) // not a header
		{
			WORDP D = StoreWord((char*) fn->word,0,FUNCTION_NAME);
			D->v.argumentCount = fn->argumentCount;
			D->x.codeIndex = (unsigned short)k;
		}
	}

	oldunmarked[0] = 0;	// global unmarking has nothing
}

void ResetFunctionSystem()
{
	//   reset function call data
	fnVarBase = callArgumentBase = callArgumentIndex = 0;
}

char* DoFunction(char* name,char* ptr,char* buffer,unsigned int &result)
{
	WORDP D = FindWord(name,0,LOWERCASE_LOOKUP);
	if (!D || !(D->systemFlags & FUNCTION_NAME))
    {
		result = UNDEFINED_FUNCTION;
		return ptr; 
	}
	result = 0;
	char word[MAX_WORD_SIZE];
    ptr = ReadCompiledWord(ptr,word); // swallow ( of argument list
	bool oldecho = echo;
	if (D->systemFlags & TRACE_MACRO) echo = true;
	SystemFunctionInfo* info = NULL;
	unsigned int id;
	unsigned int oldArgumentBase = callArgumentBase;
	unsigned int oldArgumentIndex = callArgumentIndex;
	if (D->x.codeIndex && !(D->systemFlags & IS_TABLE_MACRO)) // system function --  macroFlags are also on codeindex, but IS_TABLE_MACRO distinguishes
	{
		callArgumentBase = callArgumentIndex - 1;
		if (trace & OUTPUT_TRACE) Log(STDUSERTABLOG, "System Call %s(",name);
		info = &systemFunctionSet[D->x.codeIndex];
		char* start = ptr;
		while (ptr && *ptr != ')' && *ptr != ENDUNIT) // read arguments
		{
			if (info->argumentCount != STREAM_ARG) ptr = ReadShortCommandArg(ptr,callArgumentList[callArgumentIndex],result,OUTPUT_NOTREALBUFFER|OUTPUT_EVALCODE|OUTPUT_UNTOUCHEDSTRING);
			else // swallow unevaled arg stream
			{
				ptr = BalanceParen(start - 2,false);  // start at (, point after closing ) if one can, to next token
				size_t len = ptr - start; // length of argument bytes
				if (*ptr && *ptr != ENDUNIT && *ptr != ' ') // is a token pointer
				{
					ptr -= 2;  // ptr to )
					len -= 2;
				}
				else if (*--ptr != ')') --ptr; // aim back at ) // len may include a blank or not

				strncpy(callArgumentList[callArgumentIndex],start,len);
				char* tail = &callArgumentList[callArgumentIndex][len];
				*tail = 0;
				if (len && *(tail-1) == ' ')  *(tail-1) = 0; // remove any trailing blank
			}
			if (trace & OUTPUT_TRACE || D->systemFlags & TRACE_MACRO) Log(STDUSERLOG,"%s, ",callArgumentList[callArgumentIndex]);
			if (++callArgumentIndex >= MAX_ARG_LIST) // too many arguments
			{
				result = FAILRULE_BIT;
				echo = oldecho;
				return ptr; 
			}
			if (info->argumentCount == STREAM_ARG) break; // end of arguments
		}
		*callArgumentList[callArgumentIndex] = 0; //  mark end of arg list
		if (trace & OUTPUT_TRACE  || D->systemFlags & TRACE_MACRO) id = Log(STDUSERLOG,") = ");
		if (result & ENDCODES); // failed during argument processing
		else result = (*info->fn)(buffer);
	} 
	else //   user function, eg  ^call (_10 ^2 it ^call2 (3 ) )  spot each token has 1 space separator
	{
		unsigned int oldFnVarBase = fnVarBase;
		if (trace & OUTPUT_TRACE || D->systemFlags & TRACE_MACRO) Log(STDUSERTABLOG, "Execute %s(",name);
		char* definition = D->w.fndefinition;
        while (*ptr && *ptr != ')') //   ptr is after opening (and before an arg but may have white space
        {
			char* arg = callArgumentList[callArgumentIndex++];
			if (currentRule == NULL) //   this is a table function- DONT EVAL ITS ARGUMENTS AND... keep quoted item intact
			{
				ptr = ReadCompiledWord(ptr,arg); // return dq args as is
#ifndef DISCARDSCRIPTCOMPILER
				if (compiling && ptr == NULL) BADSCRIPT("TABLE-11 Arguments to %s ran out",name)
#endif
			}
			else ptr = ReadArgument(ptr,arg); //   ptr returns on next significant char
			
			//   within a function, seeing function argument as an argument (limit 9 calling Arguments)
			//   switch to incoming arg now, later callArgumentBase will be wrong
			if (*arg == '^' && IsDigit(arg[1]) ) strcpy(arg,callArgumentList[atoi(arg+1) + fnVarBase]); 
			if (trace & OUTPUT_TRACE || D->systemFlags & TRACE_MACRO) 
			{
				Log(STDUSERLOG, "%s",arg);
				if (*arg == '$') Log(STDUSERLOG,"(%s)",GetUserVariable(arg));
				Log(STDUSERLOG, ", ");
			}
		}
		if (trace & OUTPUT_TRACE || D->systemFlags & TRACE_MACRO) id = Log(STDUSERLOG, ")\n");
		fnVarBase = callArgumentBase = oldArgumentIndex; 
	
		//   run the definition
		if (definition)
		{
			unsigned int oldtrace = trace;
			if (D->systemFlags & TRACE_MACRO) trace = (unsigned int) -1;
			ChangeDepth(1,"HandleCall");
			if (debugger) Debugger(ENTER_CALL,0,definition);
			ChangeDepth(1,"DoFunction");
			Output(definition,buffer,result,OUTPUT_NOTREALBUFFER);
			ChangeDepth(-1,"DoFunction");
			if (debugger) Debugger(EXIT_CALL,0,0);
			ChangeDepth(-1,"HandleCall");
			trace = oldtrace;
		}
		fnVarBase = oldFnVarBase;
	}

	if (callArgumentIndex > (MAX_ARGUMENT_COUNT - 20))
	{
		ReportBug("Function nesting too deep");
		result = FAILRULE_BIT;
	}
	//   pop argument list
	callArgumentIndex = oldArgumentIndex;	 
	callArgumentBase = oldArgumentBase;

	if (trace & OUTPUT_TRACE || D->systemFlags & TRACE_MACRO) 
	{
		if (info && info->properties & SAMELINE) Log(STDUSERLOG,"%s (%s)\r\n",ResultCode(result),name);	// stay on same line to save visual space in log
		else Log(STDUSERTABLOG,"%s (%s)\r\n",ResultCode(result),name);
	}
	echo = oldecho;
	if (*ptr == ')') // skip ) and space
	{
		if (ptr[1] != ENDUNIT && ptr[1]) return ptr+2; // if this is a pattern comparison, this will NOT be a space, but will be a comparison op instead missing it
		return ptr+1;	// ptr to the end unit
	}
	else return ptr;
}

void DumpFunctions()
{
	unsigned int k = 0;
	SystemFunctionInfo *fn;
	while ( (fn = &systemFunctionSet[++k])  && fn->word )
	{
		if (*fn->word != '^') Log(STDUSERLOG,"%s\r\n",fn->word);
		else Log(STDUSERLOG,"%s - %s\r\n",fn->word,fn->comment);
	}
}

//////////////////////////////////////////////////////////
/// FUNCTION UTILITIES
//////////////////////////////////////////////////////////

char* ResultCode(unsigned int result)
{
	char* ans = "OK";
	if (result & ENDRULE_BIT) ans = "ENDRULE";
	else if (result & FAILRULE_BIT) ans = "FAILRULE";
	else if (result & RETRYRULE_BIT) ans = "RETRYRULE";

	else if (result & ENDTOPIC_BIT) ans = "ENDTOPIC";
	else if (result & FAILTOPIC_BIT) ans = "FAILTOPIC";
	else if (result & RETRYTOPIC_BIT) ans = "RETRYTOPIC";

	else if (result & ENDSENTENCE_BIT) ans = "ENDSENTENCE";
	else if (result & FAILSENTENCE_BIT) ans = "FAILSENTENCE";
	else if (result & RETRYSENTENCE_BIT) ans = "RETRYSENTENCE";

	else if (result & ENDINPUT_BIT) ans = "ENDINPUT";
	else if (result & FAILINPUT_BIT) ans = "FAILINPUT";
	return ans;
}

 static void AddInput(char* buffer)
{
	char* copy = AllocateBuffer();
	strcpy(copy,nextInput);
	strcpy(nextInput,"... "); // system separator
	char* ptr = nextInput + 4;
	unsigned int n = BurstWord(buffer);
	for (unsigned int i = 0; i < n; ++i)
	{
        strcpy(ptr,GetBurstWord(i));
		ptr += strlen(ptr);
		strcpy(ptr," ");
		++ptr;
	}
	strcpy(ptr,copy);
	FreeBuffer();
	if (strlen(nextInput) > 1000) nextInput[1000] = 0;	// overflow
}

static unsigned int ComputeSyllables(char* word)
{
	char copy[MAX_WORD_SIZE];
	MakeLowerCopy(copy,word);
	size_t len = strlen(copy);
	if (len <= 3) return 1;

	char* ptr = copy-1;
	unsigned int vowels = 0;
	int series = 0;
	while (*++ptr)
	{
		if (!IsVowel(*ptr)) 
		{
			if (series >= 4) --vowels; 
			series = 0;
		}
		else 
		{
			++vowels;
			++series;
		}
	}
	// silent e
	if (copy[len-1] == 'e' && !IsVowel(copy[len-2]) && IsVowel(copy[len-3])) --vowels;	// silent e
	
	// silent es or ed
	if ((copy[len-1] == 'd' || copy[len-1] == 's') && copy[len-2] == 'e' && !IsVowel(copy[len-3]) && IsVowel(copy[len-4])) --vowels;	// silent e

	return vowels;
}

static unsigned int RandomMember(char* buffer,char* answer) 
{
#ifdef INFORMATION
returns a random member of a set or class

returns FAILRULE if a bad set is given.

The value is recursive. If the random member chosen is a set or class, the
link is followed and a random member from the next level is chosen, and so on. 
If the value is a wordnet reference you get the synset node-- fix this sometime.

#endif
	WORDP members[3000];
loop:
	WORDP D = FindWord(answer);
	if (!D ) return FAILRULE_BIT;

    unsigned int index = 0;
    FACT* F = GetObjectHead(D);
    while (F && index < 2999)
    {
        if (F->verb == Mmember) members[index++] = Meaning2Word(F->subject);
        F = GetObjectNext(F);
    }
    if (!index) return FAILRULE_BIT; //   none found

	//   pick one at random
    D = members[random(index)];
    answer = D->word;
	static char adjust[MAX_WORD_SIZE];
	if (D->properties & SUBSTITUTE) //   they may have < or > markers -- is this even possible BUG
	{
		if (*D->word == '<') answer += 1;
		size_t len = strlen(answer);
		if (answer[len-1] == '>')
		{
			strcpy(adjust,answer);
			adjust[len-1] = 0;
			strcpy(buffer,answer);
			return 0;
		}
	}
    if (*answer == '~') goto loop; //   member is a subset or class, get from it
    if (*answer == '<') ++answer; //   interjections have < in front
	strcpy(buffer,answer);
    return 0;
}

unsigned int FLR(char* buffer,char which)
{  
	unsigned int store;
	*buffer = 0;
	char* word = ARGUMENT(1);
	if (*word != '@') 
	{
		ReportBug("bad store id")
		return FAILRULE_BIT;
	}
	store = GetSetID(word);

	unsigned int count = FACTSET_COUNT(store);
	if (!count) 
	{
		if (impliedWild != ALREADY_HANDLED)
		{
			SetWildCardIndexStart(impliedWild);
			SetWildCard("","",0,0); // subject
			SetWildCard("","",0,0);	// verb
			SetWildCard("","",0,0);	// object
			SetWildCard("","",0,0);	// flags
		}
		impliedWild = ALREADY_HANDLED;
		return ENDRULE_BIT; //   terminates but does not cancel output
	}
	
	if (trace & OUTPUT_TRACE) Log(STDUSERLOG,"[%d] => ",count);
	
	// pick fact
	unsigned int item;
	if (which == 'l') item = count; //   last
	else if (which == 'f') item = 1; //   first
	else if (which == 'n') // next
	{
		item = ++factSetNext[store];
		if (count < item) return FAILRULE_BIT; 
	}
	else item = random(count) + 1;    // random
	FACT* F = factSet[store][item];

	// remove fact from set, but next does not alter set
	if (which != 'n')
	{
		SET_FACTSET_COUNT(store,(count-1));
		memmove(&factSet[store][item],&factSet[store][item+1],sizeof(FACT*) * (count - item)); 
	}		

	char type = *GetSetType(word);

	// transfer fact pieces appropriately
	MEANING Mfirst = 0;
	MEANING Mlast = 0;
	uint64 factSubject = 0;
	uint64 factObject = 0;
	if (!type || type == 'f') // want entire fact as index
	{
		if (impliedSet == ALREADY_HANDLED) sprintf(buffer,"%d",Fact2Index(F)); 
		else AddFact(impliedSet,F);
	}
	else if (type == 's') // want subject
	{
		MEANING M = F->subject;
		if (F->flags & FACTSUBJECT) sprintf(buffer,"%d",M);
		else sprintf(buffer,"%s",Meaning2Word(M)->word);
	}
	else if (type == 'v') // want verb
	{
		MEANING M = F->verb;
		if (F->flags & FACTVERB) sprintf(buffer,"%d",M);
		else sprintf(buffer,"%s",Meaning2Word(M)->word);
	}
	else if (type == 'o') // want obect
	{
		MEANING M = F->object;
		if (F->flags & FACTOBJECT) sprintf(buffer,"%d",M);
		else sprintf(buffer,"%s",Meaning2Word(M)->word);
	}
	else if ( type == 'a' || type == '+'  || type == ' ' || !type) // want all, subject first
	{
		Mfirst = F->subject;
		factSubject = F->flags & FACTSUBJECT;
		Mlast = F->object;
		factObject = F->flags & FACTOBJECT;
	}
	else // want all, object first
	{
		Mlast = F->subject;
		factObject = F->flags & FACTSUBJECT;
		Mfirst = F->object;
		factSubject= F->flags & FACTOBJECT;
	}
	if (Mfirst) // spread
	{
		char factID[100];
		char* piece;
		if (factSubject) 
		{
			sprintf(factID,"%d",Mfirst);
			piece = factID;
		}
		else piece = Meaning2Word(Mfirst)->word;

		// _wildcard can take all, otherwise you get just a field
		// for variables. not legal for sets

		if (impliedWild == ALREADY_HANDLED) strcpy(buffer,piece);
		else 
		{
			SetWildCardIndexStart(impliedWild);
			SetWildCard(piece,piece,0,0); 

			 //   verb
			MEANING M = F->verb;
			if (F->flags & FACTVERB) 
			{
				sprintf(factID,"%d",M);
				piece = factID;
			}
			else piece = Meaning2Word(M)->word;
			SetWildCard(piece,piece,0,0);

			//   object
			if (factObject) 
			{
				sprintf(factID,"%d",Mlast);
				piece = factID;
			}
			else piece = Meaning2Word(Mlast)->word;
			SetWildCard(piece,piece,0,0); 

			if ( type == 'a') // all include flags on fact
			{
				sprintf(tmpWord,"0x%08x",F->flags);
				SetWildCard(tmpWord,tmpWord,0,0);
			}
		}
		impliedSet = impliedWild = ALREADY_HANDLED; // we spread the values out
	}
	if (trace & OUTPUT_TRACE && *buffer) Log(STDUSERLOG," %s  ",buffer);
	return 0;
}

bool RuleTest(char* data) // see if pattern matches
{
	char pattern[MAX_WORD_SIZE];
	GetPattern(data,NULL,pattern);
	unsigned int gap = 0;
	unsigned int wildcardSelector = 0;
	wildcardIndex = 0;
	unsigned int junk;
	blocked = true;
	bool answer =  Match(pattern+2,0,0,'(',true,gap,wildcardSelector,junk,junk); // start past the opening paren
	blocked = false;
	return answer;
}

unsigned int Callback(WORDP D,char* arguments) 
{
	if (! D || !(D->systemFlags & FUNCTION_NAME)) return FAILRULE_BIT;
	PushOutputBuffers();
	unsigned int oldtrace = trace;
	trace = 0;
	char args[MAX_WORD_SIZE];
	strcpy(args,arguments);
	unsigned int result;
	AllocateOutputBuffer();
	DoFunction(D->word,args,currentOutputBase,result);
	FreeOutputBuffer();
	PopOutputBuffers();
	trace = oldtrace;
	return result;
}

void ResetUser(char* input)
{
	unsigned int oldtopicid = currentTopicID;
	char* oldrule = currentRule;
	int oldruleid = currentRuleID;
	int oldruletopic = currentRuleTopic;
	
	ReadNewUser(); 
	userFirstLine = 1;
	responseIndex = 0;
	wasCommand = BEGINANEW;

	currentTopicID = oldtopicid;
	currentRule = oldrule;
	currentRuleID = oldruleid;
	currentRuleTopic = oldruletopic;
}

//////////////////////////////////////////////////////////
/// TOPIC FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int AddTopicCode(char* buffer) 
{     
	AddPendingTopic(FindTopicIDByName(ARGUMENT(1))); // does not fail, just may not become pending
	return 0;
}

static unsigned int ClearTopicsCode(char* buffer)
{
	ClearPendingTopics();
	return 0;
}

static unsigned int CountTopicCode(char* buffer) 
{     
	int topic = FindTopicIDByName(ARGUMENT(1));
	if (BlockedBotAccess(topic)) return FAILRULE_BIT;

	char* name = ARGUMENT(2);
	if (!strnicmp(name,"gambit",6)) sprintf(buffer,"%d", GAMBIT_MAX(topicMaxRule[topic])); 
	else if (!strnicmp(name,"rule",4)) sprintf(buffer,"%d", RULE_MAX(topicMaxRule[topic])); 
	else if (!stricmp(name,"used")) sprintf(buffer,"%d",TopicUsedCount(topic));
	else if (!stricmp(name,"available"))
	{
		unsigned int count = 0;
		unsigned int* map = gambitTagMap[topic];	
		unsigned int gambitID = *map;
		while (gambitID != NOMORERULES)
		{
			if (UsableRule(topic,gambitID)) ++count;
			gambitID = *++map;
		}
		sprintf(buffer,"%d",count); 
	}
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int GambitCode(char* buffer) 
{ 
	// gambit(PENDING) means from interesting stack  
	// gambit(~name) means use named topic 
	// gambit(~) means current topic we are within now
	// gambit (word) means topic with that keyword
	// if a second argument exists (FAIL) then return failure code if doesnt generate
	unsigned int oldIndex = responseIndex;
	if (all) return FAILRULE_BIT; // dont generate gambits when doing all

	//  if "~", get current topic name to use for gambits
	char* word = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	unsigned int topic;
	unsigned int result = FAILRULE_BIT;

   	if (!stricmp(word,"pending")) // pick topic from pending stack
	{
		unsigned int stack[MAX_TOPIC_STACK+1];
		memcpy(stack,pendingTopicList,pendingTopicIndex * sizeof(unsigned int)); // copy stack
		int index = pendingTopicIndex;
        while (index--) // walk topics, most recent first
		{
			topic = stack[index];
			int pushed =  PushTopic(topic);
			if (pushed < 0) return FAILRULE_BIT;
			ChangeDepth(1,"GambitCode");
			result = PerformTopic(GAMBIT,buffer);
			ChangeDepth(-1,"GambitCode");
			if (pushed) PopTopic();
			if (result & RESULTBEYONDTOPIC) return result;
			if (responseIndex > oldIndex)  return 0;
		}
		return 0;
	}

	 // do topic by name
	else if (*word == '~')
	{
		topic = FindTopicIDByName(word);
		if (topic && !(GetTopicFlags(topic) & TOPIC_BLOCKED))
		{
 			int pushed = PushTopic(topic);
			if (pushed < 0) return FAILRULE_BIT;
			ChangeDepth(1,"GambitCode1");
			result = PerformTopic(GAMBIT,buffer);
			ChangeDepth(-1,"GambitCode1");

			if (pushed) PopTopic();
		}
		if (!result && !stricmp(arg2,"FAIL") && responseIndex <= oldIndex)  return FAILRULE_BIT; // report failure
		return result;
	}
	
	// do topic by keyword
	else
	{
		WORDP D = FindWord(word);
		if (!D) return 0;
		FACT* F = GetSubjectHead(D);
		while (F) // find topics word is a direct member of
		{
			if (F->verb == Mmember)
			{
				WORDP E = Meaning2Word(F->object);
				if (E->systemFlags & TOPIC_NAME)
				{
					unsigned int topic = FindTopicIDByName(E->word);
					if (topic && !(GetTopicFlags(topic) & (TOPIC_BLOCKED|TOPIC_SYSTEM)))
					{
 						int pushed = PushTopic(topic);
						if (pushed < 0) return FAILRULE_BIT;
						ChangeDepth(1,"GambitCode2");
						result = PerformTopic(GAMBIT,buffer);
						ChangeDepth(-1,"GambitCode2");
						if (pushed) PopTopic();
						if (result & RESULTBEYONDTOPIC) return result;
						if (responseIndex > oldIndex)  return 0;
					}
				}
			}
			F = GetSubjectNext(F);
		}
		if (!stricmp(arg2,"FAIL"))  return FAILRULE_BIT; // report failure
	}
	return 0;
}

static unsigned int GetRuleCode(char* buffer) 
{     
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	char* arg3 = ARGUMENT(3);
	unsigned int topic = currentTopicID;
	int id;
	char* rule;
	bool fulllabel = false;
	bool crosstopic = false;
	char* dot = strchr(arg2,'.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg2);
	else rule = GetLabelledRule(topic,arg2,arg3,fulllabel,crosstopic,id);
	if (!rule) return FAILRULE_BIT;
	if (!stricmp(arg1,"tag")) sprintf(buffer,"%s.%d.%d",GetTopicName(topic),TOPLEVELID(id),REJOINDERID(id));
	else if (!stricmp(arg1,"topic")) strcpy(buffer,GetTopicName(topic));
	else if (!stricmp(arg1,"label")) GetLabel(rule,buffer);
	else if (!stricmp(arg1,"type")) sprintf(buffer,"%c",*rule);
	else if (!stricmp(arg1,"pattern")) // use pattern notation so it can work with ^match and will not be harmed stored as a variable
	{
		*buffer = '"';
		buffer[1] = 0;
		GetPattern(rule,NULL,buffer+1);
		if (!buffer[1]) *buffer = 0;
		else strcat(buffer,"\"");
	}
	else if (!stricmp(arg1,"usable")) strcpy(buffer,(UsableRule(topic,id)) ? (char*) "1" : (char*) "");
	else // output
	{
		 rule = GetPattern(rule,NULL,NULL);
		 char* end = strchr(rule,'`');  // will not be a useful output as blanks will become underscores, but can do ^reuse() to execute it
		 *end = 0;
		 strcpy(buffer,rule);
		 *end = '`';
	}
	if (trace & OUTPUT_TRACE)
	{
		char word[MAX_WORD_SIZE];
		strncpy(word,buffer,50);
		word[50] = 0;
		Log(STDUSERLOG," %s ",word);
	}
	return 0;
}
	
static unsigned int HasGambitCode(char* buffer)
{
	// hasgambit(~topic) means does it have any unused gambits
	// hasgambit(~topic last) means is last gambit unused
	// hasgambit(~topic any) means does it have gambits used or unused
	char* name = ARGUMENT(1);
	unsigned int topic = FindTopicIDByName(name);
	if (!topic) return FAILRULE_BIT;
	unsigned int gambits = GAMBIT_MAX(topicMaxRule[topic]); // total gambits of topic
	if (!gambits) return FAILRULE_BIT;	

	char* arg = ARGUMENT(2);
	if (!stricmp(arg,"last")) return UsableRule(topic,gambitTagMap[topic][gambits-1]) ? 0 : FAILRULE_BIT; // is last gambit unused
	else if (!stricmp(arg,"any")) return 0;
	else return (HasGambits(topic) < 1) ? FAILRULE_BIT : 0;
}

static unsigned int KeepCode(char* buffer)
{
	AddKeep(currentRule);
	return 0;
}

static unsigned int LastUsedCode(char* buffer)
{
	char* name = ARGUMENT(1);
	char* what = ARGUMENT(2);
	unsigned int topic = FindTopicIDByName(name);
	if (!topic)  return FAILRULE_BIT;  

	if (!strnicmp(what,"gambit",6)) sprintf(buffer,"%d",topicLastGambittedMap[topic]);
	else if (!strnicmp(what,"responder",8)) sprintf(buffer,"%d",topicLastResponderedMap[topic]);
	else if (!strnicmp(what,"rejoinder",6)) sprintf(buffer,"%d",topicLastRejoinderedMap[topic]);
	else // any 
	{
		unsigned int last = topicLastRejoinderedMap[topic];
		if (topicLastResponderedMap[topic] > last) last = topicLastResponderedMap[topic];
		if (topicLastGambittedMap[topic] > last) last = topicLastGambittedMap[topic];
		sprintf(buffer,"%d",last);
	}
	return 0;
}

static unsigned int PopTopicCode(char* buffer) // reconsider BUG
{     
	char* arg1 = ARGUMENT(1);
	if (*arg1 == '~') RemovePendingTopic(FindTopicIDByName(arg1)); // current topic may continue executing
	else if (!*arg1) // make current topic not interesting AND quit it
	{
		RemovePendingTopic(currentTopicID);
		return ENDTOPIC_BIT; 
	}
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int RefineCode(char* buffer) 
{
	unsigned int result = 0;
 
	char* arg1 = ARGUMENT(1); // nothing or FAIL or label of rule or topic.label
	char* arg2 = ARGUMENT(2); 
	bool fail = false;
	if (!stricmp(arg2,"FAIL")) fail = true;
	if (!stricmp(arg1,"FAIL")) 
	{
		fail = true; 
		strcpy(arg1,arg2); // promote any 2nd argument
	}
	
	char* rule;
    int id = currentRuleID;
	unsigned int topic = currentTopicID;
	if (!*ARGUMENT(1)) rule = FindNextRule(NEXTRULE,currentRule,id); // default continue AFTER the current rule
	else // designated
	{
		bool fulllabel = false;
		bool crosstopic = false;
		char* dot = strchr(arg1,'.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg1);
		else rule = GetLabelledRule(topic,arg1,"",fulllabel,crosstopic,id);
	}

	if (!rule) return FAILRULE_BIT;

	// change context now
	SAVEOLDCONTEXT()
	currentRule = rule;
	currentRuleTopic = currentTopicID = topic;
	currentRuleID = id;
	char level = *currentRule;
	if (debugger) Debugger(ENTER_REFINE);
	while (currentRule && level == *currentRule) // try all choices
    {
		if (trace & PATTERN_TRACE)
		{
			char label[MAX_WORD_SIZE];
			GetLabel(currentRule,label);
			if (*label) id = Log(STDUSERTABLOG, "try %s: \\",label); // the \\ will block linefeed on next Log call
			else id = Log(STDUSERTABLOG, "try  \\");
		}
		ChangeDepth(1,"RefineCode");
 		result = TestRule(id,currentRule,buffer);
		ChangeDepth(-1,"RefineCode");
	    if (result != FAIL_MATCH) break;
		else result = 0;

		while (currentRule && *currentRule)
		{
			currentRule = FindNextRule(NEXTRULE,currentRule,id); 
			if (currentRule && (*currentRule <= level  || !Rejoinder(currentRule))) break;	// matches our level OR is earlier than it (end of a zone like refine of a: into b: zone)
		}
    }
	if (outputRejoinderRuleID == NO_REJOINDER) outputRejoinderRuleID = BLOCKED_REJOINDER; // refine values exist instead of real rejoinders, dont let calling rule do set rejoinder
	if (debugger) Debugger(EXIT_REFINE);
	RESTOREOLDCONTEXT()
	return result; // finding none does not fail unless told to fail
}

static unsigned int RejoinderCode(char* buffer)
{ 
	if (postProcessing)
	{
		ReportBug("Not legal to use ^rejoinder in postprocessing");
		return FAILRULE_BIT;
	}
    if (!unusedRejoinder) 
	{
		if (trace & BASIC_TRACE) Log(STDUSERLOG," disabled rejoinder\r\n\r\n");
		return 0; //   an earlier response handled this
	}

	if (inputRejoinderRuleID == NO_REJOINDER) 
	{
		if (trace & BASIC_TRACE) Log(STDUSERLOG,"  rejoinder not set\r\n");
		return 0; // not a failure, just nothing done
	}

    //   we last made a QUESTIONWORD or statement, can his reply be expected for that? 
	unsigned int result = 0;
	int pushed = PushTopic(inputRejoinderTopic);
	if (pushed < 0) return FAILRULE_BIT;
	
    char* ptr = GetRule(inputRejoinderTopic,inputRejoinderRuleID);
    if (!ptr)  
	{
		if (trace & BASIC_TRACE) Log(STDUSERLOG," no rejoinder data for topic %s\r\n\r\n",GetTopicName(currentTopicID));
		if (pushed) PopTopic();
		return result;
	}

	unsigned int oldtrace = trace;
	if (trace & BASIC_TRACE) Log(STDUSERLOG,"  try rejoinder for: %s",GetTopicName(currentTopicID));

	int id = inputRejoinderRuleID;
    char level[400];
    char word[MAX_WORD_SIZE];
    ReadCompiledWord(ptr,level); //   what marks this level
	char* usedRule = NULL;
	ChangeDepth(1,"RejoinderCode");
    while (ptr && *ptr) //   loop will search for a level answer it can use
    {
        ReadCompiledWord(ptr,word); // read responder type
        if (!*word) break; //   no more data
        if (TopLevelRule(word)) break; // failed to find rejoinder
        else if (*word < *level) break;  // end of local choices
        else if (!stricmp(word,level)) // check rejoinder
        {
			result = TestRule(id,ptr,buffer);
			if (result == FAIL_MATCH) result = FAILRULE_BIT; // convert 
			if (!result) // we found a match
			{
				usedRule = ptr;
				unusedRejoinder = false;
				break; 
			}
			if (result & (RETRYTOPIC_BIT|RETRYSENTENCE_BIT|FAILTOPIC_BIT|ENDTOPIC_BIT|FAILSENTENCE_BIT|ENDSENTENCE_BIT|RETRYSENTENCE_BIT|ENDINPUT_BIT)) break;
			result = 0;
        }
       ptr = FindNextRule(NEXTRULE,ptr,id); //   wrong or failed responder, swallow this subresponder whole
    }
	if (pushed) PopTopic(); 
	ChangeDepth(-1,"RejoinderCode");

    if (inputSentenceCount) // this is the 2nd sentence that failed, give up
    {   
        inputRejoinderRuleID = NO_REJOINDER;
        unusedRejoinder = false;
    }
    trace = oldtrace;
	return  result & (-1 ^ (ENDRULE_BIT | FAILRULE_BIT | RETRYRULE_BIT));
}

static unsigned int RespondCode(char* buffer)
{  // failing to find a responder is not failure.
	char* name = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	unsigned oldIndex = responseIndex;
	unsigned int topic = FindTopicIDByName(name);
	if (!topic)  return FAILRULE_BIT; 
	if (GetTopicFlags(topic) & TOPIC_BLOCKED) 
	{
		if (!stricmp(arg2,"FAIL") && responseIndex <= oldIndex)  return FAILRULE_BIT; // report failure
		return 0;
	}
	unsigned int oldTopic = currentTopicID;
	int pushed =  PushTopic(topic); 
	if (pushed < 0) return FAILRULE_BIT;
	ChangeDepth(1,"RespondCode");
	unsigned int result = PerformTopic(0,buffer);
	ChangeDepth(-1,"RespondCode");
	if (pushed) PopTopic();

	AddKeep(currentRule);  //   do not allow responders to erase his nest call whether or not he succeeds  BUG ???
	result &= -1 ^ (ENDTOPIC_BIT|ENDRULE_BIT); // these are swallowed
	currentTopicID = oldTopic;
	if (!result && !stricmp(arg2,"FAIL") && responseIndex <= oldIndex)  result = FAILRULE_BIT; // report failure
	return result; 
}

static unsigned int ReuseCode(char* buffer) 
{ 
	int id = 0;
	char* arg1 = ARGUMENT(1); // label of rule or topic.label
	if (!*arg1) return FAILRULE_BIT;

	unsigned int topic = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* arg2 = ARGUMENT(2); // optional- if there not allowed to use erased rules
	char* arg3 = ARGUMENT(3); // possible fail value
	if (!stricmp(arg2,"FAIL")) // make it 3rd argument if it exists
	{
		strcpy(arg2,arg3);
		strcpy(arg3,"FAIL");
	}

	char* rule;
	char* dot = strchr(arg1,'.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg1);
	else rule = GetLabelledRule(topic,arg1,arg2,fulllabel,crosstopic,id);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
	int oldreuseid = currentReuseID;

	if (topic == currentTopicID) currentReuseID = currentRuleID; // LOCAL reuse

	// execute rule 
	SAVEOLDCONTEXT()
	currentRule = rule;
	currentRuleID = id;
	currentRuleTopic = currentTopicID = topic;
	
	unsigned int holdindex = responseIndex;
	ChangeDepth(1,"reuseCode");
	if (debugger)
	{
		if (oldTopic != currentTopicID) Debugger(ENTER_REUSE_TOPIC);
		Debugger(ENTER_RULE);
	}
	unsigned int result = ProcessRuleOutput(currentRule,currentRuleID,buffer); 
	if (debugger) Debugger(EXIT_RULE);
	ChangeDepth(-1,"reuseCode");
	if (crosstopic && responseIndex > holdindex) AddPendingTopic(topic); // restore caller topic as interesting
	if (debugger && oldTopic != currentTopicID) Debugger(EXIT_REUSE_TOPIC);
	RESTOREOLDCONTEXT()

	currentReuseID = oldreuseid;

	if (trace & OUTPUT_TRACE) Log(STDUSERTABLOG,""); //   restore index from lower level
	if (!result && holdindex == responseIndex && !stricmp(arg3,"FAIL")) return FAILRULE_BIT; // user wants notification of failure
	return result;
}

static unsigned int AvailableCode(char* buffer) 
{ 
	int id = 0;
	char* arg1 = ARGUMENT(1); // label of rule or topic.label
	if (!*arg1) return FAILRULE_BIT;

	unsigned int topic = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* rule;
	char* dot = strchr(arg1,'.');
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg1);
	else rule = GetLabelledRule(topic,arg1,"",fulllabel,crosstopic,id);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
	unsigned int result = UsableRule(topic,id);
	if (!result && !stricmp(ARGUMENT(2),"FAIL")) return FAILRULE_BIT; // user wants notification of failure
	sprintf(buffer,"%d",result);
	return 0;
}

static unsigned int SetRejoinderCode(char* buffer)
{
	char* tag = ARGUMENT(1);

	unsigned int topic = currentTopicID;
	bool fulllabel = false;
	bool crosstopic = false;
	char* rule;
	char* dot = strchr(tag,'.');
	int id;
	if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,tag);
	else rule = GetLabelledRule(topic,tag,"",fulllabel,crosstopic,id);
	if (!rule) return FAILRULE_BIT; // unable to find labelled rule 
	outputRejoinderRuleID = id; 
 	outputRejoinderTopic = topic;
	if (trace & OUTPUT_TRACE) Log(STDUSERLOG,"  **set rejoinder at %s.%d.%d\r\n",tag,TOPLEVELID(id),REJOINDERID(id));
	return 0;
}

static unsigned int TopicFlagsCode(char* buffer)
{
	sprintf(buffer,"%d",GetTopicFlags(FindTopicIDByName(ARGUMENT(1))));
	return 0;
}

//////////////////////////////////////////////////////////
/// TOPIC LISTS
//////////////////////////////////////////////////////////

static unsigned int GetTopicsWithGambitsCode(char* buffer)
{ 
    unsigned int topicid;
	unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store,0);
	*buffer = 0;

    unsigned int start = 0;
    while (++start <= numberOfTopics) 
    {
        topicid = start;
        if (topicid == currentTopicID || HasGambits(topicid) <= 0) continue;
		MEANING T = MakeMeaning(StoreWord(GetTopicName(topicid)));
		FACT* F = CreateFact(T, MgambitTopics,T,FACTTRANSIENT);
		AddFact(store,F);
	}
	if (impliedSet == ALREADY_HANDLED && FACTSET_COUNT(store) == 0) return FAILRULE_BIT;
	impliedSet = impliedWild = ALREADY_HANDLED;	
	return 0;
}

static int OrderTopics(unsigned short topicList[MAX_TOPIC_KEYS],unsigned int matches[MAX_TOPIC_KEYS]) // find other topics that use keywords
{
	bool newpass = topicList[1] != 0;
	unsigned int max = 2;
    unsigned int index = 0;
    unsigned int i;
	char currentTopic[MAX_WORD_SIZE];
	GetActiveTopicName(currentTopic); // current topic, not system or nostay.
	unsigned int baseid = FindTopicIDByName(currentTopic);

	//  value on each topic
    for (i = 1; i <= numberOfTopics; ++i) // value 0 means we havent computed it yet. Value 1 means it has been erased.
    {
		if (i == baseid || BlockedBotAccess(i)) continue;

        char* name = GetTopicName(i);
	    unsigned int val = topicList[i];
        if (!val) //   compute match value
        {
            char word[MAX_WORD_SIZE];
            strcpy(word,name);
			char* dot = strchr(word,'.');
			if (dot) *dot = 0;	// use base name of the topic, not topic family name.
            WORDP D = FindWord(word); //   go look up the ~word for it
            if (!D) continue; // topic not found -- shouldnt happen

			// Note- once we have found an initial match for a topic name, we dont want to match that name again...
			// E.g., we have a specific topic for a bot, and later a general one that matches all bots. We dont want that later one processed.
  			if (D->inferMark == inferMark) continue;	// already processed a topic of this name
			D->inferMark = inferMark;

            //   look at references for this topic
            int start = 0;
            while (GetIthSpot(D,++start)) // find matches in sentence
            {
                // value of match of this topic in this sentence
                for (unsigned int k = positionStart; k <= positionEnd; ++k)
                {
					if (trace & OUTPUT_TRACE) Log(STDUSERLOG,"%s->%s ",wordStarts[k],word);
                    val += 10 + strlen(wordStarts[k]);   // each hit gets credit 10 and length of word as subcredit
					if (!stricmp(wordStarts[k],name+1) || !stricmp(wordCanonical[k],name+1)) val += 20; //  exact hit on topic name
                }
				if (positionEnd < positionStart) // phrase subcomponent
				{
					if (trace & OUTPUT_TRACE)  Log(STDUSERLOG,"%s->%s",wordStarts[positionStart],word);
					val += 10;
  				}
            }

			//   Priority modifiers

			char priority = ' ';
			if (GetTopicFlags(i) & TOPIC_PRIORITY && val)
			{
				priority = '+';
				val  *= 3; //  raise its value
			}
  			else if (GetTopicFlags(i) & TOPIC_LOWPRIORITY && val)
			{
				priority = '-';
				val  /= 3; // lower its value
			}

			topicList[i] = (unsigned short)(val + 1); //  1 means we did compute it, beyond that is value
			if (trace & MATCH_TRACE && val > 1) Log(STDUSERLOG,"%c(%d) ",priority,topicList[i]);
		} //   close if

        if (val >= max) //  find all best
        {
            if (val > max) // new high value
            {
                max = val;
                index = 0;
            }
            matches[++index] = i;
        }
    }
	if (trace && newpass ) Log(STDUSERLOG,"\r\n");
	matches[0] = max;
    return index;
}

unsigned int KeywordTopicsCode(char* buffer)
{	//   find  topics best matching his input words - never FAILS but can return 0 items stored
    unsigned short topicList[MAX_TOPIC_KEYS];
    memset(topicList,0,MAX_TOPIC_KEYS * sizeof(short));
	
	int set = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(set,0);

	bool onlyGambits =  (!stricmp(ARGUMENT(1),"gambit")); 

    //   now consider topics in priority order
	SET_FACTSET_COUNT(set,0);
	unsigned int index;
    unsigned int matches[MAX_TOPIC_KEYS];
	NextinferMark();
	while ((index = OrderTopics(topicList,matches))) //   finds all at this level. 1st call evals topics. other calls just retrieve.
    {
        //   see if equally valued topics found are feasible, if so, return one chosen at random
        while (index) // items are 1-based
        {
            unsigned int which = random(index) + 1; 
            unsigned int topic = matches[which];
            topicList[topic] = 1; 
            matches[which] = matches[index--]; // swap equally valued end back to fill in position

			unsigned int flags = GetTopicFlags(topic);
			if (onlyGambits && (flags & TOPIC_SYSTEM || !HasGambits(topic))) continue;
				
			char word[MAX_WORD_SIZE];
			strcpy(word,GetTopicName(topic,true));
			if (impliedSet == ALREADY_HANDLED) // just want one
			{
				strcpy(buffer,word);
				break;
			}

			char value[100];
			sprintf(value,"%d",matches[0]);
			MEANING M = MakeMeaning(StoreWord(word));
			AddFact(set,CreateFact(M,Mkeywordtopics,MakeMeaning(StoreWord(value)),FACTTRANSIENT));
        }   
    }
	if (impliedSet == ALREADY_HANDLED && FACTSET_COUNT(set) == 0) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
    return 0;
}

static unsigned int PendingTopicsCode(char* buffer)
{
	int set = GetSetID(ARGUMENT(1));
	if (set < 0) return FAILRULE_BIT;
	PendingTopics(set);
	return 0;
}

static unsigned int QueryTopicsCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) // not in assignment
	{
		QueryTopicsOf(ARGUMENT(1),0,NULL); 
		return (FACTSET_COUNT(0)) ? 0 : FAILRULE_BIT;
	}
	return QueryTopicsOf(ARGUMENT(1),impliedSet,NULL); 
}

//////////////////////////////////////////////////////////
/// MARKINGS
//////////////////////////////////////////////////////////

static unsigned int GetParseCode(char* buffer) 
{  
	char* original = buffer;
	char* ptr = ARGUMENT(1);
	char var[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,var); // the locator _
	if (*var != '_') return FAILRULE_BIT;
	unsigned int id = GetWildcardID(var);
	unsigned int start = WILDCARD_START(wildcardPosition[id]);
	unsigned int end = WILDCARD_END(wildcardPosition[id]);
	
	ptr = ReadCompiledWord(ptr,var); // what we want
	unsigned int role = 0;
	if (!stricmp(var,"PHRASE"))
	{
		unsigned int phrase = phrases[start];
		if (!phrase) return FAILRULE_BIT;
		while (phrases[++end] == phrase){;}
		--end;
	}
	else if (!stricmp(var,"CLAUSE"))
	{
		unsigned int clause = clauses[start];
		if (!clause) return FAILRULE_BIT;
		while (clauses[++end] == clause){;}
		--end;
	}
	else if (!stricmp(var,"VERBAL"))
	{
		unsigned int verbal = verbals[start];
		if (!verbal) return FAILRULE_BIT;
		while (verbals[++end] == verbal){;}
		--end;
	}
	else if (!stricmp(var,"NOUNPHRASE"))
	{
		char* phrase = GetNounPhrase(start,"");
		if (*phrase == 0) return FAILRULE_BIT;
		sprintf(buffer,"%s",phrase);
		return 0;
	}
	if (*ptr) // hunting for this role
	{
		ReadCompiledWord(ptr,var); // the role
		role = (unsigned int) FindValue2ByName(var);
		if (!role) return FAILRULE_BIT;
	}

	ptr = buffer;
	while (start <= end)
	{
		if (role && !(roles[start] & role)) continue; // print out the role
		if (role == OBJECT2 && roles[start] & SUBJECT2) continue;	// this is clause subject where clause is object as a whole
		sprintf(buffer,"%s_",wordStarts[start++]);
		buffer += strlen(buffer);
		if (role) break;	 // just do one value of it (if coordinating conjunction exists)
	}
	if (buffer == original) return FAILRULE_BIT;

	*(--buffer); // remove trailing _
	return 0;
}

static unsigned int MarkCode(char* buffer) 
{  
	// argument1 is a word or ~set or missing
	// mark()  turn back on the unmark all system
	// mark(word _xxx) enable word mark at location of _xxx variable
	// mark(word  1) enables mark at specified location if within range of input
	// mark(word) enables mark at location 1 
	char* ptr = ARGUMENT(1);
	
	if (!*ptr) // mark() reenables global unmarking
	{
		if (oldunmarked[0]) // merge state back if have cached
		{
			memcpy(unmarked,oldunmarked,MAX_SENTENCE_LENGTH);
			oldunmarked[0] = 0;
		}
		return 0;
	}

	unsigned int result;
	char word[MAX_WORD_SIZE];
	ptr = ReadShortCommandArg(ptr,word,result); // what is being marked
	if (result & ENDCODES) return result;
	if (!*word) return FAILRULE_BIT; // missing arg

	char word1[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,word1);  // the locator

	unsigned startPosition;
	unsigned endPosition;
	if (!*word1 || *word1 == ')') startPosition = endPosition = 1; // default mark  (ran out or hit end paren of call
	else if (IsDigit(*word1)) endPosition = startPosition = atoi(word1); // named number as index
	else if (*word1 == '_') //  wildcard position designator
	{
		startPosition = wildcardPosition[GetWildcardID(word1)] & 0x0000ffff; // the match location
		endPosition = wildcardPosition[GetWildcardID(word1)] >> 16; 
	}
	else return FAILRULE_BIT;

	if (startPosition < 1) endPosition = startPosition = 1;
	if (startPosition > wordCount)  endPosition = startPosition = wordCount;
	MarkFacts(MakeMeaning(StoreWord(word)),startPosition,endPosition);
	return 0;
}

static unsigned int MarkedCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	if (*ARGUMENT(1) == '$')  // indirect thru variable
	{
		char* at = GetUserVariable(ARGUMENT(1));
		if (at) arg1 = at;
	}

	WORDP D = FindWord(arg1);
	if (!D) return FAILRULE_BIT;
	strcpy(buffer, (GetNextSpot(D,0,positionStart,positionEnd)) ?  (char*) "1" : (char*)"");
	return 0;
}

static unsigned int PositionCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	unsigned int result;
	char word[MAX_WORD_SIZE];
	ptr = ReadShortCommandArg(ptr,word,result); // start or end
	if (result & ENDCODES) return result;
	char word1[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,word1);  // the _ var
	if (*word1 == '_') //  wildcard position designator
	{
		if (!stricmp(word,"start")) sprintf(buffer,"%d",WILDCARD_START(wildcardPosition[GetWildcardID(word1)]));  // the match location
		else if (!stricmp(word,"end")) sprintf(buffer,"%d", WILDCARD_END(wildcardPosition[GetWildcardID(word1)]));
		else if (!stricmp(word,"both")) sprintf(buffer,"%d", wildcardPosition[GetWildcardID(word1)]);
		else return FAILRULE_BIT;
	}
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int SetPositionCode(char* buffer)
{
	char* ptr = ARGUMENT(1);
	unsigned int val = atoi(ptr);
	if (val == 0) return FAILRULE_BIT;
	positionStart = WILDCARD_START(val);
	unsigned int end = WILDCARD_END(val);
	positionEnd = (end) ? end : positionStart;
	return 0;
}

static unsigned int CapitalizedCode(char* buffer)
{
	if (IsDigit(*ARGUMENT(1)))
	{
		unsigned int n = atoi(ARGUMENT(1));
		if (n == 0 || n > wordCount) return FAILRULE_BIT;
		strcpy(buffer,(capState[n]) ? (char*) "1" : (char*) "0");
	}
	else if (IsAlpha(*ARGUMENT(1))) strcpy(buffer,(IsUpperCase(*ARGUMENT(1))) ? (char*) "1" : (char*) "0");
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int KeepHistoryCode(char* buffer)
{
	unsigned int count = atoi(ARGUMENT(2));
	if (count >= (MAX_USED - 1)) count = MAX_USED - 1; 
	if (!stricmp(ARGUMENT(1),"BOT"))
	{
		if (count == 0) *chatbotSaid[0] = 0;
		if (count < chatbotSaidIndex)  chatbotSaidIndex = count;
	}
	if (!stricmp(ARGUMENT(1),"USER"))
	{
		if (count == 0)  *humanSaid[0] = 0;
		if (count < humanSaidIndex) humanSaidIndex = count;
	}

	return 0;
}

static unsigned int UnmarkCode(char* buffer)
{
	// unmark() // disable global unmarks
	// unmark(* 4)	 // global unmark
	// unmark(* _location) // global unmark
	// unmark(word 4)
	// unmark(word _location)

	char* ptr = ARGUMENT(1);

	if (!*ptr) // unmark all global disable marks
	{
		if (!oldunmarked[0]) // cache current disables
		{
			memcpy(oldunmarked,unmarked,MAX_SENTENCE_LENGTH);
			oldunmarked[0] = 1;
		}
		memset(unmarked,0,MAX_SENTENCE_LENGTH); // clear all mark suppression
		return 0;
	}
	
	char word[MAX_WORD_SIZE];
	unsigned int result;
	ptr = ReadShortCommandArg(ptr,word,result);// set
	if (result & ENDCODES) return result;

	char word1[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,word1);  // the _data
	unsigned int startPosition = wordCount;
	unsigned int endPosition = 1;
	if (!*word1) startPosition = endPosition = 1;
	else if (IsDigit(*word1)) startPosition = endPosition = atoi(word1);
	else if (*word1 == '_') startPosition = WILDCARD_START(wildcardPosition[GetWildcardID(word1)]); // the match location
 	else  return FAILRULE_BIT;
	if (!startPosition || startPosition > wordCount) return 0;	// fail silently
	unsigned int endposition = WILDCARD_END(wildcardPosition[GetWildcardID(word1)]);
	if (*word == '*') // set unmark EVERYTHING -- BUG-- fail to extend unmark across the multiword
	{
		unmarked[startPosition] = 1;
		if (trace) Log(STDUSERLOG,"unmark all @word %d (%s) ",startPosition,wordStarts[startPosition]);
		for (unsigned int i = startPosition+1; i <= endposition; ++i) 
		{
			if (trace) Log(STDUSERLOG,"unmark %d %s ",i,wordStarts[i]);
			unmarked[i] = 1;
		}
	}
	else
	{
		WORDP D = FindWord(word); //   set or word to unmark
		if (!D || D->v.patternStamp != matchStamp) return 0; // not marked
		if (trace) Log(STDUSERLOG,"unmark %s @word %d  ",word,startPosition);
		RemoveMatchValue(D, GetWhereInSentence(D),startPosition);
	}
	return 0;
}

//////////////////////////////////////////////////////////
/// INPUT ROUTINES
//////////////////////////////////////////////////////////

static unsigned int InputCode(char* buffer) 
{      // when supplying multiple sentences, must do them in last first order
	if (inputCounter++ > 5) return FAILRULE_BIT;// limit per sentence reply
	if (totalCounter++ > 15) return FAILRULE_BIT; // limit per input from user

	if (trace & OUTPUT_TRACE) Log(STDUSERLOG,"\r\n");
	unsigned int result;
	char* word = ARGUMENT(1);
	Output(word,buffer,result);
	if (strlen(buffer) >= INPUT_BUFFER_SIZE) buffer[INPUT_BUFFER_SIZE-1] = 0;	// might be smaller buffer
	Convert2Blanks(buffer); // break apart underscored words
	if (!strcmp(lastInputSubstitution,buffer)) return FAILRULE_BIT; // same result as before, apparently looping

	if (showInput || trace) Log(STDUSERLOG,"input: %s\r\n",buffer);
	AddInput(buffer);
	strcpy(lastInputSubstitution,buffer);
    *buffer = 0;
	return 0;
}

static unsigned int RemoveTokenFlagsCode(char* buffer)
{
	tokenFlags ^= -1 ^ atoi(ARGUMENT(1));
	return 0;
}

static unsigned int SetTokenFlagsCode(char* buffer)
{
	tokenFlags |= atoi(ARGUMENT(1));
	return 0;
}


//////////////////////////////////////////////////////////
/// NUMBER FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int ComputeCode(char* buffer)
{
	int64 value = NOT_A_NUMBER;
	char* arg1 = ARGUMENT(1);
	char* op = ARGUMENT(2);
	char* arg2 = ARGUMENT(3);
	//   for long digits, move to float
	if (strlen(arg2) >= 11 || strlen(arg1) >= 11 || strchr(arg1,'.') || strchr(arg2,'.') || !stricmp(op,"divide") || !stricmp(op,"root") || !stricmp(op,"square_root") || !stricmp(op,"quotient") || *op == '/') //   float
	{
		float value = (float) NOT_A_NUMBER;
		float number1 = (strchr(arg1,'.')) ? (float) atof(arg1) : (float)Convert2Integer(arg1);
		float number2 = (strchr(arg2,'.')) ? (float) atof(arg2) :  (float)Convert2Integer(arg2);
		//   we must test case insenstive because arg2 might be capitalized (like add and ADD for attention deficit disorder)
		if (*op == '+' || !stricmp(op,"plus") || !stricmp(op,"add")|| !stricmp(op,"and")) value = number1 + number2; 
		else if (!stricmp(op,"minus") || !stricmp(op,"subtract")|| !stricmp(op,"deduct") || *op == '-' ) value = number1 - number2;
		else if (!stricmp(op,"x") || !strnicmp(op,"times",4) || !stricmp(op,"multiply") || *op == '*') value = number1 * number2;
		else if (!stricmp(op,"divide") || !stricmp(op,"quotient") || *op == '/' ) 
		{
			if (number2 == 0) 
			{
				strcpy(buffer,"infinity");
				return 0;
			}
			else value = number1 / number2;
		}
        else if (!stricmp(op,"remainder") || !stricmp(op,"modulo") || !stricmp(op,"mod") || *op == '%') 
		{
			ReportBug("illegal mod op in float")
			return FAILRULE_BIT;
		}
        else if (!stricmp(op,"random") )
		{
			ReportBug("illegal random op in float")
  			return FAILRULE_BIT;
		}
        else if (!stricmp(op,"root") || !stricmp(op,"square_root") ) value = (float) sqrt(number1);  
        else if (!stricmp(op,"^") || !stricmp(op,"power") || !stricmp(op,"exponent")) 
        {
			int power = (int)Convert2Integer(arg2);
            if (power >= 1 && power < 6)
            {
				value = number1;
				while (--power) value *= value;
			}
            else if (power == 0) value = 1;
		}
		if (value != NOT_A_NUMBER) 
		{
			int x = (int) value;
			strcpy(buffer,((float)x == value) ? StdIntOutput(x) : StdFloatOutput(value));
		}
		else sprintf(buffer," ?");
	}
	else //   integer
    {
		int64 value1 = Convert2Integer(arg1);
		int64 value2 = Convert2Integer(arg2);
		if (*op == '+' || !stricmp(op,"add")|| !stricmp(op,"and") || !stricmp(op,"plus")) value = value1 + value2;
		else if (*op == '-' || !stricmp(op,"deduct") || !stricmp(op,"minus") || !stricmp(op,"sub") || !stricmp(op,"subtract")) value = value1 - value2;
		else if (*op == '*' || !stricmp(op,"x") || !stricmp(op,"multiply") || !strnicmp(op,"times",4)) value = value1 * value2;
		else if ( *op == '%' || !stricmp(op,"mod") || !strcmp(op,"modulo") || !stricmp(op,"remainder")) value = value1 % value2;
		else if (!stricmp(op,"random")) value = random((unsigned int)(value2 - value1)) + value1; 
 		else if (*op == '^' || !stricmp(op,"exponent") || !stricmp(op,"power"))
		{
			if (value2 >= 1 && value2 <= 10) // only do small powers
			{
				value = value1;
				while (--value2) value *= value;
			}
			else if (value2 == 0) value = 1;
		}
        strcpy(buffer, (value == NOT_A_NUMBER) ? (char*)"?" : StdIntOutput((int)value));
	}
	return 0;
}

static unsigned int TimeFromSecondsCode(char* buffer)
{
	int64 seconds;
	char* word = ARGUMENT(1);
	ReadInt64(word,seconds);
	time_t sec = (time_t) seconds;

	// convert to text string
	strcpy(buffer,ctime(&sec));
	*strchr(buffer,'\n') = 0; // erase newline at end

	return 0;
}

//////////////////////////////////////////////////////////
/// DEBUG FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int LogCode(char* buffer)
{
	char* stream = ARGUMENT(1);
	uint64 flags;
	stream = ReadFlags(stream,flags); // try for flags
	if (flags && *stream == ')') ++stream; // skip end of flags
	char name[MAX_WORD_SIZE];
	*name = 0;
	if (!strnicmp(stream,"FILE ",5)) // write data to this file
	{
		stream = ReadCompiledWord(stream,name);
		stream = ReadCompiledWord(stream,name);
	}

	unsigned int result;
	Output(stream,buffer,result,OUTPUT_EVALCODE | (unsigned int) flags);
	if (*name)
	{
		if (*name == '"') 
		{
			size_t len = strlen(name);
			name[len-1] = 0;	// remove trailing "
		}
		FILE* out = fopen((*name == '"') ? (name+1) : name,"ab");
		if (out) 
		{
			fprintf(out,"%s",buffer);
			fclose(out);
		}
		else return FAILRULE_BIT;
	}
	else Log(STDUSERLOG,"%s",buffer);
	if (flags & OUTPUT_ECHO) printf("%s",buffer);
	*buffer = 0;
	return 0;
}


//////////////////////////////////////////////////////////
/// OUTPUT FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int FlushOutputCode(char* buffer)
{
	if (postProcessing)
	{
		ReportBug("Illegal to use ^FlushOutput during postprocessing");
		return FAILRULE_BIT;
	}
	AddResponse(currentOutputBase);
	return 0;
}

static unsigned int InsertOutput(char* stream, char* buffer, int index)
{
	// add at end, then alter order
	unsigned int result;
	Output(stream,buffer,result,OUTPUT_EVALCODE);
	if (AddResponse(buffer))
	{
		memmove(&responseOrder[index+1],&responseOrder[index],responseIndex - index); // shift order out 1
		responseOrder[index] = (unsigned char)(responseIndex-1);
	}
	return result;
}

static unsigned int InsertPrintCode(char* buffer) 
{     
	if (postProcessing)
	{
		ReportBug("Illegal to use ^InsertPrePrint during postprocessing");
		return FAILRULE_BIT;
	}
	char* stream = ARGUMENT(1);
	uint64 flags;
	stream = ReadFlags(stream,flags); // try for flags
	if (flags) ++stream; // skip end of flags
	unsigned int result;
	char beforeIndex[MAX_WORD_SIZE];
	stream = ReadShortCommandArg(stream,beforeIndex,result); 
	int index = 0;
	
	if (*beforeIndex == '~') // put before 1st reference to this topic
	{
		unsigned int topic = FindTopicIDByName(beforeIndex);
		for (int i = responseIndex-1; i > 0; --i)
		{
			if (topic == responseData[responseOrder[i]].topic) index = responseOrder[i];
		}	
	}
	else if (IsDigit(*beforeIndex)) // numeric index he gives must be 1 based, eg before %response 
	{
		index = atoi(beforeIndex);
		if (index <= 0 || index > (int)responseIndex) return FAILRULE_BIT;
		index = responseOrder[index-1]; // the current location of the output
	}
	return InsertOutput(stream,buffer,index);
}

static unsigned int PrintCode(char* buffer) 
{     
	if (postProcessing)
	{
		ReportBug("Illegal to use ^Print during postprocessing");
		return FAILRULE_BIT;
	}
	char* stream = ARGUMENT(1);
	uint64 flags;
	stream = ReadFlags(stream,flags); // try for flags
	if (flags && *stream == ')') ++stream; // skip end of flags

	unsigned int result;
	Output(stream,buffer,result,OUTPUT_EVALCODE | (unsigned int) flags);
	AddResponse(buffer);
	return result;
}

static unsigned int PrePrintCode(char* buffer)
{
	if (postProcessing)
	{
		ReportBug("Illegal to use ^PrePrint during postprocessing");
		return FAILRULE_BIT;
	}
	char* stream = ARGUMENT(1); 
	uint64 flags;
	stream = ReadFlags(stream,flags); // try for flags
	if (flags) ++stream; // skip end of flags
	return InsertOutput(stream,buffer,0);
}

static unsigned int RepeatCode(char* buffer)
{ 
	if (postProcessing)
	{
		ReportBug("Illegal to use ^Repeat during postprocessing");
		return FAILRULE_BIT;
	}
	AddRepeatable(currentRule); // local repeats allowed this volley
	return 0;
}

//////////////////////////////////////////////////
/// OUTPUT ACCESS
//////////////////////////////////////////////////

static unsigned int ResponseCode(char* buffer)
{
	unsigned int index = atoi(ARGUMENT(1)) ;
	if (index > responseIndex || !index) return FAILRULE_BIT;
	sprintf(buffer,"%s",responseData[responseOrder[index-1]].response);
	return 0;
}

static unsigned int ResponseQuestionCode(char* buffer)
{
	unsigned int index = atoi(ARGUMENT(1)) - 1; // which response (1 based)
	if (index >= responseIndex) return FAILRULE_BIT;
	char* ptr = TrimSpaces(responseData[responseOrder[index]].response,false);
	strcpy(buffer,(ptr[strlen(ptr)-1] == '?') ? (char*) "1" : (char*) ""); 
	return 0;
}

static unsigned int ResponseRuleIDCode(char* buffer)
{
	unsigned int index = atoi(ARGUMENT(1));
	if (index > responseIndex || !index) return FAILRULE_BIT;
	int topic = responseData[responseOrder[index-1]].topic;
	sprintf(buffer,"%s%s",GetTopicName(topic),responseData[responseOrder[index-1]].id);
	return 0;
}

//////////////////////////////////////////////////////////
/// POSTPROCESSING FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int AnalyzeCode(char* buffer)
{
	if (postProcessing)
	{
		char* word = ARGUMENT(1);
		SAVEOLDCONTEXT()
		unsigned int result;
		Output(word,buffer,result);
		Convert2Blanks(buffer); // remove system underscoring back to blanks
		PrepareSentence(buffer,true,false); 
		*buffer = 0; // only wanted effect of script
		RESTOREOLDCONTEXT()
		return 0;
	}
	else 
	{
		ReportBug("Cannot use ^Analyze except in postprocessing");
		return FAILRULE_BIT;
	}
}

static unsigned int PostProcessPrintCode(char* buffer) // only works if post processing
{     
	if (!postProcessing) 
	{
		ReportBug("Cannot use ^PostProcessPrint except in postprocessing");
		return FAILRULE_BIT;
	}
	
	char* stream = ARGUMENT(1);		
	uint64 flags;
	stream = ReadFlags(stream,flags); // try for flags
	if (flags) ++stream; // skip end of flags

	unsigned int result;
	Output(stream,buffer,result,OUTPUT_EVALCODE| (unsigned int)flags);

	// prepend output 
	strcat(buffer," ");
	strcat(buffer,postProcessing);
	strcpy(postProcessing,buffer);
	*buffer = 0;
	return result;
}

static unsigned int ReviseOutputCode(char* buffer)
{
	if (postProcessing) return FAILRULE_BIT;
	char* arg1 = ARGUMENT(1); // index first, rest is output
	unsigned int index = atoi(arg1);
	if (!IsDigit(*arg1)) return FAILRULE_BIT;
	if (index > responseIndex) return FAILRULE_BIT;
	char* arg2 = ARGUMENT(2);
	strcpy(responseData[index-1].response,arg2);
	return 0;
}

//////////////////////////////////////////////////////////
/// COMMAND FUNCTIONS
//////////////////////////////////////////////////////////

static unsigned int CommandCode(char* buffer) 
{
	DoCommand(ARGUMENT(1),false);
	return 0;
}

unsigned int DebugCode(char* buffer) 
{	
	return 0;
}

static unsigned int EndCode(char* buffer)
{ //   good for stopping a loop w/o stopping the rule
	char* word = ARGUMENT(1);
	if (!stricmp(word,"RULE")) return ENDRULE_BIT;
	if (!stricmp(word,"TOPIC")) return ENDTOPIC_BIT;
	if (!stricmp(word,"SENTENCE")) return ENDSENTENCE_BIT;
	if (!stricmp(word,"INPUT")) return ENDINPUT_BIT;
    return FAILRULE_BIT;
}

static unsigned int EvalCode(char* buffer) //  ??? needed with output eval instead?
{	
	unsigned int result;
	Output(ARGUMENT(1),buffer,result,OUTPUT_EVALCODE); 
	return result;
}

static unsigned int FailCode(char* buffer) 
{      
	char* word = ARGUMENT(1);
	if (!stricmp(word,"RULE")) return FAILRULE_BIT;
	if (!stricmp(word,"TOPIC")) return FAILTOPIC_BIT;
	if (!stricmp(word,"SENTENCE")) return FAILSENTENCE_BIT;
	if (!stricmp(word,"INPUT")) return FAILINPUT_BIT;
	return FAILRULE_BIT;
}

unsigned int MatchCode(char* buffer) 
{     
	char word[MAX_WORD_SIZE];
	char word1[MAX_WORD_SIZE];
	char* at = ReadCompiledWord(ARGUMENT(1),word1);
	if (*word1 == '$' && !*at) strcpy(word,GetUserVariable(word1)); //   solitary user var, decode it  eg match($var)
	else if (*word1 == '_' && !*at) strcpy(word,wildcardCanonicalText[GetWildcardID(word1)]); //   solitary user var, decode it  eg match($var)
	else 
	{
		if (word1[0] == FUNCTIONSTRING && word1[1] == '(') strcpy(word,word1+1);
		else strcpy(word,word1); // otherwise it is what to say (like from idiom table)
		Convert2Blanks(word);
	}
	char* ptr = word;
	if (*word)  
	{
		size_t len = strlen(word);
		strcpy(word+len," )"); // insure it has a closing paren (even if it has);
		if (*word == '"') 
		{
			word[len-1] = ' '; // change closing " to space
			++ptr;	// skip opening "
			if (*ptr == FUNCTIONSTRING) ++ptr; // bypass the mark

			// now purify of any internal \" " marked strings
			char* x = strchr(ptr,'\\');
			while (x)
			{
				if (x[1] == '"') memmove(x,x+1,strlen(x));	// remove escape
				x = strchr(x + 1,'\\'); // next?
			}
		}
	}
	if (*ptr == FUNCTIONSTRING) ++ptr;	// skip compiled string mark
	if (*ptr == '(') ++ptr;		// skip opening paren of a pattern
	while (*ptr == ' ') ++ptr;	// prepare for start
	unsigned int gap = 0;
	unsigned int wildcardSelector = 0;
	wildcardIndex = 0;  //   reset wildcard allocation on top-level pattern match
	unsigned int junk;
	if (!*word) return FAILRULE_BIT;	// NO DATA?
	blocked = true;
    bool match = Match(ptr,0,0,'(',true,gap,wildcardSelector,junk,junk) != 0;  //   skip paren and treat as NOT at start depth, dont reset wildcards- if it does match, wildcards are bound
	blocked = false;
	if (!match) return FAILRULE_BIT;
	return 0;
}

static unsigned int NoFailCode(char* buffer)
{      
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(ARGUMENT(1),word);
	unsigned int result;
	ChangeDepth(1,"noFailCode");
	Output(ptr,buffer,result);
	ChangeDepth(-1,"noFailCode");
	if (!stricmp(word,"RULE")) return result & (ENDTOPIC_BIT|FAILTOPIC_BIT|RETRYTOPIC_BIT|ENDSENTENCE_BIT|FAILSENTENCE_BIT|ENDINPUT_BIT|RETRYSENTENCE_BIT);
	else if (!stricmp(word,"TOPIC")) return result & (ENDSENTENCE_BIT|FAILSENTENCE_BIT|RETRYSENTENCE_BIT|ENDINPUT_BIT);
	else if (!stricmp(word,"SENTENCE") || !stricmp(word,"INPUT")) return 0;
	return FAILRULE_BIT; // not a legal choice
}

static unsigned int NotNullCode(char* buffer)
{
	unsigned int result;
	Output(ARGUMENT(1),buffer,result);
	if (*buffer) *buffer = 0;
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int RetryCode(char* buffer)
{
	char* arg = ARGUMENT(1);
	if (!stricmp(arg,"TOPIC"))  return RETRYTOPIC_BIT;
	if (!stricmp(arg,"sentence"))  return RETRYSENTENCE_BIT;
	return RETRYRULE_BIT;
}

//////////////////////////////////////////////////////////
/// WORD MANIPULATION
//////////////////////////////////////////////////////////

static unsigned int BurstCode(char* buffer) //   take value and break into facts of burst-items as subjects
{//   ^burst(^cause : )   1: data source 2: burst character
	char result[MAX_WORD_SIZE];
	result[0] = 0;

	//   prepare spot to store pieces
	MEANING verb;
	MEANING object;
	if (impliedWild != ALREADY_HANDLED)  SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
	object = verb = Mburst;
	*buffer = 0;

	char* ptr = ARGUMENT(1); //   what to burst
	if (!*ptr) return 0;

	//   get string to search for. If quoted, remove the quotes
	char* scan = ARGUMENT(2);	//   how to burst
	if (!*scan) scan = "_"; // default
	else if (*scan == '"' ) // if a quoted string, remove the quotes
	{
		++scan;
		size_t len = strlen(scan);
		if (scan[len-1] == '"') scan[len-1] = 0;
	}

	//   loop that splits into pieces and stores them

	char* hold = strstr(ptr,scan);
	size_t scanlen = strlen(scan);
	if (impliedSet != ALREADY_HANDLED) SET_FACTSET_COUNT(impliedSet,0);
	while (hold)
	{
		*hold = 0;	//   terminate first piece
		if (*ptr == 0) {;} // null piece - breaking here makes no sense at start
		//   store piece before scan marker
		else if (impliedWild != ALREADY_HANDLED)  SetWildCard(ptr,ptr,0,0);
		else if (impliedSet != ALREADY_HANDLED)
		{
			MEANING T = MakeMeaning(StoreWord(ptr));
			FACT* F = CreateFact(T, verb,object,FACTTRANSIENT);
			AddFact(impliedSet,F);
		}
		else //   dump straight to output buffer, first piece only
		{
			strcpy(buffer,ptr);
			break;
		}

		ptr = hold + scanlen; //   ptr after scan marker
		hold = strstr(ptr,scan); //   find next piece
	}

	//   assign the last piece
	if (impliedWild != ALREADY_HANDLED)  
	{
		SetWildCard(ptr,ptr,0,0);
		SetWildCard("","",0,0); // clear next one
	}
	else if (impliedSet != ALREADY_HANDLED)
	{
		if (*ptr) AddFact(impliedSet,CreateFact(MakeMeaning(StoreWord(ptr)), verb,object,FACTTRANSIENT));
	}
	else if (!*buffer) strcpy(buffer,ptr);
	impliedSet = impliedWild = ALREADY_HANDLED;	//   we did the assignment
	return 0;
}

static unsigned int ExplodeCode(char* buffer) //   take value and break into facts of burst-items as subjects
{
	char result[MAX_WORD_SIZE];
	result[0] = 0;

	//   prepare spot to store pieces
	MEANING verb;
	MEANING object;
	if (impliedWild != ALREADY_HANDLED)  SetWildCardIndexStart(impliedWild); //   start of wildcards to spawn
	object = verb = Mburst;
	*buffer = 0;

	char* ptr = ARGUMENT(1) - 1; //   what to explode
	char word[MAX_WORD_SIZE];
	word[1] = 0;
	SET_FACTSET_COUNT(impliedSet,0);
	while (*++ptr)
	{
		*word = *ptr;
		//   store piece before scan marker
		if (impliedWild != ALREADY_HANDLED)  SetWildCard(word,word,0,0);
		else if (impliedSet != ALREADY_HANDLED)
		{
			MEANING T = MakeMeaning(StoreWord(word));
			FACT* F = CreateFact(T, verb,object,FACTTRANSIENT);
			AddFact(impliedSet,F);
		}
		else //   dump straight to output buffer, first piece only
		{
			strcpy(buffer,word);
			break;
		}
	}
	impliedSet = impliedWild = ALREADY_HANDLED;	
	return 0;
}

static unsigned int FlagsCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D) return FAILRULE_BIT;
	sprintf(buffer,"%lld",D->systemFlags);
	return 0;
}

static unsigned int UppercaseCode(char* buffer)
{
	strcpy(buffer, (IsUpperCase(ARGUMENT(1)[0])) ? (char*) "1" : (char*) "0");
	return 0;
}

static unsigned int PropertiesCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D) return FAILRULE_BIT;
	sprintf(buffer,"%lld",D->properties);
	return 0;
}
static char* NextWord(char* ptr, WORDP& D,bool canon)
{
	char word[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr,word);
	MakeLowerCase(word);
	if (canon)
	{
		WORDP entry,canonical;
		GetPosData(word,entry,canonical);
		if (canonical) strcpy(word,canonical->word);
		else if (entry) strcpy(word,entry->word);
	}
	MakeLowerCase(word);
	D = StoreWord(word);
	return ptr;
}

static unsigned int IntersectWordsCode(char* buffer)
{
	unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
	SET_FACTSET_COUNT(store,0);
	WORDP words[2000];
	int index = 0;

	char* arg1 = ARGUMENT(1);
	Convert2Blanks(arg1);
	char* at = arg1;
	while ((at = strchr(at,'~'))) *at = ' '; 

	char* arg2 = ARGUMENT(2); 
	Convert2Blanks(arg2);
	at = arg2;
	while ((at = strchr(at,'~'))) *at = ' '; 

	bool canon = (!stricmp(ARGUMENT(3),"canonical"));
	WORDP D;
	while (*arg1)
	{
		arg1 = NextWord(arg1,D,canon);
		AddInternalFlag(D,INTERNAL_MARK);
		words[++index] = D;
	}

	unsigned int count = 0;
	if (index) 
	{
		while (*arg2) 
		{
			arg2 = NextWord(arg2,D,canon);
			if (D->internalBits & INTERNAL_MARK)
			{
				FACT* old = factFree;
				FACT* F = CreateFact(MakeMeaning(D),Mintersect,MakeMeaning(D),FACTTRANSIENT);
				if (old != factFree) 
				{
					AddFact(store,F);
					count = 1;
				}
			}
		}
	
		while (index) RemoveInternalFlag(words[index--],INTERNAL_MARK);
	}

	if (impliedSet == ALREADY_HANDLED && count == 0) return FAILRULE_BIT;
	return 0;
}

static unsigned int JoinCode(char* buffer) 
{     
	char* original = buffer;
	char* ptr = ARGUMENT(1);
	bool autospace = false;
	if (!strnicmp(ptr,"AUTOSPACE",9))
	{
		autospace = true;
		ptr += 10;
	}
    while (ptr)
	{
		char word[MAX_WORD_SIZE];
		char* at = ReadCompiledWord(ptr,word); 
        if (*word == ')' || !*word) break; //   done
		if (*word == '"' && FUNCTIONSTRING) // compiled code being joined
		{
			ReformatString(buffer,word+2);
			ptr = at;
		}
 		else 
		{
			unsigned int result;
			ptr = ReadShortCommandArg(ptr,word,result);
			if (result & ENDCODES) return result;
			strcpy(buffer,word);
		}
		if (trace & OUTPUT_TRACE) Log(STDUSERLOG,"%s ",buffer);
		bool did = *buffer != 0;
		buffer += strlen(buffer);
		if (autospace && did) *buffer++ = ' '; 
    }
	if (autospace && original != buffer) *--buffer = 0;
	if (trace & OUTPUT_TRACE) Log(STDUSERLOG,") = %s ",original);
 	return 0;
}

static unsigned int POSCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	if (arg2 && *arg2 == '~') ++arg2;	// concept name sans ~
	char* arg3 = ARGUMENT(3);

	if (!stricmp(arg1,"syllable"))
	{
		sprintf(buffer,"%d",ComputeSyllables(arg2));
		return 0;
	}

	if (!stricmp(arg1,"type"))
	{
		if (*arg2 == '~') strcpy(buffer,"concept");
		else if (IsDigit(*arg2)) strcpy(buffer,"number");
		else if (IsAlpha(*arg2)) strcpy(buffer,"word");
		else strcpy(buffer,"unknown");
		return 0;
	}

	if (!stricmp(arg1,"verb"))
	{
		if (!ARGUMENT(2)) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word,ARGUMENT(2));
		char* infin = word; 
		if (!stricmp(arg3,"present_participle")) 
		{
			char* use = GetPresentParticiple(word);
			if (!use) return FAILRULE_BIT;
			strcpy(buffer,use);
		}
		else if (!stricmp(arg3,"past_participle")) 
		{
			char* use = GetPastParticiple(word);
			if (!use) return FAILRULE_BIT;
			strcpy(buffer,use);
		}
		else if (!stricmp(arg3,"infinitive")) 
		{
			char* verb = GetInfinitive(word,true);
			strcpy(buffer,verb);
		}
		else if (!stricmp(arg3,"past")) 
		{
			char* past = GetPastTense(infin);
			if (!past) return FAILRULE_BIT;
			strcpy(buffer,past);
		}
		else if (!stricmp(arg3,"match"))
		{
			char* arg4 = ARGUMENT(4);
			WORDP D = FindWord(arg4);
			char* plural = GetPluralNoun(D);
			char* verb;
			if (!plural || stricmp(plural,arg4)) // singular noun
			{
				verb = GetThirdPerson(arg2);
				if (verb)  strcpy(buffer,verb);
			}
			else // plural noun
			{
				verb = GetInfinitive(arg2,false);
				if (verb) 
				{
					if (!stricmp(verb,"be")) strcpy(buffer,"are");
					else strcpy(buffer,verb);
				}
			}
			if (!*buffer) strcpy(buffer,arg2);
		}
		if (IsUpperCase(ARGUMENT(2)[0])) *buffer = toUppercaseData[*buffer];
		return 0;
	}
	else if (!stricmp(arg1,"aux"))
	{
		if (!ARGUMENT(2)) return FAILRULE_BIT;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word,ARGUMENT(2));
		char* result = word;
   
		if (!strcmp(arg2,"do")) //   present tense
		{
			if (strcmp(arg3,"I") && strcmp(arg3,"you")) result = "does"; 
			else result = "do";
		}
		else if (!strcmp(arg2,"have")) 
		{
			if (strcmp(arg3,"I") && strcmp(arg3,"you")) result = "has"; 
			else result = "have";
		}
		else if (!strcmp(arg2,"be")) 
		{
			if (!strcmp(arg3,"I") ) result = "am";
			else if (!strcmp(arg3,"you")) result = "are"; 
			else result = "is";
		}
		else if (!strcmp(arg2,"was") || !strcmp(arg2,"were")) //   past tense
		{
			if (!strcmp(arg3,"I") ) result = "was";
			result = "were";
		}
		else result = arg2;
		strcpy(buffer,result);
		if (IsUpperCase(ARGUMENT(2)[0])) *buffer = toUppercaseData[*buffer];
		return 0;
	}
	else if (!stricmp(arg1,"pronoun"))
	{
		if (!stricmp(arg3,"flip"))
		{
			if (!stricmp(arg2,"my")) strcpy(buffer,"your");
			else if (!stricmp(arg2,"your")) strcpy(buffer,"my");
			else if (!stricmp(arg2,"I")) strcpy(buffer,"you");
			else if (!stricmp(arg2,"you")) strcpy(buffer,"I");
			else strcpy(buffer,arg2);
			return 0;
		}
	}
	else if (!stricmp(arg1,"noun"))
	{
		if (!stricmp(arg3,"proper")) 
		{
			// we know the word, use its known casing for spelling
			WORDP D = FindWord(arg2,0,UPPERCASE_LOOKUP);
			if (D)
			{
				strcpy(buffer,D->word);
				return 0;
			}

			// synthesize appropriate casing
			unsigned int n = BurstWord(arg2);
			for (unsigned int i = 0; i < n; ++i)
			{
				char* word = GetBurstWord(i);
				WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
				if (D && D->properties & LOWERCASE_TITLE); //   allowable particles and connecting words that can be in lower case
				else *word = toUppercaseData[(unsigned char)*word];
				strcat(buffer,word);
				if (i != (n-1)) strcat(buffer," ");
			}
			return 0;
		}
		if (!stricmp(arg3,"lowercaseexist"))	// lower case legal?
		{
			WORDP D = FindWord(arg2,0,LOWERCASE_LOOKUP);
			return (D) ? 0 : FAILRULE_BIT;
		}
		if (!stricmp(arg3,"uppercaseexist"))	// upper case legal?
		{
			WORDP D = FindWord(arg2,0,UPPERCASE_LOOKUP);
			return (D) ? 0 : FAILRULE_BIT;
		}

		char* noun =  GetSingularNoun(arg2,true,false);
		if (!noun) return 0;
		if (!stricmp(arg3,"singular") || (atoi(arg3) == 1 && !strchr(arg3,'.'))) // allow number 1 but not float
		{
			strcpy(buffer,noun);
			return 0;		
		}
		else if (!stricmp(arg3,"plural") || IsDigit(arg3[0]) ) // allow number non-one
		{
			//   swallow the args. for now we KNOW they are wildcard references
			char* plural = GetPluralNoun(StoreWord(noun));
			if (!plural) return 0;
			strcpy(buffer,plural);
			return 0;
		}
		else if (!stricmp(arg3,"irregular") ) // generate a response only if plural is irregular from base (given)
		{
			//   swallow the args. for now we KNOW they are wildcard references
			char* plural = GetPluralNoun(StoreWord(noun));
			if (!plural) return 0;
			size_t len = strlen(noun);
			if (strnicmp(plural,noun,len)) strcpy(buffer,plural); // show plural when base not in it
			return 0;
		}
	}
	else if (!stricmp(arg1,"determiner")) //   DETERMINER noun
	{
		size_t len = strlen(arg2);
		if (arg2[len-1] == 'g' && GetInfinitive(arg2,false)) //   no determiner on gerund
		{
			strcpy(buffer,arg2);
			return 0;
		}
		//   already has one builtinto the word or phrase
		if (!strnicmp(arg2,"a_",2) || !strnicmp(arg2,"an_",3) || !strnicmp(arg2,"the_",4)) 
		{
			strcpy(buffer,arg2);
			return 0;
		}

		WORDP D = FindWord(arg2);
		if (D && D->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))  //no determiner, is mass or proper name
		{
			strcpy(buffer,arg2);
			return 0;
		}

		//   if a plural word, use no determiner
		char* s = GetSingularNoun(arg2,true,false);
		if (!s || stricmp(arg2,s)) //   if has no singular or isnt same, assume we are plural and add the
		{
			sprintf(buffer,"the %s",arg2);
			return 0;
		}

		//   provide the determiner now
		*buffer++ = 'a';
		*buffer = 0;
		if (IsVowel(*arg2)) *buffer++ = 'n'; //   make it "an"
		*buffer++ = ' ';	//   space before the word
		strcpy(buffer,arg2);
		return 0;
	}
	else if (!stricmp(arg1,"place"))
	{
		int value = (int)Convert2Integer(arg2);
		if ((value%10) == 1) sprintf(buffer,"%dst",value); 
		if ((value%10) == 2) sprintf(buffer,"%dnd",value);
		if ((value%10) == 3) sprintf(buffer,"%drd",value);
		else sprintf(buffer,"%dth",value);
		return 0;
	}
	else if (!stricmp(arg1,"capitalize") || !stricmp(arg1,"uppercase"))
	{
		strcpy(buffer,arg2);
		*buffer = toUppercaseData[*buffer];
		return 0;
	}
	else if (!stricmp(arg1,"lowercase"))
	{
		MakeLowerCopy(buffer,arg2);
		return 0;
	}
	else if (!stricmp(arg1,"canonical"))
	{
		WORDP entry,canonical;
		GetPosData(arg2,entry,canonical);
		if (canonical) strcpy(buffer,canonical->word);
		else if (entry) strcpy(buffer,entry->word);
		else strcpy(buffer,arg2);
		return 0;
	}

	else if (!stricmp(arg1,"integer"))
	{
		strcpy(buffer,arg2);
		char* period = strchr(arg2,'.');
		if (period)
		{
			float val = (float) atof(arg2);
			*period = 0;
			int vali = atoi(arg2);
			if ((float) vali == val) strcpy(buffer,arg2);
		}
		return 0;
	}
	return FAILRULE_BIT;
}

static unsigned int RhymeCode(char* buffer) 
{   
	char letter = ARGUMENT(1)[0];
	for (char i = 'a'; i <= 'z'; ++i)
	{
		if (i == letter || (i - 'a' + 'A') == letter) continue;    // don't repeat his word
		ARGUMENT(1)[0] = i;
		if (FindWord(ARGUMENT(1))) 
		{
			strcpy(buffer,ARGUMENT(1));
			break;
		}
	}
	return 0;
}

static unsigned int SubstituteCode(char* buffer) 
{ 
	bool wordMode = toLowercaseData[*ARGUMENT(1)] != 'c'; // is word or character

	// adjust substitution value
	char* substituteValue = ARGUMENT(4);
	size_t substituteLen = strlen(substituteValue);
	if (substituteLen > 1 && *substituteValue == '"' && substituteValue[substituteLen-1] == '"') // quoted expression means use the interior of it
	{
		substituteValue[substituteLen-1] = 0; 
		++substituteValue;
		substituteLen -= 2; 
	}
	Convert2Blanks(substituteValue);

	// what to search in
	char copy[MAX_WORD_SIZE * 4];
	*copy = ' '; // protective leading blank for -1 test
	strcpy(copy+1,ARGUMENT(2));
	char* target = copy+1;
	if (!*target) return FAILRULE_BIT; 

	// find value
	char* find = ARGUMENT(3);
  	if (!*find) return FAILRULE_BIT;
	size_t findLen = strlen(find);
	if (findLen > 1 && *find == '"' && find[findLen-1] == '"') // find of a quoted thing means use interior
	{
		find[findLen-1] = 0; 
		++find;
		findLen -= 2; 
	}

    char* found;
	bool changed = false;
	while ((found = strstr(target,find))) // case sensitive
    {
		// no partial matches
		if (wordMode)
		{
			char c = found[findLen];	
			if (IsAlphaOrDigit(c) || IsAlphaOrDigit(*(found-1))) // skip nonword match
			{
				target = found + findLen;
				continue;
			}
		}
		changed = true;

		// move the before
		size_t offset = found-target;
		strncpy(buffer,target,offset);   //   copy up to but not including pattern.
        buffer += offset;
        target += offset + findLen;

		// copy the replacement
		strcpy(buffer,substituteValue);
		buffer += substituteLen;
		*buffer = 0;
	}
	strcpy(buffer,target); // append the rest

	// check for FAIL request
	char* notify = ARGUMENT(5);
	if (*notify || impliedIf != ALREADY_HANDLED) return (changed) ? 0 : FAILRULE_BIT; // if user wants possible failure result
	return 0;
}

static void SpellOne(WORDP D, uint64 data)
{
	char* match = (char*) data;
	if (FACTSET_COUNT(spellSet) >= 500) return; //   limit

	if (!(D->properties & (NOUN | VERB | ADJECTIVE | ADVERB | DETERMINER | PRONOUN_BITS | CONJUNCTION_BITS | PREPOSITION | AUX_VERB ))) return;
	if (!IsAlpha(*D->word)) return;
	if (D->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_TITLE_OF_ADDRESS)) return;	// want ordinary words
	if (strchr(D->word,'_') ) return; // only normal words and not multi words either
	if (MatchesPattern(D->word,match))
	{
		WORDP E = StoreWord("1");
		AddFact(spellSet,CreateFact(MakeMeaning(E,0),MakeMeaning(FindWord("word")),MakeMeaning(D,0),FACTTRANSIENT));
	}
}

static unsigned int  Spell(char* match, unsigned int set)
{
	char pattern[MAX_WORD_SIZE];
	SET_FACTSET_COUNT(set,0);
	if (match[1] == '-') match[1] = 0;	// change 4-letter to 4
	MakeLowerCopy(pattern,match);
	spellSet = set;
	WalkDictionary(SpellOne,(uint64)pattern);
    return FACTSET_COUNT(set);
}

static unsigned int SpellCode(char* buffer) //- locates up to 100 words in dictionary matching pattern and stores them as facts in @0
{
#ifdef INFORMATION
Fails if no words are found. Words must begin with a letter and be marked as a part of speech
(noun,verb,adjective,adverb,determiner,pronoun,conjunction,prepostion).

Not all words are found in the dictionary. The system only stores singular nouns and base
forms of verbs, adverbs, and adjectives unless it is irregular.

Pattern is a sequence of characters, with * matching 0 or more characters and . matching
exactly one. Pattern must cover the entire string. Pattern may be prefixed with a number, which
indicates how long the word must be. E.g.

^spell("4*")	# retrieves 100 4-letter words
^spell("3a*")  # retrieves 3-letter words beginning with "a"
^spell("*ing") # retrieves words ending in "ing" 
#endif

	return (Spell(ARGUMENT(1),0)) ? 0 : FAILRULE_BIT;
}

static unsigned int SexedCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D || !(D->properties & (NOUN_HE|NOUN_SHE))) strcpy(buffer,ARGUMENT(4)); //   it 
	else if (D->properties & NOUN_HE) strcpy(buffer,ARGUMENT(2)); //   he
	else strcpy(buffer,ARGUMENT(3)); //   she
	return 0;
}


//////////////////////////////////////////////////////////
/// DICTIONARY
//////////////////////////////////////////////////////////

static unsigned int GetPropertyCodes(char* arg1, uint64 &val, uint64 &sysval)
{
	for (unsigned int i = 2; i < 10; ++i)
	{
		char* arg = ARGUMENT(i);
		if (!*arg) break;
		if (!stricmp(arg,"CONCEPT")) 
		{
			if (*arg1 != '~') return FAILRULE_BIT; // must be a concept name
			sysval = CONCEPT | buildID;
		}
	
		// fact marks
		else if (!stricmp(arg,"RECURSIVE")) val = 0;
		else if (IsDigit(arg[0])) ReadInt64(arg,(int64&)sysval);
		else 
		{
			uint64 bits = FindValueByName(arg);
			if (bits) val |= bits;
			else {
				bits = FindValue2ByName(arg);
				if (!bits) 
					Log(STDUSERLOG,"Unknown addproperty value %s\r\n",arg);
				else sysval |= bits;
			}
		}
	}
	return (!sysval && !val) ? FAILRULE_BIT : 0;
}

static unsigned int AddPropertyCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char arg3 = *ARGUMENT(3);
	WORDP D = NULL;
	unsigned int store = 0;
	unsigned int count = 0;
	if (*arg1 == '@') // add property to all facts in set either on a field or fact as a whole
	{
		store = GetSetID(arg1);
		count =  FACTSET_COUNT(store);
		*ARGUMENT(3) = 0;
	}
	else  D = StoreWord(arg1,0); // add property to dictionary word
	bool recursive = false;
	if (!stricmp(ARGUMENT(2),"recursive")) recursive = true;

	uint64 val = 0;
	uint64 sysval = 0;
	unsigned int result = GetPropertyCodes(arg1,val,sysval);
	if (result) return result;
	if (D && *D->word != '~') // add to dictionary entry when not a concept or topic
	{
		AddProperty(D,val);
		AddSystemFlag(D,sysval);
	}
	else if (*arg1 == '@') // add to all properties of fact set
	{
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[store][i];
			if (arg3 == 's') D = Meaning2Word(F->subject); 
			else if (arg3 == 'v') D = Meaning2Word(F->verb);
			else if (arg3 == 'o') D = Meaning2Word(F->object);
			else
			{
				F->flags |= val;
				if (trace & INFER_TRACE) TraceFact(F);
			}
			if (D)
			{
				AddProperty(D,val);
				AddSystemFlag(D,sysval);
				if (trace & INFER_TRACE) Log(STDUSERLOG," %s\n",D->word);
			}
		}
	}
	return 0;
}

static unsigned int DefineCode(char* buffer)
{ 
	char* w = ARGUMENT(1);
	WORDP D = FindWord(w,0);
	if (!D) return 0;

	bool noun = false;
	bool verb = false;
	bool adjective = false;
	bool adverb = false;
	char* which = ARGUMENT(2);
	for (unsigned int i = 1; i <= GetMeaningCount(D); ++i)
	{
		MEANING T = GetMaster( D->meanings[i]);
		WORDP E = Meaning2Word(T);
		char* gloss = GetGloss(E,i);
		if (gloss && T & NOUN && !noun && (!*which || !stricmp(which,"NOUN")))
		{
			if (verb) sprintf(buffer,"As a noun it means %s. ",gloss);
			else sprintf(buffer,"As a noun, %s means %s. ",ARGUMENT(1),gloss);
			buffer += strlen(buffer);
			noun = true;
        }
		else if (gloss && T & VERB && !verb && (!*which || !stricmp(which,"VERB")))
		{
			if (noun) sprintf(buffer,"As a verb it means %s. ",gloss);
			else sprintf(buffer,"As a verb, %s means %s. ",ARGUMENT(1),gloss);
			buffer += strlen(buffer);
			verb = true;
        }
		else if (gloss && T & ADJECTIVE && !noun && !verb && !adjective && (!*which  || !stricmp(which,"ADJECTIVE")))
		{
			sprintf(buffer,"As an adjective, %s means %s. ",ARGUMENT(1),gloss);
			buffer += strlen(buffer);
			adjective = true;
        }
 		else if (gloss && T & ADVERB && !adverb && !noun && !verb && !adjective && (!*which  || !stricmp(which,"ADVERB")))
		{
			sprintf(buffer,"As an adverb, %s means %s. ",ARGUMENT(1),gloss);
			buffer += strlen(buffer);
			adverb = true;
        }
	}
    return 0;
}

static void ArgFlags(uint64& properties, uint64& flags)
{
	properties = flags = 0;
	properties |= FindValueByName(ARGUMENT(2));
	flags |= FindValue2ByName(ARGUMENT(2));
	properties |= FindValueByName(ARGUMENT(3));
	flags |= FindValue2ByName(ARGUMENT(3));
	properties |= FindValueByName(ARGUMENT(4));
	flags |= FindValue2ByName(ARGUMENT(4));
	properties |= FindValueByName(ARGUMENT(5));
	flags |= FindValue2ByName(ARGUMENT(5));
	properties |= FindValueByName(ARGUMENT(6));
	flags |= FindValue2ByName(ARGUMENT(6));
}

static unsigned int HasAnyPropertyCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D) return FAILRULE_BIT;
	uint64 properties;
	uint64 flags;
	ArgFlags(properties,flags);
	if (!flags && !properties) return FAILRULE_BIT;
	if (D->properties & properties) return 0; 
	if (D->systemFlags & flags) return 0; 
	return FAILRULE_BIT;
}

static unsigned int HasAllPropertyCode(char* buffer)
{
	WORDP D = FindWord(ARGUMENT(1));
	if (!D) return FAILRULE_BIT;
	uint64 properties;
	uint64 flags;
	ArgFlags(properties,flags);
	if (!flags && !properties) return FAILRULE_BIT;
	return ((D->properties & properties) == properties && (D->systemFlags & flags) == flags) ? 0 : FAILRULE_BIT; // has all the bits given
}

static unsigned int RemovePropertyCode(char* buffer)
{
	char* arg1 = ARGUMENT(1);
	char arg3 = *ARGUMENT(3); // which to act upon (s, v, o,a) when arg1 is factset
	WORDP D = NULL;
	unsigned int store = 0;
	unsigned int count = 0;
	if (*arg1 == '@') 
	{
		store = GetSetID(arg1);
		count = FACTSET_COUNT(store);
		if (arg3 != 's' && arg3 != 'o' && arg3 != 'v' && arg3 != 'a') return FAILRULE_BIT;
		*ARGUMENT(3) = 0;
	}
	else  D = StoreWord(arg1,0); 

	uint64 val = 0;
	uint64 sysval = 0;
	unsigned int result = GetPropertyCodes(arg1,val,sysval);
	if (result) return result;
	if (D) // remove to dictionary entry
	{
		RemoveProperty(D,val);
		RemoveSystemFlag(D,sysval);
	}
	else // remove to all properties of set
	{
		for (unsigned int i = 1; i <= count; ++i)
		{
			FACT* F = factSet[store][i];
			if (arg3 == 's') D = Meaning2Word(F->subject);
			else if (arg3 == 'v') D = Meaning2Word(F->verb);
			else if (arg3 == 'o') D = Meaning2Word(F->object); 
			else  
			{
				F->flags &= -1 ^ val;
				if (trace & INFER_TRACE) TraceFact(F);
			}
			if (D)
			{
				RemoveProperty(D,val);
				RemoveSystemFlag(D,sysval);
				if (trace & INFER_TRACE) Log(STDUSERLOG," %s\n",D->word);
			}
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////
/// MULTIPURPOSE
//////////////////////////////////////////////////////////

static unsigned int DisableCode(char* buffer) 
{
	char* arg1 = ARGUMENT(1);
	char* arg2 = ARGUMENT(2);
	if (!stricmp(arg1,"topic"))
	{
		if (!*arg2) return FAILRULE_BIT;
		int id = FindTopicIDByName(ARGUMENT(2));
		if (id) 
		{
			if (GetTopicFlags(id) & TOPIC_SYSTEM) return FAILRULE_BIT;
			if (!(GetTopicFlags(id) & TOPIC_BLOCKED)) AddTopicFlag(id,TOPIC_BLOCKED|TOPIC_USED);
			return 0;       
		}
	}
	else if (!stricmp(arg1,"rule")) // 1st one found
	{
		int id = 0;
		unsigned int topic = currentTopicID;
		bool fulllabel;
		bool crosstopic;
		char* rule;
		char* dot = strchr(arg2,'.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg2);
		else rule = GetLabelledRule(topic,arg2,"",fulllabel,crosstopic,id);
		if (!rule) return FAILRULE_BIT;
		SetRuleDisableMark(topic,id);
		return 0;
	}
	else if (!stricmp(arg1,"rejoinder"))
	{
		outputRejoinderRuleID = NO_REJOINDER;
		return 0;
	}
	return FAILRULE_BIT;
}

static unsigned int EnableCode(char* buffer)
{
	char* arg2 = ARGUMENT(2);
	if (!stricmp(ARGUMENT(1),"topic"))
	{
		 //   topic name to enable
		if (!*arg2) return FAILRULE_BIT;
		if (!stricmp(arg2,"all"))
		{
			unsigned int start = 0;
			while (++start <= numberOfTopics) 
			{
				if (GetTopicFlags(start) & TOPIC_SYSTEM) continue;
				RemoveTopicFlag(start,TOPIC_BLOCKED);
			}
			return 0;
		}
		int id = FindTopicIDByName(arg2);
		if (!id) return FAILRULE_BIT;
		if (GetTopicFlags(id) & TOPIC_SYSTEM) return FAILRULE_BIT;
		RemoveTopicFlag(id,TOPIC_BLOCKED);
		return 0;
	}
	else if (!stricmp(ARGUMENT(1),"rule")) 
	{
		int id = 0;
		unsigned int topic = currentTopicID;
		bool fulllabel;
		bool crosstopic;
		char* rule;
		char* dot = strchr(arg2,'.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,arg2);
		else rule = GetLabelledRule(topic,arg2,ARGUMENT(3),fulllabel,crosstopic,id);
		if (!rule) return FAILRULE_BIT;
		UndoErase(rule,topic,id);
		AddTopicFlag(topic,TOPIC_USED); 
		return 0;
	}
	return FAILRULE_BIT;
}

static unsigned int LengthCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (*word == '@') 
	{
		unsigned int store = GetSetID(word);
		unsigned int count = FACTSET_COUNT(store);
		sprintf(buffer,"%d",count);
	}
	else if (*word == '~') // how many top level members in set
	{
		WORDP D = FindWord(word,0);
		if (!D) return FAILRULE_BIT;
		int count = 0;
		FACT* F = GetObjectHead(D);
		while (F)
		{
			if (F->verb == Mmember) ++count;
			F = GetObjectNext(F);
		}
		sprintf(buffer,"%d",count);
	}
	else sprintf(buffer,"%d",(int)strlen(ARGUMENT(1)));
	return 0;
}

static unsigned int NextCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	char* ptr = ReadCompiledWord(ARGUMENT(1),word);
	char* arg1 = ARGUMENT(1); // GAMBIT or RESPONDER or RULE OR FACT or INPUT
	if (!stricmp(word,"FACT")) 
	{
		strcpy(ARGUMENT(1),ptr);
		return FLR(buffer,'n');
	}
	if (!stricmp(word,"INPUT"))
	{
		SAVEOLDCONTEXT()
		*buffer = 0;
		while (ALWAYS) // revise inputs until prepass doesnt change them
		{
			if (!*nextInput) return FAILRULE_BIT;
			PrepareSentence(nextInput,true,true);
			if (!wordCount && (*nextInput | (responseIndex != 0))) // ignore this input
			{
				RESTOREOLDCONTEXT()
				return 0; 
			}
			if (!PrepassSentence(GetUserVariable("$prepass"))) break; // it was quiet
		}
 		if (!wordCount) return FAILRULE_BIT;
		++inputSentenceCount; //  sentence id of volley has moved on
		RESTOREOLDCONTEXT()
	}
	else  // gambit, responder, rule, REJOINDER
	{
		static char prior[MAX_WORD_SIZE];

		if (*ptr == '~' && !ptr[1]) strcpy(ptr,prior);
		bool gambit = (*arg1 == 'G' || *arg1 == 'g');
		bool responder = !stricmp(arg1,"responder");
		bool rejoinder = !stricmp(arg1,"rejoinder");
		unsigned int topic = currentTopicID;
		int id;
		bool fulllabel = false;
		bool crosstopic = false;
		char* rule;
		char* dot = strchr(ptr,'.');
		if (dot && IsDigit(dot[1])) rule = GetRuleTag(topic,id,ptr);
		else rule = GetLabelledRule(topic,ptr,ARGUMENT(3),fulllabel,crosstopic,id);
		if (!rule) return FAILRULE_BIT; // unable to find labelled rule 

		char* data = rule;
		while (data)
		{
			data = FindNextRule( (gambit || responder) ? NEXTTOPLEVEL : NEXTRULE,data,id);
			if (!data || !*data) break;
		
			if (gambit && TopLevelGambit(data)) break;
			else if (responder &&  (TopLevelStatement(data) || TopLevelQuestion(data))) break; 
			else if (rejoinder && Rejoinder(data)) break;
			else if (rejoinder) return FAILRULE_BIT;	// no more rejoinders
			else if (!gambit && !responder && !rejoinder) break;	// any next rule
		}
		if (!data || !*data) return FAILRULE_BIT;
		sprintf(buffer,"%s.%d.%d",GetTopicName(topic),TOPLEVELID(id),REJOINDERID(id));
		strcpy(prior,buffer);	// able to iterate easily
	}
	return 0;
}

static unsigned int FLRCodeR(char* buffer)
{
	char* word = ARGUMENT(1);
	if (*word == '$') word = GetUserVariable(word);
	else if (*word == '_') word =  GetwildcardText(GetWildcardID(word), true);

	if (*word == '@') return FLR(buffer,'r');
	else if (*word == '~')  return RandomMember(buffer,word);
	else return FAILRULE_BIT;
}

static unsigned int ResetCode(char* buffer)
{
	char* word = ARGUMENT(1);
	if (!stricmp(word,"USER"))
	{
		ResetUser(buffer);
#ifndef DISCARDTESTING
		wasCommand = false;	// lie so system will save revised user file
#endif
		return ENDINPUT_BIT;
	}
	else if (!stricmp(word,"TOPIC"))
	{
		word = ARGUMENT(2);
		unsigned int topic;
		if (*word == '*' && word[1] == 0) // all topics
		{
			if (!all) ResetTopics(); 
		}
		else if ((topic = FindTopicIDByName(word))) ResetTopic(topic);
		else return FAILRULE_BIT;
		return 0;
	}
	else if (*word == '@') // reset a fact set for browsing sequentially
	{
		unsigned int store = GetSetID(word);
		factSetNext[store] = 0;
		if (trace) Log(STDUSERLOG," @%d[%d] ",store,FACTSET_COUNT(store));
		return 0;
	}
	return FAILRULE_BIT;
}

//////////////////////////////////////////////////////////
/// EXTERNAL ACCESS
//////////////////////////////////////////////////////////

static unsigned int ExportFactCode(char* buffer)
{
	char* set = ARGUMENT(2);
	if (*set != '@') return FAILRULE_BIT;
	// optional 3rd argument is append or overwrite
	char* append = ARGUMENT(3);
	return (ExportFacts(ARGUMENT(1),GetSetID(set),append)) ? 0 : FAILRULE_BIT;
}

static unsigned int ImportFactCode(char* buffer)
{
	return (ImportFacts(ARGUMENT(1),ARGUMENT(2),ARGUMENT(3),ARGUMENT(4))) ? 0 : FAILRULE_BIT;
}

static unsigned int PopenCode(char* buffer)
{
	char   psBuffer[MAX_WORD_SIZE];
	FILE   *pPipe;
	char arg[MAX_WORD_SIZE];
	char fn[MAX_WORD_SIZE];
	unsigned int result;
	char* function = ReadCommandArg(ARGUMENT(1),arg,result,OUTPUT_NOQUOTES|OUTPUT_EVALCODE|OUTPUT_NOTREALBUFFER); 
	ReadCommandArg(function,fn,result,OUTPUT_NOQUOTES|OUTPUT_EVALCODE|OUTPUT_NOTREALBUFFER); 

	// convert \" to " within params
	char* fix;
	while ((fix = strchr(arg,'\\'))) memmove(fix,fix+1,strlen(fix)); // remove protective backslash
	
	// adjust function reference name
	function = fn;
	if (*function == '\'') ++function; // skip over the ' 

#ifdef WIN32
   if( (pPipe = _popen(arg,"rb")) == NULL ) return FAILRULE_BIT; //  "dir *.c /on /p", "rt" 
#else
   if( (pPipe = popen(arg,"rb")) == NULL ) return FAILRULE_BIT; 
#endif
   psBuffer[0] = '(';
   psBuffer[1] = ' ';
   psBuffer[2] = '"'; 
   while( !feof( pPipe ) )
   {
		psBuffer[3] = 0;
		if( fgets( psBuffer+3, MAX_WORD_SIZE - 4, pPipe ) != NULL )
		 {
			unsigned int result;
			char* p;
			while ((p = strchr(psBuffer,'\r'))) *p = ' ';
			while ((p = strchr(psBuffer,'\n'))) *p = ' ';
			char* actual = TrimSpaces(psBuffer);
			strcat(actual,"\")"); // trailing quote and ending paren
			DoFunction(function,actual,buffer,result); 
			buffer += strlen(buffer);
		}
   }
#ifdef WIN32
   _pclose( pPipe );
#else
   pclose( pPipe );
#endif
   return 0;
}

static unsigned int SystemCode(char* buffer)
{
	char word[MAX_WORD_SIZE];
	*word = 0;
	int j = 0;
	while (*ARGUMENT(++j))
	{
		char* arg = ARGUMENT(j);
		strcat(word,arg);
		strcat(word," ");
	}
	sprintf(buffer,"%d",system(word));
	return  0;
}

//////////////////////////////////////////////////////////
/// FACTS
//////////////////////////////////////////////////////////

static unsigned int CreateFactCode(char* buffer)
{ 
	currentFact = NULL;
	EatFact(ARGUMENT(1));
	return (currentFact) ? 0 : FAILRULE_BIT;
}

static unsigned int DeleteQuerySetCode(char* buffer) //   delete all facts in collection
{
	int store = GetSetID(ARGUMENT(1));
	unsigned int count = FACTSET_COUNT(store);
	for (unsigned int i = 1; i <= count; ++i) KillFact(factSet[store][i]);
	return 0;
}

static unsigned int FieldCode(char* buffer) 
{	
	FACT* F;
	char* word = ARGUMENT(1);
	if (*word == '@') return FAILRULE_BIT;
	F = FactTextIndex2Fact(word); 
	if (!F || F > factFree) return FAILRULE_BIT;

	WORDP s = Meaning2Word(F->subject);
	WORDP v = Meaning2Word(F->verb);
	WORDP o = Meaning2Word(F->object);
	char* arg2 = ARGUMENT(2);
	if (*arg2 == 's' || *arg2 == 'S') 
	{
		if (F->flags & FACTSUBJECT) sprintf(buffer,"%d",F->subject);
		else strcpy(buffer,WriteMeaning(F->subject));
	}
	else if (*arg2 == 'v' || *arg2 == 'V') 
	{
		if (F->flags & FACTVERB) sprintf(buffer,"%d",F->verb);
		else strcpy(buffer,WriteMeaning(F->verb));
	}
	else if (*arg2 == 'o' || *arg2 == 'O') 
	{
		if (F->flags & FACTOBJECT) sprintf(buffer,"%d",F->object);
		else strcpy(buffer,WriteMeaning(F->object));
	}
	else return FAILRULE_BIT;
	return 0;
}

static unsigned int FindCode(char* buffer) // given a set, find the ordered position of the 2nd argument in it 
{   
	char word[MAX_WORD_SIZE];
	strcpy(word,JoinWords(BurstWord(ARGUMENT(2)),false)); //  the value to find
	WORDP D = FindWord(ARGUMENT(1));
	if (D && *D->word == '~')
	{
		int n = -1;
		FACT* F = GetObjectHead(D);  
		while (F ) // walks set MOST recent (right to left)
		{
			if (F->verb == Mmember) 
			{
				++n;
				WORDP item = Meaning2Word(F->subject);
				if (!stricmp(item->word,word))
				{
					sprintf(buffer,"%d",n);
					return 0;
				}
			}
			F = GetObjectNext(F);
		}
	}

	return FAILRULE_BIT; 
}

static FACT* FindF(MEANING subject,WORDP verb,uint64 marker)
{ 
	FACT* F = GetSubjectHead(subject);
    while (F)
    {
		WORDP v = Meaning2Word(F->verb);
        if (v == verb) 
		{
			WORDP obj = Meaning2Word(F->object);
			if (marker != MARKED_WORD) 
			{
				if (F->flags & marker) return F;
			}
			else if (obj->internalBits & marker) return F;
			FACT* G = FindF(F->object,verb,marker);
			if (G) return G;
		}
        F = GetSubjectNext(F);
    }

	return 0;
}

static unsigned int FindMarkedFactCode(char* buffer)
{ 
	WORDP subject = FindWord(ARGUMENT(1));
	if (!subject) return FAILRULE_BIT;
	WORDP verb = FindWord(ARGUMENT(2));
	if (!verb) return FAILRULE_BIT;
	char* mark = ARGUMENT(3);
	int64 marker;
	if (IsDigit(*mark)) ReadInt64(mark,marker);
	else marker = FindValueByName(mark);
	if (!marker) return FAILRULE_BIT;

	FACT* F = FindF(MakeMeaning(subject),verb,marker);
	if (trace & INFER_TRACE) 
	{
		if (F) 
		{
			Log(STDUSERLOG,"FindMarkedFact found ");
			TraceFact(F);
		}
		else Log(STDUSERLOG,"FindMarkedFact not found ");
	}
	if (!F) return FAILRULE_BIT;

	sprintf(buffer,"%d",Fact2Index(F)); // return index
	return 0;
}

static unsigned int FLRCodeF(char* buffer)
{
	return FLR(buffer,'f');
}

static unsigned int IntersectFactsCode(char* buffer) 
{      
	char* word = ARGUMENT(1);
	char from[MAX_WORD_SIZE];
	char to[MAX_WORD_SIZE];
	unsigned int result;
	word = ReadShortCommandArg(word,from,result,OUTPUT_KEEPQUERYSET|OUTPUT_NOTREALBUFFER);
	word = ReadShortCommandArg(word,to,result,OUTPUT_KEEPQUERYSET|OUTPUT_NOTREALBUFFER);
	unsigned int store = (impliedSet == ALREADY_HANDLED) ? 0 : impliedSet;
    SET_FACTSET_COUNT(store,0);

    WORDP D;
    FACT* F;
    unsigned int usedMark = NextinferMark();
    unsigned int i;
	char toKind = 's';

	if (*to != '@') // mark word
	{
		D = FindWord(to);
		if (D) D->inferMark = usedMark;
	}
	else //  mark set
	{
		unsigned toset = GetSetID(to);
		toKind = toLowercaseData[*GetSetType(to)]; // s v o null
		unsigned int limit = FACTSET_COUNT(toset);
		for (i = 1; i <= limit; ++i)
		{
			WORDP D;
			F = factSet[toset][i];
			if (trace & INFER_TRACE)   TraceFact(F);
			if (toKind == 's') Meaning2Word(F->subject)->inferMark = usedMark;
 			else if (toKind == 'v') Meaning2Word(F->verb)->inferMark = usedMark;
 			else if (toKind == 'o') Meaning2Word(F->object)->inferMark = usedMark;
			else // mark all pieces
			{
				D = Meaning2Word(F->subject);
				D->inferMark = usedMark;
				D = Meaning2Word(F->verb);
				D->inferMark = usedMark;
				D = Meaning2Word(F->object);
				D->inferMark = usedMark;
				F->flags |= MARKED_FACT;
			}
		}
	}

    // look for matches
	unsigned int where = GetSetID(from); 
	char fromKind = toLowercaseData[*GetSetType(from)]; // s v o null
    unsigned int limit = FACTSET_COUNT(where);
  	if (trace & INFER_TRACE) Log(STDUSERLOG," // ");
	for (i = 1; i <= limit; ++i)
    {
        F = factSet[where][i];
 		if (trace & INFER_TRACE)   TraceFact(F);
 		if (fromKind == 's' && Meaning2Word(F->subject)->inferMark == usedMark) AddFact(store,F);
 		else if (fromKind == 'v' && Meaning2Word(F->verb)->inferMark == usedMark) AddFact(store,F);
		else if (fromKind == 'o' && Meaning2Word(F->object)->inferMark == usedMark) AddFact(store,F);
		else 
		{
			// entire fact found
			if (toKind != 's' && toKind != 'v' && toKind != 'o' &&  F->flags & MARKED_FACT) AddFact(store,F);
			// some piece found
			else if (Meaning2Word(F->subject)->inferMark == usedMark || Meaning2Word(F->verb)->inferMark == usedMark || Meaning2Word(F->object)->inferMark == usedMark) AddFact(store,F);
		}
    }
 	unsigned int count = FACTSET_COUNT(store);
	if (trace & INFER_TRACE)
	{
		Log(STDUSERLOG,"Found %d in IntersectFact\r\n",count);
		for (i = 1; i <= count; ++i) TraceFact(factSet[store][i]);
	}
	if (impliedSet == ALREADY_HANDLED && !count) return FAILRULE_BIT;
	impliedSet = ALREADY_HANDLED;
    return 0;
}


static unsigned int FLRCodeL(char* buffer)
{
	return FLR(buffer,'l');
}

static unsigned int QueryCode(char* buffer)
{ //   kind, s, v, o, count,  from, to, propogate, mark 
	unsigned int iarg1 = 0;
	char* ptr = ARGUMENT(1);
	int argcount = 0;
	while (ptr && *ptr) // break apart arguments, but leave any quoted arg UNEVALED.
	{
		argcount++;
		char word[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr,word);
		if (*word != '\'')
		{
			unsigned int result = 0;
			ReadShortCommandArg(word,ARGUMENT(argcount),result);
			if (result) return 0;
		}
		else strcpy(ARGUMENT(argcount),word);
	}

	for (unsigned int i = argcount+1; i <= 9; ++i) strcpy(ARGUMENT(i),""); // default rest of args to ""
	if (IsDigit(ARGUMENT(5)[0])) ReadInt(ARGUMENT(5),iarg1); // defaults to ? if not given
	if (iarg1 == 0) iarg1 = (unsigned int) -1; // infinite

	if (argcount < 9) while (++argcount <= 9) strcpy(ARGUMENT(argcount),"?"); //   default rest of calling Arguments
	char set[50];
	char* to = ARGUMENT(7);
	if (impliedSet != ALREADY_HANDLED) 
	{
		sprintf(set,"@%d",impliedSet); 
		to = set;
	}
	unsigned int result = Query(ARGUMENT(1), ARGUMENT(2), ARGUMENT(3), ARGUMENT(4), iarg1, ARGUMENT(6), to,ARGUMENT(8), ARGUMENT(9));
	// result was a count. now convert to a fail code
	if (impliedSet != ALREADY_HANDLED) result = 0;
	else result = (result != 0) ? 0 : FAILRULE_BIT; 
	impliedSet = ALREADY_HANDLED;
	return result;
}

static unsigned int SortCode(char* buffer)
{
	SortFacts(ARGUMENT(1));
	return 0;
}

static unsigned int SaveCode(char* buffer)
{
	if (ARGUMENT(1)[0] != '@') return FAILRULE_BIT;
	uint64 set = GetSetID(ARGUMENT(1));
	if (*ARGUMENT(2) == '0' || !stricmp(ARGUMENT(2),"false")) setControl &= -1 ^ (1 << set);
	else setControl |= (uint64) (1 << set);
	return 0;
}

static unsigned int UnduplicateCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) return FAILRULE_BIT;

	int from = GetSetID(ARGUMENT(1));
	if (impliedSet == from) return FAILRULE_BIT; // cant do in-place
	unsigned int count = FACTSET_COUNT(from);
	SET_FACTSET_COUNT(impliedSet,0);

	// copy unmarked facts to to
	unsigned int i;
	for (i = 1; i <= count; ++i) 
	{
		FACT* F = factSet[from][i];
		if (!(F->flags & MARKED_FACT))
		{
			AddFact(impliedSet,F);
			F->flags |= MARKED_FACT;
		}
	}

	// erase marks
	count = FACTSET_COUNT(impliedSet);
	for (i = 1; i <= count; ++i) factSet[impliedSet][i]->flags ^= MARKED_FACT; 

	if (trace & INFER_TRACE) Log(STDUSERLOG,"Unduplicated %d entries\r\n",count);
	impliedSet = ALREADY_HANDLED;
	return 0;
}

static unsigned int UnpackFactRefCode(char* buffer)
{
	if (impliedSet == ALREADY_HANDLED) return FAILRULE_BIT;
	char* arg1 = ARGUMENT(1);
	int from = GetSetID(arg1);
	int count = FACTSET_COUNT(from);
	char* type = GetSetType(arg1);
	SET_FACTSET_COUNT(impliedSet,0);
	for (int i = 1; i <= count; ++i)
	{
		FACT* F = factSet[from][i];
		if (F->flags & FACTSUBJECT && *type != 'v' && *type != 'o') 
		{
			FACT* G = Index2Fact(F->subject);
			AddFact(impliedSet,G);
		}
		if (F->flags & FACTVERB && *type != 's' && *type != 'o') 
		{
			FACT* G = Index2Fact(F->verb);
			AddFact(impliedSet,G);
		}
		if (F->flags & FACTOBJECT && *type != 's' && *type != 'v') 
		{
			FACT* G = Index2Fact(F->object);
			AddFact(impliedSet,G);
		}
	}
	impliedSet = ALREADY_HANDLED;
	return 0;
}

SystemFunctionInfo systemFunctionSet[] =
{
	{"",0,0,0,""},

	{"\r\n---- Topic",0,0,0,""},
	{"^addtopic",AddTopicCode,1,SAMELINE,"note a topic as interesting"}, //O
	{"^available",AvailableCode,VARIABLE_ARGS,0,"is rule still available or has it been disabled"}, 
	{"^cleartopics",ClearTopicsCode,0,SAMELINE,"remove all interesting topics in queue"},
	{"^counttopic",CountTopicCode,2,SAMELINE,"provide topic and count requested: GAMBIT, AVAILABLE, RULE, USED"}, 
	{"^gambit",GambitCode,VARIABLE_ARGS,0,"execute topic in gambit mode, naming ~ ~topicname PENDING or keyword"}, 
	{"^getrule",GetRuleCode,VARIABLE_ARGS,0,"get the requested data (TAG,TYPE,LABEL,PATTERN,OUTPUT,TOPIC,USABLE) for rule tag or label"},
	{"^topicflags",TopicFlagsCode,1,SAMELINE,"Get topic control bits"}, 
	{"^lastused",LastUsedCode,2,SAMELINE,"Get input count of last topic access - GAMBIT, RESPONDER, REJOINDER, ANY"}, 
	{"^hasgambit",HasGambitCode,VARIABLE_ARGS,0,"name of topic to test for an unexpired gambit, LAST/ANY/"}, 
	{"^keep",KeepCode,0,SAMELINE,"do not erase rule after use"}, 
	{"^poptopic",PopTopicCode,VARIABLE_ARGS,0,"remove current or named topic from interesting set"}, 
	{"^refine",RefineCode,VARIABLE_ARGS,0,"execute continuations until one matches"}, 
	{"^rejoinder",RejoinderCode,0,0,"try to match a pending rejoinder - not legal in postprocessing"}, 
	{"^respond",RespondCode,VARIABLE_ARGS,0,"execute a topic's responders"}, 
	{"^reuse",ReuseCode,VARIABLE_ARGS,0,"jump to a rule label or tag and execute output section"}, 
	{"^setrejoinder",SetRejoinderCode,1,0,"Set rejoinder mark to this tag"}, 

	{"\r\n---- Topic Lists",0,0,0,""},
	{"^gambittopics",GetTopicsWithGambitsCode,0,0,"get all topics that have usable gambits that are not current topic"}, 
	{"^keywordtopics",KeywordTopicsCode,VARIABLE_ARGS,0,"get facts of topics that cover keywords mentioned in input"}, 
	{"^pendingtopics",PendingTopicsCode,1,0,"return list of currently pending topics as facts in 1st arg"}, 
	{"^querytopics",QueryTopicsCode,1,0,"get topics of which 1st arg is a keyword?"}, 

	{"\r\n---- Marking & Parser Info",0,0,0,""},
	{"^getparse",GetParseCode,STREAM_ARG,SAMELINE,"get parse chunk or data"},
	{"^mark",MarkCode,STREAM_ARG,SAMELINE,"mark word/concept in sentence"},
	{"^marked",MarkedCode,1,SAMELINE,"BOOLEAN - is word/concept marked in sentence"}, 
	{"^position",PositionCode,STREAM_ARG,SAMELINE,"get FIRST or LAST position of an _ var"}, 
	{"^setposition",SetPositionCode,1,SAMELINE,"set absolute match position"}, 
	{"^unmark",UnmarkCode,STREAM_ARG,SAMELINE,"remove a mark on a word/concept in the sentence"}, 

	{"\r\n---- Input",0,0,0,""},
	{"^capitalized",CapitalizedCode,1,SAMELINE,"given index of word in sentence return 1 or 0 for whether user capitalized it"}, 
	{"^input",InputCode,STREAM_ARG,0,"submit stream as input immediately after current input"},
	{"^removetokenflags",RemoveTokenFlagsCode,1,SAMELINE,"remove value from tokenflags"}, 
	{"^settokenflags",SetTokenFlagsCode,1,SAMELINE,"add value to tokenflags"}, 
	
	{"\r\n---- Numbers",0,0,0,""},
	{"^compute",ComputeCode,3,SAMELINE,"perform a numerical computation"}, 
	{"^timefromseconds",TimeFromSecondsCode,1,SAMELINE,"given time/date in seconds, return the timeinfo string corresponding to it"}, 

	{"\r\n---- Debugging",0,0,0,""},
	{"^debug",DebugCode,0,SAMELINE,"only useful for debug code breakpoint"}, 
	{"^log",LogCode,STREAM_ARG,0,"add to logfile"}, 

	{"\r\n---- Output Generation - not legal in post processing",0,0,0,""},
	{"^flushoutput",FlushOutputCode,0,SAMELINE,"force existing output out"}, 
	{"^insertprint",InsertPrintCode,STREAM_ARG,0,"add output before named responseIndex"},
	{"^keephistory",KeepHistoryCode,2,SAMELINE,"trim history of USER or BOT to number of entries given"}, 
	{"^print",PrintCode,STREAM_ARG,0,"isolated output message from current stream"}, 
	{"^preprint",PrePrintCode,STREAM_ARG,0,"add output before existing output"}, 
	{"^repeat",RepeatCode,0,SAMELINE,"set repeat flag so can repeat output"}, 
	{"^reviseoutput",ReviseOutputCode,2,0,"takes index and output, replacing output at that index"}, 

	{"\r\n---- Output Access",0,0,0,""},
	{"^response",ResponseCode,1,0,"raw text for this response, including punctuation"},
	{"^responsequestion",ResponseQuestionCode,1,SAMELINE,"BOOLEAN - 1 if response ends in ?  0 otherwise"}, 
	{"^responseruleid",ResponseRuleIDCode,1,SAMELINE,"rule tag generating this response"},
	
	{"\r\n---- Postprocessing functions - only available in postprocessing",0,0,0,""},
	{"^analyze",AnalyzeCode,STREAM_ARG,0,"Take an output stream and do preparation on it like it was user input"}, 
	{"^postprocessprint",PostProcessPrintCode,STREAM_ARG,0,"add to front of output stream"}, 

	{"\r\n---- Control Flow",0,0,0,""},
	{"^command",CommandCode,STREAM_ARG,0,"execute a : command"}, 
	{"^end",EndCode,1,SAMELINE,"cease current processing thru this level"}, 
	{"^eval",EvalCode,STREAM_ARG,0,"evaluate stream"}, 
	{"^fail",FailCode,1,SAMELINE,"return a return code of some kind"}, 
	{"^match",MatchCode,STREAM_ARG,0,"Perform given pattern match"},
	{"^nofail",NoFailCode,STREAM_ARG,0,"execute script but ignore all failures thru some level"}, 
	{"^notnull",NotNullCode,STREAM_ARG,0,"tests that output of stream argument is not null, fails otherwise"}, 
	{"^retry",RetryCode,VARIABLE_ARGS,SAMELINE,"reexecute a rule with a later match or retry  input"},

	{"\r\n---- Word Manipulation",0,0,0,""},
	{"^burst",BurstCode,VARIABLE_ARGS,0,"break a string into component words"}, 
	{"^explode",ExplodeCode,1,0,"break a word into component letters"}, 
	{"^flags",FlagsCode,1,0,"get flag values of word"}, 
	{"^uppercase",UppercaseCode,1,0,"boolean return 1 if word was entered uppercase, 0 if not"}, 
	{"^properties",PropertiesCode,1,0,"get property values of word"}, 
	{"^intersectwords",IntersectWordsCode,VARIABLE_ARGS,0,"see if words in arg 1 are in arg2"},
	{"^join",JoinCode,STREAM_ARG,OWNTRACE,"merge words into one"}, 
	{"^pos",POSCode,VARIABLE_ARGS,0,"compute some part of speech value"},
	{"^rhyme",RhymeCode,1,0,"find a rhyming word"}, 
	{"^substitute",SubstituteCode,VARIABLE_ARGS,0,"alter a string by substitution"}, 
	{"^spell",SpellCode,1,0,"find words matching pattern and store as facts"}, 
	{"^sexed",SexedCode,4,0,"pick a word based on sex of given word"}, 
		
	{"\r\n---- Dictionary",0,0,0,""},
	{"^addproperty",AddPropertyCode,VARIABLE_ARGS,0,"Add value to dictionary entry properies or systemFlags or facts of factset properties"}, 
	{"^define",DefineCode,VARIABLE_ARGS,0,"get dictionary gloss of  word"}, 
	{"^hasanyproperty",HasAnyPropertyCode,VARIABLE_ARGS,0,"argument 1 has any of property or systemflags of argument2 .. argumentn"}, 
    {"^hasallproperty",HasAllPropertyCode,VARIABLE_ARGS,0,"argument 1 has all of the properties or systemflags of argument2 .. argumentn"}, 
	{"^removeproperty",RemovePropertyCode,VARIABLE_ARGS,0,"remove value to dictionary entry properies or systemFlags or facts of factset properties"},
	
	{"\r\n---- MultiPurpose Functions",0,0,0,""},
	{"^disable",DisableCode,VARIABLE_ARGS,SAMELINE,"stop a rule or topic or rejoinder"}, 
	{"^enable",EnableCode,VARIABLE_ARGS,SAMELINE,"allow a rule or topic"}, 
	{"^length",LengthCode,1,SAMELINE,"counts characters in a word or members of a fact set or top level concept members"}, 
	{"^next",NextCode,STREAM_ARG,0,"FACT- walk a factset without erasing it  GAMBIT,RESPONDER,RULE,REJOINDER with tag or label for next one  INPUT to go to next sentence"}, 
	{"^pick",FLRCodeR,STREAM_ARG,0,"randomly select and remove an element from a fact set or randomly select from a concept"}, 
	{"^reset",ResetCode,VARIABLE_ARGS,0,"reset a topic or all topics or user back to initial state "}, 

	{"\r\n---- Functions on facts",0,0,0,""},
	{"^createfact",CreateFactCode,STREAM_ARG,OWNTRACE,"create a triple"}, 
	{"^delete",DeleteQuerySetCode,1,0,""}, 
	{"^field",FieldCode,2,0,"get a field of a fact"}, 
	{"^find",FindCode,2,0,"Given set, find ordinal position of item within it"},
	{"^findmarkedfact",FindMarkedFactCode,3,0,"given a subject,a verb, and a mark, return a marked fact that can be found propogating from subject using verb  or null"},
	{"^first",FLRCodeF,STREAM_ARG,0,"get first element of a set and remove it"},
	{"^intersectfacts",IntersectFactsCode,STREAM_ARG,0,"find facts common to two sets, based on fields"},
	{"^last",FLRCodeL,STREAM_ARG,0,"get last element of a set and remove it"},
	{"^query",QueryCode,STREAM_ARG,0,"hunt for fact in fact database"},
	{"^sort",SortCode,STREAM_ARG,0,""},
	{"^save",SaveCode,2,0,"mark fact set to be saved into user data"},
	{"^unduplicate",UnduplicateCode,1,0,"remove duplicate facts"},
	{"^unpackfactref",UnpackFactRefCode,1,0,"copy out fields which are facts"}, 

	{"\r\n---- External Access",0,0,0,""},
	{"^export",ExportFactCode,VARIABLE_ARGS,SAMELINE,"write fact set to a file"},
	{"^import",ImportFactCode,4,SAMELINE,"read fact set from a file"}, 
	{"^system",SystemCode,STREAM_ARG,SAMELINE,"send command to the operating system"},
	{"^popen",PopenCode,STREAM_ARG,SAMELINE,"send command to the operating system and read reply strings"},

	{0,0,0,0,""}	
};
