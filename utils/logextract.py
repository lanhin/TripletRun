#/usr/bin/env python

'''
@2018-02  by lanhin

Extract the important information from triplet logs.
Usage: python logextract.py <logfiledir>
The output file 
'''

from __future__ import print_function
import sys
import csv
import os

if (len(sys.argv)) != 2:
    print("Usage: python logextract.py <logfiledir>")
    exit(1)

def output(extractlist, fileout):
    header = ['DAG', 'Cluster', 'Policy', 'DC ratio', 'Nodes', 'Makespan', 'Max parallel', 'Mean wait time', 'Total execute time']
    extractlist.insert(0, header)
    with open (fileout, "wb") as f:
        writer = csv.writer(f)
        writer.writerows(extractlist)

def fileprocess(filein):
    entry = ['']*9
    with open(filein, "rb") as source:
        startprocess = False
        for line in source:
            if startprocess:
                if "Graph file" in line:
                    entry[0] = line.strip().split(': ')[-1]
                if "Cluster:" in line:
                    entry[1] = line.strip().split(': ')[-1]
                if "Policy" in line:
                    entry[2] = line.strip().split(': ')[-1]
                if "DC Ratio" in line:
                    entry[3] = line.strip().split(': ')[-1]
                if "Total nodes" in line:
                    entry[4] = line.strip().split(': ')[-1]
                if "Global timer" in line:
                    entry[5] = line.strip().split(': ')[-1]
                if "Max parallelism" in line:
                    entry[6] = line.strip().split(': ')[-1]
                if "Mean wait time" in line:
                    entry[7] = line.strip().split(': ')[-1]
                if "Total execution" in line:
                    entry[8] = line.strip().split(': ')[-1]
            elif "Simulation Report" in line:
                startprocess = True
    return entry

def main():
    filedir = sys.argv[1]
    fileout = filedir + '.csv'

    if not os.path.exists(filedir):
        print("Error: the input dir", filedir, "doesn't exist!")
        exit(0)
    if os.path.isfile(filedir):
        dirs = [filedir]
    else:
        dirs = os.listdir(filedir)

    print("Process dir: ", filedir)
    print("Output file: ", fileout)
    if os.path.isfile(fileout):
        print("Output file already exists, remove it...")
        os.remove(fileout)

    extract = list()
    for fileinput in dirs:
        fileinput = os.path.join(filedir, fileinput)
        print(fileinput)
        extract.append(fileprocess(fileinput))

    output(extract, fileout)

if __name__ == "__main__":
    main()
