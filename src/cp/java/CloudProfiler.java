package cp.java;

public class CloudProfiler {

    private static void loadCloudProfilerLibrary() {
        try {
            System.loadLibrary("cloud_profiler");
        }
        catch(UnsatisfiedLinkError e) {
            System.err.println("ERROR: \"cloud_profiler\" native code library failed to load.\n" + e);
            System.exit(1);
        }
    }

    public static void init() {
        loadCloudProfilerLibrary(); 
    }
}
