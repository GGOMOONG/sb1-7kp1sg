export declare class IosMediaControlPlugin {
  static getInstance(): IosMediaControlPlugin;
  pauseMedia(): void;
  resumeMedia(): void;
  requestScreenTimePermission(): Promise<boolean>;
}