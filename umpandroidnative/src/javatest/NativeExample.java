package com.defold.umpandroidnativeext;

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
public class NativeExample {

    private static final String TAG = "NativeExample";
    private static ConsentInformation consentInformation;
    private static boolean isMobileAdsInitializeCalled = false;

    // Vibrate method from original code
    public static final void vibratePhone(Context context, int vibrateMilliSeconds) {
        Vibrator vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        vibrator.vibrate(vibrateMilliSeconds);
    }

    // Simple message method from original code
    public static String DoStuff() {
        return "Message From Java!";
    }

    // Method to read raw resource (example from original code)
    public static String GetRaw(Context context) {
        try {
            InputStream inputStream = context.getResources().openRawResource(R.raw.test);
            InputStreamReader inputreader = new InputStreamReader(inputStream);
            BufferedReader buffreader = new BufferedReader(inputreader);
            StringBuilder text = new StringBuilder();
            String line;
            while ((line = buffreader.readLine()) != null) {
                text.append(line);
                text.append('\n');
            }
            return text.toString();
        } catch (Exception e) {
            return "io exception: " + e.getMessage();
        }
    }

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
        UserMessagingPlatform.showPrivacyOptionsForm(activity, formDismissedError -> {
            if (formDismissedError != null) {
                Log.e(TAG, "Error showing privacy options form: " + formDismissedError.getMessage());
            } else {
                Log.d(TAG, "Privacy options form dismissed successfully.");
            }
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
}
