//
//  Mac OSX Cerberus Launcher v0.1
//

#include <Foundation/Foundation.h>
#include <CoreServices/CoreServices.h>
#include <AppKit/AppKit.h>

//#define APP_BUNDLE_ID "com.krautapps.Ted"

// Output error data
void outlist(NSMutableArray *array){
    for(int i = 1;i<array.count-1;i++){
        NSLog(@"%@", [array objectAtIndex:i]);
    }
}

// Main entry point
int main(int argc, const char * argv[]) {
     // Array for file list
    NSMutableArray *files = [[NSMutableArray alloc] initWithCapacity:0];
    
    // This should give us the path to this running instance
    NSBundle *main = [NSBundle mainBundle];
    
    // String hack
    NSString *targetPath = [main.bundleURL.absoluteString stringByReplacingOccurrencesOfString:@"Cerberus.app" withString:@"bin/Ted.app"];
    NSURL *targetURL = [NSURL URLWithString:targetPath];
    
    // Process the arguments to pass. Caution anything can be processed.
    for(int i = 0;i<argc;i++){
        NSString *str = [NSString stringWithUTF8String:argv[i]];
        [files addObject:[NSURL URLWithString:str]];
    }
    
    // Use NSWorkspace and pass the URL's to Ted
    if(![[NSWorkspace sharedWorkspace] openURLs:files
                           withApplicationAtURL:targetURL
                                        options:NSWorkspaceLaunchDefault
                                  configuration:nil
                                          error:nil])
    {
        NSLog(@"ERROR");
        outlist(files);
    } else {
        NSLog(@"%@", targetURL);
        outlist(files);
    }
    
    return 0;
}
