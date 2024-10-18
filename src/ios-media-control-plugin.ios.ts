import { ios } from '@nativescript/core/application';

declare var IosMediaControlPlugin: any;

export class IosMediaControlPlugin {
  private static instance: IosMediaControlPlugin;

  static getInstance(): IosMediaControlPlugin {
    if (!IosMediaControlPlugin.instance) {
      IosMediaControlPlugin.instance = new IosMediaControlPlugin();
    }
    return IosMediaControlPlugin.instance;
  }

  pauseMedia(): void {
    IosMediaControlPlugin.sharedInstance().pauseMedia();
  }

  resumeMedia(): void {
    IosMediaControlPlugin.sharedInstance().resumeMedia();
  }

  requestScreenTimePermission(): Promise<boolean> {
    return new Promise((resolve) => {
      IosMediaControlPlugin.sharedInstance().requestScreenTimePermissionWithCompletion((granted: boolean) => {
        resolve(granted);
      });
    });
  }
}