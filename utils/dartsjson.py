#/usr/bin/env python

'''
@2018-05  by lanhin

Transform DAG file of DARTS json format.
Usage: python2 dot2json.py <dotfile>
The output file name is <dotfile>.json
'''

import sys
import os
import re

if (len(sys.argv)) != 2:
    print "Usage: python2 dot2json.py <dotfile>"
    exit(1)

filein = sys.argv[1]
fileout = filein + '.json'

print "Input:",filein
print "Output:",fileout
if os.path.isfile(fileout):
    os.remove(fileout)

def jsonout(fileout, nodes, edges):
    with open(fileout, "wb") as result:
        outItem = "{\n    \"nodes\":\n    [\n"
        result.write(outItem)

        # Write nodes
        for i in range(len(nodes)-1):
            outItem = "\t{\"id\":\""+nodes[i][0]+"\",\n\t\"comDmd\":"+nodes[i][1]+",\n\t\"c\":"+nodes[i][2]+",\n\t\"g\":"+nodes[i][3]+"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"id\":\""+nodes[-1][0]+"\",\n\t\"comDmd\":"+nodes[-1][1]+",\n\t\"c\":"+nodes[-1][2]+",\n\t\"g\":"+nodes[-1][3]+"\n\t}\n"
        result.write(outItem)

        outItem = "    ],\n    \"edges\":\n    [\n"
        result.write(outItem)

        # Write edges
        for i in range(len(edges)-1):
            outItem = "\t{\"src\":\""+edges[i][0]+"\",\n\t\"dst\":\""+edges[i][1]+"\",\n\t\"weight\":\""+str(edges[i][2])+"\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"src\":\""+edges[-1][0]+"\",\n\t\"dst\":\""+edges[-1][1]+"\",\n\t\"weight\":\""+str(edges[-1][2])+"\"\n\t}\n"
        result.write(outItem)

        outItem = "    ]\n}\n"
        result.write(outItem)

nodes = list() #empty list
edges = list() #empty list
nodesdict = {} #empty dictionary

# Map node ID
tmpid = 1
with open(filein, "rb") as source:
    for line in source:
        if "{" in line: # graph declare starts
            continue
        if "}" in line: # graph declare ends
            continue
        if line.startswith("//"): # comment lines
            continue

        if "->" in line: # an edge
            continue
        else: # a node
            ndId = line.strip().split(' ')[0]
            if not nodesdict.has_key(ndId):
                nodesdict[ndId] = tmpid
                tmpid += 1
print (nodesdict)


with open(filein, "rb") as source:
    for line in source:
        if "{" in line: # graph declare starts
            continue
        if "}" in line: # graph declare ends
            continue
        if line.startswith("//"): # comment lines
            continue

        if "->" in line: # an edge
            splited =  line.strip().split(' ')
            src = splited[0]
            dst = splited[2]
            #print re.search('\"\d+\"', line).group()
            if re.search('\"\d+\"', line):
                weight = re.search('\"\d+\"', line).group()
                weight = weight[1:-1]
                weight = float(weight) / 1024 #KB
            else:
                weight = -1
            #print re.search('->', line).group()

            if not nodesdict.has_key(src):
                nodesdict[src] = tmpid
                tmpid += 1

            edges.append((str(nodesdict[src]), str(nodesdict[dst]), weight))
        else: # a node
            ndId = line.strip().split(' ')[0]
            if re.search('com=\"\d+\"', line):
                compCost = re.search('com=\"\d+\"', line).group()
                compCost = compCost.split('=')[-1]
                #print ndId, compCost
            else:
                continue
            if re.search('c=\"\d+\"', line):
                consume = re.search('c=\"\d+\"', line).group()
                consume = consume.split('=')[-1]
                #print ndId, compCost
            else:
                consume = "-1"
            if re.search('g=\"\d+\"', line):
                generate = re.search('g=\"\d+\"', line).group()
                generate = compCost.split('=')[-1]
                #print ndId, compCost
            else:
                generate = "-1"


            nodes.append((str(nodesdict[ndId]), compCost, consume, generate))
jsonout(fileout, nodes, edges)
