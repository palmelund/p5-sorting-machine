//#define COMPONENT_TEST_MOTOR_COAST
//#define COMPONENT_TEST_MOTOR_TIME
#define COMPONENT_TEST_INTERUPT_COST
//#define KILLALL // Uncomment this to kill the system, and stop all components

#define MOTOR_1_PIN 12
#define MOTOR_2_PIN 11
#define MOTOR_3_PIN

#define MOTOR_1_INT_PIN 3
#define MOTOR_2_INT_PIN 2
#define MOTOR_3_INT_PIN

enum Turning_Direction {
  CLOCKWISE         = -1,
  COUNTERCLOCKWISE  = 1
};

struct Motor {
  int pin = 0;
  volatile long deg = 0;
  Turning_Direction dir = CLOCKWISE;
};

Motor motor1, motor2, motor3;

void setup() {
#ifdef KILLALL
  exit(0);
#endif

  motor1.pin = MOTOR_1_PIN;
  motor2.pin = MOTOR_2_PIN;
  //motor3.pin = MOTOR_3_PIN;

  Serial.begin(9600);

  pinMode(MOTOR_1_INT_PIN, INPUT_PULLUP);
  pinMode(MOTOR_2_INT_PIN, INPUT_PULLUP);
  //pinMode(MOTOR_3_INT_PIN, INPUT_PULLUP);

  pinMode(motor1.pin, OUTPUT);
  pinMode(motor2.pin, OUTPUT);
  //pinMode(motor3.pin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(MOTOR_1_INT_PIN), motor1_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(MOTOR_2_INT_PIN), motor2_interrupt, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(MOTOR_3_INT_PIN), motor3_interrupt, CHANGE);

#ifdef COMPONENT_TEST_MOTOR_COAST
  Serial.println("Component Test: Motor Coast");
  component_test_motor_coast(&motor1);
#endif

#ifdef COMPONENT_TEST_MOTOR_TIME
  Serial.println("Component Test: Motor Time");
  component_test_motor_time(&motor1);
#endif

#ifdef COMPONENT_TEST_INTERUPT_COST
  Serial.println("Component Test: Interupt Cost");
  component_test_interupt_cost();
#endif

}

void motor1_interrupt() {
  motor1.deg += motor1.dir;
}

void motor2_interrupt() {
  motor2.deg += motor2.dir;
}

void motor3_interrupt() {
  motor3.deg += motor3.dir;
}

void motor_turn_deg(Motor* motor, int deg, Turning_Direction dir)
{
  int goal = motor->deg + (deg * dir);

  motor_turn(motor, dir);

  switch (motor->dir) {
    case CLOCKWISE:
      while (goal <= motor->deg) {
        delay(1);
      }
      break;
    case COUNTERCLOCKWISE:
      while (goal >= motor->deg) {
        delay(1);
      }
      break;
    default:
      Serial.println("NO ROTATION");
      break;
  }
  motor_stop(motor);
}

void motor_stop(Motor* motor)
{
  digitalWrite(motor->pin, LOW);
}

void motor_turn(Motor* motor, Turning_Direction dir)
{
  motor->dir = dir;
  digitalWrite(motor->pin, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
}

/*****************************
   COMPONENT TESTING METHODS
 *****************************/
#ifdef COMPONENT_TEST_MOTOR_COAST
void component_test_motor_coast(Motor* motor) {
  for (int i = 0; i < 20; i++) {
    motor->deg = 0;
    motor_turn_deg(motor, 360, COUNTERCLOCKWISE);
    if (motor->deg < 360) {     // In case it happens that the loop breaks out before 360 degrees
      Serial.println("ERROR");  // thrown an error, because that is not supposed to happen.
      exit(0);                  // The error should be fixed, but cant be sure yet.
    }

    delay(1000);
    Serial.println(motor->deg); // Value used for component testing.
    delay(500);                 // Should be 360 + coast degrees for the motor.
  }
}
#endif

#ifdef COMPONENT_TEST_MOTOR_TIME
void component_test_motor_time(Motor* motor) {
  for (int i = 0; i < 20; i++) {
    motor->deg = 0;
    unsigned long t_start = millis();
    motor_turn_deg(motor, 360, COUNTERCLOCKWISE);
    unsigned long t_end = millis();
    if (motor->deg < 360) {
      Serial.println("ERROR");
      exit(0);
    }

    Serial.println(t_end-t_start);
    delay(1000);
  }
}
#endif

#ifdef COMPONENT_TEST_INTERUPT_COST
void component_test_interupt_cost(){
  const int test_count = 10;
  const long iterations = 500000;
  
  long time_start, time_end, interrupt_count_start, interrupt_count_end;
  long no_interrupt_times[test_count], interrupt_times[test_count], interrupt_counts[test_count];

  // run measurements without a motor running
  for (int i = 0; i < test_count; i++){  
    time_start = micros();
    for (volatile long j = 0; j < iterations; j++);
    time_end = micros();
  
    no_interrupt_times[i] = time_end - time_start;
  }

  // run measurements with a motor running
  motor_turn(&motor1, COUNTERCLOCKWISE);
  delay(2500);
  for (int i = 0; i < test_count; i++){
    interrupt_count_start = motor1.deg;
    time_start = micros();
    for (volatile long j = 0; j < iterations; j++);
    time_end = micros();
    interrupt_count_end = motor1.deg;
    
    interrupt_times[i] = time_end - time_start;
    interrupt_counts[i] = interrupt_count_end - interrupt_count_start;
  }
    
  motor_stop(&motor1);
  
  long average_no_interrupt_time = 0, average_interrupt_time = 0, average_interrupt_count = 0;

  // print result table
  Serial.println("| with       | without    | count      | est cost   |");
  for (int i = 0; i < test_count; i++){
    Serial.print("|    ");
    Serial.print(interrupt_times[i]);
    Serial.print(" |    ");
    Serial.print(no_interrupt_times[i]);
    Serial.print(" |        ");
    Serial.print(interrupt_counts[i]);
    Serial.print(" | ");
    Serial.print((interrupt_times[i] - no_interrupt_times[i]) / (double)interrupt_counts[i], 8);
    Serial.println(" |");

    average_no_interrupt_time += no_interrupt_times[i];
    average_interrupt_time += interrupt_times[i];
    average_interrupt_count += interrupt_counts[i];
  }

  average_no_interrupt_time /= test_count;
  average_interrupt_time /= test_count;
  average_interrupt_count /= test_count; 
  
  Serial.println("-----------------------------------------------------");
  Serial.print("|    ");
  Serial.print(average_interrupt_time);
  Serial.print(" |    ");
  Serial.print(average_no_interrupt_time);
  Serial.print(" |        ");
  Serial.print(average_interrupt_count);
  Serial.print(" | ");
  Serial.print((average_interrupt_time - average_no_interrupt_time) / (double)average_interrupt_count, 8);
  Serial.println(" |");
}
#endif
