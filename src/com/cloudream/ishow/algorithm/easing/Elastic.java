package com.cloudream.ishow.algorithm.easing;

public class Elastic implements Easing {

	@Override
	public float easeIn( float time, float start, float end, float duration ) {
		return easeIn( time, start, end, duration, start + end, duration );
	}

	public float easeIn( float t, float b, float c, float d, float a, float p ) {
		float s;
		if ( t == 0 ) return b;
		if ( ( t /= d ) == 1 ) return b + c;
		if ( !( p > 0 ) ) p = d * 0.3f;
		if ( !( a > 0 ) || a < Math.abs( c ) ) {
			a = c;
			s = p / 4;
		} else
			s = p / (float)(2 * Math.PI * Math.asin(c/a));
		
		t -= 1;
		return -(float)( a * Math.pow( 2, 10 * t ) * Math.sin( ( t * d - s ) * ( 2 * Math.PI ) / p ) ) + b;
	}

	@Override
	public float easeOut( float time, float start, float end, float duration ) {
		return easeOut( time, start, end, duration, start + end, duration );
	}

	public float easeOut( float t, float b, float c, float d, float a, float p ) {
		float s;
		if ( t == 0 ) return b;
		if ( ( t /= d ) == 1 ) return b + c;
		if ( !( p > 0 ) ) p = d * 0.3f;
		if ( !( a > 0 ) || a < Math.abs( c ) ) {
			a = c;
			s = p / 4;
		} else
			s = p / (float)( 2 * Math.PI * Math.asin( c / a));
		return (float)( a * Math.pow( 2, -10 * t ) * Math.sin( ( t * d - s ) * ( 2 * Math.PI ) / p ) + c + b );
	}

	@Override
	public float easeInOut( float t, float b, float c, float d ) {
		return easeInOut( t, b, c, d, b + c, d );
	}

	public float easeInOut( float t, float b, float c, float d, float a, float p ) {
		float s;

		if ( t == 0 ) return b;
		if ( ( t /= d / 2 ) == 2 ) return b + c;
		if ( !( p > 0 ) ) p = d * ( 0.3f * 1.5f );
		if ( !( a > 0 ) || a < Math.abs( c ) ) {
			a = c;
			s = p / 4;
		} else
			s = p / (float)( 2 * Math.PI * Math.asin( c / a));
		if ( t < 1 ) return -0.5f * (float)( a * Math.pow( 2, 10 * ( t -= 1 ) ) * Math.sin( ( t * d - s ) * ( 2 * Math.PI ) / p ) ) + b;
		return (float)(a * Math.pow( 2, -10 * ( t -= 1 ) ) * Math.sin( ( t * d - s ) * ( 2 * Math.PI ) / p )) * 0.5f + c + b;
	}
}
