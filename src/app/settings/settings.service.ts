import { Injectable } from '@angular/core';
import { ApplicationSettings } from '@nativescript/core';

@Injectable({
  providedIn: 'root'
})
export class SettingsService {
  private readonly DISTANCE_THRESHOLD_KEY = 'distanceThreshold';
  private readonly MAX_PAUSE_COUNT_KEY = 'maxPauseCount';

  getDistanceThreshold(): number {
    return ApplicationSettings.getNumber(this.DISTANCE_THRESHOLD_KEY, 30);
  }

  setDistanceThreshold(value: number): void {
    ApplicationSettings.setNumber(this.DISTANCE_THRESHOLD_KEY, value);
  }

  getMaxPauseCount(): number {
    return ApplicationSettings.getNumber(this.MAX_PAUSE_COUNT_KEY, 3);
  }

  setMaxPauseCount(value: number): void {
    ApplicationSettings.setNumber(this.MAX_PAUSE_COUNT_KEY, value);
  }
}