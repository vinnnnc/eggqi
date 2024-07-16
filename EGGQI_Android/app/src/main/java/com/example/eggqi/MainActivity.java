package com.example.eggqi;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.eggqi.auth.AuthManager;
import com.example.eggqi.auth.LoginActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "BluetoothApp";
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // SPP UUID
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSocket bluetoothSocket;
    private OutputStream outputStream;
    private InputStream inputStream;
    private TextView topText, loggedInUser;
    public int[] eggTextViewList = {
            R.id.eggA1, R.id.eggA2, R.id.eggA3, R.id.eggA4, R.id.eggA5, R.id.eggA6,
            R.id.eggB1, R.id.eggB2, R.id.eggB3, R.id.eggB4, R.id.eggB5, R.id.eggB6,
            R.id.eggC1, R.id.eggC2, R.id.eggC3, R.id.eggC4, R.id.eggC5, R.id.eggC6,
            R.id.eggD1, R.id.eggD2, R.id.eggD3, R.id.eggD4, R.id.eggD5, R.id.eggD6,
            R.id.eggE1, R.id.eggE2, R.id.eggE3, R.id.eggE4, R.id.eggE5, R.id.eggE6,
    };
    private Button startBttn;
    private Switch switchMode;
    public ArrayList<String> eggPos = new ArrayList<>();
    public int eggValuesList[] = new int[30];
    public int eggRawValues[] = new int[30];
    public int mode = 0;
    private String eggMode = "Egg Quality";
    private static final String ESP32_CAM_ADDRESS = "E0:5A:1B:A5:B4:66";
    private Thread workerThread;
    private byte[] readBuffer;
    private int readBufferPosition = 0;
    private boolean connected = false, isSending = false;
    private Handler handler = new Handler(Looper.getMainLooper());
    char[] letters;
    private static final int BLUETOOTH_RESULT_CODE = 5;
    private static final int DANGEROUS_RESULT_CODE = 1;
    int configs[] = {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
    private boolean isLoggedIn = true;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        loggedInUser = findViewById(R.id.loggedInUser);

        if (!AuthManager.isLoggedIn(this)) {
            isLoggedIn = false;
            logIn();
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        startBttn = findViewById(R.id.saveBttn);
        topText = findViewById(R.id.topText);
        switchMode = findViewById(R.id.switchMode);
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        readBuffer = new byte[4096];
        letters = "ABCDEF".toCharArray();

        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 6; j++) {
                String temp = letters[i] + String.valueOf(j + 1);
                eggPos.add(temp);
            }
        }

        disableUI();
        for (int i = 0; i < 30; i++) {
            int eggNum = i;
            TextView temp = findViewById(eggTextViewList[i]);
            temp.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (eggValuesList[0] < 0 || eggValuesList[0] > 100) {
                        Log.d("Egg Value Display", "Values are empty");
                        return;
                    }
                    String valuesString = "EGG " + eggPos.get(eggNum) + " : " + eggRawValues[eggNum];
                    topText.setText(valuesString);

                    Log.d("Egg Value Display", "Values set as " + eggMode);
                }
            });
        }

        switchMode.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked)
                    sendData("MODE:AGE");
                else
                    sendData("MODE:QLT");
                for (int i = 0; i < 30; i++) {
                    TextView eggTextView = findViewById(eggTextViewList[i]);
                    if (isChecked) {
                        mode = 1;
                        eggMode = "Egg Age";

                    } else {
                        mode = 0;
                        eggMode = "Egg Quality";
                    }
                    eggTextView.setTextColor(Color.WHITE);
                }
                Log.d("Identifier Mode", "Set to " + eggMode);
            }
        });

        startBttn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendData("GET");
            }
        });

        if(isLoggedIn) {
            startConnectionAttempts();
        }
    }
    public void onResume() {
        super.onResume();
        if(isLoggedIn){
            startConnectionAttempts();
            String loggedInUserName = AuthManager.getLoggedInUserName(this);
            loggedInUser.setText(loggedInUserName);
        }
    }
    private void logIn(){
        Intent intent = new Intent(this, LoginActivity.class);
        startActivityForResult(intent, 2);
    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == 1) {
            if (resultCode == RESULT_OK) {
                configs = data.getIntArrayExtra("resultConfig");
                String s = "CONF:";
                for (int i = 0; i < 13; i++) {
                    s += String.valueOf(configs[i]);
                    if (i < 12)
                        s += ",";
                }
                Log.d(TAG, s);
                sendData(s);
            }
        } else if(requestCode == 2){
            isLoggedIn = data.getBooleanExtra("isLoggedIn", false);
        }
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_logs) {
            Intent intent = new Intent(this, LogsActivity.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.action_settings) {
            if (connected){
                int requestCode = 1; // You can use any unique request code
                Intent intent = new Intent(this, SettingsActivity.class);
                intent.putExtra("configs", configs);
                startActivityForResult(intent, requestCode);
            } else
                showToast("Device is not connected!");
            return true;
        } else if (id == R.id.action_save) {
            if (connected)
                sendData("SAVE");
            else
                showToast("Device is not connected!");
            return true;
        } else if (id == R.id.action_sync) {
            if (connected)
                sendData("LOGS");
            else
                showToast("Device is not connected!");
            return true;
        }else if (id == R.id.action_instructions) {
            Intent intent = new Intent(this, InstructionsActivity.class);
            startActivity(intent);
            return true;
        } else if (id == R.id.action_logout) {
            AuthManager.clearLoginStatus(this);
            isLoggedIn = false;
            logIn();
            return true;
        } else {
            return super.onOptionsItemSelected(item);
        }
    }

    private void sendData(String message) {
        isSending = true;
        try {
            outputStream.write(message.getBytes());
//            showToast("Sent: " + message);
        } catch (IOException e) {
            Log.e(TAG, "Error sending data", e);
//            showToast("Error!");
            connected = false;
            disableUI();
        }
        isSending = false;
    }
    private void startConnectionAttempts() {
        if (ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{android.Manifest.permission.BLUETOOTH},
                    BLUETOOTH_RESULT_CODE);
        } else if (ContextCompat.checkSelfPermission(this,
                android.Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                ActivityCompat.requestPermissions(this,
                        new String[]{android.Manifest.permission.BLUETOOTH_CONNECT},
                        DANGEROUS_RESULT_CODE);
            }
        }


        if (!connected && isLoggedIn){
            AsyncTask<Void, Void, Boolean> BTConnection = new ConnectBluetoothTask();
            BTConnection.execute();
        }
        if (!isSending && connected && isLoggedIn)
            sendData("B");
    }
    private class ConnectBluetoothTask extends AsyncTask<Void, Void, Boolean> {
        @SuppressLint("MissingPermission")
        @Override
        protected Boolean doInBackground(Void... params) {
            try {
                if(!isLoggedIn){
                    Log.e(TAG, "User is logged out.");
                    return false;
                }
                if(!connected && isLoggedIn){
                    BluetoothDevice device = bluetoothAdapter.getRemoteDevice(ESP32_CAM_ADDRESS);
                    bluetoothSocket = device.createRfcommSocketToServiceRecord(MY_UUID);
                    bluetoothSocket.connect();
                    outputStream = bluetoothSocket.getOutputStream();
                    inputStream = bluetoothSocket.getInputStream();
                    connected = true;
                    startWorkerThread();
                    showToast("Connected");
                }
                return true;
            } catch (IOException e) {
                connected = false;
                Log.e(TAG, "Bluetooth connection failed.", e);
                return false;
            }
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if (result) {
                enableUI();
            } else {
                disableUI();
//                startConnectionAttempts();
            }

            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    // Schedule the next connection attempt
                    startConnectionAttempts();
                }
            }, 3000); // Adjust the delay as needed
        }
    }
    private void startWorkerThread() {
        workerThread = new Thread(new Runnable() {
            public void run() {
                while (!Thread.currentThread().isInterrupted()) {
                    try {
                        int bytesAvailable = inputStream.available();
                        if (bytesAvailable > 0) {
                            byte[] packetBytes = new byte[bytesAvailable];
                            inputStream.read(packetBytes);
                            for (int i = 0; i < bytesAvailable; i++) {
                                byte b = packetBytes[i];
                                if ((b == '\n' &&  readBuffer[0] != 'L')||(b == '\0' &&  readBuffer[0] == 'L')){
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, StandardCharsets.US_ASCII);
                                    readBufferPosition = 0;

                                    // Handle received data (either command or integer)
                                    processReceivedData(data);
                                } else {
                                    readBuffer[readBufferPosition++] = b;
                                }
                            }
                        }
                    } catch (IOException e) {
//                        connected = false;
                        Log.e(TAG,"Error receiving data!", e);
                        break;
                    }
                }
            }
        });
        workerThread.start();
    }
    private void processReceivedData(String data) {
        Log.d(TAG, data);
        if (data.startsWith("CMD:")) {
            // Handle received command
            final String command = data.substring(4,11);
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    switchMode = findViewById(R.id.switchMode);
                    if (command.equals("AGEMODE")) {
                        switchMode.setChecked(true);
                    } else if (command.equals("QLTMODE")) {
                        switchMode.setChecked(false);
                    }
                }
            });
        } else if (data.startsWith("AGE:") || data.startsWith("QLT:")) {
            final String values = data.substring(4,34);
            int[] integerArray = new int[values.length()];
            try {
                for (int i = 0; i < values.length(); i++) {
                    char digitChar = values.charAt(i);
                    eggValuesList[i] = Character.getNumericValue(digitChar);
                }
            } catch (NumberFormatException e) {
                System.err.println("Error parsing integer: " + e.getMessage());
            }
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    for (int i = 0; i < 30; i++) {
                        TextView eggTextView = findViewById(eggTextViewList[i]);
                        if(mode==0){
                            if(eggValuesList[i] == 0)
                                eggTextView.setTextColor(Color.RED);
                            if(eggValuesList[i] == 1)
                                eggTextView.setTextColor(Color.YELLOW);
                            if(eggValuesList[i] == 2)
                                eggTextView.setTextColor(Color.GREEN);
                        } else {
                            if(eggValuesList[i] == 1)
                                eggTextView.setTextColor(Color.RED);
                            if(eggValuesList[i] == 2)
                                eggTextView.setTextColor(Color.YELLOW);
                            if(eggValuesList[i] == 3)
                                eggTextView.setTextColor(Color.GREEN);
                            if(eggValuesList[i] == 4)
                                eggTextView.setTextColor(Color.BLUE);
                            if(eggValuesList[i] == 5)
                                eggTextView.setTextColor(0xFF8E00EE);
                            if(eggValuesList[i] == 6)
                                eggTextView.setTextColor(Color.BLACK);
                        }
                    }
                }
            });
        } else if (data.startsWith("LOG:")){
            processLogData(data);
        } else if (data.startsWith("CONF:")){
            String confData[] = data.substring(5).split(",");
            for ( int i = 0; i < 13; i++){
                if(confData[i] != "")
                    configs[i] = Integer.parseInt(confData[i]);
            }
        }else if (data.startsWith("RAW:")) {
            final String[] values = data.substring(4).split(",");
            try {
                for (int i = 0; i < 30; i++) {
                    eggRawValues[i] = Integer.parseInt(values[i]);
                }
            } catch (NumberFormatException e) {
                System.err.println("Error parsing integer: " + e.getMessage());
            }
            runOnUiThread(new Runnable() {
                @Override
                public void run() {


                }
            });
        }
    }
    private void processLogData(String receivedData) {
        if (receivedData.length() > 5) {
            String logData = receivedData.substring(4); // Remove "LOG:"
            int separatorIndex = logData.indexOf(":");
            if (separatorIndex > 0) {
                String fileName = logData.substring(0, separatorIndex);
                String fileContents = logData.substring(separatorIndex + 1);
                saveFile(fileName, fileContents);
            }
        }
    }
    private void saveFile(String fileName, String fileContents) {
        try {
            File dir = getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS);
            File file = new File(dir, fileName);
            FileOutputStream os = new FileOutputStream(file);
            os.write(fileContents.getBytes(StandardCharsets.UTF_8));
            os.close();
        } catch (IOException e) {
            Log.e(TAG, "Error writing file: " + e.getMessage());
        }
    }
    private void disableUI() {
        startBttn = findViewById(R.id.saveBttn);
        topText = findViewById(R.id.topText);
        switchMode = findViewById(R.id.switchMode);

        startBttn.setEnabled(false);
        switchMode.setEnabled(false);
        if(!bluetoothAdapter.isEnabled())
            topText.setText("Bluetooth is not enabled.");
        else
            topText.setText("Device is offline!");

    }

    private void enableUI() {
        startBttn = findViewById(R.id.saveBttn);
        topText = findViewById(R.id.topText);
        switchMode = findViewById(R.id.switchMode);
        startBttn.setEnabled(true);
        switchMode.setEnabled(true);
        topText.setText("Connected!");
    }
    private void showToast(String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
            }
        });
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            if (bluetoothSocket != null) {
                bluetoothSocket.close();
            }
        } catch (IOException e) {
            Log.e(TAG, "Error closing the Bluetooth socket", e);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
    }
}