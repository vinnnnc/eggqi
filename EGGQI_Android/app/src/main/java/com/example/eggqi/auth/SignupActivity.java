package com.example.eggqi.auth;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.text.TextUtils;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.example.eggqi.R;

public class SignupActivity extends AppCompatActivity {

    private EditText nameEditText, emailEditText, passwordEditText, confirmPasswordEditText;
    private Button signUpButton;
    private TextView loginButton, message;
    private LinearLayout successLayout, fieldLayout;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_signup);

        fieldLayout = findViewById(R.id.fieldLayout);
        successLayout = findViewById(R.id.successLayout);
        message = findViewById(R.id.message);
        nameEditText = findViewById(R.id.name);
        emailEditText = findViewById(R.id.username);
        passwordEditText = findViewById(R.id.password);
        confirmPasswordEditText = findViewById(R.id.password2);
        signUpButton = findViewById(R.id.signup);

        loginButton = findViewById(R.id.login);

        signUpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                signUp();
            }
        });

        loginButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                finish();
            }
        });
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

    private void signUp() {
        String name = nameEditText.getText().toString().trim();
        String email = emailEditText.getText().toString().trim();
        String password = passwordEditText.getText().toString().trim();
        String confirmPassword = confirmPasswordEditText.getText().toString().trim();



        if(TextUtils.isEmpty(name)){
            emptyField(nameEditText);
        }
        if(TextUtils.isEmpty(email)){
            emptyField(emailEditText);
        }
        if(TextUtils.isEmpty(password)){
            emptyField(passwordEditText);
        }
        if(TextUtils.isEmpty(confirmPassword)){
            emptyField(confirmPasswordEditText);
        }

        if (TextUtils.isEmpty(name) || TextUtils.isEmpty(email) || TextUtils.isEmpty(password) || TextUtils.isEmpty(confirmPassword)) {
            errorMessage("You must fill in all the fields.");
            return;
        }

        if (!EmailValidator.isValidEmail(email)) {
            emptyField(emailEditText);
            errorMessage("Invalid email address.");
            return;
        }

        if (password.length() < 6) {
            errorMessage("Password is too short (must be atleast 6 characters).");
            return;
        }

        if (!password.equals(confirmPassword)) {
            errorMessage("Passwords do not match.");
            emptyField(confirmPasswordEditText);
            return;
        }

        InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(nameEditText.getWindowToken(), 0);
        imm.hideSoftInputFromWindow(emailEditText.getWindowToken(), 0);
        imm.hideSoftInputFromWindow(passwordEditText.getWindowToken(), 0);
        imm.hideSoftInputFromWindow(confirmPasswordEditText.getWindowToken(), 0);

        message.setText("");
        loginButton.setEnabled(false);
        signUpButton.setEnabled(false);
        fieldLayout.setVisibility(View.GONE);
        successLayout.setVisibility(View.VISIBLE);
        Animation anim = new AlphaAnimation(0.5f, 1.0f);
        anim.setDuration(500);
        anim.setStartOffset(0);
        successLayout.startAnimation(anim);


        new CountDownTimer(2000, 1000) {

            public void onTick(long millisUntilFinished) {
            }
            public void onFinish() {
                saveCredentials(name, email, password);
                Toast.makeText(SignupActivity.this, "Your account has been created successfully!", Toast.LENGTH_SHORT);
                finish();
            }
        }.start();
    }

    private void saveCredentials(String name, String email, String password) {
        SharedPreferences preferences = getSharedPreferences("user_credentials", Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = preferences.edit();

        String userListString = preferences.getString("user_list", "");
        StringBuilder updatedUserList = new StringBuilder(userListString);

        if (updatedUserList.length() > 0) {
            updatedUserList.append(",");
        }
        updatedUserList.append(email);

        editor.putString("user_list", updatedUserList.toString());

        editor.putString("name_" + email, name);
        editor.putString("email_" + email, email);
        editor.putString("password_" + email, password);

        editor.apply();
    }

}

