package com.cloudream.ishow.algorithm.easing;

public class Bounce implements Easing {

	@Override
	public float easeOut( float t, float b, float c, float d ) {
		if ( ( t /= d ) < ( 1.0f / 2.75f ) ) {
			return c * ( 7.5625f * t * t ) + b;
		} else if ( t < ( 2.0f / 2.75f ) ) {
			return c * ( 7.5625f * ( t -= ( 1.5f / 2.75f ) ) * t + 0.75f ) + b;
		} else if ( t < ( 2.5f / 2.75f ) ) {
			return c * ( 7.5625f * ( t -= ( 2.25f / 2.75f ) ) * t + 0.9375f ) + b;
		} else {
			return c * ( 7.5625f * ( t -= ( 2.625f / 2.75f ) ) * t + 0.984375f ) + b;
		}
	}

	@Override
	public float easeIn( float t, float b, float c, float d ) {
		return c - easeOut( d - t, 0, c, d ) + b;
	}

	@Override
	public float easeInOut( float t, float b, float c, float d ) {
		if ( t < d / 2 )
			return easeIn( t * 2, 0, c, d ) * 0.5f + b;
		else
			return easeOut( t * 2 - d, 0, c, d ) * 0.5f + c * 0.5f + b;
	}
}
