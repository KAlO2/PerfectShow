package com.cloudream.ishow.algorithm.easing;

public class Expo implements Easing {

	@Override
	public float easeOut( float time, float start, float end, float duration ) {
		return ( time == duration ) ? start + end : end * ( - (float)Math.pow( 2.0, -10.0 * time / duration ) + 1 ) + start;
	}

	@Override
	public float easeIn( float time, float start, float end, float duration ) {
		return ( time == 0 ) ? start : end * (float)Math.pow( 2.0, 10.0 * ( time / duration - 1.0 ) ) + start;
	}

	@Override
	public float easeInOut( float time, float start, float end, float duration ) {
		if ( time == 0 ) return start;
		if ( time == duration ) return start + end;
		if ( ( time /= duration / 2.0 ) < 1.0 ) return end/2 * (float)Math.pow( 2.0, 10.0 * ( time - 1.0 ) ) + start;
		return end/2 * ( -(float)Math.pow( 2.0, -10.0 * --time ) + 2.0f ) + start;
	}

}
