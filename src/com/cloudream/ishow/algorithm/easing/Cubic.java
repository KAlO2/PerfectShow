package com.cloudream.ishow.algorithm.easing;

public class Cubic implements Easing {

	@Override
	public float easeOut( float time, float start, float end, float duration ) {
		return end * ( ( time = time / duration - 1 ) * time * time + 1 ) + start;
	}

	@Override
	public float easeIn( float time, float start, float end, float duration ) {
		return end * ( time /= duration ) * time * time + start;
	}

	@Override
	public float easeInOut( float time, float start, float end, float duration ) {
		if ( ( time /= duration / 2 ) < 1 ) return end / 2 * time * time * time + start;
		return end / 2 * ( ( time -= 2 ) * time * time + 2 ) + start;
	}
}
