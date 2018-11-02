#!/bin/env python
#-*- encoding=utf8 -*-

import os
import shutil
import sys

def InstallMakefile(dir_path, makefile):
	print "Entering " + dir_path

	no_makefile_path = os.path.join(dir_path, ".no_makefile")
	self_makefile_path = os.path.join(dir_path, ".self_makefile")

	if os.path.exists(no_makefile_path):
		print "no_makefile exist in " + dir_path
		return
	if os.path.exists(self_makefile_path):
		print ".self_makefile exist in " + dir_path
		return

	if os.path.exists(os.path.join(dir_path, "Makefile")):
            os.remove(os.path.join(dir_path, "Makefile"))
	shutil.copy(makefile, os.path.join(dir_path, "Makefile"))
	print "Copying makefile"
	lst = os.listdir(dir_path)
	for i in lst:
		tmp_path = os.path.join(dir_path, i)
		if os.path.isdir(tmp_path) and i[0] != '.':
			InstallMakefile(tmp_path, makefile)

def InstallMakefileForAllSubDir(dir_path, makefile):
	for i in os.listdir(dir_path):
		tmp_path = os.path.join(dir_path, i)
		if os.path.isdir(tmp_path) and i[0] != '.':
			InstallMakefile(tmp_path, makefile)

def show_help():
	print "reinstmf: install makefile to all sub directory"
	print "usage: "
 	print "	reinstmf: install makefile($(PWD)/makefile.sub) to its sub directory"
	print "	reinstmf dir: install dir/Makefile.sub to its sub directory"
	print "	reinstmf dir mf: install mf to its sub directory"

if __name__ == "__main__":
	argc = len(sys.argv)
	if argc == 1:
		InstallMakefileForAllSubDir(os.getcwd(), os.path.join(os.getcwd(), "Makefile.sub"))
	elif argc == 2:
		if sys.argv[1] == "--help":
			show_help()
		else:
			InstallMakefileForAllSubDir(sys.argv[1], os.path.join(sys.argv[1], "Makefile.sub"))
	elif argc == 3:
		InstallMakefileForAllSubDir(sys.argv[1], sys.argv[2])
	else:
		show_help()
        
