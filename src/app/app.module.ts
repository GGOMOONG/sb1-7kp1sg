import { NgModule, NO_ERRORS_SCHEMA } from '@angular/core'
import { NativeScriptModule } from '@nativescript/angular'
import { NativeScriptFormsModule } from '@nativescript/angular'

import { AppRoutingModule } from './app-routing.module'
import { AppComponent } from './app.component'
import { MainComponent } from './main/main.component'
import { SettingsComponent } from './settings/settings.component'
import { PermissionsService } from './services/permissions.service'
import { MediaControlService } from './services/media-control.service'

@NgModule({
  bootstrap: [AppComponent],
  imports: [
    NativeScriptModule,
    NativeScriptFormsModule,
    AppRoutingModule
  ],
  declarations: [AppComponent, MainComponent, SettingsComponent],
  providers: [PermissionsService, MediaControlService],
  schemas: [NO_ERRORS_SCHEMA],
})
export class AppModule {}