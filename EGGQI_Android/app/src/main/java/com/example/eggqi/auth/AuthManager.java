package com.example.eggqi.auth;
// AuthManager.java
import android.content.Context;
import android.content.SharedPreferences;

public class AuthManager {
    private static final String PREF_NAME = "auth_prefs";
    private static final String KEY_LOGGED_IN = "logged_in";
    private static final String KEY_LOGGED_IN_USER_EMAIL = "logged_in_user_email";
    private static final String KEY_LOGGED_IN_USER_NAME = "logged_in_user_name";
    public static void saveLoginStatus(Context context, boolean isLoggedIn, String userEmail, String userName) {
        SharedPreferences preferences = context.getApplicationContext().getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean(KEY_LOGGED_IN, isLoggedIn);
        editor.putString(KEY_LOGGED_IN_USER_EMAIL, userEmail);
        editor.putString(KEY_LOGGED_IN_USER_NAME, userName);
        editor.apply();
    }
    public static boolean isLoggedIn(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        return preferences.getBoolean(KEY_LOGGED_IN, false);
    }
    public static String getLoggedInUserEmail(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        return preferences.getString(KEY_LOGGED_IN_USER_EMAIL, "");
    }
    public static String getLoggedInUserName(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        return preferences.getString(KEY_LOGGED_IN_USER_NAME, "");
    }
    public static void clearLoginStatus(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean(KEY_LOGGED_IN, false);
        editor.remove(KEY_LOGGED_IN_USER_EMAIL);
        editor.apply();
    }
}
