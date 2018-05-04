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

import trace_analysis

errorfiles=list()

if (len(sys.argv)) != 2:
    print("Usage: python logextract.py <logfiledir>")
    exit(1)

def output(extractlist, fileout):
    header = ['DAG', 'Cluster', 'Policy', 'DC ratio', 'Nodes', 'Makespan', 'Max parallel', 'Mean wait time', 'Total execute time', 'SLR', 'Speedup', 'Efficiency', 'dev used', 'unbalance issues']
    extractlist.insert(0, header)
    with open (fileout, "wb") as f:
        writer = csv.writer(f)
        writer.writerows(extractlist)

def fileprocess(filein):
    entry = ['']*14
    devused = 0
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
                if "SLR" in line:
                    entry[9] = line.strip().split(': ')[-1]
                if "Speedup" in line:
                    entry[10] = line.strip().split(': ')[-1]
                if "Efficiency" in line:
                    entry[11] = line.strip().split(': ')[-1]
                if "occupied time" in line:
                    occutime = line.strip().split('occupied time:')[1].split(' ')[0]
                    if float(occutime) > 0:
                        devused += 1
                        entry[12] = devused
            elif "Simulation Report" in line:
                startprocess = True
    if entry[0] == '':
        print("file error:", filein)
        errorfiles.append(filein)
    entry[13] = trace_analysis.loadbalanceprocess(filein)
    return entry

def main():
    filedir = sys.argv[1]
    fileout = filedir + '.csv'
    errorfilelists = filedir + '.err'

    if not os.path.exists(filedir):
        print("Error: the input dir", filedir, "doesn't exist!")
        exit(0)
    if os.path.isfile(filedir):
        dirs = [filedir]
    else:
        dirs = os.listdir(filedir)

    print("Process dir: ", filedir)
    print("Output file: ", fileout)
    print("Error file list: ", errorfilelists)
    if os.path.isfile(fileout):
        print("Output file already exists, remove it...")
        os.remove(fileout)

    extract = list()
    for fileinput in dirs:
        fileinput = os.path.join(filedir, fileinput)
        print(fileinput)
        extract.append(fileprocess(fileinput))

    output(extract, fileout)

    print("[Error files]:")
    with open(errorfilelists, "wb") as errf:
        for item in errorfiles:
            print (item)
            errf.write(' ' + item + '\n')

if __name__ == "__main__":
    main()
