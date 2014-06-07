#!/usr/bin/env python

import os;
import shutil;

def getProjectName():
	os.chdir(os.pardir);
	projName = os.path.basename(os.getcwd())
	os.chdir("proj.android")
	return projName

os.system("ant clean");
os.system("ndk-build clean");
os.system("removeData.bat");
os.system("del ..\\build\\android\\" + getProjectName() + "-debug.apk");