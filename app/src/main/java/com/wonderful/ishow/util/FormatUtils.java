package com.wonderful.ishow.util;

import java.util.Calendar;
import java.util.Locale;

public class FormatUtils {
	
	private FormatUtils() {
		throw new AssertionError("No FormatUtils instances for you!");
	}
	
	/**
	 * format current time to string.
	 * @return formatted time string.
	 */
	public static String formatTime() {
		return formatTime(0, true);
	}
	
	/**
	 * format specified time to string.
	 * @param timeMs time in microseconds.
	 * @return formatted time string.
	 */
	public static String formatTime(long timeMs) {
		return formatTime(timeMs, false);
	}
	
	/*
	 * https://stackoverflow.com/questions/2654025/how-to-get-year-month-day-hours-minutes-seconds-and-milliseconds-of-the-cur
	 */
	private static String formatTime(long timeMs, boolean current) {
		Calendar calender = Calendar.getInstance();
		if(!current)
			calender.setTimeInMillis(timeMs);
		
		int year   = calender.get(Calendar.YEAR);
		int month  = calender.get(Calendar.MONTH) + 1;  // month is zero-based.
		int day    = calender.get(Calendar.DAY_OF_MONTH);
		
		int hour   = calender.get(Calendar.HOUR_OF_DAY);
		int minute = calender.get(Calendar.MINUTE);
		int second = calender.get(Calendar.SECOND);
//		int millis = calender.get(Calendar.MILLISECOND);
		return String.format(Locale.US, "%4d%02d%02d_%02d%02d%02d", year, month, day, hour, minute, second);
	}
}
