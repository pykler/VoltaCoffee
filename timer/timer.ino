void setup(void)
{
  
  Serial.begin(9600);
  
}

void loop(void)
{
  int x;
  int i;
  x = Serial.read();
  if(x=='o'){
    Serial.println(x);
    for(i=0; i<220; i++){
     delay(1000);
    }
    Serial.println("4 Cups of coffee is ready");
    for(i=220; i<340; i++){
      delay(1000);
    }
    Serial.println("6 Cups of coffee is ready");
    for(i=340; i<465; i++){
            delay(1000);
    }
   Serial.println("8 Cups of coffee is ready"); 
    for(i=465; i<665; i++){
      delay(1000);
    }
    Serial.println("10 Cups of coffee is ready");

  }
}
