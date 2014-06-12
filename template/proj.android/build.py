#!/usr/bin/env python

import os;
import shutil;

def listAllSrc(dir):
	srcList = ' '
	for item in os.listdir(dir):
		path = dir + '/' + item
		# Android can not package the file that ends with ".gz"
		if os.path.isfile(path) and (item.endswith('.c') or item.endswith('.cpp') or item.endswith('.cxx')):
			srcList += path + ' '
		if os.path.isdir(path):
			srcList += listAllSrc(path) + ' '
	return srcList

def buildAndroidMk(projName):
	androidMk = open("jni/Android.mk", "w")
	androidMk.write("LOCAL_PATH := $(call my-dir)/../\n")
	androidMk.write("include $(CLEAR_VARS)\n")
	androidMk.write("LOCAL_MODULE    := " + projName + "\n")
	androidMk.write("LOCAL_SRC_FILES :=" + listAllSrc("../src")+" AppDirHack.c\n")
	androidMk.write("LOCAL_C_INCLUDES := ../src\n")
	androidMk.write("LOCAL_STATIC_LIBRARIES := GK\n")
	androidMk.write("include $(BUILD_SHARED_LIBRARY)\n")
	androidMk.write("$(call import-module,third_party/LibGK)\n")
	androidMk.close()

def buildApplicationMk():
	appMk = open("jni/Application.mk", "w")
	appMk.write("APP_PLATFORM := android-9\n")
	appMk.write("APP_STL := gnustl_static\n")
	appMk.close()
	
def buildAndroidManifest(package, projName):
	lines = [
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n",
	"<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\" package=\"" + package + "." + projName + "\" android:versionCode=\"1\" android:versionName=\"1.0\">\n",	
    "<uses-sdk android:minSdkVersion=\"9\" />\n",
	"<application android:label=\"" + projName + "\" android:hasCode=\"true\">\n",
	"<activity android:name=\"" + package + ".NativeDummy\" android:label=\"" + projName + "\" android:configChanges=\"orientation|keyboardHidden\">\n",
	"<meta-data android:name=\"android.app.lib_name\" android:value=\"" + projName + "\" />\n",
	"<intent-filter><action android:name=\"android.intent.action.MAIN\" /><category android:name=\"android.intent.category.LAUNCHER\" /></intent-filter>\n",
	"</activity></application></manifest>"
	]
	mf = open("AndroidManifest.xml", "w");
	mf.writelines(lines);
	mf.close();

# This function generates a java class which extends NativeActivity in order to load shared libraries needed by application
def buildJavaDummy(package, projName):
	lines = [
	"package "+package+";\n",
	"public class NativeDummy extends com.libgk.NativeBase{",
	"static{",
		"System.loadLibrary(\"MPG123\");",
		"System.loadLibrary(\"" + projName + "\");",
	"}}"
	]
	os.system("xcopy /e /D /Y stub src");
	dummy = open("src/NativeDummy.java", "w")
	dummy.writelines(lines)
	dummy.close()
	
def getProjectName():
	os.chdir(os.pardir);
	projName = os.path.basename(os.getcwd())
	os.chdir("proj.android")
	return projName

	
packName = "com.libgk"
pName = getProjectName()

os.system("mkdir jni");
os.system("mkdir src");
buildAndroidMk(pName);
buildApplicationMk();
buildJavaDummy(packName, pName);
buildAndroidManifest(packName, pName);
adhFile = open("AppDirHack.c","w")
adhFile.write("char* gkAndroidAppDir = \"/data/data/" + packName+ "." + pName +"/\";");
adhFile.close();
os.system("copyData.bat");
os.system("android update project -p . -s --target android-10 -n " + pName)
if os.system("ndk-build") == 0:
	os.system("ant debug");
	os.system("xcopy  /e /D /Y bin\\" + pName + "-debug.apk ..\\build\\android\\");
