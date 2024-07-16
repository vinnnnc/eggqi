package com.example.eggqi;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

public class SettingsActivity extends AppCompatActivity {
    int configs[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};

    private int[] editSettingList = {
            R.id.editSetting1, R.id.editSetting2, R.id.editSetting3, R.id.editSetting4,R.id.editSetting5,R.id.editSetting6,
            R.id.editSetting7, R.id.editSetting8, R.id.editSetting9, R.id.editSetting10,R.id.editSetting11,R.id.editSetting12,
            R.id.editSetting13
    };
    private Button saveBttn, cancelBttn;

    private void handleOnBackPress() {
        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        Bundle myBundle = getIntent().getExtras();
        configs = myBundle.getIntArray("configs");

        saveBttn = findViewById(R.id.saveBttn);
        cancelBttn = findViewById(R.id.cancelBttn);
        for (int i = 0; i < 13; i++){
            EditText editText = findViewById(editSettingList[i]);
            editText.setText(String.valueOf(configs[i]));
        }
        Toolbar toolbar3 = findViewById(R.id.toolbar3);
        toolbar3.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleOnBackPress();
            }
        });

        saveBttn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                for (int i = 0; i < 13; i++){
                    EditText editText = findViewById(editSettingList[i]);
                    configs[i] = Integer.parseInt(String.valueOf(editText.getText()));
                }
                Intent resultIntent = new Intent();
                resultIntent.putExtra("resultConfig", configs);
                setResult(RESULT_OK, resultIntent);
                finish();
            }
        });
        cancelBttn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });

    }
}