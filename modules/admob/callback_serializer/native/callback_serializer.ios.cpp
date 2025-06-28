#import <Foundation/Foundation.h>

class CallbackSerializerNative : public Object {
private:
    NSMutableArray* _callbackQueue;
    
public:
    CallbackSerializerNative() {
        _callbackQueue = [[NSMutableArray alloc] init];
    }
    
    ~CallbackSerializerNative() {
        [_callbackQueue release];
    }
    
    void AddCallback(int callBackId) {
        @synchronized(_callbackQueue) {
            [_callbackQueue addObject:@(callBackId)];
        }
    }
    
    Array<int> GetCallbacks() {
        NSArray* callBackIdList;
        @synchronized(_callbackQueue) {
            callBackIdList = [_callbackQueue copy];
            [_callbackQueue removeAllObjects];
        }
        
        int count = (int)[callBackIdList count];
        Array<int > callBackIds = Array<int >(count);
        for (int i = 0; i < count; i++) {
            callBackIds.At(i) = [callBackIdList[i] intValue];
        }
        
        [callBackIdList release];
        return callBackIds;
    }
};
