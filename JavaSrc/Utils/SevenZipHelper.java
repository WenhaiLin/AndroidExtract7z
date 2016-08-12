package Utils;


import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by linwenhai on 16/8/6.
 */
public final class SevenZipHelper {
    public native static void extract7z(final String file, final String outPath, boolean listenEnabled, int tag);

    public static final int EXTRACT_STATE_IDLE = 0;
    public static final int EXTRACT_STATE_ERROR = 1;
    public static final int EXTRACT_STATE_EXTRACTING = 2;
    public static final int EXTRACT_STATE_COMPLETED = 3;

    private static int s_listenerIndex = 0;
    private static Map<Integer, OnExtractListener> s_listeners = new HashMap<>();

    public interface OnExtractListener {
        void onExtractSucceed();

        void onExtractProgress(float percent);

        void onExtractFailed(String errorMsg);

        void onExtractInterrupt();

        boolean isExtractInterrupted();
    }

    public static void extract7zAsync(final String archiveFilePath, final String outPath, final OnExtractListener listener)
    {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                File archiveFile = new File(archiveFilePath);
                if(!archiveFile.exists() || !archiveFile.isFile()) {
                    listener.onExtractFailed("archive file not exist!");
                    return;
                }

                File outDir = new File(outPath);
                if (!outDir.exists()){
                    outDir.mkdirs();
                }

                s_listeners.put(s_listenerIndex, listener);
                extract7z(archiveFilePath, outPath, true, s_listenerIndex);
                s_listenerIndex += 1;
            }
        });
        thread.start();
    }

    public static int Extract7zCallbackJNI(final int tag, final float percent, final int state, final String message)
    {
        OnExtractListener listener = s_listeners.get(tag);
        if (listener == null)
            return 1;

        int ret = 0;
        switch (state) {
            case EXTRACT_STATE_EXTRACTING:
                listener.onExtractProgress(percent);
                if (listener.isExtractInterrupted()) {
                    listener.onExtractInterrupt();
                    s_listeners.remove(tag);
                    ret = 1;
                }
                break;
            case EXTRACT_STATE_COMPLETED:
                listener.onExtractSucceed();
                break;
            case EXTRACT_STATE_ERROR:
                listener.onExtractFailed(message);
                break;
            default:
                break;
        }

        return ret;
    }

    static {
        System.loadLibrary("AndroidExtract7z");
    }
}
