#!/usr/bin/env python

import os;
import shutil;

def getProjectName():
	os.chdir(os.pardir);
	projName = os.path.basename(os.getcwd())
	os.chdir("proj.android")
	return projName

pName = getProjectName();
os.system("adb install -r ..\\build\\android\\" + pName + "-debug.apk");