// #include "BluetoothSerial.h"
#include <Wire.h>
#include <math.h>

//max127 設定
int deviceAddress = 0x28;
//-10~+10Vまでの場合
int ch0 = 0x8C;
int ch1 = 0x9C;
int ch2 = 0xAC;
int ch3 = 0xBC;
int ch4 = 0xCC;
int ch5 = 0xDC;
int ch6 = 0xEC;
int ch7 = 0xFC;

int channels[8]={ch0,ch1,ch2,ch3,ch4,ch5,ch6,ch7};
hw_timer_t *timer = NULL;
hw_timer_t *timer_ramp = NULL;
hw_timer_t *timer3 = NULL;

////max127変数
volatile int result = 0;
int sampleT = 2000; //us　2000us= 500hz
volatile int flag = 0;
int measure_flag=0;
int flag_stim=0; 
int test_flag=99;
int flagx=0; //タイミング調整用
int LOGIC = 27; //Highにしたときに新しい値をSHに保持する．

//電流刺激用
char receiveData[14];
unsigned long startMillis = 0;
unsigned long currentMillis=0;
int current_phase=0; //Loop中に印加電流のどのphaseにいるか
int pre_time=0;
int stim_time=0;
int measure_t=0;
int measure_num=0;
int currentval=0; //流す最大電流値
int pole=1;
double current_current=0; //ある時点での流す電流値
int select_ch[5]={0,0,0,0,0}; //1になってるチャンネルを見る．
////max127計測用タイマ
void IRAM_ATTR onTimer(){
  flag = 1;
  measure_flag=1;
  if (flagx==1){
    //初期動作，python側とタイミングをあわせるため
    flag_stim=1;
    current_phase=1;
    startMillis = millis();
    flagx=0;
  }
   
  digitalWrite(LOGIC, HIGH);
}

int ramp_t=2000;
int rampflag=0;//0なら最初から最大電流量，1なら最初のramp_t ms間をランプにする．
float ramp_current=0;
int incflag=0;
//rampで印加する用.
void IRAM_ATTR Ramp(){
  incflag=1;
}

int read_DAC(int ch, int ch_cont){
  Wire.beginTransmission(deviceAddress);
  Wire.write(ch_cont); 
  Wire.endTransmission(true);
  Wire.requestFrom(deviceAddress, 2);//2byte返してもらう
  result = 0;
  for (int c = 0; c < 2; c++)
  {
    if (Wire.available())
    {
      result = result * 256 + Wire.read();
    }
  }
  return result / 16 + ch * 4096; //上位12bitのみがデータ，3bitがchを持つ
}

void setup() {
  Serial.begin(115200);
  //Serial.begin(250000);
  dacWrite(26, 0);
  dacWrite(25, 0);
  pinMode(4, OUTPUT);
  // max127
  pinMode(SDA, OPEN_DRAIN | INPUT);
  pinMode(SCL, OPEN_DRAIN | INPUT);
  pinMode(LOGIC, OUTPUT);
  Wire.begin();
  Wire.setClock(1000000);
  Wire.setTimeOut(10);

}



