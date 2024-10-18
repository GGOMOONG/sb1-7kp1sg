import { Injectable } from '@angular/core';
import { isAndroid, isIOS } from '@nativescript/core/application';
import { ApplicationSettings } from '@nativescript/core';
import { IosMediaControlPlugin } from 'ios-media-control-plugin';

declare var android: any;

@Injectable({
  providedIn: 'root'
})
export class MediaControlService {
  private supportedApps: string[] = ['youtube', 'netflix', 'hulu', 'amazon'];

  async pauseAllMedia() {
    if (!this.isAppSupported()) {
      console.log('Current app is not supported for media control');
      return;
    }

    if (isAndroid) {
      const audioManager = android.content.Context.getSystemService(android.content.Context.AUDIO_SERVICE);
      if (audioManager.isMusicActive()) {
        const i = new android.content.Intent("com.android.music.musicservicecommand");
        i.putExtra("command", "pause");
        android.app.Application.getInstance().sendBroadcast(i);
      }
    } else if (isIOS) {
      IosMediaControlPlugin.getInstance().pauseMedia();
    }
  }

  async resumeAllMedia() {
    if (!this.isAppSupported()) {
      console.log('Current app is not supported for media control');
      return;
    }

    if (isAndroid) {
      const i = new android.content.Intent("com.android.music.musicservicecommand");
      i.putExtra("command", "play");
      android.app.Application.getInstance().sendBroadcast(i);
    } else if (isIOS) {
      IosMediaControlPlugin.getInstance().resumeMedia();
    }
  }

  private isAppSupported(): boolean {
    const currentApp = ApplicationSettings.getString('currentApp', '');
    return this.supportedApps.includes(currentApp.toLowerCase());
  }
}