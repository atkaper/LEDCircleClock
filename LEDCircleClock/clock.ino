// zero degrees is straigh up
void drawAngle(int angle, int rings, RgbColor color, boolean overwrite = false, double brightnessFactorCutoff = 0.0) {
  // Set the correct pixels in the rings
  for (int ring = 1; ring < rings; ring++) {
    // Make sure the software watchdog does not trigger
    ESP.wdtFeed();

    // Set the correct color for all the LEDs in the ring
    int ringSize = ringSizes[ring];
    for (int led = 0; led < ringSize; led++) {
      int ledAngle = (int)((double)led * 360.0 / ringSize);
      int distance = abs(ledAngle - angle);
      if (distance > 180) distance = 360 - distance;
      double brightnessFactor = (180.0 - (double)distance) / 180.0; // 1.0 = minimal distance, 0.0 = maximum distance
      if (brightnessFactor > 0.99) brightnessFactor = 1.0;
      brightnessFactor = fastPow(brightnessFactor, ringPowers[ring]); // steaper slopes and so narrower line

      int ledNumber = startLEDs[ring] + led;
      RgbColor originalColor = strip.GetPixelColor(ledNumber);
      RgbColor targetColor = RgbColor(
        max(color.R * brightnessFactor, (double)originalColor.R * (overwrite && brightnessFactor > brightnessFactorCutoff ? 1.0 - brightnessFactor : 1.0)), 
        max(color.G * brightnessFactor, (double)originalColor.G * (overwrite && brightnessFactor > brightnessFactorCutoff ? 1.0 - brightnessFactor : 1.0)), 
        max(color.B * brightnessFactor, (double)originalColor.B * (overwrite && brightnessFactor > brightnessFactorCutoff ? 1.0 - brightnessFactor : 1.0)));

      setPixel(ledNumber, targetColor);
    }
  }
}

// This one draws a single pixel in every ring, closest to the given angle. No gradations. Always overwrite.
void drawSimpleAngle(int angle, int rings, RgbColor color) {
  for (int ring = 1; ring < rings; ring++) {
    int ringSize = ringSizes[ring];
    int startLed = startLEDs[ring];
    int led = startLed + int((ringSize * angle / 360) + 0.5);
    setPixel(led, color);
  }
}

void drawMarkers() {
  int ringSize = ringSizes[RINGS - 1];
  for (int hour = 0; hour < 12; hour++) {
    int ledNumber = startLEDs[RINGS - 1] + hour * ringSize / 12;
    RgbColor originalColor = strip.GetPixelColor(ledNumber);
    RgbColor markerColor = RgbColor(min(brightness / 3 + originalColor.R, 255), min(brightness / 4 + originalColor.G, 255),  originalColor.B);
    strip.SetPixelColor(ledNumber, markerColor);
  }
  setPixel(0, RgbColor(brightness / 3, brightness / 4, 0));
}

void updateClockHands() {
  currentTime = time(nullptr); // time_t = seconds since epoch
  
  if (isNtpOlderThanOneHour()) {
    // No fresh NTP time info? Hide clock hands...
    strip.ClearTo(RgbColor(0, 0, 0));
    drawMarkers();
    if (currentTime % 2 == 0) {
      // Flashing red dot in the middle to indicate loss of time
      setPixel(0, RgbColor(brightness / 3, 0, 0));
    }
    strip.Show();
    return;
  }

  timeinfo = localtime (&currentTime); // setup timeinfo -> tm_hour, timeinfo -> tm_min, timeinfo -> tm_sec
  int secondsOfDay = ((timeinfo -> tm_hour % 12) * 3600) + (timeinfo -> tm_min * 60) + (timeinfo -> tm_sec);

  int millisOfSecond = millis() % 1000L;
  if (previousClockSecond != secondsOfDay) {
    // Reset the millis offset
    millisOffset = -millisOfSecond;
    previousClockSecond = secondsOfDay;
  }
  
  millisOfSecond = millisOfSecond + millisOffset;
  if (millisOfSecond < 0) millisOfSecond += 1000;

  strip.ClearTo(RgbColor(0, 0, 0));
  
  int secondsAngle =  secondsOfDay % 60 * 6 + millisOfSecond * 6 / 1000;
  drawAngle(secondsAngle, RINGS, RgbColor(0, brightness, 0));

  int minutesAngle = secondsOfDay % 3600 / 10;
  drawAngle(minutesAngle, RINGS - 1, RgbColor(0, 0, brightness));

  int hoursAngle = secondsOfDay / 120;
  drawAngle(hoursAngle, RINGS - 2, RgbColor(brightness, 0, 0));

  drawMarkers();
  strip.Show();
}
