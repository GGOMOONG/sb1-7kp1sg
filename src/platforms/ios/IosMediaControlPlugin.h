#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <FamilyControls/FamilyControls.h>

@interface IosMediaControlPlugin : NSObject

+ (instancetype)sharedInstance;
- (void)pauseMedia;
- (void)resumeMedia;
- (void)requestScreenTimePermission:(void (^)(BOOL))completion;

@end