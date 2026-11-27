// SPDX-License-Identifier: GPL-3.0-or-later
import java.util.Arrays;

/** Run with the target image's unmodified app_process/ART. */
public final class AndroidMemoryStress {
    public static void main(String[] args) throws Exception {
        final int iterations = 32;
        byte[][] retained = new byte[8][];
        long[] collections = new long[iterations];
        long checksum = 0;
        long started = System.nanoTime();
        for (int iteration = 0; iteration < iterations; iteration++) {
            for (int block = 0; block < 16; block++) {
                byte[] bytes = new byte[1024 * 1024];
                for (int page = 0; page < bytes.length; page += 4096) {
                    bytes[page] = (byte) (iteration + block + 1);
                    checksum += bytes[page];
                }
                retained[block % retained.length] = bytes;
            }
            long before = System.nanoTime();
            System.gc();
            collections[iteration] = System.nanoTime() - before;
        }
        Arrays.sort(collections);
        Runtime runtime = Runtime.getRuntime();
        System.out.println("MEMORY_STRESS {\"iterations\":" + iterations
                + ",\"allocated_bytes\":" + (iterations * 16L * 1024 * 1024)
                + ",\"retained_bytes\":" + (8L * 1024 * 1024)
                + ",\"checksum\":" + checksum
                + ",\"duration_ns\":" + (System.nanoTime() - started)
                + ",\"gc_call_p50_ns\":" + collections[iterations / 2]
                + ",\"gc_call_p95_ns\":" + collections[30]
                + ",\"gc_call_max_ns\":" + collections[31]
                + ",\"heap_used_bytes\":" + (runtime.totalMemory() - runtime.freeMemory()) + "}");
        // Caller can sample the live process before it exits and releases the mm.
        if (args.length != 0) Thread.sleep(Long.parseLong(args[0]));
        // A standalone Zygote child starts Binder threads. Request normal
        // process termination explicitly instead of waiting for their shutdown.
        if (args.length > 1 && args[1].equals("exit")) System.exit(0);
    }
}
