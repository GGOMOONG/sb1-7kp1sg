import { Component, OnInit } from '@angular/core';
import { Camera } from '@nativescript/camera';
import { RouterExtensions } from '@nativescript/angular';
import { SettingsService } from '../settings/settings.service';
import { PermissionsService } from '../services/permissions.service';
import { MediaControlService } from '../services/media-control.service';
import { FaceDetectionService } from '../services/face-detection.service';
import { AlertService } from '../services/alert.service';
import { ApplicationSettings } from '@nativescript/core';

@Component({
  selector: 'ns-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.css']
})
export class MainComponent implements OnInit {
  isPlaying: boolean = true;
  pauseCount: number = 0;
  showLongAd: boolean = false;
  distanceThreshold: number;
  maxPauseCount: number;
  isFirstLaunch: boolean = true;

  constructor(
    private camera: Camera,
    private routerExtensions: RouterExtensions,
    private settingsService: SettingsService,
    private permissionsService: PermissionsService,
    private mediaControlService: MediaControlService,
    private faceDetectionService: FaceDetectionService,
    private alertService: AlertService
  ) {}

  async ngOnInit() {
    this.isFirstLaunch = ApplicationSettings.getBoolean('isFirstLaunch', true);
    if (this.isFirstLaunch) {
      await this.showWelcomeMessage();
      ApplicationSettings.setBoolean('isFirstLaunch', false);
    }
    await this.requestPermissions();
    this.loadSettings();
    this.startDistanceDetection();
  }

  async showWelcomeMessage() {
    await this.alertService.show(
      "Welcome to Eye Protection App",
      "This app helps protect your eyes by monitoring your viewing distance. " +
      "We recommend keeping a distance of at least 30cm from your device. " +
      "We'll need camera permissions to detect your face, and system permissions to control media playback. " +
      "Your privacy is important to us - no images are stored or transmitted.",
      "OK"
    );
  }

  async requestPermissions() {
    try {
      const cameraPermission = await this.camera.requestPermissions();
      const overlayPermission = await this.permissionsService.requestOverlayPermission();
      const screenTimePermission = await this.permissionsService.requestScreenTimePermission();
      
      if (!cameraPermission || !overlayPermission || !screenTimePermission) {
        await this.alertService.show(
          "Permissions Required",
          "This app requires camera and system permissions to function properly. Please grant the necessary permissions in your device settings.",
          "OK"
        );
        // Optionally, you can exit the app or disable functionality here
      }
    } catch (error) {
      console.error('Error requesting permissions:', error);
      await this.alertService.show(
        "Error",
        "There was an error requesting permissions. Please try again.",
        "OK"
      );
      // Optionally, you can exit the app or disable functionality here
    }
  }

  loadSettings() {
    this.distanceThreshold = this.settingsService.getDistanceThreshold();
    this.maxPauseCount = this.settingsService.getMaxPauseCount();
  }

  async startDistanceDetection() {
    try {
      while (true) {
        const options = { width: 300, height: 300, keepAspectRatio: true, saveToGallery: false };
        const imageAsset = await this.camera.takePicture(options);
        const distance = await this.faceDetectionService.estimateDistance(imageAsset);
        
        if (distance !== null && distance < this.distanceThreshold) {
          await this.handleTooClose();
        } else {
          await this.handleSafeDistance();
        }
        
        // Wait for a short interval before taking the next picture
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
    } catch (error) {
      console.error('Error in distance detection:', error);
      await this.alertService.show(
        "Error",
        "There was an error with distance detection. Please restart the app.",
        "OK"
      );
    }
  }

  async handleTooClose() {
    if (this.isPlaying) {
      this.isPlaying = false;
      await this.mediaControlService.pauseAllMedia();
      this.pauseCount++;
      
      if (this.pauseCount >= this.maxPauseCount) {
        this.showLongAd = true;
        // Implement long advertisement logic here
        await this.alertService.show(
          "Take a Break",
          "You've been too close to the screen for an extended period. Please take a 5-minute break.",
          "OK"
        );
      } else {
        // Implement short advertisement logic here
        await this.alertService.show(
          "Move Back",
          "You're too close to the screen. Please move back.",
          "OK"
        );
      }
    }
  }

  async handleSafeDistance() {
    if (!this.isPlaying) {
      this.isPlaying = true;
      await this.mediaControlService.resumeAllMedia();
      this.pauseCount = 0;
      this.showLongAd = false;
    }
  }

  goToSettings() {
    this.routerExtensions.navigate(['/settings']);
  }
}