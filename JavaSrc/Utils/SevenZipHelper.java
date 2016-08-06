package Utils;

/**
 * Created by linwenhai on 16/8/6.
 */
public final class SevenZipHelper {
    public native static int extract7z(final String file, final String outPath);

    static {
        System.loadLibrary("AndroidExtract7z");
    }
}
