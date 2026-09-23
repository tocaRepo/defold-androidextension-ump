# UMP Defold Extension

This Defold extension allows you to integrate Google's **User Messaging Platform (UMP)** into your Defold game to manage user consent for ads on Android. It helps you request consent information, show the consent form, and check the user's consent status.

## Warning

**Disclaimer:** I am not responsible for any issues, errors, or malfunctions caused by the implementation or the usage of this extension. Use at your own risk. Please ensure you understand the methods and make the necessary modifications to fit your specific project requirements.

## Setup

To use this extension, you need to add the defold admob extensions
https://github.com/defold/extension-admob/tree/master

As well as UMP as it's relying on the admob id provided by the admob extension.

Once you have the admob extension installed, just add my latest tag as a dependencies in defold and fetch the libraries.

UMP 4.0.0 requires Android API level 23 or higher, so your Defold project's Android minimum SDK version must be set to 23+.

On Defold 1.13.2 or newer, enable R8 by selecting
`/builtins/manifests/android/dmengine.keep` in **Android → R8 Keep Rules**
in `game.project`. The extension includes `ump/manifests/android/ump.keep`
to preserve its Java bridge classes and methods used through JNI while
allowing R8 to optimize their code.


## Methods Available
The UMP extension provides several methods that allow you to manage user consent and ads functionality. Here’s a breakdown of each available method:

```
request_consent_info_update

```
This method updates the consent information and shows the consent form if needed.
Pass a testDevice flag and a testDeviceHashedId for testing. The optional callback
receives `(self, success)` on the Defold update thread after the update and any
required form have finished. `success` is false if either step fails.

Usage:


```
ump.request_consent_info_update(testDevice, testDeviceHashedId, function(self, success)
    if success then
        print("Ads allowed:", ump.can_request_ads())
    end
end)

```

The request is asynchronous. Check `ump.can_request_ads()` inside the success
callback. The optional `ump.get_request_state()` getter remains available for
diagnostics and returns `ump.REQUEST_STATE_UPDATING`,
`ump.REQUEST_STATE_FORM_PENDING`, `ump.REQUEST_STATE_COMPLETE`, or
`ump.REQUEST_STATE_FAILED`.
`ump.was_consent_form_required()` reports whether this update required the normal
consent form. It is false before an update succeeds and resets on each request.

`ump.show_privacy_options_form(function(self, success) ... end)` opens the privacy
options form when called from a user action. Its callback runs after dismissal
or an error. `ump.get_privacy_options_state()` returns
`ump.PRIVACY_OPTIONS_STATE_NOT_SHOWN` before the form is opened,
`ump.PRIVACY_OPTIONS_STATE_SHOWING` while it is showing,
`ump.PRIVACY_OPTIONS_STATE_DISMISSED` after successful dismissal, and
`ump.PRIVACY_OPTIONS_STATE_ERROR` if the form could not be shown or completed.
A new show request resets the state to `ump.PRIVACY_OPTIONS_STATE_SHOWING`.

`ump.get_gdpr_applies()` reads the [IAB TCF GDPR applicability value published by UMP](https://developers.google.com/admob/android/privacy/gdpr)
from Android default preferences. Call it in the successful request callback.
It returns `ump.GDPR_APPLIES_NO` or
`ump.GDPR_APPLIES_YES`, or `ump.GDPR_APPLIES_UNKNOWN` if the value is missing or
invalid. This is a consent-flow signal, not a country code or vendor consent.

Note:
More methods are available, i didn't have the time to finish writing up this readme, sorry.
check the codebase or the example usage below.


## Example usage

```
local function update_consent()
    local test_device = true
    local test_device_id = "YOUR_TEST_DEVICE_HASH_ID"
    
    ump.request_consent_info_update(test_device, test_device_id, function(self, success)
        if success and ump.can_request_ads() then
            print("Ads can be requested now.")
        else
            print("Ads cannot be requested.")
        end
        print("Consent status: " .. ump.get_consent_status())
    end)
end

```
This example initializes consent info update, checks if privacy options are required, and retrieves the consent status. You can then use this information to manage your ads display logic.
