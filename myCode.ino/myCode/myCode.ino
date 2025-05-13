extern "C" {
 void cycle();
 void configure();
}
void setup() {
 configure();
}
void loop() {
 cycle();
}