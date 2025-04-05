package com.example.my_tcpip_client;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;


public class MainActivity extends AppCompatActivity {

    /*接收发送定义的常量*/
    private SendThread sendthread;
    String receive_Msg;
    String l;

    /*用户定义区****************************/
    TextView tv_time_val;

    private TextView tv_val1;
    private TextView tv_val2;
    private TextView tv_val3;
    private TextView tv_val4;

    private EditText ed_time;

    private EditText ed_2;


    /*****************************/

    private SharedPreferences pref;
    private SharedPreferences.Editor editor;
    private EditText accountEdit;
    private EditText passwordEdit;
    private Button login;
    private CheckBox rememberPass;
    String account;
    String password;
    TextView text0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        Toast.makeText(MainActivity.this,"请确保网络已连接", Toast.LENGTH_SHORT).show();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        pref = PreferenceManager.getDefaultSharedPreferences(MainActivity.this);//保存服务器ip和端口需要对应操作
        accountEdit = (EditText) findViewById(R.id.account);//IP地址输入框
        passwordEdit = (EditText) findViewById(R.id.password);//端口号输入框
        rememberPass = (CheckBox) findViewById(R.id.remember_pass);//退出保存服务器ip和端口号按钮
        login = (Button) findViewById(R.id.login);//连接服务器按钮

        text0 = (TextView)findViewById(R.id.ttv2);//接收数据显示框
        text0.setMovementMethod(ScrollingMovementMethod.getInstance());

        tv_val1 = findViewById(R.id.tv_val1);
        tv_val2 = findViewById(R.id.tv_val2);
        tv_val3 = findViewById(R.id.tv_val3);
        tv_val4 = findViewById(R.id.tv_val4);
        ed_time =  findViewById(R.id.ed_time);

        boolean isRemember = pref.getBoolean("remember_password",false);
        if (isRemember){
            account = pref.getString("account","");
            password = pref.getString("password","");
            accountEdit.setText(account);
            passwordEdit.setText(password);
            rememberPass.setChecked(true);
        }
        login.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                String account = accountEdit.getText().toString();
                String password = passwordEdit.getText().toString();
                if (password == null || account.equals("")){//判断是否输入IP
                    Toast.makeText(MainActivity.this,"请输入服务器IP",Toast.LENGTH_SHORT).show();
                } else  if (account == null || password.equals("")){//判断是否输入端口号
                    Toast.makeText(MainActivity.this,"请输入端口号",Toast.LENGTH_SHORT).show();
                } else {
                    editor = pref.edit();
                    if (rememberPass.isChecked()) {
                        editor.putBoolean("remember_password",true);
                        editor.putString("account",account);
                        editor.putString("password",password);

                        /***************连接*****************/
                        sendthread = new SendThread(account, password, mHandler);//传入ip和端口号
                        Thread1();
                        new Thread().start();

                    } else {
                        editor.clear();
                    }
                    editor.apply();
                }
            }
        });

        /**********************************/
        //获取时间
        findViewById(R.id.btn_0).setOnClickListener((View view)->{
            Calendar calendar = Calendar.getInstance();//取得当前时间的年月日 时分秒
            int hour = calendar.get(Calendar.HOUR_OF_DAY);
            int min = calendar.get(Calendar.MINUTE);
            int sec = calendar.get(Calendar.SECOND);
            ed_time.setText(Integer.toString(hour) + ":" + Integer.toString(min) + ":" + Integer.toString(sec));
        });

        //设置时间
        findViewById(R.id.btn_00).setOnClickListener((View view)->{
            sendthread.send("AT+TIME=" + ed_time.getText().toString() + ",\r\n");
        });

        findViewById(R.id.btn_1).setOnClickListener((View view)->{
            sendthread.send("AT+DATA1\r\n");
        });

        findViewById(R.id.btn_2).setOnClickListener((View view)->{
            sendthread.send("AT+DATA2\r\n");
        });

        findViewById(R.id.btn_3).setOnClickListener((View view)->{
            sendthread.send("AT+DATA3\r\n");
        });
    }

    private class FragmentAdapter extends FragmentPagerAdapter {
        List<Fragment> fragmentList = new ArrayList<Fragment>();

        public FragmentAdapter(FragmentManager fm, List<Fragment> fragmentList) {
            super(fm);
            this.fragmentList = fragmentList;
        }

        @Override
        public Fragment getItem(int position) {
            return fragmentList.get(position);
        }

        @Override
        public int getCount() {
            return fragmentList.size();
        }

    }


    /**
     * 开启socket连接线程
     */
    void Thread1(){
        new Thread(sendthread).start();//创建一个新线程
    }

    Handler mHandler = new Handler()
    {
        public void handleMessage(Message msg)
        {
            super.handleMessage(msg);
            if (msg.what == 0x00) {

                Log.i("mr_收到的数据： ", msg.obj.toString());
                receive_Msg = msg.obj.toString();
                l = receive_Msg;
                text0.setText(l);

                if(l.contains("=") != false && l.contains(",") != false && l.contains("!") != false &&
                        l.contains("@") != false && l.contains("#") != false && l.contains("=") != false)
                {
                    String str1 = l.substring(l.indexOf("=")+1, l.indexOf(","));
                    tv_val1.setText(str1);
                    String str2 = l.substring(l.indexOf(",")+1, l.indexOf("!"));
                    tv_val2.setText(str2);
                    String str3 = l.substring(l.indexOf("!")+1, l.indexOf("@"));
                    tv_val3.setText(str3);
                    String str4 = l.substring(l.indexOf("@")+1, l.indexOf("#"));
                    tv_val4.setText(str4);
                }
            }
        }
    };
}