package com.cloudream.ishow.algorithm.easing;

public class Linear implements Easing {

	public float easeNone( float time, float start, float end, float duration ) {
		return end * time / duration + start;
	}

	@Override
	public float easeOut( float time, float start, float end, float duration ) {
		return end * time / duration + start;
	}

	@Override
	public float easeIn( float time, float start, float end, float duration ) {
		return end * time / duration + start;
	}

	@Override
	public float easeInOut( float time, float start, float end, float duration ) {
		return end * time / duration + start;
	}

}
