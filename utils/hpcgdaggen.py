#/usr/bin/env python

'''
@2018-01  by lanhin

Generate DAG of HPCG's kernel function (in json format).
Usage: python2 hpcgDaggen.py
The output file name is hpcg.json
'''

n = 8
x = 8
y = 8

def jsonout(fileout, nodes, edges):
    with open(fileout, "wb") as result:
        outItem = "{\n    \"nodes\":\n    [\n"
        result.write(outItem)

        # Write nodes
        for i in range(len(nodes)-1):
            outItem = "\t{\"id\":\""+(str)(nodes[i][0])+"\",\n\t\"comDmd\":\""+(str)(nodes[i][1])+"\",\n\t\"dataDmd\":\""+(str)(nodes[i][2])+"\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"id\":\""+(str)(nodes[-1][0])+"\",\n\t\"comDmd\":\""+(str)(nodes[-1][1])+"\",\n\t\"dataDmd\":\""+(str)(nodes[-1][2])+"\"\n\t}\n"
        result.write(outItem)

        outItem = "    ],\n    \"edges\":\n    [\n"
        result.write(outItem)

        for i in range(len(edges)-1):
            outItem = "\t{\"src\":\""+(str)(edges[i][0])+"\",\n\t\"dst\":\""+(str)(edges[i][1])+"\",\n\t\"weight\":\""+(str)(edges[i][2])+"\"\n\t},\n"
            result.write(outItem)

        outItem = "\t{\"src\":\""+(str)(edges[-1][0])+"\",\n\t\"dst\":\""+(str)(edges[-1][1])+"\",\n\t\"weight\":\""+(str)(edges[-1][2])+"\"\n\t}\n"
        result.write(outItem)

        outItem = "    ]\n}\n"
        result.write(outItem)

nodes = list()
edges = list()
fileout = "hpcg.json"

#nodes
for i in range(x * y):
    if i == 0 or i == x-1 or i == x*y-x or i == x*y-1: # degree is 3
        print i, 3
        degree = 3
        compCost = (6*degree + 4) * n
        memCost = 409*n + 8*(degree + 1)
        nodes.append((i, compCost, memCost))
    elif i<x or i%x == 0 or i%x == (x-1) or i>x*y-x-1: # degree is 5
        print i,5
        degree = 5
        compCost = (6*degree + 4) * n
        memCost = 409*n + 8*(degree + 1)
        nodes.append((i, compCost, memCost))
    else: # degree is 8
        print i, 8
        degree = 8
        compCost = (6*degree + 4) * n
        memCost = 409*n + 8*(degree + 1)
        nodes.append((i, compCost, memCost))

#edges
weight = 8*n
for i in range(x * y):
    # i->i+1
    if (i+1)%x != 0:
        print i, i+1
        edges.append((i, i+1, weight))
    # i->i+x-1
    if i%x != 0 and i+x-1 < x*y:
        print i, i+x-1
        edges.append((i, i+x-1, weight))
    # i->i+x
    if i+x < x*y:
        print i, i+x
        edges.append((i, i+x, weight))
    # i->i+x+1
    if (i+1)%x != 0 and i+x+1 < x*y:
        print i, i+x+1
        edges.append((i, i+x+1, weight))
             
jsonout(fileout, nodes, edges)
