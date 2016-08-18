package Utils;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by linwenhai on 16/8/6.
 */
public final class SevenZipHelper {
    public native static int extract7z(final String file, final String outPath, boolean listenEnabled, int tag);

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

    public static int extract7zSync(final String archiveFilePath, final String outPath) {
        return extract7zSync(archiveFilePath, outPath, null);
    }

    //解压正常返回0,  解压出错返回非0
    public static int extract7zSync(final String archiveFilePath, final String outPath, final OnExtractListener listener) {
        File archiveFile = new File(archiveFilePath);
        if(!archiveFile.exists() || !archiveFile.isFile()) {
            if(listener != null)
                listener.onExtractFailed("archive file not exist!");
            return 1;
        }

        File outDir = new File(outPath);
        if (!outDir.exists()){
            outDir.mkdirs();
        }

        int listenerIndex = s_listenerIndex++;
        s_listenerIndex += 1;
        if (listener != null) {
            s_listeners.put(listenerIndex, listener);
            return extract7z(archiveFilePath, outPath, true, listenerIndex);
        }
        else {
            return extract7z(archiveFilePath, outPath, false, listenerIndex);
        }
    }

    public static int Extract7zCallbackJNI(final int tag, final float percent, final int state, final String message) {
        final OnExtractListener listener = s_listeners.get(tag);
        if (listener == null)
            return 1;

        int ret = 0;
        switch (state) {
            case EXTRACT_STATE_EXTRACTING:
                listener.onExtractProgress(percent);
                if (listener.isExtractInterrupted()) {
                    ThreadUtils.runOnUIThread(new Runnable() {
                        @Override
                        public void run() {
                            listener.onExtractInterrupt();
                        }
                    });

                    s_listeners.remove(tag);
                    ret = 1;
                }
                break;
            case EXTRACT_STATE_COMPLETED:
                s_listeners.remove(tag);

                ThreadUtils.runOnUIThread(new Runnable() {
                    @Override
                    public void run() {
                        listener.onExtractSucceed();
                    }
                });
                break;
            case EXTRACT_STATE_ERROR:
                s_listeners.remove(tag);
                ThreadUtils.runOnUIThread(new Runnable() {
                    @Override
                    public void run() {
                        listener.onExtractFailed(message);
                    }
                });
                break;
            default:
                break;
        }

        return ret;
    }

    /*static {
        System.loadLibrary("AndroidExtract7z");
    }*/
}
