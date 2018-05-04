#/usr/bin/env python

'''
@2018-05  by lanhin

Analyse a trace file and calculate the number of load imbalance occurs.
The function loadbalanceprocess is actually used in logextract.py.
Now the two files should be put under the same folder.
'''

import sys
import os
import re

if (len(sys.argv)) != 2:
    print "Usage: python2 dot2json.py <dotfile>"
    exit(1)

def loadbalanceprocess(filein):
    tmpfile = filein+".tmp"
    runninglist = list()
    dev_num = 0
    with open(tmpfile, "wb") as tmp:
        with open(filein, "rb") as source:
            for line in source:
                if "Total devices" in line: # number of devices
                    dev_num = int(line.strip().split(' ')[-1])
                    #outputline = "Total devices "+str(dev_num)+"\n"
                    #tmp.write(outputline)
                if "Node" in line: # a node issue
                    current_time = float(line.strip().split(',')[0].split(' ')[-1])
                    Nodeid = line.strip().split(' ')[1]
                    Devid = line.strip().split(' ')[3]
                    FinishTime = float(line.strip().split(' ')[-1])

                    # 1.output_before_time(current_time)
                    for itemidx in range(len(runninglist)-1, -1, -1):
                        item = runninglist[itemidx]
                        if item[2] <= current_time:
                            outputline = "Time "+str(item[2])+" node "+item[0]+" finished, on "+item[1]+"\n"
                            tmp.write(outputline)
                            del runninglist[itemidx]

                    # 2.output_time(t)
                    tmp.write(line)

                    # 3.add_time(t)
                    runninglist.append((Nodeid, Devid, FinishTime))
                    runninglist.sort(key = lambda l: (l[2]))
                    runninglist.reverse()
                    #print(runninglist)

                if "Simulation Report" in line: # an edge
                    #output_all
                    for itemidx in range(len(runninglist)-1, -1, -1):
                        item = runninglist[itemidx]
                        outputline = "Time "+str(item[2])+" node "+item[0]+" finished, on "+item[1]+"\n"
                        tmp.write(outputline)

    devlist = [0]*dev_num
    unbalancenum = 0
    with open(tmpfile, "rb") as source:
        for line in source:
            if "Node" in line: # A new node issue
                Nodeid = line.strip().split(' ')[1]
                Devid = int(line.strip().split(' ')[3])
                if devlist[Devid] > min(devlist):
                    unbalancenum += 1
                devlist[Devid] += 1
                #print (devlist)
            if "Time" in line: # A node finished execution
                Devid = int(line.strip().split(' ')[-1])
                assert devlist[Devid] >= 1
                devlist[Devid] -= 1
    if os.path.isfile(tmpfile):
        os.remove(tmpfile)
    return unbalancenum

def main():
    filein = sys.argv[1]
    print (loadbalanceprocess(filein))

if __name__ == "__main__":
    main()
