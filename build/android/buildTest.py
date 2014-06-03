#!/usr/bin/env python

import os;

os.system("ndk-build")
os.system("ant debug");
os.system("adb install -r bin\TestLibGK-debug.apk");