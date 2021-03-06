
#include <SPIFFS.h>
#include "EPD4IN_Device.h"
#include "PIC.H"
 
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "EPD_Http.h"
#include "EPD4IN_driver.h"
#include "driver/rtc_io.h"
 
   
RTC_DATA_ATTR int BootConut = 0;       //系统启动的类型 
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5       /* Time ESP32 will go to sleep (in seconds) */

 
EPD4IN_Device epd_user; 
uint64_t mask;

uint64_t chipid; 

#define KEY2 26
#define KEY1 27  

int KEY2_Process(void);
int Power_Reset_Process(void);
int Timer_Process(void);



void setup() {
  int sleeptime;
  pinMode(KEY2, INPUT); //   
  Serial.begin(115200);
  Serial.println("\nSystem Start up ");


  chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("chip ID=:%lld,HEX =%llx\n",chipid,chipid);
  
  epd_user.GetBatVol();
  
  SPIFFS.begin();
  Serial.println("\nSPIFFS on ");  
  BootConut++; 
  esp_sleep_wakeup_cause_t wakeup_reason;
  
  wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO");addap();Http_Get_Token(); Http_Get_RealWeather();Http_Get_Calendar();Http_Get_TodoList();sleeptime=10;break;// ;

    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL");sleeptime=KEY2_Process();break;//addap(); Http_Get_RealWeather();break;         
 //   case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL");epd_user.SEt_EPD_WIFICONFIG(); Auto_Web_Config();break;     
  //  case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL");; break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer");sleeptime=Timer_Process(); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason);sleeptime=Power_Reset_Process(); break;
  }
  

 
  Serial.printf("Sleep time %d\n",sleeptime);
  esp_sleep_enable_timer_wakeup(sleeptime * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to wakeup after  " + String(sleeptime) +
  " Seconds");
  Serial.println("Going to sleep now");
  Serial.flush(); 
    
  rtc_gpio_pullup_en(GPIO_NUM_4);
  rtc_gpio_pulldown_dis(GPIO_NUM_4);   //保证屏幕RST引脚高电平
//esp_sleep_enable_ext0_wakeup(GPIO_NUM_2,0);
//esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,0);   
//esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,0);  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0);

 
  mask|=  1ull << 26;
//mask|=  1ull << 27;
  esp_sleep_enable_ext1_wakeup(mask,ESP_EXT1_WAKEUP_ALL_LOW);
  
//gpio_hold_en(GPIO_NUM_2);
//gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
void loop() {

  
 
} 
int Power_Reset_Process(void)
{
  epd_user.Start_Init_EPD();   //显示开机界面
  addap();
  if(addap())
  { //成功链接WIFI
    Http_Set_Rtc();     //配置RTC
//////系统启动  更新所有内容  /////
      Http_Get_Token();

      Http_Get_Calendar();
      Http_Get_RealWeather();
      Http_Get_TodoList();
//////////////////////////////////
   epd_user.DrawFullScreen();   //
   return (60-epd_user.GetRtcSenconds()+2);    //返回休眠时间
    
  }
  else    //无法成功链接WIFI   刷屏幕后休眠
  {
   delay(2000);
   Serial.println("Show Init Page"); 
   epd_user.SEt_EPD_NeedConfig();
   return (60*60*24*365);    //返回休眠时间 100分钟 
  }
}


int KEY2_Process(void)
{
 //KEY2 处理
   pinMode(KEY1, INPUT); // 
   pinMode(KEY2, INPUT); // 
   delay(300);

   if(digitalRead(KEY2))    //KEY2 短按  立刻更新todoist 刷新全屏幕。
   {//短按 处理
     Serial.println("KEY2 UP  ");
     addap();
     Http_Get_TodoList();// 
    //刷新屏幕
    epd_user.DrawFullScreen();
    return (60-epd_user.GetRtcSenconds()+2);    //返回休眠时间
     
   }
   else
   {
    delay(3000);  
    if((!digitalRead(KEY2))&&(digitalRead(KEY1)))//长按超过3s  KEY2=0 KEY1=1    进入WIFI配置模式
    {
      RestoreWifi();
      Serial.println("KEY2 DOWN TO WEB CONFIG PROCESS  ");
      epd_user.SEt_EPD_WIFICONFIG();
      
      Auto_Web_Config();
      ESP.restart();  //复位esp32 无论成功失败 均重新复位重启
      return (60-epd_user.GetRtcSenconds()+2);    //返回休眠时间
       }
    else if((!digitalRead(KEY2))&&(!digitalRead(KEY1)))//长按超过3s  KEY2=0 KEY1=0   清除WIFI配置，立刻复位
    {
      RestoreWifi();  //清除WIFI 配置数据 复位
      ESP.restart();  //复位esp32 
      return 2;       //never 
    }     
  }
}
int Timer_Process(void)
{
//  epd_user.DrawStartScreen();
    uint16_t nowhour,nowMinute;
    
//更新周期设计。暂未按照配置时间，暂时以固定时间配置
//todo 更新周期 5分钟
//日历和RTC更新周期 每天0：0分
//天气i更新周期 30分钟
    nowhour=epd_user.GetRtcHour();
    nowMinute=epd_user.GetRtcMinute();   
    if((nowMinute==0)&&(nowhour==0))   //更新日历和RTC
    { //
      Serial.println("nowMinute=0 nowhour=0 ");      
      addap();
      Http_Get_Calendar();
      Http_Set_Rtc();
      
    }
      if((nowMinute==0)||(nowMinute==30))   //更新天气 每30分钟
    {
      Serial.println("nowMinute=0 nowhour=30 ");   
      addap();
      Http_Get_RealWeather();
 
    }
      if(nowMinute%5==0)    //更新todo    每隔5分钟
    { 
      Serial.println("nowMinute%5=0 ");   
      addap();
      Http_Get_TodoList();
      epd_user.DrawFullScreen();
      
    }
    else                  //以上都没有  只刷新时间 局部刷新
    {
      epd_user.DrawTimeUpdata();
    }
   return (60-epd_user.GetRtcSenconds()+2);
}
