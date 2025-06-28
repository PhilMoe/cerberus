
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.ArrayList;


class CallbackSerializerNative {
    private final ConcurrentLinkedQueue<Integer> callbackQueue = new ConcurrentLinkedQueue<>();
    
    
    public void AddCallback(int callBackId) {
        callbackQueue.offer(callBackId);
    }
    
    
    public int[] GetCallbacks() {
        ArrayList<Integer> callBackIdList = new ArrayList<>();
        Integer callBackId;
        
        while ((callBackId = callbackQueue.poll()) != null) {
            callBackIdList.add(callBackId);
        }
        
        int[] callBackIds = new int[callBackIdList.size()];
        for (int i = 0; i < callBackIdList.size(); i++) {
            callBackIds[i] = callBackIdList.get(i);
        }
        
        return callBackIds;
    }
    
}
