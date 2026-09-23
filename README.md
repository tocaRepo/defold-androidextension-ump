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
This method updates the consent information. You can pass a testDevice flag and a testDeviceHashedId for testing purposes.

Usage:


```
ump.request_consent_info_update(testDevice, testDeviceHashedId)

```

The request is asynchronous. Poll `ump.get_request_state()` until it returns `1`
(completed) or `2` (failed). It returns `0` while the consent information
update is running and `3` while a required form is loading or showing.
Check `ump.can_request_ads()` only after the request reaches state `1`.

Note:
More methods are available, i didn't have the time to finish writing up this readme, sorry.
check the codebase or the example usage below.


## Example usage

```
local function update_consent()
    local test_device = true
    local test_device_id = "YOUR_TEST_DEVICE_HASH_ID"
    
    ump.request_consent_info_update(test_device, test_device_id)
    timer.delay(0.1, true, function(_, handle)
        local state = ump.get_request_state()
        if state == 1 or state == 2 then
            timer.cancel(handle)
            if state == 1 and ump.can_request_ads() then
                print("Ads can be requested now.")
            else
                print("Ads cannot be requested.")
            end
            print("Consent status: " .. ump.get_consent_status())
        end
    end)
end

```
This example initializes consent info update, checks if privacy options are required, and retrieves the consent status. You can then use this information to manage your ads display logic.
