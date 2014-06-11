package com.libgk;
import android.os.Bundle;
import android.content.Context;
import android.media.AudioManager;
public class NativeBase extends android.app.NativeActivity{
	static{
		System.loadLibrary("MPG123");
		System.loadLibrary("SoundPlayer");
	}
	private static AudioManager audioMgr;
	
	@Override
    protected void onCreate(Bundle savedInstanceState) {
		this.setVolumeControlStream(AudioManager.STREAM_MUSIC);
		audioMgr = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
		super.onCreate(savedInstanceState);
	}
	
	public static void setVolume(float vol)
	{
		int v = (int)((float)audioMgr.getStreamMaxVolume(AudioManager.STREAM_MUSIC) * vol);
		audioMgr.setStreamVolume(AudioManager.STREAM_MUSIC, v, 0);
	}
	public static float getVolume()
	{
		float v = (float)audioMgr.getStreamVolume(AudioManager.STREAM_MUSIC);
		float vMax = (float)audioMgr.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
		return v/vMax;
	}	
}