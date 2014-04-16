/*******************************************************************************
 * JNotify - Allow java applications to register to File system events.
 * 
 * Copyright (C) 2005 - Content Objects
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 ******************************************************************************
 * 
 * You may also redistribute and/or modify this library under the terms of the
 * Eclipse Public License. See epl.html.
 * 
 ******************************************************************************
 *
 * Content Objects, Inc., hereby disclaims all copyright interest in the
 * library `JNotify' (a Java library for file system events).
 * 
 * Yahali Sherman, 21 November 2005
 *    Content Objects, VP R&D.
 * 
 ******************************************************************************
 * Author : Omry Yadan
 ******************************************************************************/
 
package net.contentobjects.jnotify.linux;

import net.contentobjects.jnotify.JNotifyException;

import static net.contentobjects.jnotify.Util.CHARSET_UTF;

public class JNotify_linux
{
	static final boolean DEBUG = false;
	public static boolean WARN = true;
	
	static
	{
		int res = nativeInit();
		if (res != 0)
		{
			throw new RuntimeException("Error initializing fshook_inotify library. linux error code #" + res  + ", man errno for more info");
		}
		init();
	}
	
	/* the following are legal, implemented events that user-space can watch for */
	public final static int IN_ACCESS = 0x00000001; /* File was accessed */
	public final static int IN_MODIFY = 0x00000002; /* File was modified */
	public final static int IN_ATTRIB = 0x00000004; /* Metadata changed */
	public final static int IN_CLOSE_WRITE = 0x00000008; /* Writtable file was closed */
	public final static int IN_CLOSE_NOWRITE = 0x00000010; /* Unwrittable file closed */
	public final static int IN_OPEN = 0x00000020; /* File was opened */
	public final static int IN_MOVED_FROM = 0x00000040; /* File was moved from X */
	public final static int IN_MOVED_TO = 0x00000080; /* File was moved to Y */
	public final static int IN_CREATE = 0x00000100; /* Subfile was created */
	public final static int IN_DELETE = 0x00000200; /* Subfile was deleted */
	public final static int IN_DELETE_SELF = 0x00000400; /* Self was deleted */
	public final static int IN_MOVE_SELF = 0x00000800; /* Self was moved */

	/* the following are legal events. they are sent as needed to any watch */
	public final static int IN_UNMOUNT = 0x00002000; /* Backing fs was unmounted */
	public final static int IN_Q_OVERFLOW = 0x00004000; /* Event queued overflowed */
	public final static int IN_IGNORED = 0x00008000; /* File was ignored */

	/* helper events */
	public final static int IN_CLOSE = (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE); /* close */
	public final static int IN_MOVE = (IN_MOVED_FROM | IN_MOVED_TO); /* moves */

	/* special flags */
	public final static int IN_ISDIR = 0x40000000; /*
													 * event occurred against
													 * dir
													 */
	public final static int IN_ONESHOT = 0x80000000; /* only send event once */

	/*
	 * All of the events - we build the list by hand so that we can add flags in
	 * the future and not break backward compatibility. Apps will get only the
	 * events that they originally wanted. Be sure to add new events here!
	 */
	public final static int IN_ALL_EVENT = (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE
			| IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE | IN_CREATE | IN_DELETE_SELF);
	
	
	private static INotifyListener _notifyListener;

	private static native int nativeInit();
	
	private static native int nativeAddWatch(byte[] path, int mask);

	private static native int nativeRemoveWatch(int wd);
	
	private native static int nativeNotifyLoop();
	
	private static native String getErrorDesc(long errorCode);
	
	
	public static int addWatch(String path, int mask) throws JNotifyException
	{
		int wd = nativeAddWatch(path.getBytes(CHARSET_UTF), mask);
		if (wd < 0)
		{
			throw new JNotifyException_linux("Error watching " + path + " : " + getErrorDesc(-wd), -wd);
		}
		return wd;
	}

	public static void removeWatch(int wd) throws JNotifyException
	{
		int ret = nativeRemoveWatch(wd);
		if (ret != 0)
		{
			throw new JNotifyException_linux("Error removing watch " + wd, ret);
		}
	}
	
	private static void init()
	{
		Thread thread = new Thread("INotify thread")
		{
			public void run()
			{
				nativeNotifyLoop();
			}
		};
		thread.setDaemon(true);
		thread.start();
	}
	
	static void callbackProcessEvent(byte[] name, int wd, int mask, int cookie)
	{
		if (_notifyListener != null)
		{
			_notifyListener.notify(new String(name, CHARSET_UTF), wd, mask, cookie);
		}
	}

	public static void setNotifyListener(INotifyListener notifyListener)
	{
		if (_notifyListener == null)
		{
			_notifyListener = notifyListener;
		}
		else
		{
			throw new RuntimeException("Notify listener is already set. multiple notify listeners are not supported.");
		}
	}
}
