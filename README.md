# Google UMP extension for Defold

This native extension wraps the Google User Messaging Platform (UMP) SDK on Android. It updates consent information, shows a required consent form, and lets users reopen their privacy options. The `ump` Lua table is available on Android only; check `if ump then` in cross-platform scripts.

## Setup

1. Add this extension to your Defold project's dependencies and fetch libraries. It bundles UMP SDK 4.0.0. [Google's Android setup guide](https://developers.google.com/admob/android/privacy) lists Android API 21 as the minimum for UMP 4.0.0; this repository's example project targets API 23.
2. Configure an **AdMob application ID** in the Android manifest. The [Defold AdMob extension](https://github.com/defold/extension-admob) can supply it through `[admob] app_id_android` in `game.project`. AdMob is not required by the UMP bridge itself; if you do not use that extension, add the `com.google.android.gms.ads.APPLICATION_ID` manifest metadata yourself. The `[ump] app_id_android` value in this repository's example `game.project` is not read by the bridge or its manifest.
3. Create and publish a message in AdMob **Privacy & messaging** for that application ID. Without a configured message, UMP cannot show the corresponding consent form.
4. On Defold 1.13.2 or newer, select `/builtins/manifests/android/dmengine.keep` under **Android → R8 Keep Rules** in `game.project`. This extension also supplies `ump/manifests/android/ump.keep` for its JNI bridge.

If you supply the AdMob application ID in your own Android manifest, include
this metadata inside `<application>` (replace the placeholder with your ID):

```xml
<meta-data
    android:name="com.google.android.gms.ads.APPLICATION_ID"
    android:value="ca-app-pub-xxxxxxxxxxxxxxxx~yyyyyyyyyy" />
```

## Consent flow

Call `ump.request_consent_info_update()` on **every launch**. Its callback runs on the Defold update thread after the update and any required consent form finish. `success` means the flow completed without a UMP error; it does **not** mean the user agreed to personalized ads. Check `ump.can_request_ads()` after a successful callback. A consent decision from an earlier session can make `can_request_ads()` true before the current update finishes, so do not use that early value as the result of the current request.

```lua
function init(self)
    if not ump then return end

    ump.request_consent_info_update(false, "", function(self, success)
        if not success then
            print("UMP update or form failed")
            return
        end

        if ump.can_request_ads() then
            print("Ads may be requested")
        end
        self.show_privacy_choices = ump.is_privacy_options_required()
        -- Use this value to show or hide a privacy button in your UI.
    end)
end

-- Call this from a user-pressed privacy/settings button when it is available.
local function open_privacy_choices()
    if not ump or not ump.is_privacy_options_required() then return end
    ump.show_privacy_options_form(function(self, success)
        if success then
            print("Privacy options closed; ads allowed:", ump.can_request_ads())
        else
            print("Privacy options could not be shown or completed")
        end
    end)
end
```

Google requires `show_privacy_options_form()` to be called in response to user input. The privacy-options callback's `success` means the form was dismissed without an error; it does not describe the choices made. [Google's UMP API](https://developers.google.com/admob/android/reference/privacy/kotlin/com/google/android/ump/UserMessagingPlatform) documents both form callbacks.

## Lua API

Both callbacks are optional. If omitted, the state getters remain available for diagnostics, but callbacks are the intended way to know when a request or form finishes. A newer call of the same kind supersedes an earlier callback.

| Function | Result and behavior |
| --- | --- |
| `ump.request_consent_info_update(test_device, test_device_hashed_id, callback)` | Starts an update and shows the consent form if required. `test_device` is a boolean. `test_device_hashed_id` is a required string; pass `""` when test mode is off. With test mode on, the device hash is registered and UMP debug geography is forced to EEA. Optional `callback(self, success)` receives `false` if the update or form fails. |
| `ump.show_privacy_options_form(callback)` | Opens privacy options. Call from a user action after `is_privacy_options_required()` returns true. Optional `callback(self, success)` receives `false` if the form cannot be shown or completed. |
| `ump.is_privacy_options_required()` | Returns `true` if UMP requires a privacy-options entry point. Returns `false` before consent information has been obtained. |
| `ump.can_request_ads()` | Returns UMP's ad-request eligibility. Check it after the current request callback; it may reflect an earlier session while an update is running. |
| `ump.get_consent_status()` | Returns UMP's numeric consent status: `0` unknown, `1` not required, `2` required, `3` obtained; returns `-1` before the consent-information object exists. Status `3` does not specify whether ads may be personalized. |
| `ump.get_request_state()` | Returns the latest update/form state. Prefer its completion callback over checking this every frame. See constants below. |
| `ump.was_consent_form_required()` | Returns whether the latest **successful information update** found a required normal consent form. Resets to `false` when a new request starts. It can remain true if the subsequent form fails. |
| `ump.get_privacy_options_state()` | Returns the state of the latest privacy-options form. See constants below. |
| `ump.get_gdpr_applies()` | Reads the `IABTCF_gdprApplies` value written to Android default preferences. Returns `GDPR_APPLIES_UNKNOWN` if absent or invalid. Read it after a successful update; it is a GDPR applicability signal, not a country code or vendor consent. |
| `ump.reset_consent_information()` | Clears UMP consent information **for testing** if the consent-information object has been created. Start a new update afterward; the extension's diagnostic state getters are not reset by this call. |
| `ump.initialize_mobile_ads_sdk()` | **Nonfunctional legacy export.** The native bridge calls a Java method absent from this extension. Do not call it; use `admob.initialize()` after consent if you use the AdMob extension. |

### Constants

The following numbers are exported as fields of `ump`. Request states and privacy-options states are diagnostic; they are separate from Google's consent-status numbers.

| Constant | Value | Meaning |
| --- | ---: | --- |
| `REQUEST_STATE_UPDATING` | `0` | Initial state or consent information update running. |
| `REQUEST_STATE_FORM_PENDING` | `3` | Update succeeded; any required form is loading or showing. |
| `REQUEST_STATE_COMPLETE` | `1` | Update and form step finished without a UMP error. |
| `REQUEST_STATE_FAILED` | `2` | Update or form failed. |
| `PRIVACY_OPTIONS_STATE_NOT_SHOWN` | `0` | Initial state. |
| `PRIVACY_OPTIONS_STATE_SHOWING` | `1` | A privacy-options request is in progress. |
| `PRIVACY_OPTIONS_STATE_DISMISSED` | `2` | Privacy-options form closed without an error. |
| `PRIVACY_OPTIONS_STATE_ERROR` | `3` | Privacy-options form failed. |
| `GDPR_APPLIES_UNKNOWN` | `-1` | Applicability value is absent or invalid. |
| `GDPR_APPLIES_NO` | `0` | TCF value says GDPR does not apply. |
| `GDPR_APPLIES_YES` | `1` | TCF value says GDPR applies. |

The full Defold editor API reference is in [`ump/api/ump.script_api`](ump/api/ump.script_api).
