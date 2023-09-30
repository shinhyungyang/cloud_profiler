
package org.apache.storm.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

public class Utils {

    /**
     * "{:a 1 :b 1 :c 2} -> {1 [:a :b] 2 :c}"
     *
     * Example usage in java:
     * Map<Integer, String> tasks;
     * Map<String, List<Integer>> componentTasks = Utils.reverseMap(tasks);
     *
     * The order of he resulting list values depends on the ordering
     * properties of the Map passed in. The caller is responsible for passing
     * an ordered map if they expect the result to be consistently ordered as
     * well.
     *
     * @param map to reverse
     * @return a reversed map
     */
    public static <K, V> HashMap<V, List<K>> reverseMap(Map<K, V> map) {
        HashMap<V, List<K>> rtn = new HashMap<V, List<K>>();
        if (map == null) {
            return rtn;
        }
        for (Map.Entry<K, V> entry : map.entrySet()) {
            K key = entry.getKey();
            V val = entry.getValue();
            List<K> list = rtn.get(val);
            if (list == null) {
                list = new ArrayList<K>();
                rtn.put(entry.getValue(), list);
            }
            list.add(key);
        }
        return rtn;
    }
}

