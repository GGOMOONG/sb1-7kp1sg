import { Injectable } from '@angular/core';
import { ImageSource } from '@nativescript/core';
import * as application from '@nativescript/core/application';

declare var com: any;  // For Android
declare var MLKitFaceDetection: any;  // For iOS

@Injectable({
  providedIn: 'root'
})
export class FaceDetectionService {
  async estimateDistance(imageAsset: any): Promise<number | null> {
    const image = await ImageSource.fromAsset(imageAsset);
    
    if (application.android) {
      return this.estimateDistanceAndroid(image);
    } else if (application.ios) {
      return this.estimateDistanceIOS(image);
    }
    
    return null;
  }

  private async estimateDistanceAndroid(image: ImageSource): Promise<number | null> {
    const bitmap = image.android;
    const options = new com.google.mlkit.vision.face.FaceDetectorOptions.Builder()
      .setPerformanceMode(com.google.mlkit.vision.face.FaceDetectorOptions.PERFORMANCE_MODE_FAST)
      .build();
    const detector = com.google.mlkit.vision.face.FaceDetection.getClient(options);
    
    return new Promise((resolve, reject) => {
      const inputImage = com.google.mlkit.vision.common.InputImage.fromBitmap(bitmap, 0);
      detector.process(inputImage)
        .addOnSuccessListener(new com.google.android.gms.tasks.OnSuccessListener({
          onSuccess: function(faces) {
            if (faces.size() > 0) {
              const face = faces.get(0);
              const boundingBox = face.getBoundingBox();
              const width = boundingBox.width();
              // Estimate distance based on face width
              const estimatedDistance = (1000 / width) * 10;  // Adjust this formula as needed
              resolve(estimatedDistance);
            } else {
              resolve(null);
            }
          }
        }))
        .addOnFailureListener(new com.google.android.gms.tasks.OnFailureListener({
          onFailure: function(e) {
            reject(e);
          }
        }));
    });
  }

  private async estimateDistanceIOS(image: ImageSource): Promise<number | null> {
    const ciImage = CIImage.imageWithCGImage(image.ios.CGImage);
    const options = NSDictionary.dictionaryWithObjectForKey(CIDetectorAccuracyHigh, CIDetectorAccuracy);
    const detector = CIDetector.detectorOfTypeCategoryOptions(CIDetectorTypeFace, null, options);
    
    const features = detector.featuresInImage(ciImage);
    if (features.count > 0) {
      const face = features.objectAtIndex(0);
      const bounds = face.bounds;
      const width = bounds.size.width;
      // Estimate distance based on face width
      const estimatedDistance = (1000 / width) * 10;  // Adjust this formula as needed
      return estimatedDistance;
    }
    
    return null;
  }
}