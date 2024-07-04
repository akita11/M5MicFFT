#include <Arduino.h>
#include <M5Unified.h>
#include "arduinoFFT.h"

// ref:
// https://ambidata.io/samples/m5stack/sound/

#define MIC 33 // for Core2's PortA (Pin1)
//#define SAMPLING_FREQUENCY 40000 // 40kHz
#define SAMPLING_FREQUENCY 2000 // 2kHz
const uint16_t FFTsamples = 256;  // サンプル数は2のべき乗

double vReal[FFTsamples];  // vReal[]にサンプリングしたデーターを入れる
double vImag[FFTsamples];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFTsamples, SAMPLING_FREQUENCY);  // FFTオブジェクトを作る

unsigned int sampling_period_us;

void sample(int nsamples) {
	for (int i = 0; i < nsamples; i++) {
		unsigned long t = micros();
		vReal[i] = (double)analogRead(MIC) / 4095.0 * 3.6 + 0.1132; // ESP32のADCの特性を補正
		vImag[i] = 0;
		while ((micros() - t) < sampling_period_us) ;
  }
}

int X0 = 30;
int Y0 = 20;
int _height = 240 - Y0;
int _width = 320;
float dmax = 5.0;

void drawChart(int nsamples) {
	int band_width = floor(_width / nsamples);
	int band_pad = band_width - 1;

	for (int band = 0; band < nsamples; band++) {
		int hpos = band * band_width + X0;
		float d = vReal[band];
		if (d > dmax) d = dmax;
    int h = (int)((d / dmax) * (_height));
		M5.Lcd.fillRect(hpos, _height - h, band_pad, h, WHITE);
    if ((band % (nsamples / 4)) == 0) {
			M5.Lcd.setCursor(hpos, _height + Y0 - 10);
			M5.Lcd.printf("%.1fkHz", ((band * 1.0 * SAMPLING_FREQUENCY) / FFTsamples / 1000));
		}
	}
}

void setup() {
	M5.begin();
//    M5.Speaker.write(0); // スピーカーをオフする
//	M5.Lcd.setBrightness(20);
	pinMode(MIC, INPUT);

	sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
	M5.Lcd.printf("%d", SAMPLING_FREQUENCY);
}

void DCRemoval(double *vData, uint16_t samples) {
	double mean = 0;
	for (uint16_t i = 1; i < samples; i++) {
		mean += vData[i];
	}
	mean /= samples;
	for (uint16_t i = 1; i < samples; i++) {
  	vData[i] -= mean;
	}
}

uint8_t t = 0;
void loop() {
  sample(FFTsamples);
  DCRemoval(vReal, FFTsamples);
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // 窓関数
  FFT.compute(FFT_FORWARD); // FFT処理(複素数で計算)
  FFT.complexToMagnitude(); // 複素数を実数に変換
  M5.Lcd.fillScreen(BLACK);
  drawChart(FFTsamples / 2);
}
