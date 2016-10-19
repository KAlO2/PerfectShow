package com.cloudream.ishow.algorithm.easing;

public interface Easing
{
	public float easeOut(float time, float start, float end, float duration);

	public float easeIn(float time, float start, float end, float duration);

	public float easeInOut(float time, float start, float end, float duration);
}
