#import "OPSQLite.h"
#import "../cpp/bindings.h"
#import <React/RCTBridge+Private.h>
#import <React/RCTLog.h>
#import <React/RCTUtils.h>
#import <ReactCommon/RCTTurboModule.h>
#import <jsi/jsi.h>

@implementation OPSQLite

@synthesize bridge = _bridge;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
    return YES;
}

- (NSDictionary *)constantsToExport {
    NSArray *libraryPaths = NSSearchPathForDirectoriesInDomains(
        NSLibraryDirectory, NSUserDomainMask, true);
    NSString *libraryPath = [libraryPaths objectAtIndex:0];

    NSArray *documentPaths = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, true);
    NSString *documentPath = [documentPaths objectAtIndex:0];
    return @{
        @"IOS_DOCUMENT_PATH" : documentPath,
        @"IOS_LIBRARY_PATH" : libraryPath
    };
}

- (NSDictionary *)getConstants {
    return [self constantsToExport];
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install) {
    RCTCxxBridge *cxxBridge = (RCTCxxBridge *)_bridge;
    if (cxxBridge == nil) {
        return @false;
    }

    auto jsiRuntime = (facebook::jsi::Runtime *)cxxBridge.runtime;
    if (jsiRuntime == nil) {
        return @false;
    }

    auto &runtime = *jsiRuntime;
    auto callInvoker = _bridge.jsCallInvoker;

    // Get appGroupID value from Info.plist using key "AppGroup"
    NSString *appGroupID =
        [[NSBundle mainBundle] objectForInfoDictionaryKey:@"OPSQLite_AppGroup"];
    NSString *documentPath;

    if (appGroupID != nil) {
        // Get the app groups container storage url
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSURL *storeUrl = [fileManager
            containerURLForSecurityApplicationGroupIdentifier:appGroupID];

        if (storeUrl == nil) {
            NSLog(@"OP-SQLite: Invalid AppGroup ID provided (%@). Check the "
                  @"value of "
                  @"\"AppGroup\" in your Info.plist file",
                  appGroupID);
            return @false;
        }

        documentPath = [storeUrl path];
    } else {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(
            NSLibraryDirectory, NSUserDomainMask, true);
        documentPath = [paths objectAtIndex:0];
    }

    NSBundle *crsqlite_bundle =
        [NSBundle bundleWithIdentifier:@"io.vlcn.crsqlite"];
    NSString *crsqlite_path = [crsqlite_bundle pathForResource:@"crsqlite"
                                                        ofType:@""];
    NSBundle *libsqlitevec_bundle =
        [NSBundle bundleWithIdentifier:@"com.ospfranco.sqlitevec"];
    NSString *sqlite_vec_path =
        [libsqlitevec_bundle pathForResource:@"sqlitevec" ofType:@""];
    
    if (crsqlite_path == nil) {
        crsqlite_path = @"";
    }

    if (sqlite_vec_path == nil) {
        sqlite_vec_path = @"";
    }
    
    opsqlite::install(runtime, callInvoker, [documentPath UTF8String],
                      [crsqlite_path UTF8String], [sqlite_vec_path UTF8String],
                      [@"" UTF8String]);
    return @true;
}

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(getDylibPath : (
    NSString *)bundleId andResource : (NSString *)resourceName) {
    NSBundle *bundle = [NSBundle bundleWithIdentifier:bundleId];
    NSString *path = [bundle pathForResource:resourceName ofType:@""];
    return path;
}

RCT_EXPORT_METHOD(moveAssetsDatabase : (NSDictionary *)args resolve : (
    RCTPromiseResolveBlock)resolve reject : (RCTPromiseRejectBlock)reject) {
    NSString *documentPath = [NSSearchPathForDirectoriesInDomains(
        NSLibraryDirectory, NSUserDomainMask, true) objectAtIndex:0];

    NSString *filename = args[@"filename"];
    BOOL overwrite = [args[@"overwrite"] boolValue];

    NSString *destinationPath =
        [documentPath stringByAppendingPathComponent:filename];
    NSFileManager *fileManager = [NSFileManager defaultManager];

    NSString *sourcePath = [[NSBundle mainBundle] pathForResource:filename
                                                           ofType:nil];
    
    if (sourcePath == nil) {
        if([fileManager fileExistsAtPath:destinationPath]) {
            resolve(@true);
        } else {
            NSString *msg = [NSString stringWithFormat:@"Asset not found for file: %@", filename];
            reject(@"op-sqlite-error", msg, nil);
        }
        return;
    }

    NSError *error;
    if ([fileManager fileExistsAtPath:destinationPath]) {
        if (overwrite) {
            [fileManager removeItemAtPath:destinationPath error:&error];
            if (error) {
                NSLog(@"Error: %@", error);
                reject(@"op-sqlite-error", @"Failed to remove existing database", error);
                return;
            }
        } else {
            resolve(@true);
            return;
        }
    }

    [fileManager copyItemAtPath:sourcePath toPath:destinationPath error:&error];
    if (error) {
        NSLog(@"Error: %@", error);
        reject(@"op-sqlite-error", @"Failed to copy database from assets", error);
        return;
    }
    resolve(@true);
    return;
}

- (void)invalidate {
    opsqlite::invalidate();
}

+ (void)expoUpdatesWorkaround {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(
        NSLibraryDirectory, NSUserDomainMask, true);
    NSString *documentPath = [paths objectAtIndex:0];
    opsqlite::expoUpdatesWorkaround([documentPath UTF8String]);
}

@end
