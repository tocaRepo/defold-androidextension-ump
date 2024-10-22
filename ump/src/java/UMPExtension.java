package com.defold.umpext;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.os.Vibrator;
import android.content.Context;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;

import com.google.android.ump.ConsentDebugSettings;
import com.google.android.ump.ConsentInformation;
import com.google.android.ump.ConsentRequestParameters;
import com.google.android.ump.ConsentInformation.PrivacyOptionsRequirementStatus;
import com.google.android.ump.UserMessagingPlatform;
public class UMPExtension {

    private static final String TAG = "UMPExtension";
    private static ConsentInformation consentInformation;
    private static boolean isMobileAdsInitializeCalled = false;

    // Request consent info update
    public static void requestConsentInfoUpdate(Activity activity, boolean testDevice, String testDeviceHashedId) {
        ConsentRequestParameters.Builder paramsBuilder = new ConsentRequestParameters.Builder();

        if (testDevice) {
            // Enable debug mode with test device ID
            ConsentDebugSettings debugSettings = new ConsentDebugSettings.Builder(activity)
                    .setDebugGeography(ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_EEA)
                    .addTestDeviceHashedId(testDeviceHashedId)
                    .build();
            paramsBuilder.setConsentDebugSettings(debugSettings);
        }

        consentInformation = UserMessagingPlatform.getConsentInformation(activity);
        consentInformation.requestConsentInfoUpdate(
            activity,
            paramsBuilder.build(),
            () -> {
                // Callback after the consent info update is finished
                Log.d(TAG, "Consent info update successful.");
                loadAndShowConsentFormIfRequired(activity);
            },
            formError -> {
                // Handle the error
                Log.e(TAG, "Consent info update failed: " + formError.getMessage());
            }
        );
    }

    // Load and show the consent form if required
    private static void loadAndShowConsentFormIfRequired(Activity activity) {
        UserMessagingPlatform.loadAndShowConsentFormIfRequired(
            activity,
            formError -> {
                if (formError != null) {
                    // Handle the form load error
                    Log.e(TAG, "Consent form load error: " + formError.getMessage());
                } else {
                    Log.d(TAG, "Consent form successfully presented.");
                }
            }
        );
    }

    // Check if privacy options entry point is required
    public static boolean isPrivacyOptionsRequired() {
        return consentInformation.getPrivacyOptionsRequirementStatus()
                == PrivacyOptionsRequirementStatus.REQUIRED;
    }

    // Show privacy options form
    public static void showPrivacyOptionsForm(Activity activity) {
        // Run on the main thread
        activity.runOnUiThread(() -> {
            UserMessagingPlatform.showPrivacyOptionsForm(activity, formDismissedError -> {
                if (formDismissedError != null) {
                    Log.e(TAG, "Error showing privacy options form: " + formDismissedError.getMessage());
                } else {
                    Log.d(TAG, "Privacy options form dismissed successfully.");
                }
            });
        });
    }

    // Check if ads can be requested
    public static boolean canRequestAds() {
        if (consentInformation != null) {
            return consentInformation.canRequestAds();
        }
        return false;
    }

    // Reset consent information (for testing)
    public static void resetConsentInformation() {
        if (consentInformation != null) {
            consentInformation.reset();
        }
    }

    // Get consent status method
    public static int getConsentStatus() {
        if (consentInformation != null) {
            return consentInformation.getConsentStatus();
        }
        return -1; // Return -1 for an unknown status
    }

 
}
