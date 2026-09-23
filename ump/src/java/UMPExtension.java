package com.defold.umpext;

import android.app.Activity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

import com.google.android.ump.ConsentDebugSettings;
import com.google.android.ump.ConsentInformation;
import com.google.android.ump.ConsentInformation.PrivacyOptionsRequirementStatus;
import com.google.android.ump.ConsentRequestParameters;
import com.google.android.ump.FormError;
import com.google.android.ump.UserMessagingPlatform;

public class UMPExtension {

    private static final String TAG = "UMPExtension";
    private static ConsentInformation consentInformation;
    // 0 = updating, 1 = update and required form completed, 2 = failed, 3 = form pending.
    private static volatile int requestState = 0;
    // 0 = not shown, 1 = showing, 2 = finished (including a form error).
    private static volatile int privacyOptionsState = 0;

    /**
     * Request consent info update from UMP.
     */
    public static void requestConsentInfoUpdate(Activity activity, boolean testDevice, String testDeviceHashedId) {
        requestState = 0;
        ConsentRequestParameters.Builder paramsBuilder = new ConsentRequestParameters.Builder();

        if (testDevice) {
            // Enable debug mode with test device ID and force EEA geography
            ConsentDebugSettings debugSettings = new ConsentDebugSettings.Builder(activity)
                    .setDebugGeography(ConsentDebugSettings.DebugGeography.DEBUG_GEOGRAPHY_EEA)
                    .addTestDeviceHashedId(testDeviceHashedId)
                    .build();
            paramsBuilder.setConsentDebugSettings(debugSettings);
        }

        if (consentInformation == null) {
            consentInformation = UserMessagingPlatform.getConsentInformation(activity);
        }

        consentInformation.requestConsentInfoUpdate(
                activity,
                paramsBuilder.build(),
                new ConsentInformation.OnConsentInfoUpdateSuccessListener() {
                    @Override
                    public void onConsentInfoUpdateSuccess() {
                        Log.d(TAG, "Consent info update successful.");
                        Log.d(TAG, "Consent status: " + consentInformation.getConsentStatus());
                        requestState = 3;
                        loadAndShowConsentFormIfRequired(activity);
                    }
                },
                new ConsentInformation.OnConsentInfoUpdateFailureListener() {
                    @Override
                    public void onConsentInfoUpdateFailure(FormError formError) {
                        Log.e(TAG, "Consent info update failed: " + formError.getMessage());
                        requestState = 2;
                    }
                }
        );
    }

    /**
     * Load and show the consent form if required.
     * Must run on the main UI thread (UMP 3.1+ requirement).
     */
    private static void loadAndShowConsentFormIfRequired(Activity activity) {
        activity.runOnUiThread(() -> {
            UserMessagingPlatform.loadAndShowConsentFormIfRequired(
                    activity,
                    formError -> {
                        if (formError != null) {
                            Log.e(TAG, "Consent form load error: " + formError.getMessage());
                            requestState = 2;
                        } else {
                            Log.d(TAG, "Consent form request completed.");
                            requestState = 1;
                        }
                    }
            );
        });
    }

    public static int getRequestState() {
        return requestState;
    }

    // UMP writes these IAB TCF keys to default preferences after consent resolution.
    // Missing or unexpected regional data must not be treated as outside the EEA.
    public static int getGdprApplies(Activity activity) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(activity.getApplicationContext());
        Object value = preferences.getAll().get("IABTCF_gdprApplies");
        if (value instanceof Integer && ((Integer) value == 0 || (Integer) value == 1)) {
            return (Integer) value;
        }
        if ("0".equals(value) || "1".equals(value)) {
            return Integer.parseInt((String) value);
        }
        return -1;
    }

    /**
     * Returns true if privacy options entry point should be shown.
     */
    public static boolean isPrivacyOptionsRequired() {
        if (consentInformation == null) return false;
        return consentInformation.getPrivacyOptionsRequirementStatus()
                == PrivacyOptionsRequirementStatus.REQUIRED;
    }

    /**
     * Show the privacy options form.
     */
    public static void showPrivacyOptionsForm(Activity activity) {
        privacyOptionsState = 1;
        if (consentInformation == null) {
            consentInformation = UserMessagingPlatform.getConsentInformation(activity);
        }

        activity.runOnUiThread(() -> {
            UserMessagingPlatform.showPrivacyOptionsForm(
                    activity,
                    formDismissedError -> {
                        if (formDismissedError != null) {
                            Log.e(TAG, "Error showing privacy options form: " + formDismissedError.getMessage());
                        } else {
                            Log.d(TAG, "Privacy options form dismissed successfully.");
                        }
                        privacyOptionsState = 2;
                    }
            );
        });
    }

    public static int getPrivacyOptionsState() {
        return privacyOptionsState;
    }

    /**
     * Whether ads can be requested based on consent state.
     */
    public static boolean canRequestAds() {
        return consentInformation != null && consentInformation.canRequestAds();
    }

    /**
     * Reset consent information (for testing or debug builds).
     */
    public static void resetConsentInformation() {
        if (consentInformation != null) {
            consentInformation.reset();
            Log.d(TAG, "Consent information has been reset.");
        }
    }

    /**
     * Get numeric consent status.
     * Returns -1 if consent information is unavailable.
     */
    public static int getConsentStatus() {
        if (consentInformation != null) {
            return consentInformation.getConsentStatus();
        }
        return -1;
    }
}