void stim_current(int pole){
  if(pole==2){
    dacWrite(25, (int)current_current);
    dacWrite(26, 0);
  }
  if(pole==1){
    dacWrite(26, (int)current_current);
    dacWrite(25, 0);
  }
}
int test=1;
void loop() {
  
  currentMillis = millis();
  //int receiveData[14]={1,1,1,1,1,255,2,20,10,1,40,0,10,5};
  if (Serial.available()) 
  {//信号が来たら時間計測開始・電流値の計測開始
    Serial.readBytes(receiveData,14);
    incflag=0;
    flagx=1;//flagxを立てて次のontimer時に他のflagを建てる
    currentval=(int)receiveData[5];
    pre_time=(int)receiveData[8];
    pre_time=pre_time*100;
    stim_time=((int)receiveData[7])*100;
    //stim_time=stim_time*100;
    measure_t=((int)receiveData[10])*100;    
    rampflag=(int)receiveData[11];
    ramp_t=((int)receiveData[12])*100;
    pole=(int)receiveData[6];
    current_current=0;
    ramp_current=0;
    for(int i=0;i<5;i++){
       select_ch[i]=receiveData[i];
    }
    sampleT=(int)(1000000/((int)receiveData[13]*100));
    //Serial.println(sampleT);
    timer = timerBegin(0, getApbFrequency() / 1000000, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, sampleT, true);
    timerAlarmEnable(timer);
    
    //ramp_tの間に50回のインクリメントをすることを考える，ramp_t=2000msのときには1秒立って1.5mAか
    int countx=(int)(ramp_t/50)*1000;
    timer_ramp = timerBegin(2,getApbFrequency()/1000000, true);//1カウント1μ秒
    timerAttachInterrupt(timer_ramp, &Ramp, true);
    timerAlarmWrite(timer_ramp, countx, true);
    test=0;
  }
  //計測
    if (measure_flag==1){
    //選ばれているチャンネルの計測が行われる．
     for(int i=0;i<5;i++){
      if (select_ch[i]==1){
        result = read_DAC(i, channels[i]);
        Serial.println(result, DEC);//DEC:10進数
      }
    }
    digitalWrite(LOGIC, LOW);
    measure_flag=0;
  }
  
  
  
   
  if(((currentMillis-startMillis)>=pre_time) && (current_phase==1))
  {//施行開始から刺激開始時刻までを計測
   //最大値までランプでインクリメントしていく
   //何故か開始から1秒間計測されていないっぽい？
   if (rampflag==1){
    timerAlarmEnable(timer_ramp);
   }else{
    current_current=currentval;
   }
    if((int)receiveData[9]==1){
      digitalWrite(4, LOW);
    }
    else if((int)receiveData[9]==2){
      digitalWrite(4, HIGH);
    }
    current_phase=2;
  }
  if(incflag==1){//ramptimerが呼び出されたら1/50ずつ増やして刺激をする
     //叩くたびにcurrent_currentをcurrentvalの1/50ずつ増やす
    ramp_current+=currentval/50;
    current_current=(int)ramp_current;
    
    if (current_current>currentval){
      current_current=currentval;
    }else{
      //Serial.println(current_current);
    }
    incflag=0;
    stim_current(pole);
  }


  if(((currentMillis-startMillis)>=pre_time) && (current_phase==2))
  {//一定時間刺激
   //Serial.println(currentMillis-startMillis);
//    Serial.println(measure_num);
    //timerEnd(timer2);
    if (rampflag==0){
      stim_current(pole);
    }
    
    current_phase=3;
  }
  
  else if(((currentMillis-startMillis)>=(pre_time+stim_time)) && (current_phase==3))
  {//電流刺激時間が終わったら1秒かけて減らしていく
//    Serial.println(currentMillis-startMillis);
//    Serial.println(measure_num);

    //dec_v();
    current_phase=4;
    dacWrite(26, 0);
    dacWrite(25, 0);
    timerEnd(timer_ramp);
  }

  else if(((currentMillis-startMillis)>=(pre_time+stim_time)) && (current_phase==4))
  {//電流刺激時間が終わったら電流を切る
    //Serial.println(currentMillis-startMillis);
//    Serial.println(measure_num);
    //timerEnd(timer3);
    dacWrite(26, 0);
    dacWrite(25, 0);
    digitalWrite(4, LOW);
    timerEnd(timer_ramp);
    current_phase=5;
  }

  else if(((currentMillis-startMillis)>=(measure_t)) && (current_phase==5))
  {//計測時間終了用のフラグ設定
    //Serial.println(currentMillis-startMillis);
//    Serial.println(measure_num);
    flag_stim=0;
    incflag=0;
    rampflag=0;
    ramp_t=0;
    current_phase=0;
    current_current=0;
    ramp_current=0;
    Serial.println();
    Serial.println();
    test=1;
    timerEnd(timer);
  }  
}
