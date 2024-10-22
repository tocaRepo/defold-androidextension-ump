# UMP Defold Extension

This Defold extension allows you to integrate Google's **User Messaging Platform (UMP)** into your Defold game to manage user consent for ads on Android. It helps you request consent information, show the consent form, and check the user's consent status.

## Warning

**Disclaimer:** I am not responsible for any issues, errors, or malfunctions caused by the implementation or the usage of this extension. Use at your own risk. Please ensure you understand the methods and make the necessary modifications to fit your specific project requirements.

## Setup

To use this extension, you need to add your **AdMob App ID** in the game project file under the `[ump]` section. Here's an example of how to do it:

```
[ump]
app_id_android = ca-app-pub-2112345332~1458002511
```

Make sure to replace the example App ID (ca-app-pub-2112345332~1458002511) with your actual AdMob App ID.

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

Note:
More methods are available, i didn't have the time to finish writing up this readme, sorry.
check the codebase or the example usage below.


## Example usage

```
local function update_consent()
    local test_device = true
    local test_device_id = "YOUR_TEST_DEVICE_HASH_ID"
    
    ump.request_consent_info_update(test_device, test_device_id)

    if ump.is_privacy_options_required() then
        ump.show_privacy_options_form()
    end
    
    if ump.can_request_ads() then
        print("Ads can be requested now.")
    else
        print("Ads cannot be requested yet.")
    end

    local consent_status = ump.get_consent_status()
    print("Consent status: " .. consent_status)
end

```
This example initializes consent info update, checks if privacy options are required, and retrieves the consent status. You can then use this information to manage your ads display logic.