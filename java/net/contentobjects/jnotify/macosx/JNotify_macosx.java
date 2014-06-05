package net.contentobjects.jnotify.macosx;

import net.contentobjects.jnotify.JNotifyException;

import static net.contentobjects.jnotify.Util.CHARSET_UTF;

public class JNotify_macosx
{
    public static final int MUST_SCAN_SUBDIRS   = 0x00000001;
    public static final int USER_DROPPED        = 0x00000002;
    public static final int KERNEL_DROPPED      = 0x00000004;
    public static final int EVENT_IDS_WRAPPED   = 0x00000008;
    public static final int HISTORY_DONE        = 0x00000010;
    public static final int ROOT_CHANGED        = 0x00000020;
    public static final int MOUNT               = 0x00000040;
    public static final int UNMOUNT             = 0x00000080;

    // NB: the following flags are only valid for per-file notifications
    // TODO: enable per-file notifications on recent versions of OSX (10.7+)
    public static final int ITEM_CREATED         = 0x00000100;
    public static final int ITEM_REMOVED         = 0x00000200;
    public static final int ITEM_INODE_META_MOD  = 0x00000400;
    public static final int ITEM_RENAMED         = 0x00000800;
    public static final int ITEM_MODIFIED        = 0x00001000;
    public static final int ITEM_FINDER_INFO_MOD = 0x00002000;
    public static final int ITEM_CHANGE_OWNER    = 0x00004000;
    public static final int ITEM_XATTR_MOD       = 0x00008000;
    public static final int ITEM_IS_FILE         = 0x00010000;
    public static final int ITEM_IS_DIR          = 0x00020000;
    public static final int ITEM_IS_SYMLINK      = 0x00040000;


	private static Object initCondition = new Object();
	private static Object countLock = new Object();
	private static int watches = 0;

	static
	{
		Thread thread = new Thread("fs") //$NON-NLS-1$
		{
			public void run()
			{
				nativeInit();
				synchronized (initCondition)
				{
					initCondition.notifyAll();
					initCondition = null;
				}
				while (true)
				{
					synchronized (countLock)
					{
						while (watches == 0)
						{
							try
							{
								countLock.wait();
							}
							catch (InterruptedException e)
							{
							}
						}
					}
					nativeNotifyLoop();
				}
			}
		};
		thread.setDaemon(true);
		thread.start();
	}

	private static native void nativeInit();
	private static native int nativeAddWatch(String path) throws JNotifyException;
	private static native boolean nativeRemoveWatch(int wd);
	private static native void nativeNotifyLoop();

	private static FSEventListener _eventListener;

	public static int addWatch(String path) throws JNotifyException
	{
		Object myCondition = initCondition;
		if (myCondition != null)
		{
			synchronized (myCondition)
			{
				while (initCondition != null)
				{
					try
					{
						initCondition.wait();
					}
					catch (InterruptedException e)
					{
					}
				}
			}
		}
		int wd = nativeAddWatch(path);
		synchronized (countLock)
		{
			watches++;
			countLock.notifyAll();
		}
		return wd;
	}

	public static boolean removeWatch(int wd)
	{
		boolean removed = nativeRemoveWatch(wd);
		if (removed)
		{
			synchronized (countLock)
			{
				watches--;
			}
		}
		return removed;
	}

	public static void callbackProcessEvent(int wd, byte[] filePath, int flags)
	{
		if (_eventListener != null)
		{
			_eventListener.notifyChange(wd, new String(filePath, CHARSET_UTF), flags);
		}
	}

	public static void callbackInBatch(int wd, boolean state)
	{
		if (_eventListener != null)
		{
			if (state) {
				_eventListener.batchStart(wd);
			} else {
				_eventListener.batchEnd(wd);
			}
		}
	}

	public static void setNotifyListener(FSEventListener eventListener)
	{
		if (_eventListener == null)
		{
			_eventListener = eventListener;
		}
		else
		{
			throw new RuntimeException("Notify listener is already set. multiple notify listeners are not supported.");
		}
	}
}
