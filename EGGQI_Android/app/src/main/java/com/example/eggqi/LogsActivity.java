package com.example.eggqi;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class LogsActivity extends AppCompatActivity {
    Button backBttn;
    TextView logTextView;
    private RecyclerView recyclerView;
    private FileAdapter adapter;
    private List<File> fileList = new ArrayList<>();
    private File selectedFile;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_second);
        recyclerView = findViewById(R.id.recyclerView);
        logTextView = findViewById(R.id.logTextView);
        backBttn = findViewById(R.id.backBttn);

        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        adapter = new FileAdapter(fileList);
        recyclerView.setAdapter(adapter);
        Toolbar toolbar2 = findViewById(R.id.toolbar2);
        toolbar2.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleOnBackPress();
            }
        });
        loadFiles();

        adapter.setOnItemClickListener(new FileAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(File file) {
                selectedFile = file;
                displayFileContents(selectedFile);
            }
        });
        backBttn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });
    }
    private void handleOnBackPress() {
        finish();
    }
    private void loadFiles() {
        try {
            File dir = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS);
            File[] files = dir.listFiles();

            if (files != null) {
                fileList.clear(); // Clear the list before adding files
                for (File file : files) {
                    if (file.isFile() && file.getName().endsWith(".txt")) {
                        fileList.add(file);
                    }
                }
                adapter.notifyDataSetChanged(); // Notify the adapter that the data has changed
            }
        } catch (Exception e) {
            Log.e("FILE", "Error loading files: " + e.getMessage());
        }
    }


    private void displayFileContents(File file) {
        try {
            FileInputStream fileInputStream = new FileInputStream(file);
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(fileInputStream));
            StringBuilder text = new StringBuilder();
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                text.append(line);
                text.append("\n");
            }
            logTextView.setText(text.toString());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}

