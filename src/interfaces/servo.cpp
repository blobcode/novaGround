/*!
 *  @file Adafruit_PWMServoDriver.h
 *
 *  This is a library for our Adafruit 16-channel PWM & Servo driver.
 *  Re-written for Raspberry Pi.
 *
 *  Designed specifically to work with the Adafruit 16-channel PWM & Servo
 * driver.
 */

 #include "servo.hpp"

 #define ENABLE_DEBUG_OUTPUT // comment out to suppress debug level dumps
 
 /*!
  *  @brief  Instantiates a new PCA9685 PWM driver chip with the I2C address
  *  @param  addr The 7-bit I2C address to locate this chip, default is 0x40
  */
 Adafruit_PWMServoDriver::Adafruit_PWMServoDriver(const uint8_t addr)
     : _i2caddr(addr) {}
 
 /*!
  *  @brief  Setups the I2C interface and hardware
  *  @param  prescale
  *          Sets External Clock (Optional)
  */
 void Adafruit_PWMServoDriver::begin(uint8_t prescale) {
     this->fd = wiringPiI2CSetup(_i2caddr);
     if (this->fd < 0) {
         std::cerr << "Failed to initialize I2C communication." << std::endl;
         return;
     }
     else {
         std::cout << "I2C communication initialized successfully." << std::endl;
     }
     reset();
     if (prescale) {
         setExtClk(prescale);
     } else {
         // set a default frequency
         setPWMFreq(1000);
     }
     // set the default internal frequency
     setOscillatorFrequency(FREQUENCY_OSCILLATOR);
 }
 
 /*!
  *  @brief  Sends a reset command to the PCA9685 chip over I2C
  */
 void Adafruit_PWMServoDriver::reset() {
     write8(PCA9685_MODE1, MODE1_RESTART);
     delay(10);
 }
 
 /*!
  *  @brief  Puts board into sleep mode
  */
 void Adafruit_PWMServoDriver::sleep() {
     uint8_t awake = read8(PCA9685_MODE1);
     uint8_t sleep = awake | MODE1_SLEEP; // set sleep bit high
     write8(PCA9685_MODE1, sleep);
     delay(5); // wait until cycle ends for sleep to be active
 }
 
 /*!
  *  @brief  Wakes board from sleep
  */
 void Adafruit_PWMServoDriver::wakeup() {
     uint8_t sleep = read8(PCA9685_MODE1);
     uint8_t wakeup = sleep & ~MODE1_SLEEP; // set sleep bit low
     write8(PCA9685_MODE1, wakeup);
 }
 
 /*!
  *  @brief  Sets EXTCLK pin to use the external clock
  *  @param  prescale
  *          Configures the prescale value to be used by the external clock
  */
 void Adafruit_PWMServoDriver::setExtClk(uint8_t prescale) {
     uint8_t oldmode = read8(PCA9685_MODE1);
     uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
     write8(PCA9685_MODE1, newmode); // go to sleep, turn off internal oscillator
 
     // This sets both the SLEEP and EXTCLK bits of the MODE1 register to switch
     // to use the external clock.
     write8(PCA9685_MODE1, (newmode |= MODE1_EXTCLK));
 
     write8(PCA9685_PRESCALE, prescale); // set the prescaler
 
     delay(5);
     // clear the SLEEP bit to start
     write8(PCA9685_MODE1, (newmode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Mode now 0x" << std::hex << read8(PCA9685_MODE1) << std::dec
               << std::endl;
 #endif
 }
 
 /*!
  *  @brief  Sets the PWM frequency for the entire chip, up to ~1.6 KHz
  *  @param  freq Floating point frequency that we will attempt to match
  */
 void Adafruit_PWMServoDriver::setPWMFreq(float freq) {
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Attempting to set freq " << freq << std::endl;
 #endif
     // Range output modulation frequency is dependant on oscillator
     if (freq < 1)
         freq = 1;
     if (freq > 3500)
         freq = 3500; // Datasheet limit is 3052=50MHz/(4*4096)
 
     float prescaleval = ((_oscillator_freq / (freq * 4096.0)) + 0.5) - 1;
     if (prescaleval < PCA9685_PRESCALE_MIN)
         prescaleval = PCA9685_PRESCALE_MIN;
     if (prescaleval > PCA9685_PRESCALE_MAX)
         prescaleval = PCA9685_PRESCALE_MAX;
     uint8_t prescale = (uint8_t)prescaleval;
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Final pre-scale: " << prescale << std::endl;
 #endif
 
     uint8_t oldmode = read8(PCA9685_MODE1);
     uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP; // sleep
     write8(PCA9685_MODE1, newmode);                             // go to sleep
     write8(PCA9685_PRESCALE, prescale); // set the prescaler
     write8(PCA9685_MODE1, oldmode);
     delay(5);
     // This sets the MODE1 register to turn on auto increment.
     write8(PCA9685_MODE1, oldmode | MODE1_RESTART | MODE1_AI);
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Mode now 0x" << std::hex << read8(PCA9685_MODE1) << std::dec
               << std::endl;
 #endif
 }
 
 /*!
  *  @brief  Sets the output mode of the PCA9685 to either
  *  open drain or push pull / totempole.
  *  Warning: LEDs with integrated zener diodes should
  *  only be driven in open drain mode.
  *  @param  totempole Totempole if true, open drain if false.
  */
 void Adafruit_PWMServoDriver::setOutputMode(bool totempole) {
     uint8_t oldmode = read8(PCA9685_MODE2);
     uint8_t newmode;
     if (totempole) {
         newmode = oldmode | MODE2_OUTDRV;
     } else {
         newmode = oldmode & ~MODE2_OUTDRV;
     }
     write8(PCA9685_MODE2, newmode);
 #ifdef ENABLE_DEBUG_OUTPUT
     if (totempole) {
         std::cout << "Setting output mode: totempole";
     } else {
         std::cout << "Setting output mode: open drain";
     }
     std::cout << " by setting MODE2 to " << newmode << std::endl;
 #endif
 }
 
 /*!
  *  @brief  Reads set Prescale from PCA9685
  *  @return prescale value
  */
 uint8_t Adafruit_PWMServoDriver::readPrescale(void) {
     return read8(PCA9685_PRESCALE);
 }
 
 /*!
  *  @brief  Gets the PWM duty cycle of one of the PCA9685 pins
  *  @param  num One of the PWM output pins, from 0 to 15
  *  @return returns a duty cycle between 0 and 4096
  */
 uint16_t Adafruit_PWMServoDriver::getPWM(uint8_t num) {
     uint8_t channel_base_reg = PCA9685_LED0_ON_L + 4 * num;
     uint16_t on = (read8(channel_base_reg + 1) << 8) + read8(channel_base_reg);
     uint16_t off =
         (read8(channel_base_reg + 3) << 8) + read8(channel_base_reg + 2);
 
     if (off < on)
         return 4096 + off - on;
     else
         return off - on;
 }
 
 /*!
  *  @brief  Sets the PWM output of one of the PCA9685 pins
  *  @param  num One of the PWM output pins, from 0 to 15
  *  @param  on At what point in the 4096-part cycle to turn the PWM output ON
  *  @param  off At what point in the 4096-part cycle to turn the PWM output OFF
  */
 void Adafruit_PWMServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off) {
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Setting PWM " << num << ": " << on << "->" << off
               << std::endl;
 #endif
     uint8_t channel_base_reg = PCA9685_LED0_ON_L + 4 * num;
     write8(channel_base_reg,
            (uint8_t)(on & 0xFF)); // write least significant 8 bits of ON
     write8(channel_base_reg + 1,
            on >> 8); // write most significant 8 bits of ON
     write8(channel_base_reg + 2,
            (uint8_t)(off & 0xFF)); // write least significant 8 bits of OFF
     write8(channel_base_reg + 3,
            off >> 8); // write most significant 8 bits of OFF
 }
 
 /*!
  *   @brief  Helper to set pin PWM output. Sets pin without having to deal with
  * on/off tick placement and properly handles a zero value as completely off and
  * 4095 as completely on.  Optional invert parameter supports inverting the
  * pulse for sinking to ground.
  *   @param  num One of the PWM output pins, from 0 to 15
  *   @param  val The number of ticks out of 4096 to be active, should be a value
  * from 0 to 4095 inclusive.
  *   @param  invert If true, inverts the output, defaults to 'false'
  */
 void Adafruit_PWMServoDriver::setPin(uint8_t num, uint16_t val, bool invert) {
     // Clamp value between 0 and 4095 inclusive.
     val = std::min(val, (uint16_t)4095);
     if (invert) {
         if (val == 0) {
             // Special value for signal fully on.
             setPWM(num, 4096, 0);
         } else if (val == 4095) {
             // Special value for signal fully off.
             setPWM(num, 0, 4096);
         } else {
             setPWM(num, 0, 4095 - val);
         }
     } else {
         if (val == 4095) {
             // Special value for signal fully on.
             setPWM(num, 4096, 0);
         } else if (val == 0) {
             // Special value for signal fully off.
             setPWM(num, 0, 4096);
         } else {
             setPWM(num, 0, val);
         }
     }
 }
 
 /*!
  *  @brief  Sets the PWM output of one of the PCA9685 pins based on the input
  * microseconds, output is not precise
  *  @param  num One of the PWM output pins, from 0 to 15
  *  @param  Microseconds The number of Microseconds to turn the PWM output ON
  */
 void Adafruit_PWMServoDriver::writeMicroseconds(uint8_t num,
                                                 uint16_t Microseconds) {
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << "Setting PWM Via Microseconds on output " << num << ": "
               << Microseconds << std::endl;
 #endif
 
     double pulse = Microseconds;
     double pulselength;
     pulselength = 1000000; // 1,000,000 us per second
 
     // Read prescale
     uint16_t prescale = readPrescale();
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << prescale << " PCA9685 chip prescale" << std::endl;
 #endif
 
     // Calculate the pulse for PWM based on Equation 1 from the datasheet
     // section 7.3.5
     prescale += 1;
     pulselength *= prescale;
     pulselength /= _oscillator_freq;
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << pulselength << " us per bit" << std::endl;
 #endif
 
     pulse /= pulselength;
 
 #ifdef ENABLE_DEBUG_OUTPUT
     std::cout << pulse << " pulse for PWM" << std::endl;
 #endif
 
     setPWM(num, 0, pulse);
 }
 
 /*!
  *  @brief  Getter for the internally tracked oscillator used for freq
  * calculations
  *  @returns The frequency the PCA9685 thinks it is running at (it cannot
  * introspect)
  */
 uint32_t Adafruit_PWMServoDriver::getOscillatorFrequency(void) {
     return this->_oscillator_freq;
 }
 
 /*!
  *  @brief Setter for the internally tracked oscillator used for freq
  * calculations
  *  @param freq The frequency the PCA9685 should use for frequency calculations
  */
 void Adafruit_PWMServoDriver::setOscillatorFrequency(uint32_t freq) {
     this->_oscillator_freq = freq;
 }
 
 /******************* Low level I2C interface */
 uint8_t Adafruit_PWMServoDriver::read8(uint8_t addr) {
     return wiringPiI2CReadReg8(this->fd, addr);
 }
 
 void Adafruit_PWMServoDriver::write8(uint8_t addr, uint8_t d) {
     wiringPiI2CWriteReg8(this->fd, addr, d);
 }
 
 /*!
  *  @brief C++ implementation of Arduino delay function
  *  @param ms Number of milliseconds to sleep
  */
 void Adafruit_PWMServoDriver::delay(int ms) { usleep(ms * MILLI_TO_MICRO); }
