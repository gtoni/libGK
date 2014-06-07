#!/usr/bin/env python

import os;
import shutil;

os.system("ndk-build");
shutil.copy("obj/local/armeabi/libGK.a", "../../bin/android/libGK.a");
os.system("mkdir " + os.getenv('NDK_ROOT') + "\\sources\\third_party\\LibGK");
os.system("xcopy module\\* " + os.getenv('NDK_ROOT') + "\\sources\\third_party\\LibGK /e /D");