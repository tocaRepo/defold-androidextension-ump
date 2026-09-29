#!/usr/bin/env python3
"""Run the production Java consent flow against deterministic SDK facades.

Only the native completion declaration is replaced with a recorder. This checks
ordering and request lifetimes without an Android device or JNI library.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap

SOURCE = (Path(sys.argv[1]) if len(sys.argv) > 1 else
          Path(__file__).resolve().parents[1] / "ump/src/java/UMPExtension.java")

STUBS = {
    "android/content/Context.java": """
        package android.content;
        public class Context {}
    """,
    "android/app/Activity.java": """
        package android.app;
        public class Activity extends android.content.Context {
            public void runOnUiThread(Runnable task) { task.run(); }
            public android.content.Context getApplicationContext() { return this; }
        }
    """,
    "android/content/SharedPreferences.java": """
        package android.content;
        public interface SharedPreferences { java.util.Map<String, ?> getAll(); }
    """,
    "android/preference/PreferenceManager.java": """
        package android.preference;
        public class PreferenceManager {
            public static android.content.SharedPreferences getDefaultSharedPreferences(android.content.Context context) {
                return () -> java.util.Collections.emptyMap();
            }
        }
    """,
    "android/util/Log.java": """
        package android.util;
        public class Log {
            public static int d(String tag, String message) { return 0; }
            public static int e(String tag, String message) { return 0; }
        }
    """,
    "com/google/android/ump/FormError.java": """
        package com.google.android.ump;
        public class FormError { public String getMessage() { return "test failure"; } }
    """,
    "com/google/android/ump/ConsentDebugSettings.java": """
        package com.google.android.ump;
        public class ConsentDebugSettings {
            public static class DebugGeography { public static final int DEBUG_GEOGRAPHY_EEA = 1; }
            public static class Builder {
                public Builder(android.app.Activity activity) {}
                public Builder setDebugGeography(int value) { return this; }
                public Builder addTestDeviceHashedId(String value) { return this; }
                public ConsentDebugSettings build() { return new ConsentDebugSettings(); }
            }
        }
    """,
    "com/google/android/ump/ConsentRequestParameters.java": """
        package com.google.android.ump;
        public class ConsentRequestParameters {
            public static class Builder {
                public Builder setConsentDebugSettings(ConsentDebugSettings value) { return this; }
                public ConsentRequestParameters build() { return new ConsentRequestParameters(); }
            }
        }
    """,
    "com/google/android/ump/ConsentInformation.java": """
        package com.google.android.ump;
        public class ConsentInformation {
            public static class ConsentStatus { public static final int REQUIRED = 2; }
            public enum PrivacyOptionsRequirementStatus { NOT_REQUIRED, REQUIRED }
            public interface OnConsentInfoUpdateSuccessListener { void onConsentInfoUpdateSuccess(); }
            public interface OnConsentInfoUpdateFailureListener { void onConsentInfoUpdateFailure(FormError error); }
            public int status;
            public boolean allowed;
            public OnConsentInfoUpdateSuccessListener success;
            public OnConsentInfoUpdateFailureListener failure;
            public void requestConsentInfoUpdate(android.app.Activity activity, ConsentRequestParameters params,
                    OnConsentInfoUpdateSuccessListener onSuccess, OnConsentInfoUpdateFailureListener onFailure) {
                success = onSuccess; failure = onFailure;
            }
            public int getConsentStatus() { return status; }
            public boolean canRequestAds() { return allowed; }
            public PrivacyOptionsRequirementStatus getPrivacyOptionsRequirementStatus() {
                return PrivacyOptionsRequirementStatus.REQUIRED;
            }
            public void reset() { status = 0; allowed = false; }
        }
    """,
    "com/google/android/ump/UserMessagingPlatform.java": """
        package com.google.android.ump;
        public class UserMessagingPlatform {
            public interface DismissedListener { void dismissed(FormError error); }
            public static ConsentInformation info;
            public static DismissedListener form;
            public static DismissedListener privacy;
            public static int formsShown;
            public static ConsentInformation getConsentInformation(android.app.Activity activity) { return info; }
            public static void loadAndShowConsentFormIfRequired(android.app.Activity activity, DismissedListener callback) {
                if (info.status != ConsentInformation.ConsentStatus.REQUIRED) { callback.dismissed(null); return; }
                formsShown++; form = callback;
            }
            public static void showPrivacyOptionsForm(android.app.Activity activity, DismissedListener callback) {
                privacy = callback;
            }
        }
    """,
    "com/defold/umpext/TestConsentFlow.java": """
        package com.defold.umpext;
        import android.app.Activity;
        import com.google.android.ump.*;
        import java.util.ArrayList;
        import java.util.List;

        public class TestConsentFlow {
            static final Activity activity = new Activity();
            static final List<String> events = new ArrayList<>();
            static int passed;
            static void record(int operation, int id, boolean success) {
                events.add(operation + "/" + id + "/" + success);
            }
            static void check(boolean condition) { if (!condition) throw new AssertionError(events.toString()); }
            static void expect(String... expected) { check(events.equals(java.util.Arrays.asList(expected))); }
            static void reset() throws Exception {
                var field = UMPExtension.class.getDeclaredField("consentInformation");
                field.setAccessible(true); field.set(null, null);
                UserMessagingPlatform.info = new ConsentInformation();
                UserMessagingPlatform.formsShown = 0;
                UserMessagingPlatform.form = null;
                UserMessagingPlatform.privacy = null;
                events.clear();
            }
            static void update(int id, boolean deferred, boolean required) {
                UMPExtension.requestConsentInfoUpdate(activity, false, "", id, deferred);
                check(UMPExtension.getRequestState() == 0);
                UserMessagingPlatform.info.status = required ? 2 : 1;
                UserMessagingPlatform.info.success.onConsentInfoUpdateSuccess();
            }
            static void pass(String name) { passed++; System.out.println("PASS " + name); }
            public static void main(String[] args) throws Exception {
                reset(); update(1, true, true);
                expect("3/1/true");
                check(UMPExtension.getRequestState() == 3 && UMPExtension.wasConsentFormRequired());
                check(UserMessagingPlatform.formsShown == 0);
                UMPExtension.showConsentFormIfRequired(activity, 1);
                UMPExtension.showConsentFormIfRequired(activity, 1);
                check(UserMessagingPlatform.formsShown == 1);
                expect("3/1/true");
                UserMessagingPlatform.form.dismissed(null);
                expect("3/1/true", "1/1/true");
                check(UMPExtension.getRequestState() == 1);
                pass("deferred required form waits for resume and opens once");

                reset(); update(2, true, false);
                expect("3/2/true"); check(!UMPExtension.wasConsentFormRequired());
                UMPExtension.showConsentFormIfRequired(activity, 2);
                check(UserMessagingPlatform.formsShown == 0 && UMPExtension.getRequestState() == 1);
                expect("3/2/true", "1/2/true");
                pass("deferred no-form flow completes without a window");

                reset(); update(3, false, true);
                expect(); check(UserMessagingPlatform.formsShown == 1);
                UserMessagingPlatform.form.dismissed(null);
                expect("1/3/true");
                pass("automatic required-form flow preserves its completion callback");

                reset(); update(4, false, false);
                check(UserMessagingPlatform.formsShown == 0);
                expect("1/4/true");
                pass("automatic no-form flow remains compatible");

                for (boolean deferred : new boolean[]{true, false}) {
                    reset(); UMPExtension.requestConsentInfoUpdate(activity, false, "", 5, deferred);
                    UserMessagingPlatform.info.failure.onConsentInfoUpdateFailure(new FormError());
                    check(UMPExtension.getRequestState() == 2 && UserMessagingPlatform.formsShown == 0);
                    expect((deferred ? "3" : "1") + "/5/false");
                    pass((deferred ? "deferred" : "automatic") + " information failure completes without a form");
                }

                reset(); update(6, true, true);
                UMPExtension.showConsentFormIfRequired(activity, 6);
                UserMessagingPlatform.form.dismissed(new FormError());
                expect("3/6/true", "1/6/false"); check(UMPExtension.getRequestState() == 2);
                pass("deferred form failure completes with failure");

                reset(); UMPExtension.requestConsentInfoUpdate(activity, false, "", 7, true);
                var oldUpdate = UserMessagingPlatform.info.success;
                UMPExtension.requestConsentInfoUpdate(activity, false, "", 8, true);
                UserMessagingPlatform.info.status = 2;
                oldUpdate.onConsentInfoUpdateSuccess(); expect(); check(UMPExtension.getRequestState() == 0);
                UserMessagingPlatform.info.success.onConsentInfoUpdateSuccess(); expect("3/8/true");
                UMPExtension.showConsentFormIfRequired(activity, 7);
                check(UserMessagingPlatform.formsShown == 0);
                UMPExtension.showConsentFormIfRequired(activity, 8);
                check(UserMessagingPlatform.formsShown == 1);
                pass("superseded update and resume are ignored");

                reset(); update(9, true, true);
                UMPExtension.showConsentFormIfRequired(activity, 9);
                var oldForm = UserMessagingPlatform.form;
                UMPExtension.requestConsentInfoUpdate(activity, false, "", 10, true);
                oldForm.dismissed(null);
                expect("3/9/true"); check(UMPExtension.getRequestState() == 0);
                UserMessagingPlatform.info.success.onConsentInfoUpdateSuccess();
                expect("3/9/true", "3/10/true");
                pass("superseded form cannot complete the new request");

                reset(); update(11, true, false);
                UMPExtension.showConsentFormIfRequired(activity, 11);
                UMPExtension.showPrivacyOptionsForm(activity, 12);
                check(UMPExtension.getPrivacyOptionsState() == 1);
                UserMessagingPlatform.privacy.dismissed(null);
                expect("3/11/true", "1/11/true", "2/12/true");
                check(UMPExtension.getPrivacyOptionsState() == 2);
                pass("privacy options retain their separate callback");
                System.out.println(passed + " Java consent flow checks passed");
            }
        }
    """,
}

declaration = "private static native void onNativeCompletion(int operation, int requestId, boolean success);"
production = SOURCE.read_text()
assert production.count(declaration) == 1
production = production.replace(declaration, """private static void onNativeCompletion(int operation, int requestId, boolean success) {
    TestConsentFlow.record(operation, requestId, success);
}""")

with tempfile.TemporaryDirectory(prefix="ump-consent-") as folder:
    root = Path(folder)
    sources = dict(STUBS)
    sources["com/defold/umpext/UMPExtension.java"] = production
    for name, source in sources.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(source))
    subprocess.run(["javac", "-d", str(root / "classes"),
                    *[str(root / name) for name in sources]], check=True)
    subprocess.run(["java", "-cp", str(root / "classes"), "com.defold.umpext.TestConsentFlow"], check=True)
