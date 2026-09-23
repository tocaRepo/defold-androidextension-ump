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
    // Keep these values in sync with the Lua constants registered in extension.cpp.
    private static final int REQUEST_STATE_UPDATING = 0;
    private static final int REQUEST_STATE_COMPLETE = 1;
    private static final int REQUEST_STATE_FAILED = 2;
    private static final int REQUEST_STATE_FORM_PENDING = 3;
    private static final int PRIVACY_OPTIONS_STATE_NOT_SHOWN = 0;
    private static final int PRIVACY_OPTIONS_STATE_SHOWING = 1;
    private static final int PRIVACY_OPTIONS_STATE_DISMISSED = 2;
    private static final int PRIVACY_OPTIONS_STATE_ERROR = 3;
    private static final int GDPR_APPLIES_UNKNOWN = -1;
    private static final int GDPR_APPLIES_NO = 0;
    private static final int GDPR_APPLIES_YES = 1;

    private static ConsentInformation consentInformation;
    private static volatile int requestState = REQUEST_STATE_UPDATING;
    private static volatile boolean requiredFormOnUpdate = false;
    private static volatile int privacyOptionsState = PRIVACY_OPTIONS_STATE_NOT_SHOWN;

    /**
     * Request consent info update from UMP.
     */
    public static void requestConsentInfoUpdate(Activity activity, boolean testDevice, String testDeviceHashedId) {
        requestState = REQUEST_STATE_UPDATING;
        requiredFormOnUpdate = false;
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
                        requiredFormOnUpdate = consentInformation.getConsentStatus()
                                == ConsentInformation.ConsentStatus.REQUIRED;
                        requestState = REQUEST_STATE_FORM_PENDING;
                        loadAndShowConsentFormIfRequired(activity);
                    }
                },
                new ConsentInformation.OnConsentInfoUpdateFailureListener() {
                    @Override
                    public void onConsentInfoUpdateFailure(FormError formError) {
                        Log.e(TAG, "Consent info update failed: " + formError.getMessage());
                        requestState = REQUEST_STATE_FAILED;
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
                            requestState = REQUEST_STATE_FAILED;
                        } else {
                            Log.d(TAG, "Consent form request completed.");
                            requestState = REQUEST_STATE_COMPLETE;
                        }
                    }
            );
        });
    }

    public static int getRequestState() {
        return requestState;
    }

    public static boolean wasConsentFormRequired() {
        return requiredFormOnUpdate;
    }

    // UMP writes these IAB TCF keys to default preferences after consent resolution.
    // Missing or unexpected regional data must not be treated as outside the EEA.
    public static int getGdprApplies(Activity activity) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(activity.getApplicationContext());
        Object value = preferences.getAll().get("IABTCF_gdprApplies");
        if (value instanceof Integer && ((Integer) value == GDPR_APPLIES_NO || (Integer) value == GDPR_APPLIES_YES)) {
            return (Integer) value;
        }
        if ("0".equals(value) || "1".equals(value)) {
            return Integer.parseInt((String) value);
        }
        return GDPR_APPLIES_UNKNOWN;
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
        privacyOptionsState = PRIVACY_OPTIONS_STATE_SHOWING;
        if (consentInformation == null) {
            consentInformation = UserMessagingPlatform.getConsentInformation(activity);
        }

        activity.runOnUiThread(() -> {
            UserMessagingPlatform.showPrivacyOptionsForm(
                    activity,
                    formDismissedError -> {
                        if (formDismissedError != null) {
                            Log.e(TAG, "Error showing privacy options form: " + formDismissedError.getMessage());
                            privacyOptionsState = PRIVACY_OPTIONS_STATE_ERROR;
                        } else {
                            Log.d(TAG, "Privacy options form dismissed successfully.");
                            privacyOptionsState = PRIVACY_OPTIONS_STATE_DISMISSED;
                        }
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
