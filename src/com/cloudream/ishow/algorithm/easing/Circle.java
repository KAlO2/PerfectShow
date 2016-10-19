package com.cloudream.ishow.algorithm.easing;

public class Circle implements Easing {

	@Override
	public float easeOut( float time, float start, float end, float duration )
	{
		time = time / duration - 1.0f;
		return end * (float)Math.sqrt( 1.0f - time * time ) + start;
	}

	@Override
	public float easeIn( float time, float start, float end, float duration )
	{
		time /= duration;
		return -end * ( (float)Math.sqrt( 1.0f - time * time ) - 1.0f ) + start;
	}

	@Override
	public float easeInOut( float time, float start, float end, float duration )
	{
		time /= duration / 2;
		if ( time < 1 )
			return -end / 2.0f * (float)( Math.sqrt( 1.0f - time * time ) - 1.0f ) + start;
		
		time -= 2.0;
		return end / 2.0f * ((float)Math.sqrt( 1.0f - time * time ) + 1.0f ) + start;
	}

}
