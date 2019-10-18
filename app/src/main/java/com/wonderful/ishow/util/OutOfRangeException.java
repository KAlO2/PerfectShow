package com.wonderful.ishow.util;

/**
 * Java has #IndexOutOfBoundsException, but I want a out of range exception. So I simulate C++'s 
 * <a href="http://en.cppreference.com/w/cpp/error/out_of_range">std::out_of_range</a> exception.
 * Thrown when a program attempts to use a value which is outside of the valid range.
 */
public class OutOfRangeException extends RuntimeException {
	private static final long serialVersionUID = -6171968548088501987L;

	/**
	 * Constructs a new {@code OutOfRangeException} that includes the current stack trace.
	 */
	public OutOfRangeException() {
	}

	/**
	 * Constructs a new {@code OutOfRangeException} with the current stack trace and the
	 * specified detail message.
	 *
	 * @param detailMessage the detail message for this exception.
	 */
	public OutOfRangeException(String detailMessage) {
		super(detailMessage);
	}
}
