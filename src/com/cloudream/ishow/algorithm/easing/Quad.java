package com.cloudream.ishow.algorithm.easing;

public class Quad implements Easing {

	@Override
	public float easeOut( float t, float b, float c, float d ) {
		return -c * ( t /= d ) * ( t - 2 ) + b;
	}

	@Override
	public float easeIn( float t, float b, float c, float d ) {
		return c * ( t /= d ) * t + b;
	}

	@Override
	public float easeInOut( float t, float b, float c, float d ) {
		if ( ( t /= d / 2 ) < 1 ) return c / 2 * t * t + b;
		return -c / 2 * ( ( --t ) * ( t - 2 ) - 1 ) + b;
	}

}
