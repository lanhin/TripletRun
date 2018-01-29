#/usr/bin/env python

'''
@2018-01  by lanhin

Generate DAG of HPCG's kernel function (in json format).
Usage: python2 hpcgDaggen.py
The output file name is hpcg.json
'''

import getopt
import sys


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

def daggen(n, x, y):
    nodes = list()
    edges = list()
    fileout = "hpcg"+"_"+str(n)+"x"+str(x)+"x"+str(y)+".json"

    #nodes
    for i in range(x * y):
        if i == 0 or i == x-1 or i == x*y-x or i == x*y-1: # degree is 3
            #print i, 3
            degree = 3
            compCost = (6*degree + 4) * n #flops
            memCost = float(409*n + 8*(degree + 1)) / 1024 #KB
            nodes.append((i, compCost, memCost))
        elif i<x or i%x == 0 or i%x == (x-1) or i>x*y-x-1: # degree is 5
            #print i,5
            degree = 5
            compCost = (6*degree + 4) * n
            memCost = float(409*n + 8*(degree + 1)) / 1024
            nodes.append((i, compCost, memCost))
        else: # degree is 8
            #print i, 8
            degree = 8
            compCost = (6*degree + 4) * n
            memCost = float(409*n + 8*(degree + 1)) / 1024
            nodes.append((i, compCost, memCost))

    #edges
    weight = float(8*n) / 1024 # KB
    for i in range(x * y):
        # i->i+1
        if (i+1)%x != 0:
            #print i, i+1
            edges.append((i, i+1, weight))
        # i->i+x-1
        if i%x != 0 and i+x-1 < x*y:
            #print i, i+x-1
            edges.append((i, i+x-1, weight))
        # i->i+x
        if i+x < x*y:
            #print i, i+x
            edges.append((i, i+x, weight))
        # i->i+x+1
        if (i+1)%x != 0 and i+x+1 < x*y:
            #print i, i+x+1
            edges.append((i, i+x+1, weight))

    jsonout(fileout, nodes, edges)

def usage():
    print "Usage:\n\tpython", __file__, "-h/--help -n n -x x -y y"
    print "\t-h,--help:  Usage"
    print "\t-n:  Set n"
    print "\t-x:  Set x"
    print "\t-y:  Set y"

def main():
    n = 16
    x = 16
    y = 16

    try:
        options,args = getopt.getopt(sys.argv[1:],"hn:x:y:", ["help"])
    except getopt.GetoptError:
        sys.exit()

    for name,value in options:
        if name in ("-h","--help"):
            usage()
        if name in ("-n"):
            n = int(value)
        if name in ("-x"):
            x = int(value)
        if name in ("-y"):
            y = int(value)

    daggen(n, x, y)

if __name__ == "__main__":
    main()
