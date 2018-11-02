#!/bin/env python
#-*- encoding=utf8 -*-

import os
import sys
kernel_fname = sys.argv[1]
dep_fname = sys.argv[2]

os.system("readelf -s " + kernel_fname + " >> kernel.symtab")
fsym = open("kernel.symtab", 'r')
lines = fsym.readlines()
symtab = {}
for i in range(3, len(lines)):
    line_splited = lines[i].split()
    if (len(line_splited) == 8):
        symtab[line_splited[7]] = line_splited[1]
fsym.close()

out_define = {}
out_variable = {}
fin = open(dep_fname + ".dep", 'r')
fout = open(dep_fname + ".asm", 'w')
lines = fin.readlines()
for i in lines:
    line_splited = i.split()
    if (line_splited[0] == "OBJECT"):
        val = symtab.get(line_splited[1], "UND")
        if (val == "UND"):
            print "OBJECT " + line_splited[1] + " is not exist."
        else:
            out_define[line_splited[1]] = val
    elif (line_splited[0] == "SYM"):
        out_variable[line_splited[1]] = line_splited[2]
if ("DEP_VERSION" not in out_variable):
	print "Missing Symbel DEP_VERSION."
	exit(0)
if (int(out_variable["DEP_VERSION"]) != int(symtab["DEP_VERSION"])):
	print "DEP_VERSION differ in kernel and cdep.dep"

fin.close()

fout.write("[SECTION .core_dep]\n")
for i in out_define:
    fout.write(i + "\tequ\t0x" + out_define[i] + " \n")
    fout.write("global " + i + " \n")
for i in out_variable:
    fout.write(i + " dd 0x" + out_variable[i] + " \n")
    fout.write("global " + i + "\n")

fout.close()

os.remove("kernel.symtab")
