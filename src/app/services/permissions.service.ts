import { Injectable } from '@angular/core';
import { android as androidApp, ios as iosApp, isAndroid, isIOS } from '@nativescript/core/application';
import { IosMediaControlPlugin } from 'ios-media-control-plugin';

@Injectable({
  providedIn: 'root'
})
export class PermissionsService {
  async requestOverlayPermission(): Promise<boolean> {
    if (isAndroid) {
      const context = androidApp.context;
      const Settings = android.provider.Settings;
      if (!Settings.canDrawOverlays(context)) {
        const intent = new android.content.Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
        intent.setData(android.net.Uri.parse("package:" + context.getPackageName()));
        context.startActivity(intent);
        return false;
      }
      return true;
    } else if (isIOS) {
      // iOS doesn't require explicit permission for overlays
      return true;
    }
    return false;
  }

  async requestScreenTimePermission(): Promise<boolean> {
    if (isIOS) {
      return await IosMediaControlPlugin.getInstance().requestScreenTimePermission();
    }
    return true; // For Android, no equivalent permission is required
  }
}