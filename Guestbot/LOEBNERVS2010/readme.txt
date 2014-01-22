WARNING- this builds a version that uses std windows DLL's and worked for me in 2010, BUT...
some machine MIGHT be lacking a critical DLL. I would prefer to build it with static linking so
the EXE would be completely stand-alone, but when I tried, it failed to find some DLL needed for
boring windows things. If someone can figure it out (I have no time to waste on it), then I'll
make it static link in the future.



To run the judge program to interact with your loebner chatbot, you must have perl installed.

Assuming you do, you can type into a dos command window: perl judge.pl
This brings up a 4-way window and you get a prompt "which program?"
Select P1.

This then gives you a browse for folder for "left dirctory".  We will have the computer
on the left side, so select JUDGEDATA.
This then gives you a browse for folder for "right directory". We will not have a human
on the right side, so select JUNK.

Now, run your chatbot until it says initialization complete.

Now type into the LEFT ME window what you want the judge to say.  Responses from the
chatbot should appear in LEFT OTHER. As long as you are typing data into the LEFT ME,
the chatbot will wait patiently. If you press the ENTER key or stop typing for a bit,
what you have typed will then be shipped to the chatbot and it will respond.