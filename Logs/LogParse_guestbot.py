

f = open("log-.txt", "r")
lines = f.readlines()
f.close()
f = open("playtest01_output.txt", "w")

linenumber = 0

for texte in lines:
    linenumber+=1
    u0 = texte.find(")")+1
    u1 = texte.find("=")-1
    b0 = texte.find(">")+1
    b1 = texte.find("When")-1
    f.write(str(linenumber)+":Player:"+texte[u0:u1]+"\r\n")
    f.write("Jarvis:"+texte[b0:b1]+"\r\n")

f.close()

