package com.example.eggqi.auth;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.eggqi.R;

import java.util.Arrays;
import java.util.List;
public class LoginActivity extends AppCompatActivity {
    private EditText emailEditText, passwordEditText;
    private Button loginButton;
    private TextView signUpButton, message;
    private static final int BLUETOOTH_RESULT_CODE = 5;
    private static final int DANGEROUS_RESULT_CODE = 1;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        emailEditText = findViewById(R.id.username);
        passwordEditText = findViewById(R.id.password);
        loginButton = findViewById(R.id.login);
        signUpButton = findViewById(R.id.signup);
        message = findViewById(R.id.message);



        loginButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                login();
            }
        });

        signUpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent(LoginActivity.this, SignupActivity.class));
                defaultMessage();
            }
        });

        if (ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{android.Manifest.permission.BLUETOOTH},
                    BLUETOOTH_RESULT_CODE);
        }
        else if(ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED){
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                ActivityCompat.requestPermissions(this,
                        new String[]{android.Manifest.permission.BLUETOOTH_CONNECT},
                        DANGEROUS_RESULT_CODE);
            }
        }
    }

    @Override
    public void onBackPressed() {
        finishAffinity();
    }
    private void defaultMessage(){
        message.setText("Enter email and password.");
    }

    private void errorMessage(String s){
        message.setText(s);
        message.setTextColor(0xFFDD3333);
        Animation anim = new AlphaAnimation(0.0f, 1.0f);
        anim.setDuration(50);
        anim.setStartOffset(20);
        message.startAnimation(anim);
    }
    private void emptyField(EditText edittext){
        edittext.setHintTextColor(0xAADD3333);
        Animation anim = new AlphaAnimation(0.5f, 1.0f);
        anim.setDuration(800);
        anim.setStartOffset(20);
        edittext.startAnimation(anim);
    }


    private void login() {
        String enteredEmail = emailEditText.getText().toString().trim();
        String enteredPassword = passwordEditText.getText().toString().trim();

        if (TextUtils.isEmpty(enteredEmail) || TextUtils.isEmpty(enteredPassword)) {
            errorMessage("You must fill in all the fields.");
            if(TextUtils.isEmpty(enteredEmail))
                emptyField(emailEditText);
            if(TextUtils.isEmpty(enteredPassword))
                emptyField(passwordEditText);
            return;
        }

        SharedPreferences preferences = getSharedPreferences("user_credentials", Context.MODE_PRIVATE);
        String userListString = preferences.getString("user_list", "");

        if (TextUtils.isEmpty(userListString)) {
            errorMessage("Incorrect email or password.");
            return;
        }

        List<String> userList = Arrays.asList(userListString.split(","));

        if (userList.contains(enteredEmail)) {
            String savedPassword = preferences.getString("password_" + enteredEmail, "");
            String userName = preferences.getString("name_" + enteredEmail, "");
            Log.d("AuthManager", "Username: "+userName);
            if (enteredPassword.equals(savedPassword)) {
                AuthManager.saveLoginStatus(this, true, enteredEmail, userName);

                Intent resultIntent = new Intent();

                resultIntent.putExtra("isLoggedIn", true);
                setResult(RESULT_OK, resultIntent);
                InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(emailEditText.getWindowToken(), 0);
                imm.hideSoftInputFromWindow(passwordEditText.getWindowToken(), 0);

                finish();
            } else {
                errorMessage("Incorrect email or password.");
            }
        } else {
            errorMessage("Incorrect email or password.");
        }
    }
}
