import os,sys,ast

if __name__ == "__main__":

    InputLst = './query-sparksql-6107327.csv'
    OutputLst = './unique.csv'

    seenUID = set()

    f_w = open(OutputLst, 'w')
    for idx, line in enumerate(open(InputLst)):
        if idx == 0 : continue
        ID, _ = line.strip().split(",", maxsplit=1)
        if ID in seenUID: continue
        f_w.write(line)
        seenUID.add(ID)
    f_w.close()

    print("done")
