package com.cloudream.ishow.algorithm.easing;

public class Back implements Easing {

	@Override
	public float easeOut( float time, float start, float end, float duration ) {
		return easeOut( time, start, end, duration, 0 );
	}

	@Override
	public float easeIn( float time, float start, float end, float duration ) {
		return easeIn( time, start, end, duration, 0 );
	}

	@Override
	public float easeInOut( float time, float start, float end, float duration ) {
		return easeInOut( time, start, end, duration, 0.9f );
	}

	public float easeIn( float t, float b, float c, float d, float s ) {
		if ( s == 0 ) s = 1.70158f;
		return c * ( t /= d ) * t * ( ( s + 1 ) * t - s ) + b;
	}

	public float easeOut( float t, float b, float c, float d, float s ) {
		if ( s == 0 ) s = 1.70158f;
		return c * ( ( t = t / d - 1 ) * t * ( ( s + 1 ) * t + s ) + 1 ) + b;
	}

	public float easeInOut( float t, float b, float c, float d, float s ) {
		if ( s == 0 ) s = 1.70158f;
		if ( ( t /= d / 2 ) < 1 ) return c / 2 * ( t * t * ( ( ( s *= ( 1.525 ) ) + 1 ) * t - s ) ) + b;
		return c / 2 * ( ( t -= 2 ) * t * ( ( ( s *= ( 1.525 ) ) + 1 ) * t + s ) + 2 ) + b;
	}
}
