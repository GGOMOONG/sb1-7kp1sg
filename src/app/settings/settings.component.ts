import { Component, OnInit } from '@angular/core';
import { RouterExtensions } from '@nativescript/angular';
import { SettingsService } from './settings.service';
import { AlertService } from '../services/alert.service';

@Component({
  selector: 'ns-settings',
  templateUrl: './settings.component.html',
  styleUrls: ['./settings.component.css']
})
export class SettingsComponent implements OnInit {
  distanceThreshold: number;
  maxPauseCount: number;
  supportedApps: string[] = ['YouTube', 'Netflix', 'Hulu', 'Amazon Prime Video'];

  constructor(
    private routerExtensions: RouterExtensions,
    private settingsService: SettingsService,
    private alertService: AlertService
  ) {}

  ngOnInit() {
    this.loadSettings();
  }

  loadSettings() {
    this.distanceThreshold = this.settingsService.getDistanceThreshold();
    this.maxPauseCount = this.settingsService.getMaxPauseCount();
  }

  saveSettings() {
    this.settingsService.setDistanceThreshold(this.distanceThreshold);
    this.settingsService.setMaxPauseCount(this.maxPauseCount);
    this.routerExtensions.back();
  }

  async showPrivacyPolicy() {
    await this.alertService.show(
      "Privacy Policy",
      "We respect your privacy. This app does not store or transmit any personal data or images. " +
      "Face detection is performed locally on your device. " +
      "We only control media playback for supported apps when necessary for eye protection.",
      "OK"
    );
  }
}