#include <DHT.h>

const int DHTPIN = 5;//Chân Out của cảm biến nối chân số 5 Arduino

const int DHTTYPE = DHT21;   // Khai báo kiểu cảm biến là DHT11
//const int DHTTYPE = DHT22;   // DHT 22 
//const int DHTTYPE = DHT21;   // DHT 21 

DHT dht(DHTPIN, DHTTYPE); //Khai báo thư viện chân cảm biến và kiểu cảm biến

void setup() 
{
  Serial.begin(9600);

  dht.begin(); //Khởi động cảm biến
}

void loop() 
{
  float doam = dht.readHumidity(); //Đọc độ ẩm

  float doC = dht.readTemperature(); //Đọc nhiệt độ C

  float doF = dht.readTemperature(true); //Đọc nhiệt độ F

  Serial.print("Độ ẩm: ");
  Serial.print(doam);
  
  Serial.print("%  Nhiệt độ: ");
  Serial.print(doC);
  
  Serial.print("°C | ");
  Serial.print(doF);
  Serial.println("°F");

  delay(1000);
}
