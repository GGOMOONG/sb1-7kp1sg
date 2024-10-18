import { Injectable } from '@angular/core';
import { alert } from '@nativescript/core';

@Injectable({
  providedIn: 'root'
})
export class AlertService {
  async show(title: string, message: string, okButtonText: string): Promise<void> {
    return alert({
      title: title,
      message: message,
      okButtonText: okButtonText
    });
  }
}