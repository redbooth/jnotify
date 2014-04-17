package net.contentobjects.jnotify.macosx;

public interface FSEventListener
{
	public void notifyChange(int wd, String filePath, int flags);

	public void batchStart(int wd);

	public void batchEnd(int wd);
}
