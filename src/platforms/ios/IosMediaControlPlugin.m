#import "IosMediaControlPlugin.h"

@implementation IosMediaControlPlugin

+ (instancetype)sharedInstance {
    static IosMediaControlPlugin *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

- (void)pauseMedia {
    [[MPRemoteCommandCenter sharedCommandCenter].pauseCommand addTarget:self action:@selector(handlePauseCommand:)];
    [[MPRemoteCommandCenter sharedCommandCenter].pauseCommand setEnabled:YES];
    [[MPMusicPlayerController systemMusicPlayer] pause];
}

- (void)resumeMedia {
    [[MPRemoteCommandCenter sharedCommandCenter].playCommand addTarget:self action:@selector(handlePlayCommand:)];
    [[MPRemoteCommandCenter sharedCommandCenter].playCommand setEnabled:YES];
    [[MPMusicPlayerController systemMusicPlayer] play];
}

- (void)requestScreenTimePermission:(void (^)(BOOL))completion {
    if (@available(iOS 15.0, *)) {
        [FamilyControls requestAuthorizationWithCompletion:^(FamilyControlsAuthorizationStatus status) {
            completion(status == FamilyControlsAuthorizationStatusApproved);
        }];
    } else {
        completion(NO);
    }
}

- (MPRemoteCommandHandlerStatus)handlePauseCommand:(MPRemoteCommandEvent *)event {
    [[MPMusicPlayerController systemMusicPlayer] pause];
    return MPRemoteCommandHandlerStatusSuccess;
}

- (MPRemoteCommandHandlerStatus)handlePlayCommand:(MPRemoteCommandEvent *)event {
    [[MPMusicPlayerController systemMusicPlayer] play];
    return MPRemoteCommandHandlerStatusSuccess;
}

@end