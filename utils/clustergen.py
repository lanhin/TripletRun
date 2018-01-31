#/usr/bin/env python

'''
@2018-01  by lanhin

Generate cluster json file.
Usage: python2 clustergen.py
The output file name is cluster.json
'''

import getopt
import sys
import os

def jsonout(fileout, devices, edges, nodelinks):
    with open(fileout, "wb") as result:
        outItem = "{\n    \"devices\":\n    [\n"
        result.write(outItem)

        # Write devices
        for i in range(len(devices)-1):
            outItem = "\t{\"id\":\""+(str)(devices[i][0])+"\",\n\t\"compute\":\""+(str)(devices[i][1])+"\",\n\t\"RAM\":\""+(str)(devices[i][2])+"\",\n\t\"bw\":\""+(str)(devices[i][3])+"\",\n\t\"loc\":\""+str(devices[i][4])+"\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"id\":\""+(str)(devices[-1][0])+"\",\n\t\"compute\":\""+(str)(devices[-1][1])+"\",\n\t\"RAM\":\""+(str)(devices[-1][2])+"\",\n\t\"bw\":\""+(str)(devices[-1][3])+"\",\n\t\"loc\":\""+str(devices[-1][4])+"\"\n\t}\n"
        result.write(outItem)

        outItem = "    ],\n    \"links\":\n    [\n"
        result.write(outItem)

        # Write links
        for i in range(len(edges)-1):
            outItem = "\t{\"src\":\""+(str)(edges[i][0])+"\",\n\t\"dst\":\""+(str)(edges[i][1])+"\",\n\t\"bw\":\""+(str)(edges[i][2])+"\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"src\":\""+(str)(edges[-1][0])+"\",\n\t\"dst\":\""+(str)(edges[-1][1])+"\",\n\t\"bw\":\""+(str)(edges[-1][2])+"\"\n\t}"
        result.write(outItem)

        if len(nodelinks) > 0:
            outItem = ",\n"
        else:
            outItem = "\n"
        result.write(outItem)

        for i in range(len(nodelinks)-1):
            outItem = "\t{\"src\":\""+(str)(nodelinks[i][0])+"\",\n\t\"dst\":\""+(str)(nodelinks[i][1])+"\",\n\t\"bw\":\""+(str)(nodelinks[i][2])+"\",\n\t\"BetweenNode\":\"true\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"src\":\""+(str)(nodelinks[-1][0])+"\",\n\t\"dst\":\""+(str)(nodelinks[-1][1])+"\",\n\t\"bw\":\""+(str)(nodelinks[-1][2])+"\",\n\t\"BetweenNode\":\"true\"\n\t}\n"
        result.write(outItem)

        outItem = "    ]\n}\n"
        result.write(outItem)

# compute, RAM, bw, network
devs = [("1000000", "1048576", "1111490", "1600000"),
        ("10000000", "1048576", "1342177", "3200000"),
        ("100000000", "2097152", "1342177", "3200000")]

# id, output network bandwidth in KB/s
comnodes = [(0,1000000), (1,20000000), (2,1000000), (3,1000000),
            (4,1000000), (5,1000000), (6,1000000), (7,1000000)]

# id, compute, RAM, bw, location, network bandwidth
devices = list()
edges = list()
nodelinks = list()

if (len(sys.argv)) != 2:
    print "Usage: python2 dot2json.py <dotfile>"
    exit(1)

filein = sys.argv[1]
fileout = filein + '.json'

print "Input:",filein
print "Output:",fileout
if os.path.isfile(fileout):
    os.remove(fileout)

with open(filein, "rb") as source:
    for line in source: # id, index, location
        splited = line.strip().split(' ')
        devices.append((int(splited[0]), devs[int(splited[1])][0], devs[int(splited[1])][1], devs[int(splited[1])][2], splited[2], devs[int(splited[1])][3]))
        for i in range(len(devices)):
            if int(devices[i][4]) == int(splited[-1]) and devices[i][0] != int(splited[0]): # in the same node
                bw = min(float(devices[i][-1]), float(devs[int(splited[1])][-1]))
                edges.append((splited[0], devices[i][0], str(bw)))
    for i in range(len(comnodes)):
        for j in range(i+1, len(comnodes)):
            if j >= len(comnodes):
                continue
            nodelinks.append((comnodes[i][0], comnodes[j][0], min(float(comnodes[i][1]), float(comnodes[j][1]))))

#print devices
#print edges
print nodelinks

jsonout(fileout, devices, edges, nodelinks)
